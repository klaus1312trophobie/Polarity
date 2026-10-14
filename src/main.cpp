// ============================================================================
//  POLARITY – a magnetic multiplayer arena game, hosted entirely on an ESP32
//
//  Every player is a magnet. Equal polarities repel, opposite ones attract –
//  for the energy orbs AND for the other players. Flip your polarity to pull
//  orbs in (+1) and to shove rivals off the shrinking arena (+6).
//
//  HTTP (port 80) serves the page, a WebSocket (port 81) carries the game.
//  The server is authoritative: it runs physics at 30 Hz, sends snapshots
//  at 15 Hz and fills empty seats with bots.
// ============================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include "config.h"
#include "webpage.h"
#include "apes.h"

// ---------------------------------------------------------------- tuning ---
static const float ARENA_R        = 400;   // arena radius at round start
static const float ARENA_R_MIN    = 240;   // radius at the very end
static const float SHRINK_TIME    = 30;    // last N seconds the arena shrinks
static const float ROUND_TIME     = 120;
static const float INTERMISSION   = 9;
static const float PLAYER_R       = 18;
static const float ORB_R          = 8;
static const float MAG_RANGE      = 280;   // player <-> player
static const float ORB_RANGE      = 180;   // player -> orb
static const float K_PLAYER       = 2.8e6; // player <-> player strength
static const float K_ORB          = 1.2e6; // player -> orb strength
static const float ACCEL          = 950;
static const float MAX_SPEED      = 620;
static const float FRICTION       = 2.3;
static const float FLIP_COOLDOWN  = 0.3;
static const float SURGE_TIME     = 1.2;
static const float SURGE_COOLDOWN = 5.0;
static const float SURGE_MULT     = 3.0;
static const float RESPAWN_TIME   = 2.0;
static const float CREDIT_TIME    = 3.0;   // knockout credit window
static const int   MAX_SLOTS      = 10;
static const int   MAX_ORBS       = 10;
static const float ORB_RESPAWN    = 2.0;
static const int   KO_POINTS      = 6;     // shoving someone off
static const int   FALL_PENALTY   = 3;
static const int   MIN_FIELD      = 4;     // humans + bots
static const int   LED_PIN        = 2;
static const int   BOOT_PIN       = 0;     // hold at power-up to force hotspot mode
static const int   AP_MAX_CLIENTS = 8;
// power-ups
static const float POWERUP_LIFE   = 12;    // seconds an untouched power-up stays
static const float POWERUP_GAP_MIN = 5, POWERUP_GAP_MAX = 10;
static const float POWERUP_R      = 14;
static const float EFFECT_TIME    = 6;
static const float TURBO_ACCEL    = 1.6;
static const float TURBO_SPEED    = 1.5;
static const float ANCHOR_MASS    = 4;     // anchored players are heavy and ignore magnets
static const float MAGNETAR_RANGE = 1.6;   // orb range multiplier
static const float SHOCK_RANGE    = 260;
static const float SHOCK_PUSH     = 950;
static const float GOLD_CHANCE    = 0.05;  // chance a respawned orb is golden
static const int   GOLD_POINTS    = 5;

enum Effect : uint8_t { FX_NONE, FX_TURBO, FX_ANCHOR, FX_MAGNETAR, FX_SHOCK };

static const char *BOT_NAMES[] = {"Volt", "Tesla", "Gauss", "Faraday", "Ohm", "Ampere"};

// ----------------------------------------------------------------- state ---
struct Player {
  bool used, bot, alive;
  int8_t client;            // websocket client number, -1 for bots
  char name[13];
  float x, y, vx, vy, ix, iy;
  int8_t pol;
  int score;
  float respawnT, flipCd, surgeT, surgeCd;
  int lastHitBy;
  float lastHitT;
  uint8_t fx;               // active power-up effect
  float fxT;
  // bot brain
  float thinkT, flipDelay;
  int8_t wantPol;
};

struct Orb { float x, y, vx, vy; int8_t pol; bool gold; float hideT; };   // hideT > 0: collected, respawning

struct PowerUp { uint8_t type; float x, y, t; };   // type FX_NONE: nothing on the field

