#pragma once
// AFFENSCHLACHT browser client: canvas renderer, sliders, WebAudio fart synthesizer.
const char APES_PAGE[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="de"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<title>AFFENSCHLACHT</title>
<style>
  :root{--ink:#fff3d6;--dim:#c9a97a;--card:#2b1a0e;--hot:#ffb02e}
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:#1a0f07;color:var(--ink);font-family:"Trebuchet MS",Verdana,sans-serif;
    display:flex;flex-direction:column;align-items:center;min-height:100vh;padding:8px 10px 20px;user-select:none;-webkit-user-select:none}
  h1{font-size:clamp(22px,5vw,40px);letter-spacing:3px;color:var(--hot);text-shadow:3px 3px 0 #6b3f1d;margin:4px 0}
  h1 small{font-size:.4em;color:var(--dim);letter-spacing:1px;text-shadow:none;display:block;text-align:center}
  #wrap{width:100%;max-width:900px}
  canvas{width:100%;display:block;border:4px solid #6b3f1d;border-radius:12px;background:#000;touch-action:none}
  #ctl{background:var(--card);border-radius:12px;padding:12px 14px;margin-top:10px;display:grid;gap:10px}
  #status{font-weight:700;min-height:22px;color:var(--hot)}
  label{display:grid;grid-template-columns:76px 1fr 54px;align-items:center;gap:10px;font-size:15px}
  label b{text-align:right;color:var(--hot)}
  input[type=range]{width:100%;height:30px;accent-color:#ff8a1f}
  button{font:inherit;cursor:pointer;border:0;border-radius:10px;color:#2b1a0e;background:var(--hot);font-weight:700;padding:8px 12px}
  button:disabled{opacity:.35;cursor:default}
  #fire{font-size:24px;padding:14px;letter-spacing:2px;background:linear-gradient(#ffcf4d,#ff8a1f)}
  #taunts{display:flex;gap:8px;flex-wrap:wrap}
  #taunts button{flex:1;background:#6b3f1d;color:var(--ink);min-width:90px}
  #log{margin-top:10px;font-size:13px;line-height:1.6;color:var(--dim);min-height:60px}
  #log div:first-child{color:var(--ink)}
  #foot{margin-top:10px;font-size:12px;color:var(--dim);display:flex;justify-content:space-between}
  #foot a{color:var(--hot)}
  #conn{color:#ff6b6b;font-size:13px;min-height:16px}
  #ov{position:fixed;inset:0;background:#1a0f07ee;display:flex;align-items:center;justify-content:center;z-index:9;padding:16px}
  .card{max-width:420px;text-align:center}
  .card h2{font-size:34px;color:var(--hot);text-shadow:3px 3px 0 #6b3f1d;letter-spacing:2px}
  .card p{margin:10px 0;color:var(--dim);font-size:14px;line-height:1.55}
  .card input[type=text]{width:100%;font:inherit;font-size:20px;padding:10px;border-radius:10px;border:3px solid #6b3f1d;background:#3a2412;color:var(--ink);text-align:center;outline:none}
  .card button{width:100%;font-size:22px;margin-top:12px;padding:12px}
  #mute{position:fixed;top:8px;right:10px;background:#6b3f1d;color:var(--ink);padding:4px 10px;z-index:5}
</style></head>
<body>
<button id="mute">🔊</button>
<div id="wrap">
  <h1>🐒 AFFENSCHLACHT <small>die Kackwurf-Edition</small></h1>
  <canvas id="cv" width="800" height="450"></canvas>
  <div id="conn"></div>
  <div id="ctl">
    <div id="status">Verbinde…</div>
    <label>Winkel<input id="a" type="range" min="2" max="88" value="50"><b id="av">50°</b></label>
    <label>Kraft<input id="p" type="range" min="5" max="100" value="65"><b id="pv">65%</b></label>
    <button id="fire" disabled>💩 WERFEN!</button>
    <div id="taunts"><button data-t="0">💨 Furz</button><button data-t="1">🙈 Uuh-Uuh!</button><button data-t="2">👅 Bäh</button></div>
  </div>
  <div id="log"></div>
  <div id="foot"><span>Tasten: ↑↓ Winkel · ←→ Kraft · Leertaste wirft</span><a href="/">⬅ zu POLARITY</a></div>
</div>
<div id="ov"><div class="card">
  <h2>🐒 AFFENSCHLACHT</h2>
  <p>Zwei Affen. Eine Mauer. Kein Klopapier.<br>Stell Winkel und Kraft ein und wirf deine Kacke über die Mauer. Kisten im Flug liefern Power-Ups. Wer null Leben hat, stinkt ab.</p>
  <input type="text" id="nm" maxlength="12" autocomplete="off">
  <button id="go">MITSPIELEN</button>
  <p style="font-size:12px">Allein? Dann spielt die Gorilla-KI mit. Zu zweit: einfach zweites Handy öffnen.</p>
</div></div>
<script>
'use strict';
const W=800,H=450,CW=4,NC=200,WA=96,WB=103;
const $=id=>document.getElementById(id), cv=$('cv'), g=cv.getContext('2d');
const PU=['','Mega-Haufen','Dünnschiss-Salve','Gummi-Kacke','Furz-Antrieb','Heilbanane','Platztausch','Klopapier-Schild','Verstopfung'];
const PUI=['','💥','💩💩💩','🏀','💨','🍌','🔀','🧻','🤢'];
const NAMES=['Bananen-Bernd','Kack-Kevin','Stinky Steve','Mist-Max','Affen-Alfred','Furz-Frank','Gorilla-Gisela','Lecker-Lutz'];
const FLAVOR=['%s kratzt sich am Hintern und denkt nach…','%s wühlt in der Munition…','%s flucht auf Affisch…','%s übt noch kurz den Wurfarm…'];
const WORDS=['PLATSCH!','FLATSCH!','SPLAT!','PFFFT!','BLÖRP!','MATSCH!'];
let ws,me=-1,S=null,HH=new Int16Array(NC),joined=null,anim=null,parts=[],pops=[],bubbles=[],bq=[],bqEnd=0;
let ang=50,pow=65,rx=0,thrown=false,hurt=[0,0],lastHp=[100,100],aim=[40,40],flavor=0,muted=false,shake=0,AC=null,lastTurnKey='';

// ------------------------------------------------------------- network ---
function connect(){
  ws=new WebSocket('ws://'+location.hostname+':82/');
  ws.onopen=()=>{$('conn').textContent='';if(joined)ws.send('j'+joined);};
  ws.onclose=()=>{$('conn').textContent='Verbindung weg – der Affe sucht den Stecker…';setTimeout(connect,1000);};
  ws.onmessage=e=>handle(JSON.parse(e.data));
}
function handle(m){
  if(m.k==='you'){me=m.i;$('ov').style.display='none';if(me<0)addLog('Alles besetzt – du schaust zu.');}
  else if(m.k==='st'){
    S=m;rx=performance.now();thrown=false;HH=Int16Array.from(m.h);
    for(let i=0;i<2;i++){if(m.hp[i]<lastHp[i])hurt[i]=performance.now()+1300;lastHp[i]=m.hp[i];}
    flavor=Math.floor(Math.random()*FLAVOR.length);
    m.log.forEach(addLog);
    const key=m.ph+':'+m.turn+':'+m.wind+':'+m.hp.join();
    if(m.ph===1&&m.turn===me&&key!==lastTurnKey)blip([660,880],.1);
    if(m.ph===3&&m.win>=0&&m.win!==2)win();
    lastTurnKey=key;ui();
  }
  else if(m.k==='shot'){
    anim={t0:performance.now(),p:m.p,ex:m.ex,done:m.p.map(()=>false),end:0,by:m.by};
    const p=m.p[0];if(p.length>3)aim[m.by]=Math.atan2(p[3]-p[1],Math.abs(p[2]-p[0]))*57.3;
    whoosh();
  }
  else if(m.k==='taunt'){
    const t=['*PFFFRRRT*','UUH UUH AAH AAH!','Bäääääh!'][m.n];
    bubbles.push({i:m.by,t,until:performance.now()+1800});[fart,monkey,raspberry][m.n]();
  }
}
function addLog(t){
  const d=document.createElement('div');d.textContent=t;$('log').prepend(d);
  while($('log').children.length>7)$('log').lastChild.remove();
  const now=performance.now();bqEnd=Math.max(bqEnd,now);bq.push({t,s:bqEnd});bqEnd+=2800;
}
// ------------------------------------------------------------------ UI ---
function ui(){
  const myTurn=S&&S.ph===1&&S.turn===me&&!thrown&&!anim;
  $('fire').disabled=!myTurn;
  const cap=S&&me>=0&&S.cp[me];
  $('p').max=cap?50:100;if(pow>$('p').max){pow=+$('p').max;$('p').value=pow;}
  $('av').textContent=ang+'°';$('pv').textContent=pow+'%'+(cap?' (verstopft)':'');
  let s='';
  if(!S)s='Warte auf Affen…';
  else if(S.ph===3)s=S.win===2?'Unentschieden – ihr stinkt beide.':S.n[S.win]+' gewinnt!';
  else if(me<0)s='Du schaust zu.';
  else if(S.turn===me)s='DEIN ZUG!'+(S.t?' Noch '+Math.max(0,Math.round(S.t-(performance.now()-rx)/1000))+'s':'');
  else s=FLAVOR[flavor].replace('%s',S.n[S.turn]);
  $('status').textContent=s;
}
$('a').oninput=e=>{ang=+e.target.value;ui();};
$('p').oninput=e=>{pow=+e.target.value;ui();};
function fire(){
  if($('fire').disabled)return;
  ws.send('t'+ang+','+pow);thrown=true;ui();
}
$('fire').onclick=fire;
document.querySelectorAll('#taunts button').forEach(b=>b.onclick=()=>{audio();if(ws.readyState===1)ws.send('x'+b.dataset.t);});
$('mute').onclick=()=>{muted=!muted;$('mute').textContent=muted?'🔇':'🔊';};
addEventListener('keydown',e=>{
  if(e.target.id==='nm')return;
  const k=e.key;let c=true;
  if(k==='ArrowUp')ang=Math.min(88,ang+1);else if(k==='ArrowDown')ang=Math.max(2,ang-1);
  else if(k==='ArrowRight')pow=Math.min(+$('p').max,pow+1);else if(k==='ArrowLeft')pow=Math.max(5,pow-1);
  else if(k===' '||k==='Enter')fire();else c=false;
  if(c){e.preventDefault();$('a').value=ang;$('p').value=pow;ui();}
});
$('nm').value=localStorage.getItem('apename')||NAMES[Math.floor(Math.random()*NAMES.length)];
function join(){
  audio();joined=$('nm').value.trim()||'Affe';localStorage.setItem('apename',joined);
  if(ws.readyState===1)ws.send('j'+joined);
}
$('go').onclick=join;$('nm').onkeydown=e=>{if(e.key==='Enter')join();};
setInterval(ui,500);

// --------------------------------------------------------------- sound ---
function audio(){
  if(!AC){try{AC=new(window.AudioContext||window.webkitAudioContext)();}catch(e){}}
  if(AC&&AC.state==='suspended')AC.resume();return AC;
}
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
const fart=()=>{tone(120,45,.7,'sawtooth',.5,26,28);noise(.5,300,'lowpass',.4);};
const monkey=()=>{[0,.18,.4,.58].forEach((d,i)=>tone(i%2?420:520,i%2?380:640,.15,'triangle',.4,12,40,d));};
const raspberry=()=>{tone(220,140,.6,'square',.22,38,70);};
const whoosh=()=>noise(.45,1800,'bandpass',.5);
const splat=()=>{noise(.4,700,'lowpass',.9);tone(160,40,.25,'sine',.6);};
const blip=(fs,d)=>fs.forEach((f,i)=>tone(f,f,d,'square',.15,0,0,i*d));
const win=()=>[523,659,784,1047].forEach((f,i)=>tone(f,f,.2,'triangle',.3,0,0,i*.15));

// ----------------------------------------------------------- rendering ---
const wy=y=>H-y;
function carveLocal(cx,cy,R){
  for(let c=Math.max(0,Math.floor((cx-R)/CW));c<=Math.min(NC-1,Math.floor((cx+R)/CW));c++){
    const dx=c*CW+CW/2-cx;if(Math.abs(dx)>R)continue;
    const low=Math.max(0,cy-Math.sqrt(R*R-dx*dx));if(HH[c]>low)HH[c]=Math.floor(low);
  }
}
function splatFx(x,y,R){
  for(let i=0;i<R*1.2;i++){const a=Math.random()*6.28,v=60+Math.random()*220;
    parts.push({x,y,vx:Math.cos(a)*v,vy:Math.sin(a)*v+120,life:.7+Math.random()*.8,r:2+Math.random()*4,c:Math.random()<.8?'#6b3f1d':'#8a5a2b'});}
  pops.push({x,y:y+20,t:WORDS[Math.floor(Math.random()*WORDS.length)],until:performance.now()+900,s:R>40?34:24});
  shake=R>40?14:6;splat();
}
function poop(x,y,s,r,stink){
  g.save();g.translate(x,y);g.rotate(r);g.scale(s,s);
  if(stink){g.strokeStyle='#7ddc4f';g.lineWidth=1.5;const t=performance.now()/200;
    for(let i=-1;i<=1;i++){g.beginPath();for(let k=0;k<=8;k++)g.lineTo(i*5+Math.sin(t+k*.9+i)*2,-12-k*2);g.stroke();}}
  g.fillStyle='#6b3f1d';
  [[0,5,10,6],[0,-1,8,5],[0,-6,5.5,4]].forEach(([cx,cy,a,b])=>{g.beginPath();g.ellipse(cx,cy,a,b,0,0,7);g.fill();});
  g.beginPath();g.moveTo(-2,-8);g.quadraticCurveTo(0,-15,3,-9);g.fill();
  g.fillStyle='#fff';g.beginPath();g.arc(-3,0,2.2,0,7);g.arc(3,0,2.2,0,7);g.fill();
  g.fillStyle='#000';g.beginPath();g.arc(-2.6,.4,1,0,7);g.arc(3.4,.4,1,0,7);g.fill();
  g.restore();
}
function drawApe(i,t,now){
  const x=S.x[i],fy=wy(S.y[i]),d=i?-1:1,dead=S.hp[i]<=0,ouch=now<hurt[i];
  const bob=dead?0:Math.sin(t/260+i*2)*1.6;
  const fur=i?'#a8541f':'#5b3a1e',light='#e0b185';
  g.save();g.translate(x,fy);
  g.fillStyle='#0004';g.beginPath();g.ellipse(0,1,20,5,0,0,7);g.fill();
  if(dead){g.rotate(d*-1.35);g.translate(0,-6);}
  g.lineCap='round';
  g.strokeStyle=fur;g.lineWidth=8;g.beginPath();g.moveTo(-6,-14);g.lineTo(-8,-2);g.moveTo(6,-14);g.lineTo(8,-2);g.stroke();
  g.fillStyle=fur;g.beginPath();g.ellipse(0,-28+bob,15,18,0,0,7);g.fill();
  g.fillStyle=light;g.beginPath();g.ellipse(d*2,-26+bob,9,12,0,0,7);g.fill();
  // back arm
  g.strokeStyle=fur;g.lineWidth=7;g.beginPath();g.moveTo(-d*11,-38+bob);g.lineTo(-d*14,-20+bob);g.stroke();
  // head
  g.fillStyle=fur;g.beginPath();g.arc(0,-54+bob,14,0,7);g.arc(-13,-55+bob,5,0,7);g.arc(13,-55+bob,5,0,7);g.fill();
  g.fillStyle=light;g.beginPath();g.arc(-13,-55+bob,2.6,0,7);g.arc(13,-55+bob,2.6,0,7);g.ellipse(d*3,-51+bob,10,9,0,0,7);g.fill();
  g.fillStyle='#fff';g.beginPath();g.arc(d*0,-56+bob,3.6,0,7);g.arc(d*8,-56+bob,3.6,0,7);g.fill();
  g.strokeStyle='#000';g.fillStyle='#000';g.lineWidth=1.6;
  if(dead||ouch){[0,8].forEach(o=>{const ex=d*o,ey=-56+bob;g.beginPath();g.moveTo(ex-2.5,ey-2.5);g.lineTo(ex+2.5,ey+2.5);g.moveTo(ex+2.5,ey-2.5);g.lineTo(ex-2.5,ey+2.5);g.stroke();});
    g.beginPath();g.ellipse(d*4,-46+bob,3,4,0,0,7);g.fill();}
  else{g.beginPath();g.arc(d*1.5,-56+bob,1.6,0,7);g.arc(d*9.5,-56+bob,1.6,0,7);g.fill();
    g.beginPath();g.arc(d*4,-49+bob,4,.2,2.9);g.stroke();}
  g.beginPath();g.arc(d*2,-52+bob,.9,0,7);g.arc(d*6,-52+bob,.9,0,7);g.fill();
  // front arm (aims along the launch angle)
  const aimA=(me===i&&S.ph===1&&S.turn===i?ang:aim[i])*Math.PI/180,sx=d*11,sy=-38+bob;
  const hx=sx+d*Math.cos(aimA)*22,hy=sy-Math.sin(aimA)*22;
  g.strokeStyle=fur;g.lineWidth=7;g.beginPath();g.moveTo(sx,sy);g.lineTo(hx,hy);g.stroke();
  g.fillStyle=light;g.beginPath();g.arc(hx,hy,4.5,0,7);g.fill();
  if(S.ph===1&&S.turn===i&&!anim&&!dead)poop(hx+d*1,hy-8,.8,Math.sin(t/200)*.2,true);
  if(ouch){for(let k=0;k<3;k++){const a=t/180+k*2.1;g.fillStyle='#ffe14d';g.beginPath();g.arc(Math.cos(a)*18,-72+bob+Math.sin(a)*5,3,0,7);g.fill();}}
  g.restore();
  if(S.sh[i]){g.strokeStyle='#fff';g.lineWidth=2;g.setLineDash([4,3]);g.beginPath();g.arc(x,fy-30,34,0,7);g.stroke();g.setLineDash([]);}
  // name tag + turn marker
  g.font='bold 12px Trebuchet MS,sans-serif';g.textAlign='center';g.fillStyle='#fff';g.strokeStyle='#000';g.lineWidth=3;
  g.strokeText(S.n[i],x,fy-84);g.fillText(S.n[i],x,fy-84);
  if(S.ph===1&&S.turn===i&&!dead){g.font='22px sans-serif';g.fillText('🍌',x,fy-96-Math.abs(Math.sin(t/250))*8);}
}
function crateAt(x,y,t){
  g.save();g.translate(x,wy(y)+Math.sin(t/400+x)*3);
  g.fillStyle='#d99a3d';g.fillRect(-14,-14,28,28);g.strokeStyle='#7a4a12';g.lineWidth=2;g.strokeRect(-14,-14,28,28);
  g.beginPath();g.moveTo(-14,-14);g.lineTo(14,14);g.moveTo(14,-14);g.lineTo(-14,14);g.stroke();
  g.fillStyle='#fff';g.fillRect(-8,-9,16,18);g.fillStyle='#d6212a';g.font='bold 16px sans-serif';g.textAlign='center';g.fillText('?',0,6);
  g.strokeStyle='#444';g.lineWidth=2;const a=t/40;g.beginPath();g.moveTo(-14*Math.cos(a),-18);g.lineTo(14*Math.cos(a),-18);g.moveTo(0,-14);g.lineTo(0,-18);g.stroke();
  g.restore();
}
function scene(t,now){
  // sky
  const sk=g.createLinearGradient(0,0,0,H);sk.addColorStop(0,'#3b2a6b');sk.addColorStop(.55,'#e8744f');sk.addColorStop(1,'#ffc46b');
  g.fillStyle=sk;g.fillRect(0,0,W,H);
  // sun with sunglasses
  g.fillStyle='#ffe27a';g.beginPath();g.arc(620,120,42,0,7);g.fill();
  g.fillStyle='#111';g.fillRect(594,108,24,10);g.fillRect(622,108,24,10);g.fillRect(616,111,8,3);
  g.strokeStyle='#111';g.lineWidth=3;g.beginPath();g.arc(620,132,14,.25,2.9);g.stroke();
  // clouds drifting with the wind
  const wd=S?S.wind:0;g.fillStyle='#ffffff55';
  for(let i=0;i<4;i++){const cx=((i*230+t*wd*.02)%1000+1000)%1000-100,cy=40+i*28;
    g.beginPath();g.ellipse(cx,cy,50,14,0,0,7);g.ellipse(cx+30,cy-6,30,12,0,0,7);g.fill();}
  // palms on the horizon
  g.strokeStyle='#2a1a2e';g.fillStyle='#2a1a2e';g.lineWidth=5;
  [60,250,520,760].forEach((px,i)=>{const sw=Math.sin(t/900+i)*4;g.beginPath();g.moveTo(px,H-90);g.quadraticCurveTo(px+10,H-150,px+sw,H-190);g.stroke();
    for(let k=0;k<5;k++){const a=-2.6+k*.55;g.beginPath();g.moveTo(px+sw,H-190);g.quadraticCurveTo(px+sw+Math.cos(a)*25,H-190+Math.sin(a)*25-12,px+sw+Math.cos(a)*45,H-190+Math.sin(a)*45+8);g.lineWidth=3;g.stroke();}
    g.lineWidth=5;});
  // terrain
  g.beginPath();g.moveTo(0,H);for(let c=0;c<NC;c++){g.lineTo(c*CW,wy(HH[c]));g.lineTo(c*CW+CW,wy(HH[c]));}g.lineTo(W,H);g.closePath();
  const dg=g.createLinearGradient(0,H-200,0,H);dg.addColorStop(0,'#8a5a2b');dg.addColorStop(1,'#3d2412');g.fillStyle=dg;g.fill();
  g.strokeStyle='#5fb04a';g.lineWidth=4;g.beginPath();
  for(let c=0;c<NC;c++){if(c>=WA&&c<=WB){g.stroke();g.beginPath();continue;}g.lineTo(c*CW+2,wy(HH[c])+1);}g.stroke();
  // the wall
  for(let c=WA;c<=WB;c++){const h=HH[c];if(h<=0)continue;g.fillStyle=c%2?'#8d8d96':'#9a9aa3';g.fillRect(c*CW,wy(h),CW,h);}
  g.strokeStyle='#6b6b73';g.lineWidth=1;
  for(let y=0;y<260;y+=16){const yy=wy(y);let a=-1;
    for(let c=WA;c<=WB;c++){if(HH[c]>y){if(a<0)a=c;}else if(a>=0){g.beginPath();g.moveTo(a*CW,yy);g.lineTo(c*CW,yy);g.stroke();a=-1;}}
    if(a>=0){g.beginPath();g.moveTo(a*CW,yy);g.lineTo((WB+1)*CW,yy);g.stroke();}}
  g.fillStyle='#fff';g.font='bold 11px sans-serif';g.textAlign='center';
  g.save();g.translate(400,wy(HH[WA]*.5));g.rotate(-Math.PI/2);g.fillText('DIE GROSSE MAUER DES GESTANKS',0,4);g.restore();
}
function hud(now){
  g.textAlign='left';g.font='bold 13px Trebuchet MS,sans-serif';
  for(let i=0;i<2;i++){
    const x=i?W-210:10,hp=S.hp[i],c=hp>50?'#59d14b':(hp>25?'#ffc02e':'#ff4a3a');
    g.fillStyle='#000a';g.fillRect(x,8,200,36);
    g.fillStyle='#fff';g.textAlign='left';g.fillText(S.n[i]+(S.b[i]?' 🤖':''),x+6,22);
    g.fillStyle='#3a2412';g.fillRect(x+6,27,188,10);g.fillStyle=c;g.fillRect(x+6,27,188*hp/100,10);
    g.textAlign='right';g.fillStyle='#fff';g.fillText(hp,x+194,22);
    let chips=[];if(S.pu[i])chips.push(PUI[S.pu[i]]+' '+PU[S.pu[i]]);if(S.sh[i])chips.push('🧻 Schild');if(S.cp[i])chips.push('🤢 verstopft');
    if(chips.length){g.font='bold 11px Trebuchet MS,sans-serif';g.textAlign=i?'right':'left';g.fillStyle='#ffe27a';g.strokeStyle='#000';g.lineWidth=3;
      const tx=i?x+200:x;g.strokeText(chips.join(' · '),tx,58);g.fillText(chips.join(' · '),tx,58);g.font='bold 13px Trebuchet MS,sans-serif';}
  }
  // wind
  const w=S.wind;g.fillStyle='#000a';g.fillRect(W/2-60,8,120,36);g.fillStyle='#fff';g.textAlign='center';g.fillText('WIND '+Math.abs(w)+(w<0?' ◀':w>0?' ▶':''),W/2,22);
  g.strokeStyle='#8fe3ff';g.lineWidth=2;const off=(now/60*Math.sign(w))%12;
  for(let k=0;k<7;k++){const ax=W/2-42+((k*12+off+84)%84);if(!w)break;g.beginPath();g.moveTo(ax,34);g.lineTo(ax+Math.sign(w)*Math.min(10,2+Math.abs(w)/5),34);g.stroke();}
}
function draw(now){
  requestAnimationFrame(draw);
  const t=now,dt=Math.min(.05,(now-(draw.l||now))/1000);draw.l=now;
  g.save();
  if(shake>.5){g.translate((Math.random()-.5)*shake,(Math.random()-.5)*shake);shake*=.88;}
  scene(t,now);
  if(!S){g.fillStyle='#fff';g.font='bold 22px sans-serif';g.textAlign='center';g.fillText('Die Affen ziehen sich noch an…',W/2,H/2);g.restore();return;}
  S.cr.forEach(c=>{if(c[0]>=0)crateAt(c[0],c[1],t);});
  for(let i=0;i<2;i++)drawApe(i,t,now);
  // preview stub of my throw
  if(me>=0&&S.ph===1&&S.turn===me&&!thrown&&!anim){
    const d=me?-1:1,r=ang*Math.PI/180,v=pow/100*700;let x=S.x[me]+d*18,y=S.y[me]+52,vx=d*v*Math.cos(r),vy=v*Math.sin(r);
    g.fillStyle='#fff9';
    for(let s=0;s<30;s++){vx+=S.wind/60;vy-=450/60;x+=vx/60;y+=vy/60;if(s%3===2){g.beginPath();g.arc(x,wy(y),2.2,0,7);g.fill();}}
  }
  // flying poop
  if(anim){
    let all=true;const f=(now-anim.t0)/50;
    anim.p.forEach((p,k)=>{const n=p.length/2;
      if(f<n-1){all=false;const i=Math.floor(f),u=f-i,x=p[i*2]+(p[i*2+2]-p[i*2])*u,y=p[i*2+1]+(p[i*2+3]-p[i*2+1])*u;poop(x,wy(y),1,t/90+k,false);}
      else if(!anim.done[k]){anim.done[k]=true;const e=anim.ex[k];if(e.length){carveLocal(e[0],e[1],e[2]);splatFx(e[0],wy(e[1]),e[2]);}}
    });
    if(all&&!anim.end)anim.end=now+1800;if(anim.end&&now>anim.end){anim=null;ui();}
  }
  // particles
  parts=parts.filter(p=>(p.life-=dt)>0);
  parts.forEach(p=>{p.vy-=520*dt;p.x+=p.vx*dt;p.y-=p.vy*dt;g.fillStyle=p.c;g.beginPath();g.arc(p.x,p.y,p.r,0,7);g.fill();});
  pops=pops.filter(p=>now<p.until);
  pops.forEach(p=>{const u=1-(p.until-now)/900;g.font='bold '+p.s+'px Impact,sans-serif';g.textAlign='center';g.fillStyle='#ffe14d';g.strokeStyle='#6b1d0a';g.lineWidth=4;g.strokeText(p.t,p.x,p.y-u*40);g.fillText(p.t,p.x,p.y-u*40);});
  // speech bubbles
  bubbles=bubbles.filter(b=>now<b.until);
  bubbles.forEach(b=>{const x=S.x[b.i],y=wy(S.y[b.i])-112;g.font='bold 13px sans-serif';const tw=g.measureText(b.t).width+16;
    g.fillStyle='#fff';g.beginPath();g.roundRect(x-tw/2,y-22,tw,24,8);g.fill();g.beginPath();g.moveTo(x-5,y+2);g.lineTo(x,y+10);g.lineTo(x+5,y+2);g.fill();
    g.fillStyle='#000';g.textAlign='center';g.fillText(b.t,x,y-5);});
  hud(now);
  // commentary banner
  const cur=bq.find(b=>now>=b.s&&now<b.s+2800);bq=bq.filter(b=>now<b.s+2800);
  if(cur){g.font='bold 15px Trebuchet MS,sans-serif';const tw=Math.min(W-20,g.measureText(cur.t).width+24);
    g.fillStyle='#000b';g.beginPath();g.roundRect(W/2-tw/2,H-44,tw,30,10);g.fill();g.fillStyle='#ffe27a';g.textAlign='center';g.fillText(cur.t,W/2,H-24,tw-12);}
  if(S.ph===3){g.font='bold 44px Impact,sans-serif';g.textAlign='center';g.fillStyle='#ffe14d';g.strokeStyle='#6b1d0a';g.lineWidth=6;
    const tx=S.win===2?'UNENTSCHIEDEN!':S.n[S.win]+' GEWINNT!';g.strokeText(tx,W/2,150);g.fillText(tx,W/2,150);}
  g.restore();
}
connect();requestAnimationFrame(draw);
</script>
</body></html>
)rawliteral";
