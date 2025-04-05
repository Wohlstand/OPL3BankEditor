/*
 * Copyright (C) 2023 nukeykt
 *
 * This file is part of YMF262-LLE.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  YMF262 emulator
 *  Thanks:
 *      John McMaster (siliconpr0n.org):
 *          YMF262 decap and die shot
 *
 */

#include "nuked_fmopl3.h"
#include <stdint.h>
#include <stdlib.h>
#include "nopl3.h"

#define OPL_WRITEBUF_SIZE   2048
#define OPL_WRITEBUF_DELAY  1
#define RSM_FRAC 10

typedef struct _opl3_writebuf {
    uint64_t time;
    uint8_t reg;
    uint8_t data;
} opl3_writebuf;

typedef struct {
    fmopl3_t chip;

    int sample_a;
    int sample_b;
    int o_sy;
    int o_smpbd;
    int o_smpac;
    int shifter_ab;

    int32_t rateratio;
    int32_t samplecnt;
    int oldsample[2];

    uint64_t writebuf_samplecnt;
    uint32_t writebuf_cur;
    uint32_t writebuf_last;
    uint64_t writebuf_lasttime;
    opl3_writebuf writebuf[OPL_WRITEBUF_SIZE];
} nopl3_t;

void nopl3_cycle(nopl3_t *chip)
{
    int i;
    for (i = 0; i < 576/4; i++)
    {
        chip->chip.input.mclk = i & 1;
        FMOPL3_Clock(&chip->chip);

        if (chip->o_sy && !chip->chip.o_sy)
        {

            if (chip->o_smpac && !chip->chip.o_smpac)
            {
                chip->sample_a = chip->shifter_ab & 0xffff;
                if (chip->sample_a & 0x8000)
                {
                    chip->sample_a &= 0x7fff;
                }
                else
                {
                    chip->sample_a |= ~0x7fff;
                }
            }
            if (chip->o_smpbd && !chip->chip.o_smpbd)
            {
                chip->sample_b = chip->shifter_ab & 0xffff;
                if (chip->sample_b & 0x8000)
                {
                    chip->sample_b &= 0x7fff;
                }
                else
                {
                    chip->sample_b |= ~0x7fff;
                }
            }
            chip->shifter_ab = (chip->shifter_ab >> 1) | (chip->chip.o_doab << 15);

            chip->o_smpac = chip->chip.o_smpac;
            chip->o_smpbd = chip->chip.o_smpbd;
        }

        chip->o_sy = chip->chip.o_sy;
    }
}

void *nopl3_init(int clock, int samplerate)
{
    nopl3_t *chip = calloc(1, sizeof(nopl3_t));
    nopl3_set_rate(chip, clock, samplerate);
    return chip;
}

void nopl3_set_rate(void* chip_p, int clock, int samplerate)
{
    nopl3_t *chip = (nopl3_t*)chip_p;

    chip->chip.input.cs = 0;
    chip->chip.input.ic = 1;
    chip->chip.input.rd = 1;
    chip->chip.input.wr = 1;

    samplerate *= 1;

    chip->rateratio = ((samplerate << RSM_FRAC) * (int64_t)288) / clock;

    /* printf("%i %i\n", clock, samplerate); */

    nopl3_reset(chip);
}

void nopl3_shutdown(void *chip)
{
    free(chip);
}

void nopl3_reset(void *chip)
{
    nopl3_t* chip2 = chip;
    int i = 0;

    chip2->chip.input.ic = 0;
    for (i = 0; i < 100; i++)
    {
        nopl3_cycle(chip2);
    }
    chip2->chip.input.ic = 1;
    for (i = 0; i < 100; i++)
    {
        nopl3_cycle(chip2);
    }

}

