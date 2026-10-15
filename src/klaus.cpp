// ============================================================================
//  KLAUSTROPHOBIE – the overcrowded elevator, hosted on the ESP32
//
//  2..6 passengers (humans + bots) are squeezed into a 6x4 cabin on the way up
//  to the penthouse. Annoying NPCs board at every floor, everybody farts.
//  Every passenger has a discomfort meter; at 100 % you get thrown out.
//  Last one in (or the least miserable one at the top) wins.
//  The server is authoritative; clients get a full snapshot 4x per second.
//  WebSocket on port 83, page at /klaus.
// ============================================================================
#include <Arduino.h>
#include <WebSocketsServer.h>
#include <stdarg.h>
#include "klaus.h"
#include "klaus_page.h"

static WebSocketsServer ws3(83);

// ---------------------------------------------------------------- tuning ---
static const int GW = 6, GH = 4, NCELL = GW * GH;
static const int NSEAT = 6, MINPLAYERS = 4, FLOORS = 10, MAXLOAD = 21;
static const uint32_t TRAVEL_MS = 6000, DOORS_MS = 4800, OVER_MS = 9000, TICK_MS = 500, SNAP_MS = 250;
static const uint32_t CD_MOVE = 280, CD_ELBOW = 1500, CD_FART = 5500, CD_DOOR = 8000;

enum Phase { P_WAIT, P_TRAVEL, P_DOORS, P_OVER };
enum Kind { K_PLAYER, K_KARL, K_PARFUEM, K_KEVIN, K_TELEFON, K_HUND, K_JOGGER, K_COUNT };
static const char *KNAME[] = {"", "Zwiebelbrot-Karl", "Parfümtante Gisela", "Kinderwagen-Kevin",
                              "Telefon-Tobi", "Wauwi der Hund", "Jogger-Jens"};
static const float ANN[] = {0.35f, 1.2f, 1.6f, 0.5f, 0.8f, 0.4f, 1.1f};   // discomfort per tick for an adjacent neighbour
static const int8_t DX[8] = {0, 1, 1, 1, 0, -1, -1, -1};                  // N NE E SE S SW W NW
static const int8_t DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
static const char *BOTN[] = {"Bodo", "Gisela", "Horst", "Uschi", "Dieter", "Mandy"};

struct Ent { bool on; uint8_t kind, x, y, who; };
struct Seat {
  bool act, human, alive; uint8_t client; char name[14];
  float dis, rate; int8_t e; uint8_t place, face;
  uint32_t cdF, cdD, cdM, cdE, tauntAt, botAt;
};
struct Waiter { uint8_t client; char name[14]; };
struct Fx { uint8_t t, x, y; };

static Ent ent[NCELL];
static int8_t cell[NCELL];
static uint8_t gas[NCELL];
static Seat seat[NSEAT];
static Waiter queue[8];
static int qn = 0;
static Fx fx[12];
static int fxN = 0;
static int phase = P_WAIT, floorNo = 1, winner = -1, outN = 0, boardLeft = 0, evKind = 0;
static uint32_t phaseEnd, nextBoard, evAt, blackoutEnd, lastTick, lastSnap, botEnd;
static char buf[3600];
static char logLine[8][110];
static int logN = 0;

// --------------------------------------------------------------- helpers ---
static int rndi(int a, int b) { return a + (int)(esp_random() % (uint32_t)(b - a + 1)); }
template <size_t N> static const char *pick(const char *(&a)[N]) { return a[esp_random() % N]; }
static bool inb(int x, int y) { return x >= 0 && y >= 0 && x < GW && y < GH; }

static void sayf(const char *fmt, ...) {
  if (logN >= 8) return;
  va_list ap; va_start(ap, fmt); vsnprintf(logLine[logN++], sizeof(logLine[0]), fmt, ap); va_end(ap);
}
static void fxAdd(uint8_t t, int x, int y) { if (fxN < 12) { fx[fxN].t = t; fx[fxN].x = x; fx[fxN].y = y; fxN++; } }

static const char *FART[] = {
  "%s lässt einen fahren. Der Fahrstuhl riecht jetzt nach Eiersalat.",
  "%s drückt leise einen raus. Leise, aber tödlich.",
  "%s pupst und guckt dabei unschuldig an die Decke.",
  "PFFFFT! %s wird den Döner von gestern endlich los.",
  "%s hat Blähungen und keine Hemmungen.",
  "%s lässt die Hosen knallen. Nachbarn bitte anschnallen."};
