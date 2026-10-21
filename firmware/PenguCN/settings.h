// settings.h - Wi-Fi name/password, the OpenAI key, the model and the voice, saved inside the chip.
// Also the little setup web page Pengu shows when you join its "Pengu-Setup" Wi-Fi.
#pragma once
#include <Arduino.h>

struct PenguSettings {
  String wifi_ssid;
  String wifi_pass;
  String api_key;
  String model;        // the live voice model
  String voice;        // which voice Pengu speaks with
  bool has_wifi() const { return wifi_ssid.length() > 0; }
  bool has_key()  const { return api_key.length() > 10; }
};

extern PenguSettings settings;

void settings_load();
void settings_save();

// Starts the "Pengu-Setup" hotspot + web page and blocks until the person saves (then restarts the board).
// on_status is called with a short message to put on the screen.
void setup_portal_run(void (*on_status)(const char *line));
