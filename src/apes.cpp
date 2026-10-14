// ============================================================================
//  AFFENSCHLACHT – turn-based two-player poop throwing, hosted on the ESP32
//
//  Two apes, a wall in the middle, wind, craters, power-up crates.
//  The server is authoritative: it simulates every throw, then sends the
//  recorded flight paths ("shot") and afterwards the resulting world ("st").
//  With only one human the second seat is played by a bot.
//  WebSocket on port 82, page at /apes.
// ============================================================================
#include <Arduino.h>
#include <WebSocketsServer.h>
#include <stdarg.h>
#include "apes.h"
#include "apes_page.h"

static WebSocketsServer ws2(82);

// ---------------------------------------------------------------- tuning ---
static const int   W = 800, CW = 4, NC = W / CW;    // world width, column width, columns
static const int   WALL_A = 96, WALL_B = 103;       // columns of the wall in the middle
static const float GRAV = 450, VMAX = 700;          // px/s^2, px/s at 100 % power
static const int   APE_R = 22, APE_DY = 24;         // hit radius, body centre above feet
static const float CRATER = 30, CRATER_MEGA = 58;
static const int   CRATE_R = 28;
static const int   MAXPTS = 170, MAXSTEPS = 500;    // path points (every 3rd step), 60 Hz steps
static const uint32_t TURN_MS = 30000, OVER_MS = 9000;

enum Phase { P_WAIT, P_AIM, P_FLY, P_OVER };
enum Pu { PU_NONE, PU_MEGA, PU_TRIPLE, PU_BOUNCE, PU_NOWIND, PU_HEAL, PU_SWAP, PU_SHIELD, PU_CONSTIP, PU_COUNT };

static const char *PU_NAME[] = {"", "Mega-Haufen", "Dünnschiss-Salve", "Gummi-Kacke", "Furz-Antrieb",
                                "Heilbanane", "Platztausch", "Klopapier-Schild", "Verstopfung"};
static const char *PU_DESC[] = {"", "Riesenkrater, mehr Schaden.", "Drei Kacken auf einmal.", "Prallt einmal ab.",
                                "Wind? Interessiert den Furz nicht.", "+35 Leben, schmeckt nach Rettung.",
                                "Beide Affen tauschen die Plätze.", "Der nächste Treffer wird halbiert.",
                                "Der Gegner kann nur noch mit halber Kraft werfen."};

struct Seat {
  bool human; uint8_t client; char name[14];
  int hp; float x, y; int pu; bool shield, cap; uint32_t tauntAt;
};
struct Crate { bool on; int x, y; };
struct Shot { int n; int16_t px[MAXPTS], py[MAXPTS]; bool impact; int ex, ey; uint8_t crates; };

static Seat seat[2];
static Crate crate[2];
static uint16_t hh[NC];                  // terrain height per column (y up from the bottom)
static Shot shots[3];
static int phase = P_WAIT, turn = 0, wind = 0, winner = -1;
static uint32_t turnEnd, animEnd, overEnd, botAt;
static char buf[6400];
static char logLine[5][110];
static int logN = 0;

// --------------------------------------------------------------- helpers ---
static float rnd(float a, float b) { return a + (b - a) * (esp_random() / 4294967295.0f); }
static int   rndi(int a, int b) { return a + (int)(esp_random() % (uint32_t)(b - a + 1)); }
static int   colOf(float x) { int c = (int)(x / CW); return c < 0 ? 0 : (c >= NC ? NC - 1 : c); }

static void say(const char *fmt, ...) {
  if (logN >= 5) return;
  va_list ap; va_start(ap, fmt); vsnprintf(logLine[logN++], sizeof(logLine[0]), fmt, ap); va_end(ap);
}
template <size_t N> static const char *pick(const char *(&a)[N]) { return a[esp_random() % N]; }

// args: thrower, victim, damage
static const char *HIT[] = {
  "%s trifft %s voll ins Gesicht. Ein Schrei, ein Gestank: -%d",
  "Volltreffer! %s hat %s mit frischer Ware erwischt (-%d)",
  "%s tauft %s im Haufen-Becken. -%d",
  "Das war kein Bananensplit! %s nagelt %s: -%d",
  "%s serviert %s ein warmes Geschenk. -%d"};