static const char *NPCFART[] = {
  "%s lässt einen fahren und starrt vorwurfsvoll den Nachbarn an.",
  "%s hat sich was Warmes gegönnt. Der Fahrstuhl nimmt's persönlich."};
static const char *ELBOW[] = {
  "%s rammt %s den Ellbogen in die Rippen. Platz da!",
  "%s schubst %s weg. Persönlicher Freiraum ist ein Menschenrecht.",
  "%s verpasst %s einen Hüftcheck. Sehr unhöflich, sehr effektiv.",
  "%s serviert %s einen Ellbogen mit Senf."};
static const char *JAM[] = {
  "%s steckt zwischen zwei Rücken fest. Autsch.",
  "Hinter %s ist kein Platz mehr. Die Rippen quietschen."};
static const char *OUT[] = {
  "%s kippt um und wird vom Fahrstuhl ausgespuckt. Frischluft!",
  "%s hält's nicht mehr aus und stürzt durch die Notluke.",
  "%s brüllt 'ICH MUSS HIER RAUS!' und trampelt alle nieder. Raus ist er.",
  "%s kotzt auf die Schuhe und wird aus der Kabine geschoben.",
  "%s kriegt Platzangst und rennt kreischend ins Treppenhaus."};
static const char *BOARD[] = {"",
  "Zwiebelbrot-Karl steigt zu. Halbes Zwiebelbrot in der Hand, kaut mit offenem Mund.",
  "Parfümtante Gisela steigt zu. Sie hat in Eau de Mottenkugel gebadet.",
  "Kinderwagen-Kevin steigt zu. Der Wagen ist leer, aber Kevin riecht nach Windel.",
  "Telefon-Tobi steigt zu und brüllt: 'JA, ICH BIN IM AUFZUG!!'",
  "Wauwi der Hund steigt zu. Hunger und Mundgeruch inklusive.",
  "Jogger-Jens steigt zu. Verschwitzt wie ein Walross in der Sauna."};
static const char *LEAVE[] = {
  "%s steigt aus. Der Fahrstuhl atmet auf.",
  "%s verschwindet im Flur und hinterlässt eine Duftmarke."};
static const char *BITE[] = {
  "Wauwi beißt %s in den Hintern. Gute Besserung.",
  "Wauwi hat %s zum Fressen gern. Wortwörtlich."};
static const char *CLOSE[] = {
  "%s hämmert auf Tür-zu. Der Rest bleibt draußen.",
  "%s drückt Tür-zu und guckt dabei grimmig. Nix mehr mit Zusteigen."};

// ------------------------------------------------------------------ grid ---
static void moveEnt(int e, int nx, int ny) {
  cell[ent[e].y * GW + ent[e].x] = -1;
  ent[e].x = nx; ent[e].y = ny;
  cell[ny * GW + nx] = e;
}
static int addEnt(uint8_t kind, int x, int y, uint8_t who) {
  for (int i = 0; i < NCELL; i++)
    if (!ent[i].on) { ent[i] = {true, kind, (uint8_t)x, (uint8_t)y, who}; cell[y * GW + x] = i; return i; }
  return -1;
}
static void delEnt(int e) { cell[ent[e].y * GW + ent[e].x] = -1; ent[e].on = false; }
static int load() { int n = 0; for (int i = 0; i < NCELL; i++) n += ent[i].on; return n; }
static const char *nameOf(int e) { return ent[e].kind == K_PLAYER ? seat[ent[e].who].name : KNAME[ent[e].kind]; }

static bool nextToPlayer(int c) {
  int x = c % GW, y = c / GW;
  for (int k = 0; k < 8; k++) {
    int nx = x + DX[k], ny = y + DY[k];
    if (inb(nx, ny) && cell[ny * GW + nx] >= 0 && ent[cell[ny * GW + nx]].kind == K_PLAYER) return true;
  }
  return false;
}
// random free cell; with nearPlayer, newcomers prefer to squeeze right next to a passenger
static bool pickFree(int &x, int &y, bool nearPlayer) {
  int fc[NCELL], n = 0, nc[NCELL], m = 0;
  for (int c = 0; c < NCELL; c++) if (cell[c] < 0) { fc[n++] = c; if (nextToPlayer(c)) nc[m++] = c; }
  if (!n) return false;
  int c = (nearPlayer && m) ? nc[esp_random() % m] : fc[esp_random() % n];
  x = c % GW; y = c / GW;
  return true;
}