static Player players[MAX_SLOTS];
static Orb orbs[MAX_ORBS];
static int orbCount = 0;
static PowerUp powerup = {FX_NONE, 0, 0, 0};
static float powerupGap = 0;

static float gameTime = 0;           // seconds since boot (sim clock)
static float timeLeft = ROUND_TIME;
static float arenaR = ARENA_R;
static bool  intermission = false;
static float interT = 0;
static bool  needReset = true;
static int   lastWinner = -1;
static char  lastWinnerName[13] = "";
static int   lastWinnerScore = 0;

static bool botsEnabled = true;   // off = duel mode, humans only (stored in NVS)

static int  bestScore = 0;
static char bestName[13] = "-";

// events for the next snapshot: [type, a, b]
//   1 = a collected an orb (b = points), 2 = a fell (b = who pushed, -1 none),
//   3 = round over (a = winner), 4 = a grabbed power-up b, 5 = a switched bots on/off (b = 1/0)
static int events[24][3];
static int eventCount = 0;

static float ledT = 0;

WebServer http(80);
WebSocketsServer ws(81);
WiFiUDP dnsUdp;
Preferences prefs;
static bool hotspot = false;   // running our own access point instead of home WiFi

// --------------------------------------------------------------- helpers ---
static float frand(float a, float b) { return a + (b - a) * (esp_random() / 4294967295.0f); }

static void pushEvent(int type, int a, int b) {
  if (eventCount < 24) { events[eventCount][0] = type; events[eventCount][1] = a; events[eventCount][2] = b; eventCount++; }
}

static int humanCount() {
  int n = 0;
  for (auto &p : players) if (p.used && !p.bot) n++;
  return n;
}

static int playerCount() {
  int n = 0;
  for (auto &p : players) if (p.used) n++;
  return n;
}

static void spawnPlayer(Player &p) {
  for (int attempt = 0; attempt < 12; attempt++) {
    float a = frand(0, TWO_PI), r = sqrtf(frand(0, 1)) * arenaR * 0.6f;
    p.x = cosf(a) * r; p.y = sinf(a) * r;
    bool clear = true;
    for (auto &o : players)
      if (&o != &p && o.used && o.alive && hypotf(o.x - p.x, o.y - p.y) < 90) clear = false;
    if (clear) break;
  }
  p.vx = p.vy = 0;
  p.alive = true;
  p.lastHitBy = -1;
}

static void spawnOrb(Orb &o) {
  float a = frand(0, TWO_PI), r = sqrtf(frand(0, 1)) * arenaR * 0.85f;
  o.x = cosf(a) * r; o.y = sinf(a) * r;
  o.vx = o.vy = 0;
  o.pol = (esp_random() & 1) ? 1 : -1;
  o.gold = frand(0, 1) < GOLD_CHANCE;
  o.hideT = 0;
}

static void resetPlayer(Player &p) {
  p.score = 0;
  p.pol = (esp_random() & 1) ? 1 : -1;
  p.ix = p.iy = 0;
  p.flipCd = p.surgeT = p.surgeCd = 0;
  p.fx = FX_NONE; p.fxT = 0;
  p.thinkT = 0; p.flipDelay = 0; p.wantPol = p.pol;
  spawnPlayer(p);
}

static void startRound() {
  timeLeft = ROUND_TIME;
  arenaR = ARENA_R;
  intermission = false;
  for (auto &p : players) if (p.used) resetPlayer(p);
  orbCount = min(MAX_ORBS, 3 + playerCount() / 2);
  for (int i = 0; i < orbCount; i++) spawnOrb(orbs[i]);
  powerup.type = FX_NONE;
  powerupGap = frand(3, 6);
}

static int addPlayer(bool bot, int client, const char *name) {
  for (int i = 0; i < MAX_SLOTS; i++) {
    Player &p = players[i];
    if (p.used) continue;
    memset(&p, 0, sizeof(p));
    p.used = true; p.bot = bot; p.client = client;
    strncpy(p.name, name, 12);
    resetPlayer(p);
    if (intermission) p.alive = false;
    int oldCount = orbCount;
    orbCount = min(MAX_ORBS, max(orbCount, 3 + playerCount() / 2));
    for (int k = oldCount; k < orbCount; k++) spawnOrb(orbs[k]);
    return i;
  }
  return -1;
}

