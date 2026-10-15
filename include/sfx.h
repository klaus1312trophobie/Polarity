#pragma once
// Shared chiptune sound engine, served at /sfx.js and loaded by every arcade page.
// One global mute (localStorage "arcade-mute") for the menu and all games.
const char SFX_JS[] PROGMEM = R"rawliteral(
const SFX=(()=>{
  let ac=null,out=null,noiseBuf=null,muted=false;const pw={},cbs=[];
  try{muted=localStorage.getItem('arcade-mute')==='1';}catch(e){}
  const clamp=(v,a,b)=>Math.max(a,Math.min(b,v));
  const mtof=m=>440*Math.pow(2,(m-69)/12);
  function ctx(){
    if(!ac){
      try{
        ac=new(window.AudioContext||window.webkitAudioContext)();
        const comp=ac.createDynamicsCompressor(),g=ac.createGain();
        g.gain.value=muted?0:.9;g.connect(comp);comp.connect(ac.destination);out=g;
      }catch(e){ac=null;return null;}
    }
    if(ac.state==='suspended')ac.resume().catch(()=>{});
    return ac;
  }
  function wave(duty){
    if(!pw[duty]){
      const re=new Float32Array(48),im=new Float32Array(48);
      for(let k=1;k<48;k++)im[k]=2*Math.sin(Math.PI*k*duty)/(Math.PI*k);
      pw[duty]=ac.createPeriodicWave(re,im);
    }
    return pw[duty];
  }
  function route(node,pan){
    if(pan&&ac.createStereoPanner){const p=ac.createStereoPanner();p.pan.value=clamp(pan,-1,1);node.connect(p);p.connect(out);}
    else node.connect(out);
  }
  // o: to (end freq), type (square|p12|p25|triangle|sawtooth|sine), vol, gain, at (delay s), pan, vib [hz,depth Hz], sus (0..1 hold)
  function tone(f,dur,o){
    o=o||{};if(muted)return;const c=ctx();if(!c)return;
    const v=(o.vol==null?.2:o.vol)*(o.gain==null?1:o.gain);if(v<.002)return;
    const t=c.currentTime+(o.at||0),type=o.type||'square',os=c.createOscillator(),g=c.createGain();
    if(type==='p25')os.setPeriodicWave(wave(.25));else if(type==='p12')os.setPeriodicWave(wave(.125));else os.type=type;
    os.frequency.setValueAtTime(f,t);
    if(o.to)os.frequency.exponentialRampToValueAtTime(Math.max(20,o.to),t+dur);
    if(o.vib){const l=c.createOscillator(),lg=c.createGain();l.frequency.value=o.vib[0];lg.gain.value=o.vib[1];l.connect(lg);lg.connect(os.frequency);l.start(t);l.stop(t+dur+.02);}
    g.gain.setValueAtTime(.0001,t);g.gain.linearRampToValueAtTime(v,t+.005);
    if(o.sus)g.gain.setValueAtTime(v,t+dur*o.sus);
    g.gain.exponentialRampToValueAtTime(.0001,t+dur);
    os.connect(g);route(g,o.pan);os.start(t);os.stop(t+dur+.02);
  }
  // o: type (filter), fc, fc2 (sweep target), vol, gain, at, pan
  function noise(dur,o){
    o=o||{};if(muted)return;const c=ctx();if(!c)return;
    const v=(o.vol==null?.2:o.vol)*(o.gain==null?1:o.gain);if(v<.002)return;
    if(!noiseBuf){noiseBuf=c.createBuffer(1,c.sampleRate,c.sampleRate);const d=noiseBuf.getChannelData(0);for(let i=0;i<d.length;i++)d[i]=Math.random()*2-1;}
    const t=c.currentTime+(o.at||0),s=c.createBufferSource(),f=c.createBiquadFilter(),g=c.createGain();
    s.buffer=noiseBuf;s.loop=true;f.type=o.type||'bandpass';f.frequency.setValueAtTime(o.fc||1500,t);
    if(o.fc2)f.frequency.exponentialRampToValueAtTime(o.fc2,t+dur);
    g.gain.setValueAtTime(v,t);g.gain.exponentialRampToValueAtTime(.0001,t+dur);
    s.connect(f);f.connect(g);route(g,o.pan);s.start(t,Math.random()*.5);s.stop(t+dur+.02);
  }
  // notes: [[midi|null,beats],...]; o.step = seconds per beat
  function seq(notes,o){
    o=o||{};const st=o.step||.1;let at=o.at||0;
    notes.forEach(([m,b])=>{if(m!=null)tone(mtof(m),b*st*.94,Object.assign({},o,{at,sus:.6}));at+=b*st;});
    return at;
  }
  // volume + stereo position for a sound source `dx,dy` away from the listener
  function spat(dx,dy,range){return{gain:Math.pow(Math.max(0,1-Math.hypot(dx,dy)/range),1.4),pan:clamp(dx/300,-1,1)};}

  // ---- stock sounds used by several games ----
  const lib={
    click(){tone(1300,.04,{to:800,vol:.07});},
    tick(hi){tone(hi?1480:880,.05,{vol:.1});},
    coin(g,p){tone(mtof(83),.06,{type:'p25',vol:.12,gain:g,pan:p});tone(mtof(88),.3,{type:'p25',vol:.12,gain:g,pan:p,at:.06});},
    power(g,p){seq([[76,1],[83,1],[88,2]],{step:.05,type:'p12',vol:.11,gain:g,pan:p});},
    go(){tone(660,.1,{type:'p25',vol:.14});tone(660,.1,{type:'p25',vol:.14,at:.22});tone(1320,.35,{type:'p25',vol:.15,at:.44});},
    fanfare(){
      seq([[72,1],[76,1],[79,1],[84,2],[79,1],[84,5]],{step:.09,type:'p25',vol:.15});
      seq([[60,1],[64,1],[67,1],[72,2],[67,1],[72,5]],{step:.09,type:'triangle',vol:.22});
    },
    sad(){
      seq([[67,2],[66,2],[65,2],[60,7]],{step:.13,type:'p25',vol:.14});
      seq([[55,2],[54,2],[53,2],[48,7]],{step:.13,type:'triangle',vol:.2});
    },
    siren(g){for(let i=0;i<6;i++)tone(i%2?520:780,.16,{type:'sawtooth',vol:.1,gain:g,at:i*.17});},
    boom(g,p){noise(.8,{type:'lowpass',fc:1200,fc2:60,vol:.9,gain:g,pan:p});tone(120,.7,{to:28,type:'sine',vol:.7,gain:g,pan:p});},
    whoosh(g,p,len){noise(len||.35,{type:'bandpass',fc:500,fc2:3500,vol:.35,gain:g,pan:p});},
    fall(g,p){tone(900,.6,{to:90,type:'p25',vol:.12,gain:g,pan:p});}
  };

  const api={
    ctx,tone,noise,seq,spat,mtof,clamp,lib,
    get out(){return out;},
    get muted(){return muted;},
    onMute(f){cbs.push(f);},
    set(m){
      muted=!!m;
      try{localStorage.setItem('arcade-mute',muted?'1':'0');}catch(e){}
      if(out)out.gain.setTargetAtTime(muted?0:.9,ac.currentTime,.015);
      document.querySelectorAll('.mutebtn').forEach(b=>{b.textContent=muted?'🔇':'🔊';b.title=muted?'Ton an (M)':'Ton aus (M)';});
      cbs.forEach(f=>{try{f(muted);}catch(e){}});
      return muted;
    },
    toggle(){return api.set(!muted);}
  };
  document.querySelectorAll('.mutebtn').forEach(b=>b.addEventListener('click',e=>{e.stopPropagation();api.toggle();}));
  api.set(muted);
  addEventListener('keydown',e=>{
    if(e.repeat||e.ctrlKey||e.metaKey||e.altKey||(e.target&&e.target.tagName==='INPUT'))return;
    if(e.key==='m'||e.key==='M')api.toggle();
  });
  const unlock=()=>{ctx();};
  addEventListener('pointerdown',unlock,{passive:true});addEventListener('keydown',unlock);addEventListener('touchstart',unlock,{passive:true});
  return api;
})();
)rawliteral";
