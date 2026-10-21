// audio_out.cpp - see audio_out.h
#include <Arduino.h>
#include "audio_out.h"
#include "audio_bsp.h"

#define RING_SAMPLES (24000 * 30)     // 30 s of 24 kHz mono = 1.4 MB of PSRAM
#define BLOCK        480              // 20 ms per speaker write

static int16_t *ring = nullptr;
static volatile size_t head = 0, tail = 0;     // head: next write (cloud side), tail: next read (speaker side)
static portMUX_TYPE ring_mux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t last_play_ms = 0;
static volatile bool playing = false;
static volatile int write_errors = 0;

static size_t queued_unlocked() { return (head + RING_SAMPLES - tail) % RING_SAMPLES; }

size_t audio_out_queued()
{
  portENTER_CRITICAL(&ring_mux); size_t q = queued_unlocked(); portEXIT_CRITICAL(&ring_mux);
  return q;
}

// one writer (the live task): copy outside the lock, then publish the new head under it
void audio_out_push(const int16_t *mono, size_t n)
{
  if (!ring) return;
  portENTER_CRITICAL(&ring_mux);
  size_t space = RING_SAMPLES - 1 - queued_unlocked(), h = head;
  portEXIT_CRITICAL(&ring_mux);
  if (n > space) n = space;
  size_t first = RING_SAMPLES - h < n ? RING_SAMPLES - h : n;       // two copies when it wraps around
  memcpy(ring + h, mono, first * sizeof(int16_t));
  memcpy(ring, mono + first, (n - first) * sizeof(int16_t));
  portENTER_CRITICAL(&ring_mux);
  if (head == h) head = (h + n) % RING_SAMPLES;                     // (a clear in between moved tail, not head)
  portEXIT_CRITICAL(&ring_mux);
}

void audio_out_clear()
{
  portENTER_CRITICAL(&ring_mux); tail = head; portEXIT_CRITICAL(&ring_mux);
}

bool audio_out_busy()
{
  return playing || audio_out_queued() > 0 || (millis() - last_play_ms < 400);
}

int audio_out_errors() { return write_errors; }

static void player_task(void *)
{
  static int16_t mono[BLOCK], stereo[BLOCK * 2];
  for (;;) {
    size_t n = 0;
    portENTER_CRITICAL(&ring_mux);
    size_t q = queued_unlocked();
    n = q < BLOCK ? q : BLOCK;
    for (size_t i = 0; i < n; i++) { mono[i] = ring[tail]; tail = (tail + 1) % RING_SAMPLES; }
    portEXIT_CRITICAL(&ring_mux);
    if (n == 0) {
      if (playing) {                                   // the end of a sentence: a little silence pushes the tail out
        memset(stereo, 0, sizeof(stereo));
        for (int i = 0; i < 3; i++) audio_write(stereo, BLOCK * 2);
        playing = false;
        last_play_ms = millis();
      }
      vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }
    playing = true;
    for (size_t i = 0; i < n; i++) { stereo[2 * i] = mono[i]; stereo[2 * i + 1] = mono[i]; }
    for (size_t i = n; i < BLOCK; i++) { stereo[2 * i] = 0; stereo[2 * i + 1] = 0; }
    if (audio_write(stereo, BLOCK * 2) != 0) write_errors++;   // blocks until the speaker takes it: this paces playback
    last_play_ms = millis();
  }
}

void audio_out_begin(int volume)
{
  if (ring) return;
  ring = (int16_t *)heap_caps_malloc(RING_SAMPLES * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!ring) { Serial.println("audio_out: no PSRAM for the voice buffer"); return; }
  audio_volume(volume);
  xTaskCreatePinnedToCore(player_task, "player", 4096, nullptr, 6, nullptr, 1);
}
