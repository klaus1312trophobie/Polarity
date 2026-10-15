#pragma once
// The complete browser client: canvas renderer, keyboard + touch controls, WebAudio blips.
const char PAGE[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>POLARITY</title>
<style>
  :root{--plus:#ff3b6b;--minus:#2ee6ff;--ink:#e8ecff;--dim:#8a90b8}
  *{box-sizing:border-box;margin:0;padding:0}
  html,body{height:100%;overflow:hidden;background:#05060d;color:var(--ink);
    font-family:ui-monospace,"JetBrains Mono",Menlo,Consolas,monospace;user-select:none;-webkit-user-select:none;touch-action:none}
  canvas{position:fixed;inset:0;width:100%;height:100%}
  .panel{position:fixed;pointer-events:none;text-shadow:0 0 8px #000}
  #timer{top:14px;left:18px;font-size:30px;font-weight:700;letter-spacing:2px}
  #timer small{display:block;font-size:11px;color:var(--dim);letter-spacing:1px;font-weight:400}
  #board{top:14px;right:18px;font-size:14px;line-height:1.6;text-align:right;min-width:170px}
  #board .me{color:#fff;font-weight:700}
  #board .bot{color:var(--dim)}
  #feed{left:18px;bottom:18px;font-size:13px;line-height:1.7}
  #feed div{transition:opacity 1s}
  #self{left:50%;bottom:18px;transform:translateX(-50%);text-align:center;font-size:13px;color:var(--dim)}
  #pol{font-size:34px;font-weight:700;line-height:1}
  #surge{width:120px;height:5px;background:#1b1f38;border-radius:3px;margin:6px auto 4px;overflow:hidden}
  #surge i{display:block;height:100%;background:#ffd23f;width:100%}
  #banner{top:30%;left:50%;transform:translate(-50%,-50%);text-align:center;font-size:30px;font-weight:700;display:none}
  #banner small{display:block;font-size:15px;color:var(--dim);margin-top:10px;font-weight:400}
  #menu{position:fixed;inset:0;display:flex;align-items:center;justify-content:center;
    background:radial-gradient(circle at 50% 40%,#151a3acc,#05060df2 70%);z-index:5}
  .card{max-width:440px;padding:28px 30px;text-align:center}
  h1{font-size:52px;letter-spacing:8px;margin-bottom:4px}
  h1 .p{color:var(--plus);text-shadow:0 0 18px var(--plus)} h1 .m{color:var(--minus);text-shadow:0 0 18px var(--minus)}
  .tag{color:var(--dim);font-size:13px;margin-bottom:22px}
  input{font:inherit;font-size:18px;padding:10px 14px;width:100%;background:#0d1024;color:var(--ink);
    border:1px solid #2a3060;border-radius:8px;outline:none;text-align:center}
  input:focus{border-color:var(--minus)}
  button.play{font:inherit;font-size:20px;font-weight:700;letter-spacing:3px;margin-top:12px;width:100%;padding:12px;
    border:0;border-radius:8px;cursor:pointer;color:#05060d;background:linear-gradient(90deg,var(--plus),var(--minus))}
  .rules{text-align:left;font-size:13px;line-height:1.75;color:#b9bee0;margin-top:22px}
  .rules b{color:#fff} .rules .k{border:1px solid #3a4078;border-radius:4px;padding:0 5px;font-size:11px}
  .best{margin-top:16px;font-size:12px;color:var(--dim)}
  #status{margin-top:10px;font-size:12px;color:#ffd23f;min-height:16px}
  .tbtn{position:fixed;bottom:26px;width:84px;height:84px;border-radius:50%;display:none;align-items:center;justify-content:center;
    font:700 15px ui-monospace,monospace;color:#fff;border:2px solid #ffffff44;background:#ffffff12;z-index:3}
  #bots{position:fixed;top:78px;left:18px;font:700 12px ui-monospace,monospace;letter-spacing:1px;color:var(--ink);
    background:#ffffff10;border:1px solid #3a4078;border-radius:6px;padding:6px 10px;cursor:pointer;display:none;z-index:3}
  #bots.off{border-color:#ffd23f;color:#ffd23f}
  #bflip{right:22px} #bsurge{right:118px;width:66px;height:66px;bottom:34px;font-size:12px;border-color:#ffd23f88}
  #topbtns{position:fixed;top:12px;left:50%;transform:translateX(-50%);display:flex;gap:8px;z-index:6}
  #topbtns a,.mutebtn{font:700 13px ui-monospace,monospace;color:var(--ink);text-decoration:none;background:#ffffff10;
    border:1px solid #3a4078;border-radius:6px;padding:6px 11px;cursor:pointer;touch-action:manipulation}
  .mutebtn{font-size:16px;padding:4px 10px}
  #menu .mutebtn{position:absolute;top:12px;right:12px}
  body.touch .tbtn{display:flex} body.touch #self{bottom:120px}
</style></head>
<body>
<canvas id="c"></canvas>
<div id="topbtns"><a id="back" href="/">⬅ Spiele</a><button class="mutebtn" id="mute">🔊</button></div>
<div id="timer" class="panel"></div>
<div id="board" class="panel"></div>
<div id="feed" class="panel"></div>
<div id="self" class="panel"><div id="pol"></div><div id="surge"><i></i></div><span id="hint">SPACE flip &middot; SHIFT surge</span></div>
<div id="banner" class="panel"></div>
<div id="bots" title="B"></div>
<div class="tbtn" id="bsurge">SURGE</div>
<div class="tbtn" id="bflip">FLIP</div>

<div id="menu"><button class="mutebtn" id="mute2">🔊</button><div class="card">
  <h1><span class="p">POLA</span><span class="m">RITY</span></h1>
  <div class="tag">a magnetic arena game &middot; served by a tiny ESP32</div>
  <input id="name" maxlength="12" placeholder="your name" autocomplete="off">
  <button class="play" id="play">PLAY</button>
  <div id="status"></div>
  <div class="rules">
    You are a magnet. <b style="color:var(--plus)">&oplus;</b> and <b style="color:var(--minus)">&ominus;</b>
    repel their own kind and attract the opposite &mdash; orbs <i>and</i> players.<br>
    &bull; Flip to pull charged orbs into you: <b>+1</b><br>
    &bull; Shove rivals off the edge: <b>+6</b> &middot; falling costs <b>&minus;3</b><br>
    &bull; <b style="color:#ffd23f">Surge</b> triples your magnetism for a moment<br>
    &bull; The arena shrinks in the last 30 seconds<br>
    &bull; Touch a hexagon for a power-up:
    <b style="color:#b6ff3f">&raquo; Turbo</b>, <b style="color:#b48cff">&#9875; Anchor</b> (immune &amp; heavy),
    <b style="color:#ffd23f">&#10022; Magnetar</b> (all orbs come to you), <b>&#10042; Shockwave</b><br>
    &bull; Rare <b style="color:#ffd23f">golden orbs</b> are worth <b>+5</b> and pull toward everyone<br>
    <span class="k">WASD</span>/<span class="k">&larr;&uarr;&rarr;&darr;</span> move &nbsp;
    <span class="k">SPACE</span> flip &nbsp; <span class="k">SHIFT</span> surge &nbsp; <span class="k">B</span> bots on/off<br>
    On a phone: left thumb steers, buttons on the right.
  </div>
  <div class="best" id="best"></div>
  <div class="best"><a href="/" style="color:#ffd23f">⬅ Spielauswahl</a></div>
</div></div>

<script src="/sfx.js"></script>
<script>
const PLUS = '#ff3b6b', MINUS = '#2ee6ff', GOLD = '#ffd23f', ARENA0 = 400, RANGE = 280, ORB_RANGE = 180;
// power-ups: id -> [name, color, icon]
const POWER = {1: ['TURBO', '#b6ff3f', '»'], 2: ['ANCHOR', '#b48cff', '⚓'],
               3: ['MAGNETAR', '#ffd23f', '✦'], 4: ['SHOCKWAVE', '#ffffff', '✺']};
const orbColor = pol => pol === 2 ? GOLD : pol > 0 ? PLUS : MINUS;
const cv = document.getElementById('c'), ctx = cv.getContext('2d');
const $ = id => document.getElementById(id);
const isTouch = 'ontouchstart' in window || navigator.maxTouchPoints > 0;
if (isTouch) document.body.classList.add('touch'), $('hint').textContent = '';

let ws, me = -1, joined = false, prev = null, cur = null, snapAt = 0, snapGap = 66;
let names = {}, effects = [], feed = [], stars = [];
let W = 0, H = 0, S = 1, DPR = 1;

// ------------------------------------------------------------- canvas ---
function resize() {
  DPR = Math.min(2, window.devicePixelRatio || 1);
  W = innerWidth; H = innerHeight;
  cv.width = W * DPR; cv.height = H * DPR;
  S = Math.min(W, H) / (2 * ARENA0 + 90);
  stars = Array.from({length: 140}, () => [Math.random() * W, Math.random() * H, Math.random() * 1.4 + .2, Math.random() * 6]);
}
addEventListener('resize', resize); resize();

// -------------------------------------------------------------- sound ---
// everything goes through the shared engine (/sfx.js); other players' sounds are
// attenuated by distance and panned left/right relative to you.
const L = SFX.lib;
const ears = () => { const p = cur && cur.byId[me]; return p && p[5] ? [p[1], p[2]] : [0, 0]; };
const spat = (x, y, range = 520) => { const [mx, my] = ears(); return SFX.spat(x - mx, y - my, range); };
let lastBump = 0;
const snd = {
  orb(mine, x, y, gold) {
    if (mine) { gold ? SFX.seq([[79, 1], [84, 1], [88, 1], [91, 2]], {step: .06, type: 'p25', vol: .13}) : L.coin(.7); return; }
    const q = spat(x, y, 460); if (gold) L.power(q.gain * .5, q.pan); else SFX.tone(1320, .05, {type: 'p25', vol: .05, gain: q.gain, pan: q.pan});
  },
  flip(pol) { SFX.tone(pol > 0 ? 330 : 494, .07, {to: pol > 0 ? 494 : 330, vol: .1}); },
  surge(mine, x, y) { const q = mine ? {gain: 1, pan: 0} : spat(x, y, 600); L.whoosh(q.gain * (mine ? .9 : .5), q.pan, .45); SFX.tone(180, .4, {to: 760, type: 'sawtooth', vol: .07, gain: q.gain, pan: q.pan}); },
  power(mine, x, y, type) {
    if (type === 4) { const q = spat(x, y, 900); L.boom(Math.max(.35, q.gain), q.pan); return; }
    const q = mine ? {gain: 1, pan: 0} : spat(x, y, 500); L.power(q.gain * (mine ? 1 : .45), q.pan);
  },
  fell(mine, x, y) { const q = mine ? {gain: 1, pan: 0} : spat(x, y, 700); L.fall(q.gain * (mine ? 1 : .5), q.pan); },
  bump(x, y, same, speed, mine) {
    const t = performance.now(); if (t - lastBump < 70) return; lastBump = t;
    const q = spat(x, y, 650), g = Math.min(1, .35 + speed / 14) * (mine ? 1.3 : 1) * Math.max(.2, q.gain);
    if (same) { SFX.tone(240, .14, {to: 90, type: 'square', vol: .22, gain: g, pan: q.pan}); SFX.noise(.08, {type: 'lowpass', fc: 900, vol: .3, gain: g, pan: q.pan}); }
    else { SFX.tone(1568, .18, {to: 980, type: 'p12', vol: .14, gain: g, pan: q.pan, vib: [30, 40]}); SFX.tone(392, .1, {to: 200, type: 'triangle', vol: .25, gain: g, pan: q.pan}); }
  },
  bots(on) { SFX.tone(on ? 330 : 660, .2, {to: on ? 220 : 880, type: 'triangle', vol: .12}); }
};

// ------------------------------------------------------------ network ---
$('name').value = localStorage.getItem('polarity-name') || '';
$('play').onclick = start;
$('name').onkeydown = e => { if (e.key === 'Enter') start(); };

function start() {
  SFX.ctx(); L.click();
  const n = $('name').value.trim() || 'Player';
  localStorage.setItem('polarity-name', n);
  joined = true;
  $('menu').style.display = 'none';
  connect();
}

function connect() {
  $('status').textContent = 'connecting...';
  ws = new WebSocket('ws://' + location.hostname + ':81/');
  ws.onopen = () => { $('status').textContent = ''; ws.send('j' + (localStorage.getItem('polarity-name') || 'Player')); };
  ws.onmessage = m => onSnap(JSON.parse(m.data));
  ws.onclose = () => {
    me = -1;
    if (!joined) return;
    addFeed('connection lost, reconnecting...', GOLD);
    setTimeout(connect, 1500);
  };
}

function onSnap(s) {
  if (s.you !== undefined) { me = s.you; return; }
  if (s.full) { addFeed('server full, try again later', GOLD); return; }
  const now = performance.now();
  if (cur) snapGap = Math.min(150, Math.max(40, now - snapAt));
  prev = cur; cur = s; snapAt = now;
  cur.byId = {};
  for (const p of s.p) { cur.byId[p[0]] = p; names[p[0]] = p[9]; }
  $('best').textContent = s.hs[0] ? 'all-time best: ' + s.hs[1] + ' with ' + s.hs[0] : '';
  for (const e of s.e) onEvent(e);
  snapSounds();
}

function onEvent([type, a, b]) {
  const pa = cur.byId[a];
  if (type === 1) {
    const gold = b > 1;
    if (pa) burst(pa[1], pa[2], gold ? GOLD : pa[3] > 0 ? PLUS : MINUS, gold ? 30 : 10, gold ? 200 : 120);
    if (gold) addFeed(names[a] + ' grabbed a golden orb +' + b, GOLD);
    if (pa) snd.orb(a === me, pa[1], pa[2], gold);
  } else if (type === 4) {
    const [nm, col] = POWER[b];
    addFeed(names[a] + ' grabbed ' + nm, col);
    if (pa) {
      burst(pa[1], pa[2], col, 24, 180);
      if (b === 4) effects.push({ring: 1, x: pa[1], y: pa[2], t: 0, big: 1});
      snd.power(a === me, pa[1], pa[2], b);
    }
  } else if (type === 2) {
    const last = prev && prev.byId[a];
    if (last) { burst(last[1], last[2], '#ffffff', 26, 220); effects.push({ring: 1, x: last[1], y: last[2], t: 0}); snd.fell(a === me, last[1], last[2]); }
    if (b >= 0) addFeed(names[b] + ' → ' + names[a] + ' into the void', b === me ? GOLD : '#c8ccef');
    else addFeed(names[a] + ' fell off', '#8a90b8');
    if (b === me) setTimeout(() => L.coin(1), 250);
  } else if (type === 5) {
    addFeed(names[a] + (b ? ' brought the bots back' : ' started a duel – no bots'), GOLD);
    snd.bots(b);
  } else if (type === 3) {
    if (a === me) L.fanfare();
    else if (me >= 0) L.sad();
  }
}

// sounds derived from comparing two snapshots: collisions, countdown, shrink alarm, spawns
let lastSec = -1, shrinkAlarm = false, edgeT = 0;
function snapSounds() {
  if (!prev) return;
  if (prev.im && !cur.im) { L.go(); shrinkAlarm = false; lastSec = -1; }
  // magnets touching: loudest for hits that involve you
  const ps = cur.p;
  for (let i = 0; i < ps.length; i++) for (let j = i + 1; j < ps.length; j++) {
    const A = ps[i], B = ps[j], qa = prev.byId[A[0]], qb = prev.byId[B[0]];
    if (!A[5] || !B[5] || !qa || !qb) continue;
    const d = Math.hypot(A[1] - B[1], A[2] - B[2]), pd = Math.hypot(qa[1] - qb[1], qa[2] - qb[2]);
    if (d < 44 && pd >= 44) snd.bump((A[1] + B[1]) / 2, (A[2] + B[2]) / 2, A[3] === B[3], pd - d, A[0] === me || B[0] === me);
  }
  // other players' surges / respawns
  for (const p of ps) {
    const q = prev.byId[p[0]];
    if (!q) continue;
    if (p[6] && !q[6] && p[0] !== me) snd.surge(false, p[1], p[2]);
    if (p[5] && !q[5] && p[0] === me) SFX.seq([[60, 1], [67, 1], [72, 2]], {step: .06, type: 'p25', vol: .1});
  }
  // new power-up / golden orb appeared on the field
  const nu = cur.u && cur.u.length, pu = prev.u && prev.u.length;
  if (nu && (!pu || cur.u[0] !== prev.u[0] || cur.u[1] !== prev.u[1])) { const q = spat(cur.u[1], cur.u[2], 900); L.power(Math.max(.3, q.gain) * .6, q.pan); }
  const gold = o => o.o.reduce((n, x) => n + (x[2] === 2), 0);
  if (gold(cur) > gold(prev)) SFX.seq([[91, 1], [96, 1], [100, 1], [96, 1], [103, 2]], {step: .05, type: 'p12', vol: .06});
  if (cur.im) return;
  // round clock: tick over the last 10 s, higher for the last 3
  const sec = Math.ceil(cur.t / 10);
  if (sec !== lastSec) { lastSec = sec; if (sec <= 10 && sec > 0) { L.tick(sec <= 3); if (sec <= 3) setTimeout(() => L.tick(true), 120); } }
  // arena shrinking: alarm once, then a warning beep while you stand near the edge
  if (cur.R < ARENA0) {
    if (!shrinkAlarm) { shrinkAlarm = true; L.siren(.9); }
    const mp = cur.byId[me], now = performance.now();
    if (mp && mp[5] && Math.hypot(mp[1], mp[2]) > cur.R - 70 && now - edgeT > 260) { edgeT = now; SFX.tone(1000, .06, {vol: .09}); }
  } else shrinkAlarm = false;
}

function addFeed(text, color) {
  feed.push({text, color, t: performance.now()});
  if (feed.length > 5) feed.shift();
}

// -------------------------------------------------------------- input ---
const keys = {};
let joy = null, lastSent = '', lastSendT = 0;
addEventListener('keydown', e => {
  if (!joined || e.target.tagName === 'INPUT') return;
  if (e.code === 'Space') { e.preventDefault(); if (!e.repeat) flip(); }
  else if (e.code === 'KeyB') { if (!e.repeat) send('b'); }
  else if (e.key === 'Shift') { if (!e.repeat) surge(); }
  keys[e.code] = true;
});
addEventListener('keyup', e => { keys[e.code] = false; });
addEventListener('blur', () => { for (const k in keys) keys[k] = false; });

function send(m) { if (ws && ws.readyState === 1) ws.send(m); }
function flip() {
  send('f');
  const p = cur && cur.byId[me];
  if (p) snd.flip(p[3]);
}
function surge() { send('s'); snd.surge(true); }

function bindBtn(id, fn) {
  $(id).addEventListener('touchstart', e => { e.preventDefault(); e.stopPropagation(); fn(); }, {passive: false});
}
bindBtn('bflip', flip); bindBtn('bsurge', surge); bindBtn('bots', () => send('b'));
$('bots').onclick = () => send('b');

cv.addEventListener('touchstart', e => {
  e.preventDefault();
  for (const t of e.changedTouches)
    if (!joy && t.clientX < W * .6) joy = {id: t.identifier, ox: t.clientX, oy: t.clientY, x: t.clientX, y: t.clientY};
}, {passive: false});
cv.addEventListener('touchmove', e => {
  e.preventDefault();
  for (const t of e.changedTouches) if (joy && t.identifier === joy.id) { joy.x = t.clientX; joy.y = t.clientY; }
}, {passive: false});
const endTouch = e => { for (const t of e.changedTouches) if (joy && t.identifier === joy.id) joy = null; };
cv.addEventListener('touchend', endTouch); cv.addEventListener('touchcancel', endTouch);

function inputVector() {
  let x = 0, y = 0;
  if (keys.KeyA || keys.ArrowLeft) x--;
  if (keys.KeyD || keys.ArrowRight) x++;
  if (keys.KeyW || keys.ArrowUp) y--;
  if (keys.KeyS || keys.ArrowDown) y++;
  if (joy) { x = (joy.x - joy.ox) / 55; y = (joy.y - joy.oy) / 55; }
  const l = Math.hypot(x, y);
  if (l > 1) { x /= l; y /= l; }
  return [Math.round(x * 100), Math.round(y * 100)];
}

setInterval(() => {
  if (me < 0) return;
  const m = 'i' + inputVector().join(','), now = performance.now();
  if (m !== lastSent || now - lastSendT > 400) { send(m); lastSent = m; lastSendT = now; }
}, 40);

// ------------------------------------------------------------ effects ---
function burst(x, y, color, n, speed) {
  for (let i = 0; i < n; i++) {
    const a = Math.random() * Math.PI * 2, v = speed * (.3 + Math.random() * .7);
    effects.push({x, y, vx: Math.cos(a) * v, vy: Math.sin(a) * v, t: 0, life: .5 + Math.random() * .4, color});
  }
}

// ------------------------------------------------------------- render ---
function lerpPlayer(p, a) {
  const q = prev && prev.byId[p[0]];
  if (!q || !q[5] || !p[5] || Math.hypot(p[1] - q[1], p[2] - q[2]) > 160) return [p[1], p[2]];
  return [q[1] + (p[1] - q[1]) * a, q[2] + (p[2] - q[2]) * a];
}

function sx(x) { return W / 2 + x * S; }
function sy(y) { return H / 2 + y * S; }

let lastFrame = performance.now();
function frame(now) {
  const dt = Math.min(.05, (now - lastFrame) / 1000); lastFrame = now;
  ctx.setTransform(DPR, 0, 0, DPR, 0, 0);
  ctx.fillStyle = '#05060d'; ctx.fillRect(0, 0, W, H);

  for (const s of stars) {
    ctx.globalAlpha = .35 + .35 * Math.sin(now / 900 + s[3]);
    ctx.fillStyle = '#9aa4ff'; ctx.fillRect(s[0], s[1], s[2], s[2]);
  }
  ctx.globalAlpha = 1;

  if (!cur) { requestAnimationFrame(frame); return; }
  const a = Math.min(1, (now - snapAt) / snapGap);
  const R = cur.R, shrinking = R < ARENA0 && !cur.im;

  // arena
  ctx.setLineDash([4, 10]); ctx.strokeStyle = '#ffffff14'; ctx.lineWidth = 1;
  ctx.beginPath(); ctx.arc(sx(0), sy(0), ARENA0 * S, 0, 7); ctx.stroke(); ctx.setLineDash([]);
  const g = ctx.createRadialGradient(sx(0), sy(0), 0, sx(0), sy(0), R * S);
  g.addColorStop(0, '#141a3a'); g.addColorStop(1, '#0a0d22');
  ctx.fillStyle = g; ctx.beginPath(); ctx.arc(sx(0), sy(0), R * S, 0, 7); ctx.fill();
  ctx.strokeStyle = '#ffffff0a'; ctx.lineWidth = 1;
  for (let r = 80; r < R; r += 80) { ctx.beginPath(); ctx.arc(sx(0), sy(0), r * S, 0, 7); ctx.stroke(); }
  const pulse = shrinking ? .55 + .45 * Math.sin(now / 120) : 1;
  ctx.shadowBlur = 24; ctx.shadowColor = shrinking ? '#ff3030' : '#6a7bff';
  ctx.strokeStyle = shrinking ? `rgba(255,70,70,${pulse})` : '#6a7bffcc'; ctx.lineWidth = 3;
  ctx.beginPath(); ctx.arc(sx(0), sy(0), R * S, 0, 7); ctx.stroke(); ctx.shadowBlur = 0;

  const players = cur.p.map(p => ({p, pos: lerpPlayer(p, a)}));
  const mine = players.find(o => o.p[0] === me);

  // orbs (interpolated by index) + my field lines
  cur.o.forEach((o, i) => {
    let x = o[0], y = o[1];
    const q = prev && prev.o[i];
    if (q && q[2] === o[2] && Math.hypot(q[0] - x, q[1] - y) < 120) { x = q[0] + (x - q[0]) * a; y = q[1] + (y - q[1]) * a; }
    const col = orbColor(o[2]), gold = o[2] === 2;
    if (mine && mine.p[5]) {
      const d = Math.hypot(x - mine.pos[0], y - mine.pos[1]);
      const range = ORB_RANGE * (mine.p[10] === 3 ? 1.6 : 1);
      if (d < range) {
        const attract = gold || mine.p[10] === 3 || o[2] !== mine.p[3];
        ctx.globalAlpha = (1 - d / range) * .55;
        ctx.strokeStyle = attract ? '#ffffff' : col; ctx.lineWidth = attract ? 1.5 : 1;
        ctx.setLineDash(attract ? [] : [3, 6]);
        ctx.beginPath(); ctx.moveTo(sx(mine.pos[0]), sy(mine.pos[1])); ctx.lineTo(sx(x), sy(y)); ctx.stroke();
        ctx.setLineDash([]); ctx.globalAlpha = 1;
      }
    }
    ctx.shadowBlur = 14; ctx.shadowColor = col; ctx.fillStyle = col;
    const orbR = (gold ? 11 + Math.sin(now / 150) : 8) * S;
    if (gold) ctx.shadowBlur = 26;
    ctx.beginPath(); ctx.arc(sx(x), sy(y), Math.max(3, orbR), 0, 7); ctx.fill(); ctx.shadowBlur = 0;
    ctx.fillStyle = '#05060d'; ctx.font = `bold ${Math.max(8, 11 * S)}px monospace`;
    ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
    ctx.fillText(gold ? '5' : o[2] > 0 ? '+' : '−', sx(x), sy(y) + .5);
  });

  // power-up waiting on the field: a spinning hexagon, blinking before it vanishes
  if (cur.u && cur.u.length && !(cur.u[3] < 30 && Math.floor(now / 120) % 2)) {
    const [type, ux, uy] = cur.u, [, col, icon] = POWER[type], r = 15 * S, rot = now / 700;
    ctx.shadowBlur = 22; ctx.shadowColor = col; ctx.strokeStyle = col; ctx.lineWidth = 2.5;
    ctx.fillStyle = col + '22';
    ctx.beginPath();
    for (let k = 0; k < 6; k++) ctx.lineTo(sx(ux) + Math.cos(rot + k * Math.PI / 3) * r * 1.25, sy(uy) + Math.sin(rot + k * Math.PI / 3) * r * 1.25);
    ctx.closePath(); ctx.fill(); ctx.stroke(); ctx.shadowBlur = 0;
    ctx.fillStyle = col; ctx.font = `bold ${Math.max(12, 18 * S)}px monospace`;
    ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
    ctx.fillText(icon, sx(ux), sy(uy) + 1);
  }

  // player <-> player tension arcs
  for (let i = 0; i < players.length; i++) for (let j = i + 1; j < players.length; j++) {
    const A = players[i], B = players[j];
    if (!A.p[5] || !B.p[5]) continue;
    const d = Math.hypot(A.pos[0] - B.pos[0], A.pos[1] - B.pos[1]);
    if (d > RANGE * .8) continue;
    const repel = A.p[3] === B.p[3];
    ctx.globalAlpha = (1 - d / (RANGE * .8)) * .5;
    ctx.strokeStyle = repel ? '#ff9b3f' : '#b48cff'; ctx.lineWidth = 2;
    ctx.setLineDash(repel ? [2, 7] : [10, 4]); ctx.lineDashOffset = -now / (repel ? 30 : 60) * (repel ? -1 : 1);
    ctx.beginPath(); ctx.moveTo(sx(A.pos[0]), sy(A.pos[1])); ctx.lineTo(sx(B.pos[0]), sy(B.pos[1])); ctx.stroke();
  }
  ctx.setLineDash([]); ctx.lineDashOffset = 0; ctx.globalAlpha = 1;

  // players
  for (const {p, pos} of players) {
    if (!p[5]) continue;
    const [x, y] = pos, col = p[3] > 0 ? PLUS : MINUS, r = 18 * S;
    if (p[6]) {   // surging aura
      ctx.strokeStyle = GOLD; ctx.lineWidth = 2; ctx.globalAlpha = .7;
      ctx.beginPath(); ctx.arc(sx(x), sy(y), r * (1.8 + .3 * Math.sin(now / 60)), 0, 7); ctx.stroke(); ctx.globalAlpha = 1;
    }
    if (p[10]) {  // power-up aura
      const pc = POWER[p[10]][1];
      ctx.strokeStyle = pc; ctx.globalAlpha = p[11] < 15 && Math.floor(now / 100) % 2 ? .25 : .9;
      if (p[10] === 1) {        // turbo: speed streaks behind
        const q = prev && prev.byId[p[0]], vx = q ? p[1] - q[1] : 0, vy = q ? p[2] - q[2] : 0, l = Math.hypot(vx, vy);
        if (l > 2) for (let k = -1; k <= 1; k++) {
          const ox = -vy / l * k * 9 * S, oy = vx / l * k * 9 * S;
          ctx.lineWidth = 2; ctx.beginPath();
          ctx.moveTo(sx(x) + ox - vx / l * r, sy(y) + oy - vy / l * r);
          ctx.lineTo(sx(x) + ox - vx / l * (r + 30 * S), sy(y) + oy - vy / l * (r + 30 * S)); ctx.stroke();
        }
      } else if (p[10] === 2) { // anchor: heavy double ring
        ctx.lineWidth = 4; ctx.beginPath(); ctx.arc(sx(x), sy(y), r + 9, 0, 7); ctx.stroke();
        ctx.lineWidth = 1.5; ctx.beginPath(); ctx.arc(sx(x), sy(y), r + 15, 0, 7); ctx.stroke();
      } else if (p[10] === 3) { // magnetar: spinning dashed field
        ctx.lineWidth = 2; ctx.setLineDash([6, 8]); ctx.lineDashOffset = -now / 20;
        ctx.beginPath(); ctx.arc(sx(x), sy(y), ORB_RANGE * 1.6 * S, 0, 7); ctx.stroke();
        ctx.setLineDash([]); ctx.lineDashOffset = 0;
      }
      ctx.globalAlpha = 1;
    }
    ctx.shadowBlur = 26; ctx.shadowColor = col;
    ctx.fillStyle = col; ctx.beginPath(); ctx.arc(sx(x), sy(y), r, 0, 7); ctx.fill();
    ctx.shadowBlur = 0;
    ctx.fillStyle = '#05060d'; ctx.beginPath(); ctx.arc(sx(x), sy(y), r * .62, 0, 7); ctx.fill();
    ctx.fillStyle = col; ctx.font = `bold ${Math.max(11, 22 * S)}px monospace`;
    ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
    ctx.fillText(p[3] > 0 ? '+' : '−', sx(x), sy(y) + 1);
    if (p[0] === me) {
      ctx.strokeStyle = '#ffffff'; ctx.lineWidth = 2;
      ctx.beginPath(); ctx.arc(sx(x), sy(y), r + 5, 0, 7); ctx.stroke();
    }
    ctx.fillStyle = p[0] === me ? '#ffffff' : '#c8ccef'; ctx.font = `${Math.max(10, 13 * S)}px monospace`;
    ctx.fillText(p[9] + (p[8] ? ' ⚙' : ''), sx(x), sy(y) - r - 12);
  }

  // particles and shockwaves
  effects = effects.filter(e => {
    e.t += dt;
    if (e.ring) {
      const speed = e.big ? 440 : 260;   // shockwave rings reach SHOCK_RANGE
      ctx.strokeStyle = `rgba(255,255,255,${1 - e.t / .6})`; ctx.lineWidth = e.big ? 6 : 3;
      ctx.beginPath(); ctx.arc(sx(e.x), sy(e.y), (20 + e.t * speed) * S, 0, 7); ctx.stroke();
      return e.t < .6;
    }
    e.x += e.vx * dt; e.y += e.vy * dt; e.vx *= .94; e.vy *= .94;
    ctx.globalAlpha = Math.max(0, 1 - e.t / e.life); ctx.fillStyle = e.color;
    ctx.fillRect(sx(e.x) - 2, sy(e.y) - 2, 4, 4); ctx.globalAlpha = 1;
    return e.t < e.life;
  });

  // touch joystick
  if (joy) {
    ctx.strokeStyle = '#ffffff40'; ctx.lineWidth = 2;
    ctx.beginPath(); ctx.arc(joy.ox, joy.oy, 55, 0, 7); ctx.stroke();
    const [ix, iy] = inputVector();
    ctx.fillStyle = '#ffffff55'; ctx.beginPath(); ctx.arc(joy.ox + ix * .55, joy.oy + iy * .55, 22, 0, 7); ctx.fill();
  }

  hud(now, mine);
  requestAnimationFrame(frame);
}

function hud(now, mine) {
  const bt = $('bots');
  bt.style.display = 'block';
  bt.textContent = cur.bt ? 'BOTS: ON' : 'DUEL: NO BOTS';
  bt.className = cur.bt ? '' : 'off';

  const t = Math.max(0, cur.t / 10);
  $('timer').innerHTML = cur.im ? '' : `${Math.floor(t / 60)}:${String(Math.floor(t % 60)).padStart(2, '0')}` +
    (cur.R < ARENA0 ? '<small style="color:#ff6a6a">ARENA SHRINKING</small>' : '<small>ROUND</small>');

  const sorted = [...cur.p].sort((x, y) => y[4] - x[4]);
  $('board').innerHTML = sorted.map((p, i) =>
    `<div class="${p[0] === me ? 'me' : p[8] ? 'bot' : ''}">${i === 0 && p[4] > 0 ? '★ ' : ''}${esc(p[9])} ` +
    `<span style="color:${p[3] > 0 ? PLUS : MINUS}">${p[3] > 0 ? '⊕' : '⊖'}</span> ${p[4]}</div>`).join('');

  $('feed').innerHTML = feed.map(f => {
    const age = (now - f.t) / 1000;
    return `<div style="color:${f.color};opacity:${age > 5 ? 0 : 1}">${esc(f.text)}</div>`;
  }).join('');

  if (mine) {
    const p = mine.p;
    $('pol').innerHTML = `<span style="color:${p[3] > 0 ? PLUS : MINUS};text-shadow:0 0 14px currentColor">${p[3] > 0 ? '⊕' : '⊖'}</span>`;
    $('surge').firstChild.style.width = (p[7] ? 100 - p[7] * 2 : 100) + '%';
    $('surge').firstChild.style.background = p[7] ? '#5a5f86' : GOLD;
    if (p[10]) $('pol').innerHTML += ` <span style="font-size:15px;color:${POWER[p[10]][1]}">${POWER[p[10]][0]} ${Math.ceil(p[11] / 10)}s</span>`;
    if (!p[5] && !cur.im) $('pol').innerHTML = '<span style="font-size:15px;color:#ff6a6a">respawning...</span>';
  }

  const b = $('banner');
  if (cur.im) {
    b.style.display = 'block';
    const won = cur.lw[0] === me;
    b.innerHTML = (cur.lw[0] < 0 ? 'ROUND OVER' : (won ? 'YOU WIN!' : esc(cur.lw[1]) + ' wins')) +
      `<small>${cur.lw[2]} points &middot; next round in ${Math.ceil(cur.t / 10)}</small>`;
    b.style.color = won ? GOLD : '#fff';
  } else b.style.display = 'none';
}

function esc(s) { return String(s).replace(/[&<>"]/g, c => ({'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;'}[c])); }

requestAnimationFrame(frame);
</script>
</body></html>
)rawliteral";
