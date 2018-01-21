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

#ifndef AO_ALSA_H
#define AO_ALSA_H

#include "ao_base.h"

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <atomic>

class AudioOutALSA : public AudioOutBase
{
    pthread_t m_thread;
    std::atomic_bool m_playing;

    snd_pcm_t *playback_handle;

    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_sframes_t frames_to_deliver;
    snd_pcm_uframes_t frames;
    unsigned int periods;
    int nfds;
    int err;
    struct pollfd *pfds;
    QByteArray m_buffer;

    static void *playSound(void *self);
public:
    explicit AudioOutALSA(QObject *parent);
    ~AudioOutALSA();

    bool init(int sampleRate, int channels);
    void start();
    void stop();
};

#endif // AO_ALSA_H