static const char *SPLASH[] = {
  "%s streift %s nur. Immerhin, es stinkt. -%d",
  "Spritzer! %s sprenkelt %s ein bisschen an. -%d",
  "%s trifft %s mit den Resten. Eklig, aber nicht tödlich. -%d"};
static const char *MISS[] = {
  "%s wirft daneben. Die Kacke landet im Nirgendwo.",
  "%s verfehlt alles. Die Mauer lacht.",
  "Platsch! %s hat nur den Boden gedüngt.",
  "%s zielt wie ein Weltmeister, trifft wie ein Affe."};
static const char *SELF[] = {
  "%s hat sich selbst getroffen. Evolution rückwärts. -%d",
  "Eigentor! %s wirft sich die Kacke selbst ins Gesicht. -%d"};

// ---------------------------------------------------------------- world ---
static void genTerrain() {
  float p1 = rnd(0, 6.28f), p2 = rnd(0, 6.28f), a1 = rnd(14, 30), a2 = rnd(6, 14);
  int wallH = 230 + rndi(0, 40);
  for (int c = 0; c < NC; c++) {
    float x = c * CW;
    float h = 95 + a1 * sinf(x * 0.011f + p1) + a2 * sinf(x * 0.029f + p2);
    if (h < 40) h = 40;
    if (h > 170) h = 170;
    hh[c] = (uint16_t)h;
    if (c >= WALL_A && c <= WALL_B) hh[c] = wallH;
  }
}

static void ground() {
  for (int i = 0; i < 2; i++) seat[i].y = hh[colOf(seat[i].x)];
}

static void carve(float cx, float cy, float R) {
  int c0 = (int)((cx - R) / CW), c1 = (int)((cx + R) / CW);
  if (c0 < 0) c0 = 0;
  if (c1 > NC - 1) c1 = NC - 1;
  for (int c = c0; c <= c1; c++) {
    float dx = c * CW + CW / 2 - cx;
    if (fabsf(dx) > R) continue;
    float low = cy - sqrtf(R * R - dx * dx);
    if (low < 0) low = 0;
    if (hh[c] > low) hh[c] = (uint16_t)low;
  }
}

static void spawnCrates() {
  for (int k = 0; k < 2; k++) {
    if (crate[k].on || rndi(0, 99) >= 60) continue;
    int x;
    do { x = rndi(130, 670); } while (x > 340 && x < 460);
    crate[k].on = true; crate[k].x = x; crate[k].y = hh[colOf(x)] + rndi(60, 190);
  }
}