void nopl3_write2(nopl3_t *chip, int port, int val)
{
    chip->chip.input.address = port;
    chip->chip.input.data_i = val;
    chip->chip.input.wr = 0;
    FMOPL3_Clock(&chip->chip); /* propagate */
    chip->chip.input.wr = 1;
    FMOPL3_Clock(&chip->chip); /* propagate */
}

void nopl3_getsample(void *chip, short *sndptr, int numsamples)
{
    nopl3_t* chip2 = chip;
    int i, buf0, buf1;
    short *p = sndptr;
    opl3_writebuf* writebuf;

    for (i = 0; i < numsamples; i++)
    {
        while (chip2->samplecnt >= chip2->rateratio)
        {
            chip2->oldsample[0] = chip2->sample_a;
            chip2->oldsample[1] = chip2->sample_b;
            nopl3_cycle(chip2);

            while ((writebuf = &chip2->writebuf[chip2->writebuf_cur]), writebuf->time <= chip2->writebuf_samplecnt)
            {
                if (!(writebuf->reg & 4))
                {
                    break;
                }
                writebuf->reg &= 3;
                nopl3_write2(chip2, writebuf->reg, writebuf->data);
                chip2->writebuf_cur = (chip2->writebuf_cur + 1) % OPL_WRITEBUF_SIZE;
            }
            chip2->writebuf_samplecnt++;
            chip2->samplecnt -= chip2->rateratio;
        }

        buf0 = (chip2->oldsample[0] * (chip2->rateratio - chip2->samplecnt)
            + chip2->sample_a * chip2->samplecnt) / chip2->rateratio;
        buf1 = (chip2->oldsample[1] * (chip2->rateratio - chip2->samplecnt)
            + chip2->sample_b * chip2->samplecnt) / chip2->rateratio;
        chip2->samplecnt += 1 << RSM_FRAC;
        *p++ = buf1;
        *p++ = buf0;
    }
}

void nopl3_getsample_one_native(void *chip, short *sndptr)
{
    nopl3_t* chip2 = chip;
    short *p = sndptr;
    opl3_writebuf* writebuf;

    chip2->oldsample[0] = chip2->sample_a;
    chip2->oldsample[1] = chip2->sample_b;
    nopl3_cycle(chip2);

    while ((writebuf = &chip2->writebuf[chip2->writebuf_cur]), writebuf->time <= chip2->writebuf_samplecnt)
    {
        if (!(writebuf->reg & 4))
            break;

        writebuf->reg &= 3;
        nopl3_write2(chip2, writebuf->reg, writebuf->data);
        chip2->writebuf_cur = (chip2->writebuf_cur + 1) % OPL_WRITEBUF_SIZE;
    }

    chip2->writebuf_samplecnt++;
    chip2->samplecnt -= chip2->rateratio;
    chip2->samplecnt += 1 << RSM_FRAC;

    *p++ = chip2->sample_b;
    *p++ = chip2->sample_a;
}

void nopl3_write(void *chip, int port, int val)
{
    nopl3_t* chip2 = chip;
    uint64_t time1, time2;
    opl3_writebuf *writebuf;
    uint32_t writebuf_last;

    writebuf_last = chip2->writebuf_last;
    writebuf = &chip2->writebuf[writebuf_last];

    if (writebuf->reg & 4)
    {
        nopl3_write2(chip2, writebuf->reg & 1, writebuf->data);
        nopl3_cycle(chip2);

        chip2->writebuf_cur = (writebuf_last + 1) % OPL_WRITEBUF_SIZE;
        chip2->writebuf_samplecnt = writebuf->time;
    }

    writebuf->reg = port | 4;
    writebuf->data = val;
    time1 = chip2->writebuf_lasttime + OPL_WRITEBUF_DELAY;
    time2 = chip2->writebuf_samplecnt;

    if (time1 < time2)
    {
        time1 = time2;
    }

    writebuf->time = time1;
    chip2->writebuf_lasttime = time1;
    chip2->writebuf_last = (writebuf_last + 1) % OPL_WRITEBUF_SIZE;
}
