// headless KLAUSTROPHOBIE clients: node test/klaus_bot.mjs [humans=2] [seconds=130]
const N = +process.argv[2] || 2, SECS = +process.argv[3] || 130, host = process.env.ESP || '192.168.178.97';
const DX = [0,1,1,1,0,-1,-1,-1], DY = [-1,-1,0,1,1,1,0,-1];
let rounds = 0, maxBytes = 0, msgs = 0, logs = 0, fxSeen = {}, lastPh = -1, phases = new Set(), outs = 0;
function client(i) {
  const ws = new WebSocket(`ws://${host}:83/`); let me = -2, S = null;
  ws.onopen = () => ws.send('jTest' + i);
  ws.onmessage = ev => {
    const m = JSON.parse(ev.data); maxBytes = Math.max(maxBytes, ev.data.length);
    if (m.k === 'you') me = m.i;
    if (m.k !== 'st') return;
    S = m;
    if (i === 0) {
      msgs++; logs += m.log.length; m.fx.forEach(f => fxSeen[f[0]] = (fxSeen[f[0]] || 0) + 1);
      phases.add(m.ph);
      if (m.ph === 3 && lastPh !== 3) { rounds++; console.log('round over, winner', m.win, m.s[m.win]?.[1], 'floor', m.fl, 'dis', m.s.map(s => s[0] ? s[4] : '-').join(',')); }
      lastPh = m.ph;
    }
  };
  setInterval(() => {
    if (!S || me < 0 || S.ph === 0 || S.ph === 3 || !S.s[me][3]) return;
    const mine = S.e.find(e => e[1] === 0 && e[4] === me); if (!mine) return;
    const r = Math.random();
    if (r < .4) { const d = Math.floor(Math.random() * 8); ws.send('m' + d); }
    else if (r < .55) ws.send('f');
    else if (r < .6) ws.send('b');
    else if (r < .62) ws.send('x' + Math.floor(Math.random() * 8));
  }, 350);
  return ws;
}
const cs = Array.from({ length: N }, (_, i) => client(i));
setTimeout(async () => {
  const h = await (await fetch(`http://${host}/health`)).json();
  console.log({ rounds, msgs, logs, maxBytes, fxSeen, phases: [...phases], health: h });
  process.exit(0);
}, SECS * 1000);
