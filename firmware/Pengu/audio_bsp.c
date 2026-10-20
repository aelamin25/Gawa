// audio_bsp.c - microphone setup, adapted from Waveshare's 08_Audio_Test example.
#include <stdio.h>
#include <string.h>
#include "audio_bsp.h"
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "src/codec_board/codec_board.h"
#include "src/codec_board/codec_init.h"
#include "src/esp_codec_dev/include/esp_codec_dev.h"

static esp_codec_dev_handle_t playback = NULL;
static esp_codec_dev_handle_t record = NULL;

void audio_bsp_init(void)
{
  set_codec_board_type("S3_ePaper_1_54");   // pin map lives in src/codec_board/board_cfg.h
  codec_init_cfg_t codec_cfg = {
    .in_mode = CODEC_I2S_MODE_STD,
    .out_mode = CODEC_I2S_MODE_STD,
    .in_use_tdm = false,
    .reuse_dev = false,
  };
  ESP_ERROR_CHECK(init_codec(&codec_cfg));
  playback = get_playback_handle();
  record = get_record_handle();

  esp_codec_dev_set_in_gain(record, 30.0);    // microphone gain (dB). Raise if it struggles to hear you.

  esp_codec_dev_sample_info_t fs = {
    .bits_per_sample = 16,
    .channel = 2,
    .channel_mask = 0,
    .sample_rate = 16000,   // ESP-SR needs 16 kHz, 16-bit
    .mclk_multiple = 0,
  };
  int r1 = esp_codec_dev_open(playback, &fs);   // the codec wants both directions open
  int r2 = esp_codec_dev_open(record, &fs);
  int r3 = esp_codec_dev_set_out_vol(playback, 0);   // speaker quiet until a word is played
  printf("audio: open speaker %d, open mic %d, mute %d (0 = ok)\n", r1, r2, r3);
}

int audio_mic_read(void *buf, size_t len)
{
  return esp_codec_dev_read(record, buf, (int)len) == ESP_CODEC_DEV_OK ? 0 : -1;
}

static int current_volume = -1;
void audio_volume(int volume)
{
  if (current_volume == volume) return;
  int rv = esp_codec_dev_set_out_vol(playback, volume);   // 0..100; the codec maps 100 to 0 dB and 50 to -25 dB, so keep it high
  current_volume = volume;
  if (rv != 0) printf("audio: set volume %d failed (%d)\n", volume, rv);
}

int audio_write(const int16_t *stereo, size_t n)
{
  int w = esp_codec_dev_write(playback, (void *)stereo, (int)(n * sizeof(int16_t)));
  return w == ESP_CODEC_DEV_OK ? 0 : w;
}

int audio_play(const int16_t *mono, size_t n, int volume)
{
  static int16_t stereo[512];
  audio_volume(volume);
  int bad = 0, first_err = 0;
  for (size_t i = 0; i < n; i += 256) {
    size_t k = (n - i < 256) ? n - i : 256;
    for (size_t j = 0; j < k; j++) { stereo[2 * j] = mono[i + j]; stereo[2 * j + 1] = mono[i + j]; }
    int w = audio_write(stereo, k * 2);
    if (w != 0) { bad++; if (!first_err) first_err = w; }
  }
  memset(stereo, 0, sizeof(stereo));                       // a little silence so the last samples leave the speaker
  for (int i = 0; i < 2; i++) audio_write(stereo, 512);
  printf("audio: played %u samples at volume %d (failed writes %d, first error %d)\n", (unsigned)n, volume, bad, first_err);
  if (first_err != 0) return first_err;
  return bad;
}
