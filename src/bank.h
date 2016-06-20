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

#include <QVector>

/* *********** FM Operator indexes *********** */
#define CARRIER1    0
#define MODULATOR1  1
#define CARRIER2    2
#define MODULATOR2  3
/* *********** FM Operator indexes *end******* */

/**
 * @brief OPL instrument bank container
 */
class FmBank
{
public:
    FmBank();
    FmBank(const FmBank &fb);
    FmBank &operator=(const FmBank &fb);
    bool operator==(const FmBank &fb);
    bool operator!=(const FmBank &fb);

    /**
     * @brief Set everything to zero
     */
    void reset();

    /**
     * @brief Operator specs
     */
    struct Operator
    {
        /*
         * --- About operator data bytes ---
         *
         * AM/VIB/EG/KSR/Multiple bits
         *      1-bit Tremolo
         *      1-bit Vibrato
         *      1-bit Systain sound
         *      1-bit Envelope scale (KSR)
         *      4-bit Frequency Multiplication
         *
         * KSL/attenuation settings
         *      2-bit KSL (Key Scale Rate)
         *      6-bit Level (0 max, 63 minimal)
         *
         * Attack/decay rates
         *      4-bit attak (0-zero, F-max)
         *      4-bit decay (0-zero, F-max)
         *
         * Sustain/release rates
         *      4-bit systain (0-max, F-zero)
         *      4-bit release(0-zero, F-max)
         *
         * Wave select settings
         *      4-bit unused
         *      4-bit wave-from-0-to-7
         *
         * Feedback/connection bits  &~0x30
         *      4-bits 0000 Unused zone
         *      3-bits 000-feedback
         *      1-bit 0-fm, 1-am
         */

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
        //! Custom instrument name
        char name[33];
        //! FM operators
        Operator OP[4];
        //! Feedback for a first modulator and carrier
        unsigned char feedback1;
        //! Feedback for a second modulator and carrier (used in 4-operators or double-voice mode)
        unsigned char feedback2;
        /**
         * @brief Types of synthesis
         */
        enum Connections
        {
            //! Additive
            AM = true,
            //! Frequency modulation
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
        //! Enable double-voice mode instead of real 4-operator mode
        bool en_pseudo4op;
        //! Fine tuning of second voice (pseudo-4-operators only, DMX-Specific)
        char fine_tune;
        //! Note offset (first operator pair)
        short note_offset1;
        //! Note offset (second operator pair, DMX-Specific)
        short note_offset2;
        //! Note velocity offset (TMB-Specific)
        char velocity_offset;
        //! AdLib specific parameter: Drum Number
        unsigned char adlib_drum_number;

        /* ******** OPL3 merged values ******** */

        /*!
         * \brief Merge into AM/VIB/EG/KSR/Multiple bits byte
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        unsigned char getAVEKM(int OpID);

        /*!
         * \brief Split AM/VIB/EG/KSR/Multiple bits byte into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setAVEKM(int OpID, unsigned char in);

        /*!
         * \brief Merge into KSL/attenuation settings byte
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        unsigned char getKSLL(int OpID);

        /*!
         * \brief Split KSL/attenuation settings byte into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setKSLL(int OpID, unsigned char in);

        /*!
         * \brief Merge into KSL/attenuation settings byte without attenuation
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        unsigned char getKSL(int OpID);
        /*!
         * \brief Store KSL value into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setKSL(int OpID, unsigned char in);

        /*!
         * \brief Merge into KSL/attenuation settings byte without KSL
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        unsigned char getLevel(int OpID);

        /*!
         * \brief Store attenuation value into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setLevel(int OpID, unsigned char in);

        /*!
         * \brief Merge into Attack/decay rates byte
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        unsigned char getAtDec(int OpID);
        /*!
         * \brief Split Attack/decay rates byte into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setAtDec(int OpID, unsigned char in);

        /*!
         * \brief Merge into Sustain/release rates byte
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        unsigned char getSusRel(int OpID);

        /*!
         * \brief Split Sustain/release rates byte into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setSusRel(int OpID, unsigned char in);

        /*!
         * \brief Merge into waveform byte
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        unsigned char getWaveForm(int OpID);

        /*!
         * \brief Store and clear-up waveform byte into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setWaveForm(int OpID, unsigned char in);

        /*!
         * \brief Merge into Feedback/connection byte
         * \return merged byte
         */
        unsigned char getFBConn1();

        /*!
         * \brief Split Feedback/connection byte for first carrier and modulator into internal values
         * \param in merged byte
         */
        void setFBConn1(unsigned char in);

        /*!
         * \brief Merge into Feedback/connection byte
         * \return merged byte
         */
        unsigned char getFBConn2();

        /*!
         * \brief Split Feedback/connection byte for second carrier and modulator into internal values
         * \param in merged byte
         */
        void setFBConn2(unsigned char in);

        /*!
         * \brief Merge operator data to send into 0xE862 register
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged 4-byte register data
         */
        unsigned int getDataE862(int OpID);

    };

    /**
     * @brief Get empty instrument entry
     * @return null-filled instrument entry
     */
    static Instrument emptyInst();

    inline int countMelodic()   { return Ins_Melodic_box.size(); }

    inline int countDrums()     { return Ins_Percussion_box.size(); }

    //! Pointer to array of melodic instruments
    Instrument* Ins_Melodic;
    //! Pointer to array of percussion instruments
    Instrument* Ins_Percussion;
    //! Array of melodic instruments
    QVector<Instrument> Ins_Melodic_box;
    //! Array of percussion instruments
    QVector<Instrument> Ins_Percussion_box;
};

#endif // BANK_H