static void balanceBots() {
  int humans = humanCount();
  int want = humans == 0 || !botsEnabled ? 0 : max(0, MIN_FIELD - humans);
  int bots = 0;
  for (auto &p : players) if (p.used && p.bot) bots++;
  while (bots > want) {
    for (int i = MAX_SLOTS - 1; i >= 0; i--) if (players[i].used && players[i].bot) { players[i].used = false; break; }
    bots--;
  }
  while (bots < want && playerCount() < MAX_SLOTS) {
    // pick a bot name not in use
    const char *nm = "Bot";
    for (auto *cand : BOT_NAMES) {
      bool taken = false;
      for (auto &p : players) if (p.used && !strcmp(p.name, cand)) taken = true;
      if (!taken) { nm = cand; break; }
    }
    addPlayer(true, -1, nm);
    bots++;
  }
}

// -------------------------------------------------------------- bot brain ---
static void thinkBot(int idx, float dt) {
  Player &b = players[idx];
  if (!b.alive) return;

  // delayed polarity flips make bots beatable
  if (b.wantPol != b.pol) {
    b.flipDelay -= dt;
    if (b.flipDelay <= 0 && b.flipCd <= 0) { b.pol = b.wantPol; b.flipCd = FLIP_COOLDOWN; }
  }

  b.thinkT -= dt;
  if (b.thinkT > 0) return;
  b.thinkT = frand(0.2f, 0.4f);

  float myR = hypotf(b.x, b.y);
  float tx = 0, ty = 0;
  int8_t want = b.pol;

  if (myR > arenaR * 0.72f) {
    // too close to the edge: get back to the middle
    tx = -b.x; ty = -b.y;
  } else {
    // go for the nearest orb and attract it
    float best = 1e9; int bo = -1;
    for (int i = 0; i < orbCount; i++) {
      if (orbs[i].hideT > 0) continue;
      float d = hypotf(orbs[i].x - b.x, orbs[i].y - b.y);
      if (d < best) { best = d; bo = i; }
    }
    if (bo >= 0) { tx = orbs[bo].x - b.x; ty = orbs[bo].y - b.y; want = -orbs[bo].pol; }

    // power-ups beat orbs unless they're much farther away
    if (powerup.type != FX_NONE) {
      float d = hypotf(powerup.x - b.x, powerup.y - b.y);
      if (d < 220 || d < best * 1.3f) { tx = powerup.x - b.x; ty = powerup.y - b.y; }
    }

    // a rival nearer the edge than us? repel them outward
    for (int i = 0; i < MAX_SLOTS; i++) {
      Player &e = players[i];
      if (i == idx || !e.used || !e.alive) continue;
      float d = hypotf(e.x - b.x, e.y - b.y);
      float eR = hypotf(e.x, e.y);
      if (d < 170 && eR > myR + 20 && eR > arenaR * 0.55f) {
        want = e.pol;
        tx = e.x - b.x; ty = e.y - b.y;
        if (b.surgeCd <= 0 && frand(0, 1) < 0.35f) { b.surgeT = SURGE_TIME; b.surgeCd = SURGE_COOLDOWN; }
        break;
      }
    }
  }

  if (want != b.wantPol) { b.wantPol = want; b.flipDelay = frand(0.25f, 0.7f); }
  float len = hypotf(tx, ty);
  if (len > 1) { b.ix = tx / len * 0.85f; b.iy = ty / len * 0.85f; }
  else { b.ix = b.iy = 0; }
}

// ---------------------------------------------------------------- physics ---
static void endRound() {
  intermission = true;
  interT = INTERMISSION;
  lastWinner = -1;
  int top = -1;
  for (int i = 0; i < MAX_SLOTS; i++)
    if (players[i].used && players[i].score > top) { top = players[i].score; lastWinner = i; }
  if (lastWinner >= 0) {
    strncpy(lastWinnerName, players[lastWinner].name, 12);
    lastWinnerScore = top;
    if (!players[lastWinner].bot && top > bestScore) {
      bestScore = top;
      strncpy(bestName, players[lastWinner].name, 12);
      prefs.putInt("best", bestScore);
      prefs.putString("bestName", bestName);
    }
  }
  pushEvent(3, lastWinner, top);
}

