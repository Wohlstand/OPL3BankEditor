/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2017 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ao_alsa.h"
#include <stdio.h>
#include <QtDebug>

#define PCM_DEVICE "default"

AudioOutALSA::AudioOutALSA(QObject *parent) :
    AudioOutBase(parent),
    m_thread(0)
{
    m_playing = false;
}

AudioOutALSA::~AudioOutALSA()
{
    stop();
    if(playback_handle)
        snd_pcm_close(playback_handle);
}

bool AudioOutALSA::init(int sampleRate, int channels)
{
    unsigned int sr = (unsigned int)sampleRate;
    frames = 512;
    periods = 2;

    qDebug() << "Initializing ALSA...";

    if((err = snd_pcm_open(&playback_handle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0)
    {
        fprintf(stderr, "cannot open audio device %s (%s)\n", PCM_DEVICE, snd_strerror(err));
        return false;
    }

    if((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
    {
        fprintf(stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
        return false;
    }

    if((err = snd_pcm_hw_params_any(playback_handle, hw_params)) < 0)
    {
        fprintf(stderr, "cannot initialize hardware parameter structure (%s)\n", snd_strerror(err));
        return false;
    }

    if((err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0)
    {
        fprintf(stderr, "cannot set access type (%s)\n", snd_strerror(err));
        return false;
    }

    if((err = snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0)
    {
        fprintf(stderr, "cannot set sample format (%s)\n",
                snd_strerror(err));
        return false;
    }

    if((err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &sr, 0)) < 0)
    {
        fprintf(stderr, "cannot set sample rate (%s)\n",
                snd_strerror(err));
        return false;
    }

    if((err = snd_pcm_hw_params_set_period_size_near(playback_handle, hw_params, &frames, 0)) < 0)
    {
        fprintf(stderr, "cannot set minimum period count (%s)\n", snd_strerror(err));
        return false;
    }

    if((err = snd_pcm_hw_params_set_periods_near(playback_handle, hw_params, &periods, 0)) < 0)
    {
        fprintf(stderr, "cannot set minimum periods count (%s)\n", snd_strerror(err));
        return false;
    }

    if((err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, (unsigned int)channels)) < 0)
    {
        fprintf(stderr, "cannot set channel count (%s)\n", snd_strerror(err));
        return false;
    }

    if((err = snd_pcm_hw_params(playback_handle, hw_params)) < 0)
    {
        fprintf(stderr, "cannot set parameters (%s)\n", snd_strerror(err));
        return false;
    }

    snd_pcm_hw_params_free(hw_params);

    /* tell ALSA to wake us up whenever 4096 or more frames
       of playback data can be delivered. Also, tell
       ALSA that we'll start the device ourselves.
    */

    if((err = snd_pcm_sw_params_malloc(&sw_params)) < 0)
    {
        fprintf(stderr, "cannot allocate software parameters structure (%s)\n", snd_strerror(err));
        return false;
    }
    if((err = snd_pcm_sw_params_current(playback_handle, sw_params)) < 0)
    {
        fprintf(stderr, "cannot initialize software parameters structure (%s)\n", snd_strerror(err));
        return false;
    }

    if((err = snd_pcm_sw_params_set_start_threshold(playback_handle, sw_params, 0U)) < 0)
    {
        fprintf(stderr, "cannot set start mode (%s)\n", snd_strerror(err));
        return false;
    }
    if((err = snd_pcm_sw_params(playback_handle, sw_params)) < 0)
    {
        fprintf(stderr, "cannot set software parameters (%s)\n", snd_strerror(err));
        return false;
    }

    /* the interface will interrupt the kernel every 4096 frames, and ALSA
       will wake up this program very soon after that.
    */

    if((err = snd_pcm_prepare(playback_handle)) < 0)
    {
        fprintf(stderr, "cannot prepare audio interface for use (%s)\n",
                snd_strerror(err));
        return false;
    }

    m_buffer.resize((int)frames * channels * 2 /* 2 -> sample size */);
    m_buffer.fill(0, m_buffer.size());

    qDebug() << "ALSA Audio output is ready!";

    return true;
}

void AudioOutALSA::start()
{
    if(!m_playing)
    {
        m_playing = true;
        pthread_create(&m_thread, NULL, playSound, (void *)this);
    }
}

void AudioOutALSA::stop()
{
    if(m_playing)
    {
        m_playing = false;
        pthread_join(m_thread, NULL);
    }
}

void *AudioOutALSA::playSound(void *self)
{
    AudioOutALSA *s = (AudioOutALSA *)self;
    snd_pcm_sframes_t frames_to_deliver = 0;

    while(s->m_playing)
    {
        /* wait till the interface is ready for data, or 1 second
                       has elapsed.
                    */
        if((s->err = snd_pcm_wait(s->playback_handle, 10)) < 0)
        {
            fprintf(stderr, "poll failed (%s)\n", strerror(errno));
            fflush(stderr);
            break;
        }

        /* find out how much space is available for playback data */
        frames_to_deliver = (snd_pcm_sframes_t)s->frames;

        /* deliver the data */
        frames_to_deliver = s->m_audioSource->read(s->m_buffer.data(), (qint64)frames_to_deliver * 4) / 4;

        if((s->err = (int)snd_pcm_writei(s->playback_handle, s->m_buffer.data(), (snd_pcm_uframes_t)frames_to_deliver)) < 0)
        {
            fprintf(stderr, "write failed (%s), recovering...\n", snd_strerror(s->err));
            fflush(stderr);
            s->err = snd_pcm_recover(s->playback_handle, s->err, (int)frames_to_deliver);
            if(s->err < 0)
            {
                fprintf(stderr, "write failed (%s)!\n", snd_strerror(s->err));
                fflush(stderr);
            }
        }
        if(s->err == 0)
        {
            /* No frames were written (no available space in pcm device).
               Allow other threads to catch up. */
            unsigned int delay = (((unsigned int)s->frames - (unsigned int)frames_to_deliver) / 2 * 1000) / 44100;
            usleep(delay * 1000);
        }
    }

    return NULL;
}
