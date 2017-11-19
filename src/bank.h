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
     * @brief Set everything to zero (one bank per melodic and percussion)
     */
    void reset();

    /**
     * @brief Set everything to zero and set count of banks
     * @param melodic_banks Count of melodic banks
     * @param percussion_banks Count of percussion banks
     */
    void reset(uint16_t melodic_banks, uint16_t percussion_banks);

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
        uint8_t attack;
        //! Decaying level (from 0 to 15)
        uint8_t decay;
        //! Sustain level (from 0 to 15)
        uint8_t sustain;
        //! Release level (from 0 to 15)
        uint8_t release;
        //! Wave shape (from 0 to 7)
        uint8_t waveform;
        //! Frequency multiplication (from 0 to 15)
        uint8_t fmult;
        //! Volume level (from 0 to 63)
        uint8_t level;
        //! Key Scale level (from 0 to 3)
        uint8_t ksl;
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
        uint8_t feedback1;
        //! Feedback for a second modulator and carrier (used in 4-operators or double-voice mode)
        uint8_t feedback2;
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
        uint8_t percNoteNum;
        //! Is enabled four operators mode
        bool en_4op;
        //! Enable double-voice mode instead of real 4-operator mode
        bool en_pseudo4op;
        //! Fine tuning of second voice (pseudo-4-operators only, DMX-Specific)
        int8_t fine_tune;
        //! Note offset (first operator pair)
        int16_t note_offset1;
        //! Note offset (second operator pair, DMX-Specific)
        int16_t note_offset2;
        //! Note velocity offset (TMB-Specific)
        int8_t velocity_offset;
        //! AdLib specific parameter: Drum Number
        uint8_t adlib_drum_number;
        //! Number of milliseconds of produced sound while sustaining
        uint16_t ms_sound_kon;
        //! Number of milliseconds of produced sound while release
        uint16_t ms_sound_koff;

        /* ******** OPL3 merged values ******** */

        /*!
         * \brief Merge into AM/VIB/EG/KSR/Multiple bits byte
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        uint8_t getAVEKM(int OpID) const;

        /*!
         * \brief Split AM/VIB/EG/KSR/Multiple bits byte into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setAVEKM(int OpID, uint8_t in);

        /*!
         * \brief Merge into KSL/attenuation settings byte
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        uint8_t getKSLL(int OpID) const;

        /*!
         * \brief Split KSL/attenuation settings byte into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setKSLL(int OpID, uint8_t in);

        /*!
         * \brief Merge into KSL/attenuation settings byte without attenuation
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        uint8_t getKSL(int OpID) const;
        /*!
         * \brief Store KSL value into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setKSL(int OpID, uint8_t in);

        /*!
         * \brief Merge into KSL/attenuation settings byte without KSL
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        uint8_t getLevel(int OpID) const;

        /*!
         * \brief Store attenuation value into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setLevel(int OpID, uint8_t in);

        /*!
         * \brief Merge into Attack/decay rates byte
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        uint8_t getAtDec(int OpID) const;
        /*!
         * \brief Split Attack/decay rates byte into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setAtDec(int OpID, uint8_t in);

        /*!
         * \brief Merge into Sustain/release rates byte
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        uint8_t getSusRel(int OpID) const;

        /*!
         * \brief Split Sustain/release rates byte into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setSusRel(int OpID, uint8_t in);

        /*!
         * \brief Merge into waveform byte
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged byte
         */
        uint8_t getWaveForm(int OpID) const;

        /*!
         * \brief Store and clear-up waveform byte into internal values
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \param in merged byte
         */
        void setWaveForm(int OpID, uint8_t in);

        /*!
         * \brief Merge into Feedback/connection byte
         * \return merged byte
         */
        uint8_t getFBConn1() const;

        /*!
         * \brief Split Feedback/connection byte for first carrier and modulator into internal values
         * \param in merged byte
         */
        void setFBConn1(uint8_t in);

        /*!
         * \brief Merge into Feedback/connection byte
         * \return merged byte
         */
        uint8_t getFBConn2() const;

        /*!
         * \brief Split Feedback/connection byte for second carrier and modulator into internal values
         * \param in merged byte
         */
        void setFBConn2(uint8_t in);

        /*!
         * \brief Merge operator data to send into 0xE862 register
         * \param OpID Operator type (CARRIER1, MODULATOR1, CARRIER2, MODULATOR2)
         * \return merged 4-byte register data
         */
        uint32_t getDataE862(int OpID) const;

    };

    struct MidiBank
    {
        //! Custom bank name
        char name[33];
        //! MIDI bank MSB index
        uint8_t msb;
        //! MIDI bank LSB index
        uint8_t lsb;
    };

    bool    deep_vibrato   = false;
    bool    deep_tremolo   = false;

    enum VolumesScale
    {
        VOLUME_Generic,
        VOLUME_CMF,
        VOLUME_DMX,
        VOLUME_APOGEE,
        VOLUME_9X
    };
    uint8_t volume_model    = (uint8_t)VOLUME_Generic;

    /**
     * @brief Get empty instrument entry
     * @return null-filled instrument entry
     */
    static Instrument emptyInst();

    /**
     * @brief Get empty bank meta-data entry
     * @return null-filled bank entry
     */
    static MidiBank emptyBank(uint16_t index = 0);

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
    //! Array of melodic MIDI bank meta-data per every index
    QVector<MidiBank>   Banks_Melodic;
    //! Array of percussion MIDI bank meta-data per every index
    QVector<MidiBank>   Banks_Percussion;
};

class TmpBank
{
public:
    TmpBank(FmBank &bank, int minMelodic, int minPercusive);

    //! Pointer to array of melodic instruments
    FmBank::Instrument* insMelodic;
    //! Pointer to array of percussion instruments
    FmBank::Instrument* insPercussion;
    //! Array of melodic instruments
    QVector<FmBank::Instrument> tmpMelodic;
    //! Array of percussion instruments
    QVector<FmBank::Instrument> tmpPercussion;
};

#endif // BANK_H