static void shockwave(int src) {
  Player &s = players[src];
  for (int j = 0; j < MAX_SLOTS; j++) {
    Player &p = players[j];
    if (j == src || !p.used || !p.alive) continue;
    float dx = p.x - s.x, dy = p.y - s.y, d = hypotf(dx, dy);
    if (d > SHOCK_RANGE || d < 0.01f) continue;
    float push = (SHOCK_PUSH * (1 - d / SHOCK_RANGE) + 250) / (p.fx == FX_ANCHOR ? ANCHOR_MASS : 1);
    p.vx += dx / d * push; p.vy += dy / d * push;
    p.lastHitBy = src; p.lastHitT = gameTime;
  }
  for (int k = 0; k < orbCount; k++) {
    Orb &o = orbs[k];
    float dx = o.x - s.x, dy = o.y - s.y, d = hypotf(dx, dy);
    if (o.hideT > 0 || d > SHOCK_RANGE || d < 0.01f) continue;
    o.vx += dx / d * 500; o.vy += dy / d * 500;
  }
}

static void updatePowerup(float dt) {
  if (powerup.type == FX_NONE) {
    powerupGap -= dt;
    if (powerupGap > 0) return;
    float a = frand(0, TWO_PI), r = sqrtf(frand(0, 1)) * arenaR * 0.65f;
    powerup = {(uint8_t)(FX_TURBO + esp_random() % 4), cosf(a) * r, sinf(a) * r, POWERUP_LIFE};
    return;
  }
  powerup.t -= dt;
  bool gone = powerup.t <= 0 || hypotf(powerup.x, powerup.y) > arenaR - POWERUP_R;
  for (int i = 0; i < MAX_SLOTS && !gone; i++) {
    Player &p = players[i];
    if (!p.used || !p.alive || hypotf(p.x - powerup.x, p.y - powerup.y) > PLAYER_R + POWERUP_R) continue;
    if (powerup.type == FX_SHOCK) shockwave(i);
    else { p.fx = powerup.type; p.fxT = EFFECT_TIME; }
    pushEvent(4, i, powerup.type);
    gone = true;
  }
  if (gone) { powerup.type = FX_NONE; powerupGap = frand(POWERUP_GAP_MIN, POWERUP_GAP_MAX); }
}

