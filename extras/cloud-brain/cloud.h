// cloud.h - sends what you said to OpenAI and gets back Gawa's mood and reply.
#pragma once
#include <Arduino.h>
#include "faces.h"

struct CloudReply {
  bool   ok;
  mood_t mood;
  String reply;     // what Gawa says (short)
  String heard;     // what the cloud thinks you said
  String error;     // short error word for the screen when ok == false
};

// Connects to the saved Wi-Fi. Returns true when online. Non-blocking after the first call.
bool cloud_wifi_connect(uint32_t timeout_ms);
bool cloud_online();

// pcm: 16 kHz, mono, 16-bit samples. Blocks for a few seconds.
CloudReply cloud_talk(const int16_t *pcm, size_t samples);

// Forget the conversation so far.
void cloud_forget();