static void gasBurst(int x, int y) {
  for (int k = 0; k < 8; k++) {
    int nx = x + DX[k], ny = y + DY[k];
    if (inb(nx, ny) && gas[ny * GW + nx] < 6) gas[ny * GW + nx] = 6;
  }
  if (gas[y * GW + x] < 2) gas[y * GW + x] = 2;      // the culprit suffers less
}

// discomfort per tick from the people around (x,y), without gas
static float exposure(int x, int y, int self, int &nbr) {
  float d = 0; nbr = 0;
  for (int k = 0; k < 8; k++) {
    int nx = x + DX[k], ny = y + DY[k];
    if (!inb(nx, ny)) continue;
    int e = cell[ny * GW + nx];
    if (e < 0 || e == self) continue;
    nbr++; d += ANN[ent[e].kind];
  }
  return d + 0.3f * nbr;
}

static int humansSeated() { int n = 0; for (int i = 0; i < NSEAT; i++) n += seat[i].human; return n; }
static int aliveCount(int *humans = nullptr) {
  int a = 0, h = 0;
  for (int i = 0; i < NSEAT; i++) if (seat[i].act && seat[i].alive) { a++; h += seat[i].human; }
  if (humans) *humans = h;
  return a;
}

// ------------------------------------------------------------- messaging ---
static int put(int n, const char *fmt, ...) {
  if (n >= (int)sizeof(buf) - 1) return n;
  va_list ap; va_start(ap, fmt);
  int r = vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
  va_end(ap);
  return r < 0 ? n : min(n + r, (int)sizeof(buf) - 1);
}

static int buildState() {
  uint32_t now = millis();
  int rem = phaseEnd > now ? (phaseEnd - now) / 100 : 0;
  int n = put(0, "{\"k\":\"st\",\"ph\":%d,\"fl\":%d,\"t\":%d,\"bo\":%d,\"win\":%d,\"s\":[",
              phase, floorNo, rem, blackoutEnd > now, winner);
  for (int i = 0; i < NSEAT; i++) {
    Seat &s = seat[i];
    if (!s.act) { n = put(n, "%s[0]", i ? "," : ""); continue; }
    int cf = s.cdF > now ? (s.cdF - now) / 100 : 0, cd = s.cdD > now ? (s.cdD - now) / 100 : 0;
    n = put(n, "%s[1,\"%s\",%d,%d,%d,%d,%d,%d,%d,%d]", i ? "," : "", s.name, !s.human, s.alive,
            (int)s.dis, cf, cd, s.face, s.place, (int)(s.rate * 10));
  }
  n = put(n, "],\"e\":[");
  bool first = true;
  for (int i = 0; i < NCELL; i++) {
    if (!ent[i].on) continue;
    n = put(n, "%s[%d,%d,%d,%d,%d]", first ? "" : ",", i, ent[i].kind, ent[i].x, ent[i].y, ent[i].who);
    first = false;
  }
  n = put(n, "],\"g\":\"");
  for (int c = 0; c < NCELL; c++) n = put(n, "%c", '0' + gas[c]);
  n = put(n, "\",\"fx\":[");
  for (int i = 0; i < fxN; i++) n = put(n, "%s[%d,%d,%d]", i ? "," : "", fx[i].t, fx[i].x, fx[i].y);
  n = put(n, "],\"log\":[");
  for (int i = 0; i < logN; i++) n = put(n, "%s\"%s\"", i ? "," : "", logLine[i]);
  return put(n, "]}");
}

static void broadcastState() {
  int n = buildState();
  ws3.broadcastTXT(buf, n);
  logN = 0; fxN = 0;
}

static void sendYou(uint8_t num, int s, int q) {
  char m[40]; int n = snprintf(m, sizeof(m), "{\"k\":\"you\",\"i\":%d,\"q\":%d}", s, q);
  ws3.sendTXT(num, m, n);
}

// ------------------------------------------------------------ game logic ---
static void knockOut(int s) {
  Seat &me = seat[s];
  sayf(pick(OUT), me.name);
  fxAdd(3, ent[me.e].x, ent[me.e].y);
  delEnt(me.e);
  me.alive = false; me.e = -1; me.place = outN++;
}

