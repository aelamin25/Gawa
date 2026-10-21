// live.h - one live voice call with OpenAI's Realtime API.
//
//   live_begin();                                   once, after audio_bsp_init() + audio_out_begin()
//   live_start(cfg);                                opens the call (returns at once, the work runs in a task)
//   live_view(&v);  if (v.version != shown) redraw   what the screen should show right now
//   live_stop();                                    hang up (the gear)
//
// While Pengu talks the mic is muted, so Pengu never hears (or interrupts) itself.
#pragma once
#include <stdint.h>

enum LiveState {
  LV_OFF = 0,      // no call
  LV_CONNECTING,   // Wi-Fi + secure link + session being set up ("喂？")
  LV_PENGU_TALKS,  // Pengu's voice is playing; the mic is muted
  LV_LISTENING,    // your turn: the mic streams to the cloud
  LV_ENDED,        // the call finished normally (you hung up, 20 s of quiet, or 15 minutes)
  LV_ERROR         // it could not connect or the link broke: see error
};

struct LiveView {
  LiveState state = LV_OFF;
  char hanzi[256] = "";      // what Pengu said last, in characters
  char pinyin[256] = "";     // filled in a moment later by the card request
  char english[128] = "";
  char heard[128] = "";      // what the cloud heard you say (for the serial log)
  char error[40] = "";       // short, for the screen: NO WIFI, BAD KEY, MODEL?, CLOUD? 503 ...
  uint32_t version = 0;      // goes up whenever anything above changes
};

struct LiveConfig {
  const char *api_key;
  const char *model;          // gpt-realtime-2.1-mini
  const char *voice;          // marin, coral, shimmer ...
  const char *instructions;   // who Pengu is and how it teaches
  const char *greeting;       // instructions for Pengu's first line
};

void live_begin();
bool live_start(const LiveConfig &cfg);   // false if a call is already running
void live_stop();
void live_view(LiveView *out);
bool live_running();