static void step(float dt) {
  gameTime += dt;
  if (ledT > 0) { ledT -= dt; digitalWrite(LED_PIN, ledT > 0); }

  if (humanCount() == 0) { needReset = true; return; }
  if (needReset) { needReset = false; balanceBots(); startRound(); }

  if (intermission) {
    interT -= dt;
    if (interT <= 0) startRound();
    return;
  }

  timeLeft -= dt;
  if (timeLeft <= 0) { endRound(); return; }
  if (timeLeft < SHRINK_TIME)
    arenaR = ARENA_R_MIN + (ARENA_R - ARENA_R_MIN) * (timeLeft / SHRINK_TIME);

  float ax[MAX_SLOTS] = {0}, ay[MAX_SLOTS] = {0};

  for (int i = 0; i < MAX_SLOTS; i++) {
    Player &p = players[i];
    if (!p.used) continue;
    if (p.flipCd > 0) p.flipCd -= dt;
    if (p.surgeT > 0) p.surgeT -= dt;
    if (p.surgeCd > 0) p.surgeCd -= dt;
    if (p.fxT > 0 && (p.fxT -= dt) <= 0) p.fx = FX_NONE;
    if (!p.alive) {
      p.respawnT -= dt;
      if (p.respawnT <= 0) spawnPlayer(p);
      continue;
    }
    if (p.bot) thinkBot(i, dt);
    float acc = ACCEL * (p.fx == FX_TURBO ? TURBO_ACCEL : 1);
    ax[i] = p.ix * acc;
    ay[i] = p.iy * acc;
  }

  updatePowerup(dt);

  // magnetic forces between players
  for (int i = 0; i < MAX_SLOTS; i++) {
    Player &a = players[i];
    if (!a.used || !a.alive) continue;
    float ma = a.surgeT > 0 ? SURGE_MULT : 1;
    for (int j = i + 1; j < MAX_SLOTS; j++) {
      Player &b = players[j];
      if (!b.used || !b.alive) continue;
      float dx = b.x - a.x, dy = b.y - a.y, d = hypotf(dx, dy);
      if (d > MAG_RANGE || d < 1) continue;
      float mb = b.surgeT > 0 ? SURGE_MULT : 1;
      float dc = max(d, 36.0f);
      float s = K_PLAYER * a.pol * b.pol * ma * mb / (dc * dc);   // >0 repel
      float nx = dx / d, ny = dy / d;
      if (a.fx != FX_ANCHOR) { ax[i] -= s * nx; ay[i] -= s * ny; }
      if (b.fx != FX_ANCHOR) { ax[j] += s * nx; ay[j] += s * ny; }
      if (fabsf(s) > 140) {
        a.lastHitBy = j; a.lastHitT = gameTime;
        b.lastHitBy = i; b.lastHitT = gameTime;
      }
    }
  }

  // integrate players
  for (int i = 0; i < MAX_SLOTS; i++) {
    Player &p = players[i];
    if (!p.used || !p.alive) continue;
    p.vx += ax[i] * dt; p.vy += ay[i] * dt;
    // soft speed cap: excess speed (from shockwaves, bumps) bleeds off over a few frames
    float sp = hypotf(p.vx, p.vy), cap = MAX_SPEED * (p.fx == FX_TURBO ? TURBO_SPEED : 1);
    if (sp > cap) { float k = max(cap / sp, 1 - 4 * dt); p.vx *= k; p.vy *= k; }
    float f = max(0.0f, 1 - FRICTION * dt);
    p.vx *= f; p.vy *= f;
    p.x += p.vx * dt; p.y += p.vy * dt;
  }

  // bumper collisions
  for (int i = 0; i < MAX_SLOTS; i++) {
    Player &a = players[i];
    if (!a.used || !a.alive) continue;
    for (int j = i + 1; j < MAX_SLOTS; j++) {
      Player &b = players[j];
      if (!b.used || !b.alive) continue;
      float dx = b.x - a.x, dy = b.y - a.y, d = hypotf(dx, dy);
      if (d >= 2 * PLAYER_R || d < 0.01f) continue;
      float nx = dx / d, ny = dy / d, overlap = 2 * PLAYER_R - d;
      float wa = 1 / (a.fx == FX_ANCHOR ? ANCHOR_MASS : 1), wb = 1 / (b.fx == FX_ANCHOR ? ANCHOR_MASS : 1);
      a.x -= nx * overlap * wa / (wa + wb); a.y -= ny * overlap * wa / (wa + wb);
      b.x += nx * overlap * wb / (wa + wb); b.y += ny * overlap * wb / (wa + wb);
      float rel = (a.vx - b.vx) * nx + (a.vy - b.vy) * ny;
      if (rel > 0) {
        float imp = rel * 2.3f / (wa + wb);   // restitution > 1: a little extra bounce makes bumping fun
        a.vx -= imp * wa * nx; a.vy -= imp * wa * ny;
        b.vx += imp * wb * nx; b.vy += imp * wb * ny;
      }
      a.lastHitBy = j; a.lastHitT = gameTime;
      b.lastHitBy = i; b.lastHitT = gameTime;
    }
  }

  // orbs: pushed and pulled by every player in range
  for (int k = 0; k < orbCount; k++) {
    Orb &o = orbs[k];
    if (o.hideT > 0) { o.hideT -= dt; continue; }
    float oax = 0, oay = 0;
    for (int i = 0; i < MAX_SLOTS; i++) {
      Player &p = players[i];
      if (!p.used || !p.alive) continue;
      float dx = o.x - p.x, dy = o.y - p.y, d = hypotf(dx, dy);
      if (d < PLAYER_R + ORB_R) {
        int pts = o.gold ? GOLD_POINTS : 1;
        p.score += pts;
        pushEvent(1, i, pts);
        spawnOrb(o);
        o.hideT = ORB_RESPAWN;
        goto nextOrb;
      }
      bool magnetar = p.fx == FX_MAGNETAR;
      if (d > ORB_RANGE * (magnetar ? MAGNETAR_RANGE : 1)) continue;
      float m = p.surgeT > 0 ? SURGE_MULT : 1;
      float dc = max(d, 30.0f);
      float s = K_ORB * p.pol * o.pol * m / (dc * dc);
      if (magnetar) s = -1.3f * fabsf(s);         // everything comes to you
      else if (o.gold) s = -0.7f * fabsf(s);      // gold is drawn to everyone
      oax += s * dx / d; oay += s * dy / d;
    }
    o.vx += oax * dt; o.vy += oay * dt;
    {
      float sp = hypotf(o.vx, o.vy);
      if (sp > 520) { o.vx *= 520 / sp; o.vy *= 520 / sp; }
      float f = max(0.0f, 1 - 3.0f * dt);
      o.vx *= f; o.vy *= f;
      o.x += o.vx * dt; o.y += o.vy * dt;
      if (hypotf(o.x, o.y) > arenaR + 30) spawnOrb(o);
    }
  nextOrb:;
  }

  // falling into the void
  for (int i = 0; i < MAX_SLOTS; i++) {
    Player &p = players[i];
    if (!p.used || !p.alive) continue;
    if (hypotf(p.x, p.y) <= arenaR) continue;
    p.alive = false;
    p.respawnT = RESPAWN_TIME;
    p.fx = FX_NONE; p.fxT = 0;
    int killer = (p.lastHitBy >= 0 && gameTime - p.lastHitT < CREDIT_TIME &&
                  players[p.lastHitBy].used) ? p.lastHitBy : -1;
    if (killer >= 0) players[killer].score += KO_POINTS;
    p.score = max(0, p.score - FALL_PENALTY);
    pushEvent(2, i, killer);
    ledT = 0.4f;
  }
}