static void endRound() {
  if (phase == P_OVER) return;
  phase = P_OVER; phaseEnd = millis() + OVER_MS;
  int a = 0, w = -1;
  for (int i = 0; i < NSEAT; i++) if (seat[i].act && seat[i].alive) { a++; if (w < 0 || seat[i].dis < seat[w].dis) w = i; }
  if (a == 0) for (int i = 0; i < NSEAT; i++) if (seat[i].act && (w < 0 || seat[i].place > seat[w].place)) w = i;
  for (int i = 0; i < NSEAT; i++) if (seat[i].act && seat[i].alive) seat[i].place = 100;
  winner = w;
  if (a <= 1) sayf("%s steht als Einziger noch in der Kabine und gewinnt. Der Rest nimmt die Treppe.", seat[w].name);
  else sayf("Penthouse! %s hat's mit dem wenigsten Unbehagen geschafft. Der Rest stinkt ab.", seat[w].name);
  fxAdd(11, 0, 0);
}

static void checkEnd() {
  if (phase != P_TRAVEL && phase != P_DOORS) return;
  int ha, a = aliveCount(&ha);
  if (a <= 1) { endRound(); return; }
  if (ha == 0 && humansSeated() > 0) {            // all humans are out: let the bots finish quickly
    if (!botEnd) botEnd = millis() + 4000;
    else if (millis() >= botEnd) endRound();
  } else botEnd = 0;
}

static void startTravel() {
  uint32_t now = millis();
  phase = P_TRAVEL; phaseEnd = now + TRAVEL_MS;
  evAt = rndi(0, 99) < 40 ? now + rndi(1500, 4200) : 0;
  evKind = rndi(1, 2);
}

static void openDoors() {
  uint32_t now = millis();
  phase = P_DOORS; phaseEnd = now + DOORS_MS;
  fxAdd(4, 0, 0);
  sayf("DING! Etage %d. Die Türen gehen auf.", floorNo);
  int left = 0;                                   // some NPCs get off
  for (int i = 0; i < NCELL && left < 2; i++)
    if (ent[i].on && ent[i].kind != K_PLAYER && rndi(0, 99) < 22) {
      sayf(pick(LEAVE), KNAME[ent[i].kind]);
      fxAdd(10, ent[i].x, ent[i].y);
      delEnt(i); left++;
    }
  boardLeft = 1 + rndi(0, 2) + (floorNo - 1) / 3;
  nextBoard = now + 900;
}

static void closeDoors() { fxAdd(5, 0, 0); startTravel(); }

static void boardOne() {
  int x, y;
  if (load() >= MAXLOAD || !pickFree(x, y, rndi(0, 99) < 55)) return;
  int k = rndi(1, K_COUNT - 1);
  addEnt(k, x, y, 0);
  sayf("%s", BOARD[k]);
  fxAdd(6, x, y);
}

static void shuffleCabin() {
  int es[NCELL], pos[NCELL], n = 0;
  for (int i = 0; i < NCELL; i++) if (ent[i].on) { es[n] = i; pos[n] = ent[i].y * GW + ent[i].x; n++; }
  for (int i = n - 1; i > 0; i--) { int j = esp_random() % (i + 1); int t = pos[i]; pos[i] = pos[j]; pos[j] = t; }
  for (int i = 0; i < n; i++) cell[ent[es[i]].y * GW + ent[es[i]].x] = -1;
  for (int i = 0; i < n; i++) { ent[es[i]].x = pos[i] % GW; ent[es[i]].y = pos[i] / GW; cell[pos[i]] = es[i]; }
}

static void doEvent() {
  if (evKind == 1) { shuffleCabin(); sayf("RUCK-ZUCK! Der Fahrstuhl ruckelt und alle wechseln unfreiwillig die Plätze."); fxAdd(8, 0, 0); }
  else { blackoutEnd = millis() + 4500; sayf("Stromausfall! Es ist stockdunkel. Man hört nur Furzen und Fluchen."); fxAdd(9, 0, 0); }
}

static void doFart(int s) {
  Seat &me = seat[s];
  uint32_t now = millis();
  if ((phase != P_TRAVEL && phase != P_DOORS) || !me.alive || now < me.cdF) return;
  me.cdF = now + CD_FART;
  Ent &p = ent[me.e];
  gasBurst(p.x, p.y);
  fxAdd(1, p.x, p.y);
  sayf(pick(FART), me.name);
}

