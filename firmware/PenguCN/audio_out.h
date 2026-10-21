// audio_out.h - Pengu's voice: a 30 s ring buffer in PSRAM and a player task that feeds the speaker.
// The cloud sends speech faster than real time, in bursts; this smooths it into one steady stream.
#pragma once
#include <stdint.h>
#include <stddef.h>

void   audio_out_begin(int volume);                 // after audio_bsp_init(); starts the player task
void   audio_out_push(const int16_t *mono, size_t n); // 24 kHz mono samples (dropped if the buffer is full)
void   audio_out_clear();                            // stop talking now
bool   audio_out_busy();                             // playing, or played something in the last 400 ms (echo guard)
size_t audio_out_queued();                           // samples waiting
int    audio_out_errors();                           // failed speaker writes since boot (for the debug corner)
