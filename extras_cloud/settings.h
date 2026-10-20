// settings.h - Wi-Fi name/password, OpenAI key and model names, saved inside the chip.
// Also the little setup web page Gawa shows when you join its "Gawa-Setup" Wi-Fi.
#pragma once
#include <Arduino.h>

struct GawaSettings {
  String wifi_ssid;
  String wifi_pass;
  String api_key;
  String model_chat;   // the "brain"
  String model_stt;    // speech to text
  bool has_wifi() const { return wifi_ssid.length() > 0; }
  bool has_key()  const { return api_key.length() > 10; }
};

extern GawaSettings settings;

void settings_load();
void settings_save();

// Starts the "Gawa-Setup" hotspot + web page and blocks until the person saves (then restarts the board).
// on_status is called with a short message to put on the screen.
void setup_portal_run(void (*on_status)(const char *line1, const char *line2));
