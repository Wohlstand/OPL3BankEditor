#pragma once
#ifndef NOPL3_H
#define NOPL3_H

#ifdef __cplusplus
extern "C" {
#endif

void *nopl3_init(int clock, int samplerate);
void nopl3_set_rate(void *chip, int clock, int samplerate);
void nopl3_shutdown(void *chip);
void nopl3_reset(void *chip);

void nopl3_getsample(void *chip, short *sndptr, int numsamples);

void nopl3_write(void *chip, int port, int val);

#ifdef __cplusplus
}
#endif

#endif // NOPL3_H