// ------------------------------------------------------------ simulation ---
// One throw. Records every 3rd step, stops at the first impact or when it leaves the world.
static void simShot(Shot &s, int by, float ang, float pw, bool bounce, float wnd, bool rec) {
  const float dt = 1.0f / 60;
  float dir = by == 0 ? 1 : -1, rad = ang * 0.0174533f, v = pw / 100.0f * VMAX;
  float x = seat[by].x + dir * 18, y = seat[by].y + 52;
  float vx = dir * v * cosf(rad), vy = v * sinf(rad);
  bool bounced = false;
  s.n = 0; s.impact = false; s.crates = 0; s.ex = (int)x; s.ey = (int)y;
  for (int step = 0; step < MAXSTEPS; step++) {
    vx += wnd * dt; vy -= GRAV * dt; x += vx * dt; y += vy * dt;
    if (x < -40 || x > W + 40 || y < -60) break;                       // flew away
    for (int k = 0; k < 2; k++)
      if (crate[k].on && hypotf(x - crate[k].x, y - crate[k].y) < CRATE_R) s.crates |= 1 << k;
    bool hit = false;
    for (int a = 0; a < 2 && !hit; a++)
      if ((step > 12 || a != by) && hypotf(x - seat[a].x, y - (seat[a].y + APE_DY)) < APE_R + 5) hit = true;
    if (!hit && x >= 0 && x < W) {
      float g = hh[colOf(x)];
      if (y <= g) {
        if (bounce && !bounced) {                                      // rubber poop: one bounce
          bounced = true; y = g + 1; vy = fabsf(vy) * 0.55f; vx *= 0.7f;
          if (rec && s.n < MAXPTS) { s.px[s.n] = (int16_t)x; s.py[s.n] = (int16_t)y; s.n++; }
          continue;
        }
        y = g; hit = true;
      }
    }
    if (hit) {
      s.impact = true; s.ex = (int)roundf(x); s.ey = (int)roundf(y);
      if (rec && s.n < MAXPTS) { s.px[s.n] = s.ex; s.py[s.n] = s.ey; s.n++; }
      return;
    }
    if (rec && step % 3 == 0 && s.n < MAXPTS) { s.px[s.n] = (int16_t)x; s.py[s.n] = (int16_t)y; s.n++; }
  }
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
  int t = (phase == P_AIM && seat[turn].human && turnEnd > now) ? (turnEnd - now) / 1000 : 0;
  int n = put(0, "{\"k\":\"st\",\"ph\":%d,\"turn\":%d,\"wind\":%d,\"t\":%d,\"win\":%d,"
                 "\"n\":[\"%s\",\"%s\"],\"b\":[%d,%d],\"hp\":[%d,%d],\"x\":[%d,%d],\"y\":[%d,%d],"
                 "\"pu\":[%d,%d],\"sh\":[%d,%d],\"cp\":[%d,%d],\"cr\":[[%d,%d],[%d,%d]],\"log\":[",
              phase, turn, wind, t, winner, seat[0].name, seat[1].name,
              !seat[0].human, !seat[1].human, seat[0].hp, seat[1].hp,
              (int)seat[0].x, (int)seat[1].x, (int)seat[0].y, (int)seat[1].y,
              seat[0].pu, seat[1].pu, seat[0].shield, seat[1].shield, seat[0].cap, seat[1].cap,
              crate[0].on ? crate[0].x : -1, crate[0].on ? crate[0].y : -1,
              crate[1].on ? crate[1].x : -1, crate[1].on ? crate[1].y : -1);
  for (int i = 0; i < logN; i++) n = put(n, "%s\"%s\"", i ? "," : "", logLine[i]);
  n = put(n, "],\"h\":[");
  for (int c = 0; c < NC; c++) n = put(n, "%s%d", c ? "," : "", hh[c]);
  return put(n, "]}");
}

static void broadcastState() {
  int n = buildState();
  ws2.broadcastTXT(buf, n);
  logN = 0;
}

static void sendTaunt(int by, int kind) {
  char m[48]; int n = snprintf(m, sizeof(m), "{\"k\":\"taunt\",\"by\":%d,\"n\":%d}", by, kind);
  ws2.broadcastTXT(m, n);
}

// ------------------------------------------------------------ game logic ---
static void startTurn() {
  uint32_t now = millis();
  wind = rndi(-45, 45);
  spawnCrates();
  phase = P_AIM;
  turnEnd = now + TURN_MS;
  botAt = now + 1600 + rndi(0, 1400);
}

static void startMatch() {
  genTerrain();
  seat[0].x = 70 + rndi(0, 50); seat[1].x = 730 - rndi(0, 50);
  for (int i = 0; i < 2; i++) {
    seat[i].hp = 100; seat[i].pu = 0; seat[i].shield = seat[i].cap = false;
  }
  crate[0].on = crate[1].on = false;
  ground();
  winner = -1; turn = rndi(0, 1);
  logN = 0;
  say("Neue Runde: %s gegen %s. Möge der Stinkendere gewinnen.", seat[0].name, seat[1].name);
  startTurn();
  broadcastState();
}

static void awardPu(int by) {
  int p = 1 + esp_random() % (PU_COUNT - 1);
  Seat &me = seat[by];
  switch (p) {
    case PU_HEAL:    me.hp = min(100, me.hp + 35); break;
    case PU_SWAP:    { float t = seat[0].x; seat[0].x = seat[1].x; seat[1].x = t; ground(); break; }
    case PU_SHIELD:  me.shield = true; break;
    case PU_CONSTIP: seat[1 - by].cap = true; break;
    default:         me.pu = p;
  }
  say("%s sammelt eine Kiste: %s! %s", me.name, PU_NAME[p], PU_DESC[p]);
}