// -------------------------------------------------------------- networking ---
static char snap[3072];

static void broadcastState() {
  int n = snprintf(snap, sizeof(snap), "{\"im\":%d,\"bt\":%d,\"t\":%d,\"R\":%d,\"hs\":[%d,\"%s\"],\"lw\":[%d,\"%s\",%d],\"p\":[",
                   intermission ? 1 : 0, botsEnabled ? 1 : 0, (int)((intermission ? interT : timeLeft) * 10), (int)arenaR,
                   bestScore, bestName, lastWinner, lastWinnerName, lastWinnerScore);
  bool first = true;
  for (int i = 0; i < MAX_SLOTS; i++) {
    Player &p = players[i];
    if (!p.used) continue;
    n += snprintf(snap + n, sizeof(snap) - n, "%s[%d,%d,%d,%d,%d,%d,%d,%d,%d,\"%s\",%d,%d]", first ? "" : ",",
                  i, (int)p.x, (int)p.y, p.pol, p.score, p.alive, p.surgeT > 0,
                  (int)(max(0.0f, p.surgeCd) * 10), p.bot, p.name, p.fx, (int)(max(0.0f, p.fxT) * 10));
    first = false;
  }
  n += snprintf(snap + n, sizeof(snap) - n, "],\"o\":[");
  bool firstOrb = true;
  for (int k = 0; k < orbCount; k++) {
    if (orbs[k].hideT > 0) continue;
    n += snprintf(snap + n, sizeof(snap) - n, "%s[%d,%d,%d]", firstOrb ? "" : ",", (int)orbs[k].x, (int)orbs[k].y,
                  orbs[k].gold ? 2 : orbs[k].pol);   // 2 = golden
    firstOrb = false;
  }
  if (powerup.type != FX_NONE)
    n += snprintf(snap + n, sizeof(snap) - n, "],\"u\":[%d,%d,%d,%d", powerup.type, (int)powerup.x, (int)powerup.y,
                  (int)(powerup.t * 10));
  else
    n += snprintf(snap + n, sizeof(snap) - n, "],\"u\":[");
  n += snprintf(snap + n, sizeof(snap) - n, "],\"e\":[");
  for (int e = 0; e < eventCount; e++)
    n += snprintf(snap + n, sizeof(snap) - n, "%s[%d,%d,%d]", e ? "," : "", events[e][0], events[e][1], events[e][2]);
  n += snprintf(snap + n, sizeof(snap) - n, "]}");
  eventCount = 0;
  if (n < (int)sizeof(snap)) ws.broadcastTXT(snap, n);
}

