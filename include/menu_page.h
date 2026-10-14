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
  .live{position:absolute;right:14px;top:14px;font-size:12px;background:#000a;border-radius:99px;padding:4px 10px;min-height:22px}
  .foot{margin-top:28px;color:#8a90b8;font-size:12px;text-align:center;line-height:1.7}
</style></head>
<body>
  <h1>ESP ARCADE</h1>
  <div class="sub">Zwei Spiele, ein kleiner Chip. Such dir eins aus.</div>
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
  </div>
  <div class="foot">Tipp: Zum Wechseln einfach hierher zurück (⬅ Spielauswahl).<br>Bots füllen leere Plätze auf.</div>
<script>
async function poll(){
  try{
    const h=await (await fetch('/health')).json();
    document.getElementById('lp').textContent=h.humans?h.humans+' spielen':'frei';
    const a=h.apes;
    document.getElementById('la').textContent=a.humans>=2?(a.queue?a.queue+' warten':'läuft – zuschauen'):(a.humans?'1 wartet auf dich':'frei');
  }catch(e){}
  setTimeout(poll,3000);
}
poll();
</script>
</body></html>
)rawliteral";
