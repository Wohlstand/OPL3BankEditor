#pragma once
#ifndef NOPL2_H
#define NOPL2_H

#ifdef __cplusplus
extern "C" {
#endif

void *nopl2_init(int clock, int samplerate);
void nopl2_set_rate(void *chip, int clock, int samplerate);
void nopl2_shutdown(void *chip);
void nopl2_reset(void *chip);

void nopl2_getsample(void *chip, short *sndptr, int numsamples);
void nopl2_getsample_one_native(void *chip, short *sndptr);

void nopl2_write(void *chip, int port, int val);

#ifdef __cplusplus
}
#endif

#endif // NOPL2_H