static void doElbow(int s, int victim, int d) {
  Seat &me = seat[s];
  Ent &v = ent[victim];
  int bx = v.x + DX[d], by = v.y + DY[d];
  fxAdd(2, v.x, v.y);
  if (inb(bx, by) && cell[by * GW + bx] < 0) {
    sayf(pick(ELBOW), me.name, nameOf(victim));
    moveEnt(victim, bx, by);
  } else if (v.kind == K_PLAYER) {
    seat[v.who].dis = min(100.0f, seat[v.who].dis + 6);
    sayf(pick(JAM), seat[v.who].name);
  }
}

static void doMove(int s, int d) {
  Seat &me = seat[s];
  uint32_t now = millis();
  if ((phase != P_TRAVEL && phase != P_DOORS) || !me.alive || d < 0 || d > 7) return;
  Ent &p = ent[me.e];
  int nx = p.x + DX[d], ny = p.y + DY[d];
  if (!inb(nx, ny)) return;
  int t = cell[ny * GW + nx];
  if (t < 0) {
    if (now < me.cdM) return;
    me.cdM = now + CD_MOVE;
    moveEnt(me.e, nx, ny);
  } else {
    if (now < me.cdE) return;
    me.cdE = now + CD_ELBOW;
    doElbow(s, t, d);
  }
}

static void doDoor(int s) {
  Seat &me = seat[s];
  uint32_t now = millis();
  if (phase != P_DOORS || !me.alive || now < me.cdD) return;
  me.cdD = now + CD_DOOR;
  sayf(pick(CLOSE), me.name);
  closeDoors();
}

static void botAct(int s) {
  Seat &b = seat[s];
  uint32_t now = millis();
  Ent &p = ent[b.e];
  int nbr, c = p.y * GW + p.x;
  float cur = exposure(p.x, p.y, b.e, nbr) + 0.45f * gas[c];
  b.botAt = now + rndi(700, 1700);

  // 1. run away from the stench
  if (cur > 0.9f && now >= b.cdM) {
    int bestD = -1; float best = cur - 0.5f;
    for (int d = 0; d < 8; d++) {
      int nx = p.x + DX[d], ny = p.y + DY[d], n2;
      if (!inb(nx, ny) || cell[ny * GW + nx] >= 0) continue;
      float sc = exposure(nx, ny, b.e, n2) + 0.45f * gas[ny * GW + nx];
      if (sc < best) { best = sc; bestD = d; }
    }
    if (bestD >= 0) { doMove(s, bestD); return; }
  }
  // 2. elbow the worst neighbour away
  if (now >= b.cdE && nbr > 0 && rndi(0, 99) < 55) {
    int bd = -1; float bw = 0.9f;
    for (int d = 0; d < 8; d++) {
      int nx = p.x + DX[d], ny = p.y + DY[d];
      if (!inb(nx, ny)) continue;
      int e = cell[ny * GW + nx];
      if (e < 0) continue;
      int bx = nx + DX[d], by = ny + DY[d];
      float w = ANN[ent[e].kind] + (ent[e].kind == K_PLAYER ? rndi(0, 99) / 100.0f : 0);
      if (inb(bx, by) && cell[by * GW + bx] < 0 && w > bw) { bw = w; bd = d; }
    }
    if (bd >= 0) { doMove(s, bd); return; }
  }
  // 3. fart in company
  if (now >= b.cdF && nbr >= 2 && rndi(0, 99) < 32) { doFart(s); return; }
  // 4. smash the door-close button
  if (phase == P_DOORS && now >= b.cdD && boardLeft > 0 && rndi(0, 99) < 18) doDoor(s);
}

