#pragma once
// AFFENSCHLACHT – second game on the same ESP32: two apes, one wall, lots of poop.
void apesBegin();               // start the WebSocket server (port 82)
void apesLoop();                // call every loop()
const char *apesPage();         // the HTML client (served at /apes)
