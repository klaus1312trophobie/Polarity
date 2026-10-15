#pragma once
// KLAUSTROPHOBIE – third game on the same ESP32: an overcrowded elevator, everybody stinks.
void klausBegin();               // start the WebSocket server (port 83)
void klausLoop();                // call every loop()
const char *klausPage();         // the HTML client (served at /klaus)
int klausHumans();               // seated human players (0..6)
int klausQueue();                // visitors waiting for a seat