static void doThrow(int by, float ang, float pw, bool blind) {
  Seat &me = seat[by];
  int en = 1 - by;
  int kind = me.pu; me.pu = 0;
  float wnd = kind == PU_NOWIND ? 0 : wind;
  if (me.cap) { pw = min(pw, 50.0f); me.cap = false; }
  int cnt = kind == PU_TRIPLE ? 3 : 1;
  float R = kind == PU_MEGA ? CRATER_MEGA : CRATER;
  float base = kind == PU_MEGA ? 52 : (kind == PU_TRIPLE ? 27 : 38);

  if (blind) say("%s hat zu lange nachgedacht und wirft blind.", me.name);
  int maxN = 0; uint8_t got = 0;
  for (int i = 0; i < cnt; i++) {
    float a = ang + (cnt == 3 ? (i - 1) * 6.0f : 0), p = pw * (cnt == 3 ? 1.0f + (i - 1) * 0.04f : 1.0f);
    simShot(shots[i], by, a, p, kind == PU_BOUNCE, wnd, true);
    maxN = max(maxN, shots[i].n);
    got |= shots[i].crates;
  }

  // flight paths go out first, the resulting world follows when the animation is over
  int n = put(0, "{\"k\":\"shot\",\"by\":%d,\"kind\":%d,\"p\":[", by, kind);
  for (int i = 0; i < cnt; i++) {
    n = put(n, "%s[", i ? "," : "");
    for (int k = 0; k < shots[i].n; k++) n = put(n, "%s%d,%d", k ? "," : "", shots[i].px[k], shots[i].py[k]);
    n = put(n, "]");
  }
  n = put(n, "],\"ex\":[");
  for (int i = 0; i < cnt; i++) {
    if (shots[i].impact) n = put(n, "%s[%d,%d,%d]", i ? "," : "", shots[i].ex, shots[i].ey, (int)R);
    else n = put(n, "%s[]", i ? "," : "");
  }
  n = put(n, "]}");
  ws2.broadcastTXT(buf, n);

  // damage at the pre-crater positions
  int dmg[2] = {0, 0};
  for (int i = 0; i < cnt; i++) {
    if (!shots[i].impact) continue;
    for (int a = 0; a < 2; a++) {
      float d = hypotf(shots[i].ex - seat[a].x, shots[i].ey - (seat[a].y + APE_DY)), lim = R + APE_R;
      if (d < lim) dmg[a] += (int)(base * (1 - 0.75f * d / lim) + 0.5f);
    }
  }
  for (int i = 0; i < cnt; i++) if (shots[i].impact) carve(shots[i].ex, shots[i].ey, R);
  for (int a = 0; a < 2; a++) {
    if (dmg[a] > 0 && seat[a].shield) {
      dmg[a] = (dmg[a] + 1) / 2; seat[a].shield = false;
      say("%ss Klopapier-Schild fängt die Hälfte ab.", seat[a].name);
    }
    seat[a].hp = max(0, seat[a].hp - dmg[a]);
  }
  ground();

  if (dmg[en] > 0 && by != en) say(dmg[en] >= 30 ? pick(HIT) : pick(SPLASH), me.name, seat[en].name, dmg[en]);
  if (dmg[by] > 0) say(pick(SELF), me.name, dmg[by]);
  if (!dmg[0] && !dmg[1]) say(pick(MISS), me.name);

  for (int k = 0; k < 2; k++)
    if ((got & (1 << k)) && crate[k].on) { crate[k].on = false; awardPu(by); }

  phase = P_FLY;
  animEnd = millis() + maxN * 50 + 1600;
  if (!seat[by].human && rndi(0, 3) == 0) sendTaunt(by, rndi(0, 2));
}

static void botThrow() {
  Seat &me = seat[turn], &en = seat[1 - turn];
  float wnd = me.pu == PU_NOWIND ? 0 : wind, maxP = me.cap ? 50 : 100;
  float bestA = 45, bestP = 60, bestD = 1e9;
  Shot tmp;
  for (int i = 0; i < 25; i++) {
    float a = rnd(25, 80), p = rnd(30, maxP);
    simShot(tmp, turn, a, p, me.pu == PU_BOUNCE, wnd, false);
    float d = tmp.impact ? hypotf(tmp.ex - en.x, tmp.ey - (en.y + APE_DY)) : 1e9f;
    if (d < bestD) { bestD = d; bestA = a; bestP = p; }
  }
  doThrow(turn, constrain(bestA + rnd(-7, 7), 2, 88), constrain(bestP + rnd(-10, 10), 5, maxP), false);
}

