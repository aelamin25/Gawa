// audio_bsp.c - microphone setup, adapted from Waveshare's 08_Audio_Test example.
#include <stdio.h>
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

  esp_codec_dev_set_out_vol(playback, 0.0);   // speaker stays silent
  esp_codec_dev_set_in_gain(record, 30.0);    // microphone gain (dB). Raise if it struggles to hear you.

  esp_codec_dev_sample_info_t fs = {
    .bits_per_sample = 16,
    .channel = 2,
    .channel_mask = 0,
    .sample_rate = 16000,   // ESP-SR needs 16 kHz, 16-bit
    .mclk_multiple = 0,
  };
  esp_codec_dev_open(playback, &fs);   // the codec wants both directions open
  esp_codec_dev_open(record, &fs);
}

int audio_mic_read(void *buf, size_t len)
{
  return esp_codec_dev_read(record, buf, (int)len) == ESP_CODEC_DEV_OK ? 0 : -1;
}
