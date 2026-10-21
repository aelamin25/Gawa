#ifndef AUDIO_BSP_H
#define AUDIO_BSP_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Brings up the ES8311 codec + I2S, speaker and microphone at sample_rate (24000 for PenguCN), 16-bit, stereo.
void audio_bsp_init(int sample_rate);

// Reads raw stereo 16-bit frames from the microphone. Returns 0 on success.
int audio_mic_read(void *buf, size_t len);

// Plays a short 16-bit mono clip (at the rate the codec was opened with) on the speaker (blocks until done), at 0..100 volume.
// Returns 0 when every write went through, otherwise the first error code from the codec (negative) or the number of failed writes.
int audio_play(const int16_t *mono, size_t n, int volume);

// Lower level, used by audio_out.cpp: set the speaker volume (0..100, only re-sent when it changes) and write
// 16-bit STEREO interleaved samples (n = number of int16 values). Returns 0 on success.
void audio_volume(int volume);
int  audio_write(const int16_t *stereo, size_t n);

#ifdef __cplusplus
}
#endif

#endif
