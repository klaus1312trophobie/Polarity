#pragma once
// Game selection page, served at "/" (also the captive-portal landing page).
const char MENU_PAGE[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="de"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP Arcade</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{min-height:100vh;background:radial-gradient(circle at 50% 0,#232a52,#07080f 70%);color:#e8ecff;
    font-family:"Trebuchet MS",Verdana,sans-serif;display:flex;flex-direction:column;align-items:center;padding:24px 16px}
  h1{font-size:clamp(28px,7vw,48px);letter-spacing:6px;margin:10px 0 4px;text-align:center}
  .sub{color:#8a90b8;font-size:14px;margin-bottom:26px;text-align:center}
  .grid{display:grid;gap:18px;width:100%;max-width:760px;grid-template-columns:repeat(auto-fit,minmax(280px,1fr))}
  a.game{display:block;text-decoration:none;color:inherit;border-radius:18px;padding:26px 22px;min-height:230px;
    border:3px solid transparent;transition:transform .15s;position:relative;overflow:hidden}
  a.game:active,a.game:hover{transform:scale(1.03)}
  a.game h2{font-size:30px;letter-spacing:3px;margin-bottom:8px}
  a.game p{font-size:14px;line-height:1.55;opacity:.9}
  .ic{font-size:54px;margin-bottom:10px}
  .pol{background:linear-gradient(135deg,#1b0a2b,#05162b);border-color:#2ee6ff55}
  .pol h2{color:#2ee6ff;text-shadow:0 0 14px #2ee6ff}
  .pol h2 b{color:#ff3b6b;text-shadow:0 0 14px #ff3b6b}
  .ape{background:linear-gradient(135deg,#4a2a10,#a8541f);border-color:#ffb02e88}
  .ape h2{color:#ffe27a;text-shadow:3px 3px 0 #4a2a10}
  .kl{background:linear-gradient(135deg,#1c2229,#3a424c);border-color:#ffcf5a88}
  .kl h2{color:#ffcf5a;text-shadow:3px 3px 0 #5a4310}
  .live{position:absolute;right:14px;top:14px;font-size:12px;background:#000a;border-radius:99px;padding:4px 10px;min-height:22px}
  #mus{position:fixed;right:12px;top:12px;font-size:18px;background:#000a;color:#e8ecff;border:2px solid #ffffff33;border-radius:99px;
    padding:8px 14px;font-size:14px;cursor:pointer;font-family:inherit}
  .foot{margin-top:28px;color:#8a90b8;font-size:12px;text-align:center;line-height:1.7}
</style></head>
<body>
  <button id="mus" class="mutebtn">🔊</button>
  <h1>ESP ARCADE</h1>
  <div class="sub">Drei Spiele, ein kleiner Chip. Such dir eins aus.</div>
  <div class="grid">
    <a class="game pol" href="/polarity">
      <div class="live" id="lp"></div>
      <div class="ic">🧲</div>
      <h2>POLARI<b>TY</b></h2>
      <p>Magnet-Arena für bis zu 8 Spieler. Gleiche Pole stoßen sich ab, gegensätzliche ziehen sich an. Sammle Energie und schubse die anderen von der schrumpfenden Arena.</p>
    </a>
    <a class="game ape" href="/apes">
      <div class="live" id="la"></div>
      <div class="ic">🐒💩</div>
      <h2>AFFENSCHLACHT</h2>
      <p>Zwei Affen, eine Mauer, jede Menge Kacke. Rundenbasiert über die Mauer werfen, Power-Ups einsammeln, den Gegner einsauen. Ist besetzt? Dann schaust du zu und löst den Verlierer ab.</p>
    </a>
    <a class="game kl" href="/klaus">
      <div class="live" id="lk"></div>
      <div class="ic">🛗💨</div>
      <h2>KLAUSTROPHOBIE</h2>
      <p>Ein überfüllter Fahrstuhl, neun Etagen, null Luft. Zwiebelbrot-Karl, die Parfümtante und ein Hund steigen zu. Furze, ellbogen und drück Tür-zu, bis nur noch einer drinsteht. Bis zu 6 Spieler.</p>
    </a>
  </div>
  <div class="foot">Tipp: Zum Wechseln einfach hierher zurück (⬅ Spielauswahl).<br>Bots füllen leere Plätze auf.</div>
<script src="/sfx.js"></script>
<script>
async function poll(){
  try{
    const h=await (await fetch('/health')).json();
    document.getElementById('lp').textContent=h.humans?h.humans+' spielen':'frei';
    const k=h.klaus;
    document.getElementById('lk').textContent=k.humans?k.humans+' fahren'+(k.queue?', '+k.queue+' warten':''):'frei';
    const a=h.apes;
    document.getElementById('la').textContent=a.humans>=2?(a.queue?a.queue+' warten':'läuft – zuschauen'):(a.humans?'1 wartet auf dich':'frei');
  }catch(e){}
  setTimeout(poll,3000);
}
poll();

// 16-bit style chiptune: pulse lead + triangle bass + noise hat, looped
(function(){
  const E=60/113/2;                       // eighth note @113 bpm
  const D=[0,2,4,5,7,9];                  // major scale degrees 1..6 (key of Ab)
  const n=d=>D[d-1];
  const intro=[[n(1),1],[n(2),1],[n(4),1],[n(2),1]];
  const A=[[n(1),1],[n(2),1],[n(4),1],[n(2),1]];
  // "give you up" / "let you down" share one tune, "run around and desert you" differs - 8 eighths each
  const B=[[n(6),1],[n(6),1],[n(5),1.5],[null,.5]];
  const mel=[].concat(
    A,B,
    A,B,
    A,[[n(4),1],[n(4),.5],[n(4),.5],[n(5),.5],[n(3),.5],[n(2),.5],[n(1),.5]]);
  const bassRoots=[-7,-5,-7,-5,-7,-12];
  const f=s=>415.3*Math.pow(2,s/12);
  let ac=null,on=false,timer=null,t0=0,idx=0,bi=0,noiseBuf=null;
  const btn=document.getElementById('mus');
  function pulse(freq,t,dur,vol,duty){
    const o=ac.createOscillator(),g=ac.createGain();
    const re=new Float32Array(32),im=new Float32Array(32);
    for(let k=1;k<32;k++)im[k]=2*Math.sin(Math.PI*k*duty)/(Math.PI*k);
    o.setPeriodicWave(ac.createPeriodicWave(re,im));
    o.frequency.value=freq;
    g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(vol,t+.01);
    g.gain.setValueAtTime(vol,t+dur*.7);g.gain.linearRampToValueAtTime(0,t+dur);
    o.connect(g).connect(SFX.out);o.start(t);o.stop(t+dur+.02);
  }
  function tri(freq,t,dur,vol){
    const o=ac.createOscillator(),g=ac.createGain();
    o.type='triangle';o.frequency.value=freq;
    g.gain.setValueAtTime(vol,t);g.gain.linearRampToValueAtTime(0,t+dur);
    o.connect(g).connect(SFX.out);o.start(t);o.stop(t+dur+.02);
  }
  function hat(t){
    const s=ac.createBufferSource(),g=ac.createGain(),h=ac.createBiquadFilter();
    s.buffer=noiseBuf;h.type='highpass';h.frequency.value=6000;
    g.gain.setValueAtTime(.05,t);g.gain.linearRampToValueAtTime(0,t+.04);
    s.connect(h).connect(g).connect(SFX.out);s.start(t);s.stop(t+.05);
  }
  let pos=0;                              // position in eighths
  function sched(){
    while(t0<ac.currentTime+.4){
      const [semi,len]=mel[idx];
      if(semi!==null){pulse(f(semi),t0,len*E*.9,.09,.25);pulse(f(semi+12),t0,len*E*.9,.025,.5);}
      for(let k=Math.ceil(pos);k<pos+len-1e-6;k++){
        const t=t0+(k-pos)*E;
        if(k%4===0)tri(f(bassRoots[(k/4)%bassRoots.length]),t,E*3.5,.16);
        else if(k%2===1)hat(t);
      }
      t0+=len*E;pos+=len;idx=(idx+1)%mel.length;
      if(idx===0)pos=0;
    }
    timer=setTimeout(sched,100);
  }
  function start(){
    if(on)return;
    try{
      ac=SFX.ctx();if(!ac)return;
      if(!noiseBuf){noiseBuf=ac.createBuffer(1,2048,ac.sampleRate);
        const d=noiseBuf.getChannelData(0);for(let i=0;i<d.length;i++)d[i]=Math.random()*2-1;}
      ac.resume();
      if(ac.state!=='running'&&ac.state!=='suspended')return;
      on=true;t0=ac.currentTime+.1;idx=0;pos=0;sched();
    }catch(e){}
  }
  function stop(){on=false;clearTimeout(timer);}
  SFX.onMute(m=>{m?stop():start();});
  // browsers block autoplay: start on first interaction unless user muted
  const first=()=>{if(ac&&ac.state!=="running")ac.resume();if(!SFX.muted&&!on)start();window.removeEventListener('pointerdown',first);window.removeEventListener('keydown',first);};
  window.addEventListener('pointerdown',first);window.addEventListener('keydown',first);
  if(!SFX.muted)start();
})();
</script>
</body></html>
)rawliteral";
