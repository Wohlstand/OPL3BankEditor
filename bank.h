/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef BANK_H
#define BANK_H

#define CARRIER1    1
#define MODULATOR1  0
#define CARRIER2    3
#define MODULATOR2  2

/**
 * @brief OPL instrument bank container
 */
class FmBank
{
public:
    FmBank();
    FmBank(const FmBank &fb);

    /**
     * @brief Set everything to zero
     */
    void reset();

    /**
     * @brief Operator specs
     */
    struct Operator
    {
        //! Attacking level (from 0 to 15)
        unsigned char attack;
        //! Decaying level (from 0 to 15)
        unsigned char decay;
        //! Sustain level (from 0 to 15)
        unsigned char sustain;
        //! Release level (from 0 to 15)
        unsigned char release;
        //! Wave shape (from 0 to 7)
        unsigned char waveform;
        //! Frequency multiplication (from 0 to 15)
        unsigned char fmult;
        //! Volume level (from 0 to 63)
        unsigned char level;
        //! Key Scale level (from 0 to 3)
        unsigned char ksl;
        //! Vibrato (0 or 1)
        bool vib;
        //! Tremolo (0 or 1)
        bool am;
        //! Enable sustaining (0 or 1)
        bool eg;
        //! Key scale Rate (Envelope scale) (0 or 1)
        bool ksr;
    };

    /**
     * @brief Instrument specs
     */
    struct Instrument
    {
        //! FM operators
        Operator OP[4];
        //! Feedback for a first modulator and carrier
        unsigned char feedback1;
        //! Feedback for a second modulator and carrier (used in 4-operators or double-voice mode)
        unsigned char feedback2;
        /**
         * @brief Types of operators (modulator and carrier) connections
         */
        enum Connections
        {
            AM = true,
            FM = false
        };
        //! Connections for a first operators pair
        bool connection1;
        //! Connections for a second operators pair
        bool connection2;
        //! Play only this note number independent from a requested key
        unsigned char percNoteNum;
        //! Is enabled four operators mode
        bool en_4op;
    };
    //! Array of melodic instruments
    Instrument Ins_Melodic[128];
    //! Array of percussion instruments
    Instrument Ins_Percussion[128];
};

#endif // BANK_H
