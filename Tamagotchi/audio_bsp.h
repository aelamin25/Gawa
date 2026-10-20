#ifndef AUDIO_BSP_H
#define AUDIO_BSP_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Brings up the ES8311 codec + I2S and opens the microphone at 16 kHz / 16-bit / stereo.
void audio_bsp_init(void);

// Reads raw stereo 16-bit frames from the microphone. Returns 0 on success.
int audio_mic_read(void *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