static void tick() {
  // NPC behaviour
  for (int i = 0; i < NCELL; i++) {
    if (!ent[i].on || ent[i].kind == K_PLAYER) continue;
    Ent &n = ent[i];
    if ((n.kind == K_KARL && rndi(0, 99) < 4) || (n.kind == K_JOGGER && rndi(0, 99) < 3)) {
      gasBurst(n.x, n.y); fxAdd(1, n.x, n.y); sayf(pick(NPCFART), KNAME[n.kind]);
    }
    if (n.kind == K_HUND && rndi(0, 99) < 10) {
      int v[8], m = 0;
      for (int k = 0; k < 8; k++) {
        int nx = n.x + DX[k], ny = n.y + DY[k];
        if (inb(nx, ny) && cell[ny * GW + nx] >= 0 && ent[cell[ny * GW + nx]].kind == K_PLAYER) v[m++] = cell[ny * GW + nx];
      }
      if (m) {
        Seat &s = seat[ent[v[esp_random() % m]].who];
        s.dis = min(100.0f, s.dis + 8); fxAdd(7, n.x, n.y); sayf(pick(BITE), s.name);
      }
    }
    if ((n.kind == K_HUND || n.kind == K_JOGGER) && rndi(0, 99) < 30) {
      int d = rndi(0, 7), nx = n.x + DX[d], ny = n.y + DY[d];
      if (inb(nx, ny) && cell[ny * GW + nx] < 0) moveEnt(i, nx, ny);
    }
  }
  // discomfort
  for (int i = 0; i < NSEAT; i++) {
    Seat &s = seat[i];
    if (!s.act || !s.alive) continue;
    Ent &p = ent[s.e];
    int nbr, c = p.y * GW + p.x;
    float d = exposure(p.x, p.y, s.e, nbr) + 0.45f * gas[c];
    d *= 0.7f;                                    // balance: a ride lasts the whole 9 floors for a careful player
    if (nbr == 0 && gas[c] == 0) d = -1.5f;
    s.rate = d;
    s.dis = constrain(s.dis + d, 0.0f, 100.0f);
  }
  for (int c = 0; c < NCELL; c++) if (gas[c]) gas[c]--;
  for (int i = 0; i < NSEAT; i++) if (seat[i].act && seat[i].alive && seat[i].dis >= 100) knockOut(i);
  checkEnd();
}

// ------------------------------------------------------------------ match ---
static void setBot(int i) { seat[i].human = false; strcpy(seat[i].name, BOTN[i]); }

static void notifyQueue() { for (int k = 0; k < qn; k++) sendYou(queue[k].client, -1, k + 1); }
static int queueIndex(uint8_t num) { for (int k = 0; k < qn; k++) if (queue[k].client == num) return k; return -1; }
static void enqueue(uint8_t num, const char *name) {
  if (qn >= 8 || queueIndex(num) >= 0) return;
  queue[qn].client = num; strncpy(queue[qn].name, name, 13); queue[qn].name[13] = 0; qn++;
}
static void dequeueAt(int k) { for (; k < qn - 1; k++) queue[k] = queue[k + 1]; qn--; }
static int seatOf(uint8_t num) {
  for (int i = 0; i < NSEAT; i++) if (seat[i].human && seat[i].client == num) return i;
  return -1;
}

