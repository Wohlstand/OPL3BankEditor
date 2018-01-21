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

#ifndef AO_BASE_H
#define AO_BASE_H

#include <QIODevice>

class AudioOutBase :
        public QObject
{
    Q_OBJECT

protected:
    QIODevice *m_audioSource = nullptr;
public:
    AudioOutBase(QObject *parent);
    virtual ~AudioOutBase();;
    virtual bool init(int sampleRate, int channels) = 0;
    virtual void setAudioSource(QIODevice *audioSource);
    virtual void start() = 0;
    virtual void stop() = 0;
};

#endif // AO_BASE_H