static int playerOf(uint8_t client) {
  for (int i = 0; i < MAX_SLOTS; i++)
    if (players[i].used && !players[i].bot && players[i].client == client) return i;
  return -1;
}

static void onWs(uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
  if (type == WStype_DISCONNECTED) {
    int i = playerOf(num);
    if (i >= 0) { players[i].used = false; balanceBots(); }
    return;
  }
  if (type != WStype_TEXT || len == 0) return;
  char cmd = payload[0];
  int i = playerOf(num);

  if (cmd == 'j' && i < 0) {               // join: j<name>
    char name[13]; int n = 0;
    for (size_t k = 1; k < len && n < 12; k++) {
      char c = payload[k];
      if (isalnum(c) || c == ' ' || c == '-' || c == '_') name[n++] = c;
    }
    name[n] = 0;
    if (n == 0) strcpy(name, "Player");
    // humans may take a bot's seat
    if (playerCount() >= MAX_SLOTS)
      for (int k = MAX_SLOTS - 1; k >= 0; k--) if (players[k].used && players[k].bot) { players[k].used = false; break; }
    i = addPlayer(false, num, name);
    if (i < 0) { ws.sendTXT(num, "{\"full\":1}"); return; }
    balanceBots();
    char msg[24]; snprintf(msg, sizeof(msg), "{\"you\":%d}", i);
    ws.sendTXT(num, msg);
    return;
  }
  if (i < 0) return;
  Player &p = players[i];
  if (cmd == 'i') {                        // input: i<dx>,<dy>  (-100..100)
    int dx = 0, dy = 0;
    char buf[24]; size_t n = min(len - 1, sizeof(buf) - 1);
    memcpy(buf, payload + 1, n); buf[n] = 0;
    sscanf(buf, "%d,%d", &dx, &dy);
    float fx = dx / 100.0f, fy = dy / 100.0f, l = hypotf(fx, fy);
    if (l > 1) { fx /= l; fy /= l; }
    p.ix = fx; p.iy = fy;
  } else if (cmd == 'f') {                 // flip polarity
    if (p.flipCd <= 0 && p.alive) { p.pol = -p.pol; p.flipCd = FLIP_COOLDOWN; }
  } else if (cmd == 's') {                 // surge
    if (p.surgeCd <= 0 && p.alive) { p.surgeT = SURGE_TIME; p.surgeCd = SURGE_COOLDOWN; }
  } else if (cmd == 'b') {                 // toggle bots for everyone, fresh round so a duel starts at 0:0
    botsEnabled = !botsEnabled;
    prefs.putBool("bots", botsEnabled);
    balanceBots();
    startRound();
    pushEvent(5, i, botsEnabled);
  }
}

// ------------------------------------------------------------------ setup ---
// Minimal captive-portal DNS: answers every A query with our own IP.
// (The core's DNSServer replies NXDOMAIN to queries carrying EDNS, which many phones send.)
static void handleDns() {
  int len = dnsUdp.parsePacket();
  if (len <= 0) return;
  uint8_t buf[512];
  if (len < 12 || len > (int)sizeof(buf) - 16) { dnsUdp.flush(); return; }
  dnsUdp.read(buf, len);
  if ((buf[2] & 0x80) || buf[4] || buf[5] != 1) return;     // only plain queries with one question
  int z = 12;
  while (z < len && buf[z]) z += buf[z] + 1;                 // walk the question name
  int end = z + 5;                                           // zero byte + qtype + qclass
  if (end > len) return;
  bool typeA = buf[z + 1] == 0 && buf[z + 2] == 1;
  buf[2] = 0x84 | (buf[2] & 0x01);                           // response, authoritative, keep RD
  buf[3] = 0x80;                                             // RA, NOERROR
  buf[6] = 0; buf[7] = typeA ? 1 : 0;                        // answers
  buf[8] = buf[9] = buf[10] = buf[11] = 0;                   // drop authority + EDNS records
  int n = end;
  if (typeA) {
    IPAddress ip = WiFi.softAPIP();
    const uint8_t rr[] = {0xC0, 0x0C, 0, 1, 0, 1, 0, 0, 0, 60, 0, 4, ip[0], ip[1], ip[2], ip[3]};
    memcpy(buf + n, rr, sizeof(rr));
    n += sizeof(rr);
  }
  dnsUdp.beginPacket(dnsUdp.remoteIP(), dnsUdp.remotePort());
  dnsUdp.write(buf, n);
  dnsUdp.endPacket();
}

