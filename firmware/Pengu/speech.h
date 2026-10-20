// speech.h - Pengu says exactly what is on the screen.
//
// The spoken lines live in the "speech" flash partition (releases/speech.bin, made by tools/make_speech.py) as 8 kHz
// IMA ADPCM. speech_index.h says where each line starts. speech_say(text) finds the clip for that exact text and plays it
// on a small background task, so the face can animate while Pengu talks. If a line has no clip, nothing is said.
#pragma once
#include <Arduino.h>
#include "esp_partition.h"
#include "speech_index.h"
#include "audio_bsp.h"

static const uint8_t *g_speech_blob = nullptr;   // the whole partition, memory mapped (no RAM used)
static volatile bool  g_speech_busy = false;
static volatile uint32_t g_speech_end = 0;      // when the last spoken line finished (so Pengu does not answer itself)
static int            g_speech_volume = 85;
static int            g_speech_last = 0;        // result of the last play, 0 = fine (for the on-screen debug)

static bool speech_init(int volume)
{
  g_speech_volume = volume;
  const esp_partition_t *p = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "speech");
  if (!p) { Serial.println("speech: no 'speech' partition"); return false; }
  const void *map = nullptr; esp_partition_mmap_handle_t h;
  if (esp_partition_mmap(p, 0, p->size, ESP_PARTITION_MMAP_DATA, &map, &h) != ESP_OK) { Serial.println("speech: mmap failed"); return false; }
  g_speech_blob = (const uint8_t *)map;
  Serial.printf("speech: %u lines, partition %u KB\n", (unsigned)SPEECH_COUNT, (unsigned)(p->size / 1024));
  return true;
}

static const int16_t ADPCM_STEP[89] = {7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
static const int8_t  ADPCM_INDEX[8] = {-1,-1,-1,-1,2,4,6,8};

struct SpeechJob { const uint8_t *data; uint32_t len; };

static void speech_task(void *arg)
{
  SpeechJob job = *(SpeechJob *)arg; free(arg);
  static int16_t out[2048];                 // 512 ADPCM samples -> 1024 at 16 kHz -> stereo
  int pred = 0, idx = 0, prev = 0, bad = 0;
  uint32_t pos = 0;
  while (pos < job.len) {
    uint32_t chunk = (job.len - pos < 256) ? job.len - pos : 256;   // 256 bytes = 512 samples = 32 ms
    int n = 0;
    for (uint32_t i = 0; i < chunk; i++) {
      uint8_t byte = job.data[pos + i];
      for (int half = 0; half < 2; half++) {
        int code = half ? (byte >> 4) : (byte & 15);
        int step = ADPCM_STEP[idx], d = step >> 3;
        if (code & 4) d += step;
        if (code & 2) d += step >> 1;
        if (code & 1) d += step >> 2;
        pred += (code & 8) ? -d : d;
        if (pred > 32767) pred = 32767; else if (pred < -32768) pred = -32768;
        idx += ADPCM_INDEX[code & 7]; if (idx < 0) idx = 0; else if (idx > 88) idx = 88;
        int mid = (prev + pred) / 2;        // 8 kHz -> 16 kHz: one in-between sample
        out[n++] = mid;  out[n++] = mid;    // stereo
        out[n++] = pred; out[n++] = pred;
        prev = pred;
      }
    }
    if (audio_write(out, n) != 0) bad++;
    pos += chunk;
  }
  memset(out, 0, sizeof(out));
  audio_write(out, 1024);                   // a little silence so the tail leaves the speaker
  g_speech_last = bad;
  g_speech_end = millis();
  g_speech_busy = false;
  vTaskDelete(NULL);
}

static void speech_wait()
{
  uint32_t t0 = millis();
  while (g_speech_busy && millis() - t0 < 4000) delay(10);
}

// say this exact line (as shown on screen). Returns true if a clip exists.
static bool speech_say(const char *text)
{
  if (!g_speech_blob || !text || !*text || g_speech_volume <= 0) return false;
  for (size_t i = 0; i < SPEECH_COUNT; i++) {
    if (strcmp(SPEECH[i].text, text) != 0) continue;
    speech_wait();
    if (g_speech_busy) { Serial.println("speech: still talking, skipped a line"); return false; }
    SpeechJob *job = (SpeechJob *)malloc(sizeof(SpeechJob));
    if (!job) return false;
    job->data = g_speech_blob + SPEECH[i].offset; job->len = SPEECH[i].length;
    audio_volume(g_speech_volume);
    g_speech_busy = true;
    if (xTaskCreatePinnedToCore(speech_task, "speech", 4096, job, 3, NULL, 1) != pdPASS) { g_speech_busy = false; free(job); return false; }
    return true;
  }
  Serial.printf("speech: no clip for \"%s\"\n", text);
  return false;
}