static void startMatch() {
  uint32_t now = millis();
  while (qn > 0 && humansSeated() < NSEAT) {      // waiting visitors take free seats
    int s = 0; while (seat[s].human) s++;
    seat[s].human = true; seat[s].client = queue[0].client; strcpy(seat[s].name, queue[0].name);
    dequeueAt(0);
  }
  int nh = humansSeated(), bots = max(MINPLAYERS, nh) - nh;
  uint8_t faces[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  for (int i = 7; i > 0; i--) { int j = esp_random() % (i + 1); uint8_t t = faces[i]; faces[i] = faces[j]; faces[j] = t; }

  memset(cell, -1, sizeof(cell)); memset(gas, 0, sizeof(gas)); memset(ent, 0, sizeof(ent));
  outN = 0; winner = -1; floorNo = 1; blackoutEnd = 0; botEnd = 0; logN = 0; fxN = 0;
  for (int i = 0; i < NSEAT; i++) {
    Seat &s = seat[i];
    if (!s.human) { if (bots > 0) { bots--; setBot(i); s.act = true; } else s.act = false; }
    else s.act = true;
    s.alive = s.act; s.dis = 0; s.rate = 0; s.place = 0; s.e = -1; s.face = faces[i];
    s.cdF = s.cdD = s.cdM = s.cdE = 0; s.botAt = now + rndi(1500, 3000);
    if (s.act) { int x, y; pickFree(x, y, false); s.e = addEnt(K_PLAYER, x, y, i); }
  }
  for (int k = 0; k < 3; k++) { int x, y; if (pickFree(x, y, false)) addEnt(rndi(1, K_COUNT - 1), x, y, 0); }
  sayf("Neue Fahrt: Erdgeschoss bis Penthouse. Bitte tief einatmen. Besser: gar nicht.");
  lastTick = now;
  startTravel();
  for (int i = 0; i < NSEAT; i++) if (seat[i].human) sendYou(seat[i].client, i, 0);
  notifyQueue();
  broadcastState();
}

static void onWs(uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
  if (type == WStype_CONNECTED) {
    if (phase != P_WAIT) { int n = buildState(); ws3.sendTXT(num, buf, n); }
    return;
  }
  if (type == WStype_DISCONNECTED) {
    int q = queueIndex(num);
    if (q >= 0) { dequeueAt(q); notifyQueue(); }
    int i = seatOf(num);
    if (i < 0) return;
    setBot(i);                                    // a bot takes over the seat, discomfort and all
    if (humansSeated() == 0) { if (qn > 0) startMatch(); else phase = P_WAIT; }
    return;
  }
  if (type != WStype_TEXT || len == 0) return;
  char cmd = payload[0];
  int i = seatOf(num);

  if (cmd == 'j' && i < 0) {                      // join: j<name>
    char name[13]; int n = 0;
    for (size_t k = 1; k < len && n < 12; k++) {
      char c = payload[k];
      if (isalnum(c) || c == ' ' || c == '-' || c == '_') name[n++] = c;
    }
    name[n] = 0;
    if (n == 0) strcpy(name, "Passagier");
    if (queueIndex(num) < 0) enqueue(num, name);
    if (humansSeated() == 0 || phase == P_WAIT) { startMatch(); return; }
    int q = queueIndex(num);
    sendYou(num, -1, q < 0 ? 0 : q + 1);
    int m = buildState(); ws3.sendTXT(num, buf, m);
    return;
  }
  if (i < 0) return;
  if (cmd == 'm' && len > 1) doMove(i, payload[1] - '0');
  else if (cmd == 'f') doFart(i);
  else if (cmd == 'b') doDoor(i);
  else if (cmd == 'x' && len > 1 && millis() > seat[i].tauntAt) {
    seat[i].tauntAt = millis() + 1200;
    char m[48]; int n = snprintf(m, sizeof(m), "{\"k\":\"taunt\",\"by\":%d,\"n\":%d}", i, constrain(atoi((const char *)payload + 1), 0, 7));
    ws3.broadcastTXT(m, n);
  }
}

// ------------------------------------------------------------------ public ---
void klausBegin() {
  for (int i = 0; i < NSEAT; i++) setBot(i);
  memset(cell, -1, sizeof(cell));
  ws3.begin();
  ws3.onEvent(onWs);
  ws3.enableHeartbeat(3000, 2000, 2);
}

void klausLoop() {
  ws3.loop();
  if (phase == P_WAIT) return;
  uint32_t now = millis();
  if (phase == P_OVER) {
    if (now >= phaseEnd) {
      for (int sw = 0; sw < 2 && qn > 0 && humansSeated() >= NSEAT; sw++) {     // king of the hill: first one out makes room
        int worst = -1;
        for (int i = 0; i < NSEAT; i++) if (seat[i].human && (worst < 0 || seat[i].place < seat[worst].place)) worst = i;
        enqueue(seat[worst].client, seat[worst].name);
        seat[worst].human = false;
        // enqueue() put the old player at the back; the new one takes his seat in startMatch()
      }
      startMatch();
    }
  } else {
    if (now - lastTick >= TICK_MS) { lastTick += TICK_MS; if (now - lastTick > 3 * TICK_MS) lastTick = now; tick(); }
    for (int i = 0; i < NSEAT && (phase == P_TRAVEL || phase == P_DOORS); i++)
      if (seat[i].act && !seat[i].human && seat[i].alive && now >= seat[i].botAt) botAct(i);
    if (phase == P_TRAVEL) {
      if (evAt && now >= evAt) { evAt = 0; doEvent(); }
      if (now >= phaseEnd) {
        floorNo++;
        if (floorNo >= FLOORS) { fxAdd(4, 0, 0); endRound(); }
        else openDoors();
      }
    } else if (phase == P_DOORS) {
      if (boardLeft > 0 && now >= nextBoard) { boardOne(); boardLeft--; nextBoard = now + 700; }
      if (now >= phaseEnd) closeDoors();
    }
  }
  if (now - lastSnap >= SNAP_MS) {
    lastSnap = now;
    if (ws3.connectedClients() > 0) broadcastState(); else { logN = 0; fxN = 0; }
  }
}

const char *klausPage() { return KLAUS_PAGE; }
int klausHumans() { return humansSeated(); }
int klausQueue() { return qn; }