// Home WiFi if it's in range, otherwise (or with BOOT held at power-up) an own
// hotspot with a captive portal, so phones on the go land straight in the game.
static void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);

  bool forceHotspot = digitalRead(BOOT_PIN) == LOW;
#ifdef FORCE_HOTSPOT
  forceHotspot = true;   // test builds only
#endif
  bool homeInRange = false;
  if (!forceHotspot) {
    Serial.print("Scanning for home WiFi... ");
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n && !homeInRange; i++) homeInRange = WiFi.SSID(i) == WIFI_SSID;
    WiFi.scanDelete();
    Serial.println(homeInRange ? "found" : "not in range");
  }

  if (homeInRange) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting");
    for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) {
      delay(500); Serial.print('.');
      digitalWrite(LED_PIN, i & 1);
    }
    digitalWrite(LED_PIN, LOW);
  }

  if (WiFi.status() == WL_CONNECTED) {
    WiFi.setSleep(false);   // lower latency for the game
    Serial.printf("\nConnected! Play at http://%s/  or  http://%s.local/\n",
                  WiFi.localIP().toString().c_str(), HOSTNAME);
    return;
  }

  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, nullptr, 1, 0, AP_MAX_CLIENTS);
  WiFi.setSleep(false);
  hotspot = true;
  dnsUdp.begin(53);   // every hostname resolves to us -> captive portal
  Serial.printf("\n%s – open hotspot \"%s\", play at http://%s/\n", forceHotspot ? "BOOT held" : "No home WiFi",
                AP_SSID, WiFi.softAPIP().toString().c_str());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BOOT_PIN, INPUT_PULLUP);
  Serial.println("\n=== POLARITY ===");

  prefs.begin("polarity", false);
  bestScore = prefs.getInt("best", 0);
  botsEnabled = prefs.getBool("bots", true);
  if (prefs.isKey("bestName")) prefs.getString("bestName").toCharArray(bestName, sizeof(bestName));

  setupWifi();
  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.begin();

  http.on("/", [] { http.send_P(200, "text/html", PAGE); });
  http.on("/apes", [] { http.send_P(200, "text/html", apesPage()); });
  http.on("/health", [] {
    char b[96];
    snprintf(b, sizeof(b), "{\"players\":%d,\"humans\":%d,\"heap\":%u,\"uptime\":%lu}",
             playerCount(), humanCount(), ESP.getFreeHeap(), millis() / 1000);
    http.send(200, "application/json", b);
  });
  // unknown paths (incl. phones' connectivity checks in hotspot mode) land on the game
  http.onNotFound([] {
    http.sendHeader("Location", hotspot ? "http://" + WiFi.softAPIP().toString() + "/" : String("/"));
    http.send(302);
  });
  http.begin();

  apesBegin();
  ws.begin();
  ws.onEvent(onWs);
  ws.enableHeartbeat(3000, 2000, 2);   // drop dead tabs quickly
}

void loop() {
  static uint32_t lastStep = millis(), lastSnap = 0;
  ArduinoOTA.handle();
  if (hotspot) handleDns();
  http.handleClient();
  ws.loop();
  apesLoop();

  uint32_t now = millis();
  int steps = 0;
  while (now - lastStep >= 33 && steps < 4) { step(0.033f); lastStep += 33; steps++; }
  if (steps == 4) lastStep = now;            // don't spiral after a hiccup
  if (now - lastSnap >= 66) {
    lastSnap = now;
    if (ws.connectedClients() > 0) broadcastState();
  }
  delay(1);
}
