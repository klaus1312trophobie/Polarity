# POLARITY

A magnetic multiplayer arena game that runs entirely on an ESP32 and is played in the browser, on phones or PCs.

Every player is a magnet. Equal polarities repel, opposite ones attract, and that applies to the energy orbs *and* to the other players. Flip your polarity to pull orbs in and to shove rivals off a shrinking arena.

## Rules

- Collect an orb: **+1**. Golden orbs: **+5** (they pull toward everyone)
- Push someone off the edge: **+6**. Falling off yourself: **−3**
- **Surge** makes your magnet three times as strong for a moment
- 2-minute rounds. The arena shrinks during the last 30 seconds
- Power-ups (touch the hexagon): **Turbo**, **Anchor** (immune to magnets and heavy), **Magnetar** (all orbs come to you), **Shockwave**
- Bots fill the field up to 4 players. Up to 8 humans can play

Controls: WASD / arrow keys to move, Space to flip, Shift to surge. On phones, steer with your left thumb and use the buttons on the right.

## Playing

- **At home:** open `http://polarity.local` on any device in the same WiFi.
- **On the go:** if the home WiFi isn't in range (or BOOT is held at power-up), the ESP32 opens its own open hotspot `Polarity-Game`. Join it and the game opens through the captive portal (or go to `http://192.168.4.1`).

## Building

PlatformIO, Arduino framework, any ESP32 dev board.

```sh
cp include/config.example.h include/config.h   # enter your WiFi
pio run -e esp32dev -t upload                  # USB
pio run -e ota -t upload                       # over WiFi afterwards
```

## How it works

The ESP32 is authoritative: it runs the physics at 30 Hz and sends JSON snapshots over a WebSocket (port 81) at 15 Hz. The browser client (`include/webpage.h`) interpolates between snapshots and draws everything on a canvas. The all-time best score is stored in flash. `/health` returns player count and free heap.
