#pragma once
// KLAUSTROPHOBIE browser client: canvas elevator, d-pad, WebAudio muzak and farts.
const char KLAUS_PAGE[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="de"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover">
<meta name="theme-color" content="#0e1115">
<title>KLAUSTROPHOBIE</title>
<style>
  :root{--ink:#eef2f5;--dim:#9aa6b2;--card:#1d2229;--brass:#ffcf5a;--gas:#7dff4f}
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:#0e1115;color:var(--ink);font-family:"Trebuchet MS",Verdana,sans-serif;display:flex;flex-direction:column;
    align-items:center;min-height:100vh;padding:8px 10px 20px;user-select:none;-webkit-user-select:none}
  h1{text-align:center;font-size:clamp(22px,5vw,40px);letter-spacing:4px;color:var(--brass);text-shadow:3px 3px 0 #5a4310;margin:4px 0}
  h1 small{font-size:.4em;color:var(--dim);letter-spacing:1px;text-shadow:none;display:block}
  #wrap{width:100%;max-width:900px}
  canvas{width:100%;display:block;border:4px solid #3a424c;border-radius:12px;background:#000;touch-action:none}
  #conn{color:#ff6b6b;font-size:13px;min-height:16px}
  #status{margin-top:8px;font-weight:700;min-height:22px;color:var(--brass)}
  #ctl{background:var(--card);border-radius:12px;padding:12px;margin-top:8px;display:grid;gap:12px;grid-template-columns:auto 1fr;align-items:center}
  #pad{display:grid;grid-template-columns:repeat(3,64px);grid-template-rows:repeat(3,64px);gap:6px}
  button{font:inherit;touch-action:manipulation;-webkit-tap-highlight-color:transparent;cursor:pointer;border:0;border-radius:10px;
    color:#1d2229;background:var(--brass);font-weight:700}
  #pad button{font-size:26px}
  #pad button:active{background:#fff}
  #pad .mid{background:#2a313a;color:var(--dim);font-size:12px;border-radius:50%}
  #acts{display:grid;gap:10px}
  .act{position:relative;overflow:hidden;font-size:20px;padding:14px 8px;letter-spacing:1px;background:#2a313a;color:var(--ink);border:2px solid #4a5560}
  .act i{position:absolute;left:0;top:0;bottom:0;width:0;background:#ffcf5a55;pointer-events:none}
  .act.ready{background:var(--brass);color:#1d2229;border-color:#fff2}
  .act span{position:relative}
  #sb{margin-top:8px;display:grid;gap:5px}
  .row{display:grid;grid-template-columns:34px 1fr 90px;align-items:center;gap:8px;background:var(--card);border-radius:8px;padding:4px 8px;font-size:14px}
  .row.me{outline:2px solid var(--brass)}
  .row.out{opacity:.45;text-decoration:line-through}
  .row .bar{height:10px;border-radius:5px;background:#2a313a;overflow:hidden}
  .row .bar b{display:block;height:100%}
  .row small{color:var(--dim);text-align:right}
  #taunts{margin-top:8px;display:grid;grid-template-columns:repeat(4,1fr);gap:6px}
  #taunts button{background:#2a313a;color:var(--ink);padding:8px 2px;font-size:12px;border:2px solid #4a5560;line-height:1.2}
  #taunts button:active{background:#4a5560}
  #log{margin-top:10px;font-size:13px;line-height:1.6;color:var(--dim);min-height:60px}
  #log div:first-child{color:var(--ink)}
  #foot{margin-top:10px;font-size:12px;color:var(--dim);display:flex;justify-content:space-between}
  #foot a{color:var(--brass)}
  @media(pointer:coarse){#kbd{display:none}}
  body{-webkit-touch-callout:none;-webkit-text-size-adjust:100%}
  #pad button,.act,#taunts button{touch-action:none}
  #ov{position:fixed;inset:0;background:#0e1115ee;display:flex;align-items:center;justify-content:center;z-index:9;padding:16px}
  .card{max-width:430px;text-align:center}
  .card h2{font-size:32px;color:var(--brass);text-shadow:3px 3px 0 #5a4310;letter-spacing:2px}
  .card p{margin:10px 0;color:var(--dim);font-size:14px;line-height:1.55}
  .card input{width:100%;font:inherit;font-size:20px;padding:10px;border-radius:10px;border:3px solid #3a424c;background:#1d2229;color:var(--ink);text-align:center;outline:none}
  .card button{width:100%;font-size:22px;margin-top:12px;padding:12px}
  @media(max-width:700px) and (orientation:portrait){
    body{padding:4px 6px calc(214px + env(safe-area-inset-bottom))}
    h1{font-size:20px;letter-spacing:2px;margin:2px 0}h1 small{display:none}
    #status{font-size:14px;margin-top:6px}
    #ctl{position:fixed;left:0;right:0;bottom:0;z-index:4;margin:0;border-radius:16px 16px 0 0;
      padding:10px 14px calc(10px + env(safe-area-inset-bottom));box-shadow:0 -6px 16px #000b;grid-template-columns:auto 1fr;gap:14px}
    #pad{grid-template-columns:repeat(3,64px);grid-template-rows:repeat(3,64px);gap:6px}
    #pad button{font-size:28px;border-radius:14px}
    #acts{grid-template-rows:1fr 1fr;align-self:stretch;gap:10px}
    .act{font-size:19px;padding:6px;border-radius:14px}
    #sb{grid-template-columns:1fr 1fr}
    .row{grid-template-columns:26px 1fr 52px;font-size:12px;padding:3px 6px;gap:5px}
    #taunts{grid-template-columns:repeat(4,1fr)}
    #log{font-size:12px}#foot{flex-direction:column;gap:6px}
    #mute{top:4px;right:6px}
    .card h2{font-size:26px}.card p{font-size:13px}
  }
  @media(orientation:landscape) and (max-height:520px){
    body{padding:6px;align-items:flex-start}
    h1,#log,#foot,#taunts,#sb,#conn{display:none}
    #wrap{max-width:none;display:grid;grid-template-columns:auto 1fr;gap:10px;align-items:start}
    canvas{grid-row:1/4;height:calc(100vh - 20px);width:auto;max-width:100%}
    #status{margin:0;font-size:14px;padding-right:44px}
    #ctl{margin:0;grid-template-columns:1fr;justify-items:center;padding:8px}
    #pad{grid-template-columns:repeat(3,52px);grid-template-rows:repeat(3,52px);gap:5px}
    #acts{grid-template-columns:1fr 1fr;width:100%}
    .act{font-size:15px;padding:12px 4px}
    #ov{align-items:flex-start;overflow:auto}.card h2{font-size:24px}.card p{font-size:12px;margin:6px 0}
  }
  #ov{overflow:auto}
  #mute{position:fixed;top:8px;right:10px;background:#3a424c;color:var(--ink);padding:4px 10px;z-index:5}
</style></head>
<body>
<button id="mute">🔊</button>
<div id="wrap">
  <h1>🛗 KLAUSTROPHOBIE <small>der Fahrstuhl zur Hölle</small></h1>
  <canvas id="cv" width="800" height="450"></canvas>
  <div id="conn"></div>
  <div id="status">Verbinde…</div>
  <div id="ctl">
    <div id="pad"></div>
    <div id="acts">
      <button class="act" id="bf"><i></i><span>💨 PUPSEN</span></button>
      <button class="act" id="bd"><i></i><span>🚪 TÜR ZU!</span></button>
    </div>
  </div>
  <div id="sb"></div>
  <div id="taunts"></div>
  <div id="log"></div>
  <div id="foot"><span id="kbd">Pfeile/WASD (Q E Z C diagonal): laufen – gegen jemanden laufen = Ellbogen · Leertaste pupst · T Tür zu</span><a href="/">⬅ Spielauswahl</a></div>
</div>
<div id="ov"><div class="card">
  <h2>🛗 KLAUSTROPHOBIE</h2>
  <p>Bis zum Penthouse sind es neun Etagen. In der Kabine stehen Zwiebelbrot-Karl, die Parfümtante und ein Hund mit Mundgeruch. Dein Unbehagen steigt mit jedem Nachbarn. Bei 100 % fliegst du raus.<br>
  Lauf weg, ellbogen Leute weiter, furz zurück und hau auf Tür-zu. Wer zuletzt drinnen steht, gewinnt.</p>
  <input type="text" id="nm" maxlength="12" autocomplete="off">
  <button id="go">EINSTEIGEN</button>
  <p style="font-size:12px">Allein? Dann fahren drei Bots mit. Bis zu sechs Menschen passen rein.</p>
</div></div>
<script>
'use strict';
const GW=6,GH=4;let CWD=104,CHT=76,X0=88,Y0=104,W=800,H=450,PORT=false;
const $=id=>document.getElementById(id),cv=$('cv'),g=cv.getContext('2d');
const FACES=['😀','🤓','😎','🥸','🤠','🧐','🥳','😇'];
const NPC=['','🥖','👵','👶','📱','🐕','🏃'];
const NPCN=['','Karl','Gisela','Kevin','Tobi','Wauwi','Jens'];
const NAMES=['Schweiß-Sven','Pups-Petra','Mief-Michi','Quetsch-Quentin','Duft-Dieter','Stink-Steffi','Klaus-Trophobie'];
const TAUNTS=[['🤢','Boah!'],['😡','Rück weg!'],['🙏','Bitte nicht!'],['😂','Haha!'],['🧼','Dusch mal!'],['🪟','Fenster!'],['👃','Wer war das?'],['🚪','Ich will raus!']];
let ws,me=-2,myQ=0,S=null,joined=null,rx=0,AC=null,muted=false,lastFl=0,lastPh=0;
const vis={},parts=[],pops=[],bubbles=[],gasP=[];let banner=null,shake=0;

function connect(){
  ws=new WebSocket('ws://'+location.hostname+':83/');
  ws.onopen=()=>{$('conn').textContent='';if(joined)ws.send('j'+joined);};
  ws.onclose=()=>{$('conn').textContent='Verbindung weg – der Fahrstuhl steckt fest…';setTimeout(connect,1000);};
  ws.onmessage=e=>handle(JSON.parse(e.data));
}
function handle(m){
  if(m.k==='you'){const was=me;me=m.i;myQ=m.q||0;$('ov').style.display='none';
    if(me>=0&&was<0&&was!==-2)addLog('Du bist drin! Ein Platz war frei.');
    ui();}
  else if(m.k==='st'){
    S=m;rx=performance.now();
    m.fx.forEach(fxEvent);
    m.log.forEach(addLog);
    if(m.ph===2&&lastPh!==2)banner={t:'DING! Etage '+m.fl,until:performance.now()+1500};
    if(m.ph===3&&lastPh!==3){banner={t:m.win>=0?m.s[m.win][1]+' gewinnt!':'Ende',until:performance.now()+4000};if(m.win===me)win();}
    lastPh=m.ph;ui();sidebar();
  }
  else if(m.k==='taunt'&&S){
    const T=TAUNTS[m.n]||TAUNTS[0];bubbles.push({i:m.by,t:T[0]+' '+T[1],until:performance.now()+2200});blip([500,420],.08);
  }
}
function cellXY(x,y){const c=PORT?y:x,r=PORT?x:y;return[X0+c*CWD+CWD/2,Y0+r*CHT+CHT/2];}
function setLayout(){
  PORT=innerWidth<=700&&innerHeight>innerWidth;
  if(PORT){W=440;H=530;CWD=98;CHT=66;X0=24;Y0=112;}else{W=800;H=450;CWD=104;CHT=76;X0=88;Y0=104;}
  cv.width=W;cv.height=H;for(const k in vis)delete vis[k];
}
addEventListener('resize',()=>{if((innerWidth<=700&&innerHeight>innerWidth)!==PORT)setLayout();});
function fxEvent(f){
  if(me>=0&&navigator.vibrate){try{if(f[0]===3)navigator.vibrate([120,60,200]);else if(f[0]===2)navigator.vibrate(25);else if(f[0]===1)navigator.vibrate(15);}catch(e){}}
  const t=f[0],[px,py]=cellXY(f[1],f[2]),now=performance.now();
  if(t===1){fart();for(let i=0;i<14;i++)parts.push({x:px,y:py,vx:(Math.random()-.5)*160,vy:(Math.random()-.5)*110,r:8+Math.random()*10,until:now+1000+Math.random()*600,c:'#7dff4f'});pops.push({x:px,y:py-30,t:'PFFFT!',until:now+900});}
  else if(t===2){thump();pops.push({x:px,y:py-30,t:'BAMM!',until:now+700});shake=6;}
  else if(t===3){wah();pops.push({x:px,y:py-20,t:'RAUS!!',until:now+1400,s:36});shake=10;}
  else if(t===4){ding();}
  else if(t===5){tone(300,160,.3,'square',.15);}
  else if(t===6){blip([440,660],.08);}
  else if(t===7){tone(700,300,.12,'sawtooth',.3);pops.push({x:px,y:py-30,t:'WAU!',until:now+700});}
  else if(t===8){thump();shake=14;pops.push({x:W/2,y:80,t:'RUCK-ZUCK!',until:now+1200,s:40});}
  else if(t===9){tone(200,40,.9,'sawtooth',.3);}
  else if(t===10){blip([500,380],.06);}
  else if(t===11){laugh();}
}
function addLog(t){
  const d=document.createElement('div');d.textContent=t;$('log').prepend(d);
  while($('log').children.length>7)$('log').lastChild.remove();
}
// ------------------------------------------------------------------ UI ---
function faceFor(i,dis){const b=FACES[S.s[i][7]%8];return dis>=90?'🤮':dis>=75?'🤢':dis>=50?'😖':dis>=25?'😬':b;}
function ui(){
  let s='';
  if(!S)s='Warte auf Passagiere…';
  else if(S.ph===3)s=S.win>=0?S.s[S.win][1]+' gewinnt die Fahrt!':'Fahrt zu Ende.';
  else if(me<0)s=myQ?'Zuschauer – Warteschlange '+myQ+': bei Rundenende rückst du nach.':'Du schaust zu.';
  else if(!S.s[me][3])s='Du bist raus. Genieß die frische Luft und schimpf von draußen.';
  else s='Etage '+S.fl+' von 10 · Unbehagen '+S.s[me][4]+' %'+(S.ph===2?' · Türen offen!':'');
  $('status').textContent=s;
}
function sidebar(){
  const a=[];
  for(let i=0;i<6;i++){const s=S.s[i];if(!s[0])continue;
    const dis=s[4],col=dis<40?'#7dff4f':dis<70?'#ffcf5a':'#ff4f4f';
    a.push('<div class="row'+(i===me?' me':'')+(s[3]?'':' out')+'"><span>'+(s[3]?faceFor(i,dis):'👻')+'</span>'+
      '<div><div>'+esc(s[1])+(s[2]?' 🤖':'')+'</div><div class="bar"><b style="width:'+dis+'%;background:'+col+'"></b></div></div>'+
      '<small>'+(s[3]?dis+' %'+(s[9]>5?' ↑':s[9]<0?' ↓':''):'raus #'+(s[8]+1))+'</small></div>');}
  $('sb').innerHTML=a.join('');
}
function esc(t){return t.replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));}
function cdBtn(id,rem,max){
  const b=$(id),r=Math.max(0,rem-(performance.now()-rx)/100);
  b.querySelector('i').style.width=(r>0?100*r/max:0)+'%';b.classList.toggle('ready',r<=0&&me>=0&&S&&S.s[me][3]===1);
}
setInterval(()=>{ui();if(S&&me>=0){cdBtn('bf',S.s[me][5],55);cdBtn('bd',S.ph===2?S.s[me][6]:99,80);}},120);

const SD=[[0,-1],[1,-1],[1,0],[1,1],[0,1],[-1,1],[-1,0],[-1,-1]];
function sdir(sx,sy){const dx=PORT?sy:sx,dy=PORT?sx:sy;for(let d=0;d<8;d++)if(SD[d][0]===dx&&SD[d][1]===dy)return d;return -1;}
const PADS=[['↖',-1,-1],['⬆',0,-1],['↗',1,-1],['⬅',-1,0],['',0,0],['➡',1,0],['↙',-1,1],['⬇',0,1],['↘',1,1]];
PADS.forEach(([t,sx,sy])=>{const b=document.createElement('button');b.textContent=t;
  if(!t){b.className='mid';b.innerHTML='geh<br>boxe';b.disabled=true;}
  else{let rep=0;const go=()=>send('m'+sdir(sx,sy)),stop=()=>{clearInterval(rep);rep=0;};
    b.onpointerdown=e=>{e.preventDefault();audio();go();stop();rep=setInterval(go,170);};
    b.onpointerup=b.onpointercancel=b.onpointerleave=stop;b.oncontextmenu=e=>e.preventDefault();}
  $('pad').appendChild(b);});
TAUNTS.forEach((t,i)=>{const b=document.createElement('button');b.innerHTML=t[0]+'<br>'+t[1];b.onclick=()=>send('x'+i);$('taunts').appendChild(b);});
function send(s){if(ws&&ws.readyState===1)ws.send(s);}
$('bf').onpointerdown=e=>{e.preventDefault();audio();send('f');};
$('bd').onpointerdown=e=>{e.preventDefault();audio();send('b');};
$('mute').onclick=()=>{muted=!muted;$('mute').textContent=muted?'🔇':'🔊';};
const KEYS={ArrowUp:[0,-1],w:[0,-1],ArrowRight:[1,0],d:[1,0],ArrowDown:[0,1],s:[0,1],ArrowLeft:[-1,0],a:[-1,0],e:[1,-1],c:[1,1],z:[-1,1],q:[-1,-1]};
addEventListener('keydown',e=>{
  if(e.target.id==='nm')return;
  const k=e.key.length===1?e.key.toLowerCase():e.key;
  if(k in KEYS){e.preventDefault();send('m'+sdir(KEYS[k][0],KEYS[k][1]));}
  else if(k===' '){e.preventDefault();send('f');}
  else if(k==='t'||k==='Enter'){send('b');}
});
cv.addEventListener('pointerdown',e=>{
  if(!S||me<0)return;audio();
  const r=cv.getBoundingClientRect(),px=(e.clientX-r.left)*W/r.width,py=(e.clientY-r.top)*H/r.height;
  const col=Math.floor((px-X0)/CWD),row=Math.floor((py-Y0)/CHT),x=PORT?row:col,y=PORT?col:row;
  const mine=S.e.find(q=>q[1]===0&&q[4]===me);if(!mine)return;
  const dx=x-mine[2],dy=y-mine[3];
  if(dx===0&&dy===0){send('f');return;}
  if(Math.abs(dx)>1||Math.abs(dy)>1)return;
  for(let d=0;d<8;d++)if(SD[d][0]===dx&&SD[d][1]===dy){send('m'+d);break;}
});
$('nm').value=localStorage.getItem('klausname')||NAMES[Math.floor(Math.random()*NAMES.length)];
function join(){audio();try{navigator.wakeLock&&navigator.wakeLock.request('screen').catch(()=>{});}catch(e){}
  joined=$('nm').value.trim()||'Passagier';localStorage.setItem('klausname',joined);if(ws.readyState===1)ws.send('j'+joined);}
$('go').onclick=join;$('nm').onkeydown=e=>{if(e.key==='Enter')join();};

// --------------------------------------------------------------- sound ---
function audio(){if(!AC){try{AC=new(window.AudioContext||window.webkitAudioContext)();}catch(e){}}if(AC&&AC.state==='suspended')AC.resume();return AC;}
function tone(f0,f1,dur,type,vol,wob,wd,delay){
  const c=audio();if(!c||muted)return;const t=c.currentTime+(delay||0);
  const o=c.createOscillator(),gn=c.createGain();o.type=type;
  o.frequency.setValueAtTime(f0,t);o.frequency.exponentialRampToValueAtTime(f1,t+dur);
  gn.gain.setValueAtTime(vol,t);gn.gain.exponentialRampToValueAtTime(.001,t+dur);
  if(wob){const l=c.createOscillator(),lg=c.createGain();l.frequency.value=wob;lg.gain.value=wd;l.connect(lg);lg.connect(o.frequency);l.start(t);l.stop(t+dur);}
  o.connect(gn);gn.connect(c.destination);o.start(t);o.stop(t+dur);
}
function noise(dur,fc,type,vol){
  const c=audio();if(!c||muted)return;
  const b=c.createBuffer(1,c.sampleRate*dur,c.sampleRate),d=b.getChannelData(0);
  for(let i=0;i<d.length;i++)d[i]=(Math.random()*2-1)*(1-i/d.length);
  const s=c.createBufferSource(),f=c.createBiquadFilter(),gn=c.createGain();
  s.buffer=b;f.type=type;f.frequency.value=fc;gn.gain.value=vol;s.connect(f);f.connect(gn);gn.connect(c.destination);s.start();
}
const fart=()=>{tone(120+Math.random()*60,45,.7,'sawtooth',.45,26,28);noise(.5,300,'lowpass',.35);};
const thump=()=>{tone(150,50,.15,'square',.4);noise(.1,600,'lowpass',.5);};
const wah=()=>{tone(260,230,.3,'sawtooth',.25);tone(240,210,.3,'sawtooth',.25,0,0,.3);tone(220,150,.6,'sawtooth',.25,7,12,.6);};
const laugh=()=>{for(let i=0;i<6;i++)tone(380+(i%2)*70,330+(i%2)*60,.1,'square',.1,0,0,i*.12);};
const ding=()=>{tone(1320,1318,.9,'sine',.3);tone(880,878,1.1,'sine',.25,0,0,.25);};
function blip(a,v){tone(a[0],a[1],.1,'square',v||.1);}
function win(){[0,.15,.3,.5].forEach((d,i)=>tone(520+i*130,540+i*130,.2,'triangle',.3,0,0,d));}
// elevator muzak that drifts out of tune the higher you get
const TUNE=[261,329,392,329,293,349,440,349,261,392,523,392];let mi=0;
setInterval(()=>{
  if(!S||muted||!AC||S.bo||S.ph===3||S.ph===0)return;
  const det=Math.pow(2,((S.fl-1)*(Math.random()-.3)*14)/1200),f=TUNE[mi++%TUNE.length]*det;
  tone(f,f*.995,.38,'sine',.05);
},420);

// ------------------------------------------------------------ rendering ---
function entPos(e){
  const key=e[0];let v=vis[key];const [tx,ty]=cellXY(e[2],e[3]);
  if(!v||v.kind!==e[1]||v.who!==e[4]){
    const fromEdge=S&&S.ph===2&&e[1]>0;
    v=vis[key]={kind:e[1],who:e[4],x:fromEdge?(Math.random()<.5?-40:W+40):tx,y:ty};
  }
  v.x+=(tx-v.x)*.22;v.y+=(ty-v.y)*.22;return v;
}
function bg(t){
  const open=S&&S.ph===2?1:0,DW=PORT?14:22,NC=PORT?GH:GW,NR=PORT?GW:GH,cw=NC*CWD,ch=NR*CHT;
  g.fillStyle='#10141a';g.fillRect(0,0,W,H);
  g.fillStyle=open?'#f4e7b0':'#262b33';g.fillRect(0,Y0-14,X0-DW,ch+40);g.fillRect(X0+cw+DW,Y0-14,W-(X0+cw+DW),ch+40);
  const gr=g.createLinearGradient(0,Y0-14,0,Y0+ch+14);gr.addColorStop(0,'#5c6670');gr.addColorStop(1,'#3a424c');
  g.fillStyle=gr;g.fillRect(X0-DW,Y0-14,cw+2*DW,ch+28);
  g.fillStyle='#8a97a3';for(let i=0;i<=NC;i++)g.fillRect(X0+i*CWD-1,Y0,2,ch);
  for(let r=0;r<NR;r++)for(let c=0;c<NC;c++){g.fillStyle=(r+c)%2?'#4b545e':'#434b54';g.fillRect(X0+c*CWD+1,Y0+r*CHT+1,CWD-2,CHT-2);}
  g.fillStyle='#d4af37';g.fillRect(X0,Y0-6,cw,5);
  const dw=open?4:DW;g.fillStyle='#9aa6b2';g.fillRect(X0-DW,Y0-14,dw,ch+28);g.fillRect(X0+cw+DW-dw,Y0-14,dw,ch+28);
  g.fillStyle='#000';g.fillRect(X0-DW,Y0-14,cw+2*DW,3);
  const cx=W/2,fy=PORT?10:14;
  g.fillStyle='#000';g.fillRect(cx-90,fy,180,60);g.strokeStyle='#d4af37';g.lineWidth=3;g.strokeRect(cx-90,fy,180,60);
  g.fillStyle='#ff3030';g.font='bold 40px monospace';g.textAlign='center';g.textBaseline='middle';
  g.fillText(S?(S.fl>=10?'PH':String(S.fl).padStart(2,' ')):'--',cx,fy+32);
  if(S&&S.ph===1){g.font='22px sans-serif';g.fillText((Math.floor(t/300)%2)?'▲':'△',cx+62,fy+32);}
  const sp=PORT?36:48;
  for(let i=1;i<=10;i++){g.fillStyle=S&&i<=S.fl?'#ffcf5a':'#3a424c';g.beginPath();g.arc(cx+(i-5.5)*sp,fy+78,5,0,7);g.fill();}
}
function frame(t){
  requestAnimationFrame(frame);
  const FS=Math.round(Math.min(CWD*.44,CHT*.58)),sc=CHT/76,bo=Math.min(40,CHT*.52);
  g.save();
  if(shake>0){g.translate((Math.random()-.5)*shake,(Math.random()-.5)*shake);shake*=.9;if(shake<.5)shake=0;}
  bg(t);
  if(S){
    for(let c=0;c<24;c++){const lv=+S.g[c];if(!lv)continue;const [cx,cy]=cellXY(c%GW,Math.floor(c/GW));
      const rr=CWD*.62+Math.sin(t/180+c)*5,gr=g.createRadialGradient(cx,cy,4,cx,cy,rr);
      gr.addColorStop(0,'rgba(125,255,79,'+(lv/6*.6)+')');gr.addColorStop(1,'rgba(125,255,79,0)');g.fillStyle=gr;g.fillRect(cx-rr,cy-rr,rr*2,rr*2);}
    const list=S.e.slice().sort((a,b)=>PORT?a[2]-b[2]:a[3]-b[3]);
    list.forEach(e=>{
      const v=entPos(e),dis=e[1]===0?S.s[e[4]][4]:0,wob=Math.sin(t/(120-dis*.6)+e[0])*(dis/100)*4;
      g.fillStyle='#0006';g.beginPath();g.ellipse(v.x,v.y+22*sc,24,8*sc,0,0,7);g.fill();
      g.textAlign='center';g.textBaseline='middle';g.font=FS+'px sans-serif';
      if(e[1]===0){
        g.fillText(faceFor(e[4],dis),v.x+wob,v.y-2);
        if(dis>45){g.font='16px sans-serif';g.fillText('💦',v.x+24+wob,v.y-20*sc+Math.sin(t/200)*3);}
        g.fillStyle='#000a';g.fillRect(v.x-28,v.y-bo,56,8);g.fillStyle=dis<40?'#7dff4f':dis<70?'#ffcf5a':'#ff4f4f';g.fillRect(v.x-27,v.y-bo+1,54*dis/100,6);
        g.font='bold 12px sans-serif';g.fillStyle=e[4]===me?'#ffcf5a':'#fff';g.fillText(S.s[e[4]][1],v.x,v.y+CHT*.42);
        if(e[4]===me){g.strokeStyle='#ffcf5a';g.lineWidth=3;g.beginPath();g.arc(v.x,v.y-2,FS*.65,0,7);g.stroke();}
      }else{
        g.fillText(NPC[e[1]],v.x,v.y-2);
        g.font='11px sans-serif';g.fillStyle='#c8d0d8';g.fillText(NPCN[e[1]],v.x,v.y+CHT*.42);
        if(e[1]===1||e[1]===6||e[1]===5){g.fillStyle='#7dff4f88';g.font='14px sans-serif';g.fillText('~',v.x+26,v.y-14+Math.sin(t/150)*4);}
        if(e[1]===2){g.fillStyle='#ff7ad988';g.font='16px sans-serif';g.fillText('✿',v.x+26,v.y-14+Math.sin(t/170)*4);}
        if(e[1]===4&&Math.floor(t/700)%2){g.fillStyle='#fff';g.font='bold 11px sans-serif';g.fillText('JA GENAU!!',v.x,v.y-bo);}
      }
    });
  }
  if(S&&S.bo){g.fillStyle='rgba(0,0,8,.9)';g.fillRect(0,0,W,H);
    for(let c=0;c<24;c++){const lv=+S.g[c];if(!lv)continue;const [cx,cy]=cellXY(c%GW,Math.floor(c/GW));
      const gr=g.createRadialGradient(cx,cy,2,cx,cy,CWD*.6);gr.addColorStop(0,'rgba(125,255,79,'+(lv/6*.8)+')');gr.addColorStop(1,'rgba(125,255,79,0)');g.fillStyle=gr;g.fillRect(cx-70,cy-70,140,140);}}
  const now=performance.now();
  for(let i=parts.length-1;i>=0;i--){const p=parts[i];if(now>p.until){parts.splice(i,1);continue;}p.x+=p.vx*.016;p.y+=p.vy*.016;p.vx*=.97;p.vy*=.97;
    g.globalAlpha=Math.max(0,(p.until-now)/1400);g.fillStyle=p.c;g.beginPath();g.arc(p.x,p.y,p.r,0,7);g.fill();g.globalAlpha=1;}
  pops.forEach((p,i)=>{if(now>p.until){pops.splice(i,1);return;}const a=(p.until-now)/700;g.globalAlpha=Math.min(1,a);
    g.font='bold '+(p.s||26)+'px sans-serif';g.textAlign='center';g.lineWidth=5;g.strokeStyle='#000';g.fillStyle='#ffcf5a';
    const y=p.y-(1-Math.min(1,a))*20;g.strokeText(p.t,p.x,y);g.fillText(p.t,p.x,y);g.globalAlpha=1;});
  bubbles.forEach((b,i)=>{if(now>b.until){bubbles.splice(i,1);return;}
    const e=S&&S.e.find(q=>q[1]===0&&q[4]===b.i);let x=70,y=130+i*28;
    if(e){const v=vis[e[0]];if(v){x=Math.max(70,Math.min(W-70,v.x));y=v.y-bo-14;}}
    g.font='bold 14px sans-serif';g.textAlign='center';const w=g.measureText(b.t).width+16;
    g.fillStyle='#fff';g.fillRect(x-w/2,y-12,w,24);g.fillStyle='#000';g.fillText(b.t,x,y);});
  if(banner&&now<banner.until){g.globalAlpha=Math.min(1,(banner.until-now)/400);g.font='bold '+(PORT?44:54)+'px sans-serif';g.textAlign='center';g.lineWidth=8;g.strokeStyle='#000';g.fillStyle='#ffcf5a';
    g.strokeText(banner.t,W/2,H/2);g.fillText(banner.t,W/2,H/2);g.globalAlpha=1;}
  g.restore();
}
setLayout();connect();requestAnimationFrame(frame);
</script>
</body></html>
)rawliteral";