static void endFly() {
  bool d0 = seat[0].hp <= 0, d1 = seat[1].hp <= 0;
  if (d0 || d1) {
    phase = P_OVER; overEnd = millis() + OVER_MS;
    winner = (d0 && d1) ? 2 : (d0 ? 1 : 0);
    if (winner == 2) say("Unentschieden: beide stinken gleich schlimm.");
    else say("%s gewinnt! %s muss jetzt duschen.", seat[winner].name, seat[1 - winner].name);
  } else {
    turn = 1 - turn;
    startTurn();
  }
  broadcastState();
}

// ---------------------------------------------------------------- network ---
static int seatOf(uint8_t num) {
  for (int i = 0; i < 2; i++) if (seat[i].human && seat[i].client == num) return i;
  return -1;
}

static void setBot(int i) {
  seat[i].human = false;
  strcpy(seat[i].name, i ? "Affe 2.0" : "Gorilla-KI");
}

static void onWs(uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
  if (type == WStype_CONNECTED) {
    if (phase != P_WAIT) { int n = buildState(); ws2.sendTXT(num, buf, n); }
    return;
  }
  if (type == WStype_DISCONNECTED) {
    int i = seatOf(num);
    if (i < 0) return;
    setBot(i);
    if (seat[1 - i].human) startMatch();       // the one left plays against the bot
    else { phase = P_WAIT; }
    return;
  }
  if (type != WStype_TEXT || len == 0) return;
  char cmd = payload[0];
  int i = seatOf(num);

  if (cmd == 'j' && i < 0) {                   // join: j<name>
    char name[13]; int n = 0;
    for (size_t k = 1; k < len && n < 12; k++) {
      char c = payload[k];
      if (isalnum(c) || c == ' ' || c == '-' || c == '_') name[n++] = c;
    }
    name[n] = 0;
    if (n == 0) strcpy(name, "Affe");
    int s = !seat[0].human ? 0 : (!seat[1].human ? 1 : -1);
    char msg[24]; snprintf(msg, sizeof(msg), "{\"k\":\"you\",\"i\":%d}", s);
    ws2.sendTXT(num, msg);
    if (s < 0) { int m = buildState(); ws2.sendTXT(num, buf, m); return; }   // full: spectator
    seat[s].human = true; seat[s].client = num; strcpy(seat[s].name, name);
    if (!seat[1 - s].human) setBot(1 - s);
    startMatch();
    return;
  }
  if (i < 0) return;
  if (cmd == 't' && phase == P_AIM && turn == i) {          // throw: t<angle>,<power>
    int a = 45, p = 60;
    char b[24]; size_t n = min(len - 1, sizeof(b) - 1);
    memcpy(b, payload + 1, n); b[n] = 0;
    sscanf(b, "%d,%d", &a, &p);
    doThrow(i, constrain(a, 2, 88), constrain(p, 5, 100), false);
  } else if (cmd == 'x' && len > 1 && millis() > seat[i].tauntAt) {   // taunt: x<0..2>
    seat[i].tauntAt = millis() + 1200;
    sendTaunt(i, constrain(payload[1] - '0', 0, 2));
  }
}

// ------------------------------------------------------------------ public ---
void apesBegin() {
  setBot(0); setBot(1);
  ws2.begin();
  ws2.onEvent(onWs);
  ws2.enableHeartbeat(3000, 2000, 2);
}

void apesLoop() {
  ws2.loop();
  if (phase == P_WAIT) return;
  uint32_t now = millis();
  if (phase == P_AIM) {
    if (seat[turn].human) { if (now >= turnEnd) doThrow(turn, rnd(30, 70), rnd(30, 80), true); }
    else if (now >= botAt) botThrow();
  } else if (phase == P_FLY) {
    if (now >= animEnd) endFly();
  } else if (phase == P_OVER) {
    if (now >= overEnd) startMatch();
  }
}

const char *apesPage() { return APES_PAGE; }
