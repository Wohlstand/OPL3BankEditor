#include "dosbox_opl3.h"
#include "dosbox/dbopl.h"
#include <cstdlib>
#include <assert.h>

DosBoxOPL3::DosBoxOPL3() :
    OPLChipBase(),
    chip(nullptr)
{
    reset();
}

DosBoxOPL3::DosBoxOPL3(const DosBoxOPL3 &c) :
    OPLChipBase(c),
    chip(nullptr)
{
    setRate(c.m_rate);
}

DosBoxOPL3::~DosBoxOPL3()
{
    DBOPL::Handler *chip_r = reinterpret_cast<DBOPL::Handler*>(chip);
    delete chip_r;
}

void DosBoxOPL3::setRate(uint32_t rate)
{
    OPLChipBase::setRate(rate);
    reset();
}

void DosBoxOPL3::reset()
{
    DBOPL::Handler *chip_r = reinterpret_cast<DBOPL::Handler*>(chip);
    if(chip && chip_r)
        delete chip_r;
    chip = new DBOPL::Handler;
    chip_r = reinterpret_cast<DBOPL::Handler*>(chip);
    chip_r->Init(m_rate);
}

void DosBoxOPL3::reset(uint32_t rate)
{
    setRate(rate);
}

void DosBoxOPL3::writeReg(uint16_t addr, uint8_t data)
{
    DBOPL::Handler *chip_r = reinterpret_cast<DBOPL::Handler*>(chip);
    chip_r->WriteReg(static_cast<Bit32u>(addr), data);
}

int DosBoxOPL3::generate(int16_t *output, size_t frames)
{
    DBOPL::Handler *chip_r = reinterpret_cast<DBOPL::Handler*>(chip);
    ssize_t left = (ssize_t)frames;
    while(left > 0)
    {
        ssize_t frames_i = left;
        chip_r->GenerateArr(output, &frames_i);
        output += (frames_i * 2);
        left -= frames_i;
    }
    return (int)frames;
}

int DosBoxOPL3::generateAndMix(int16_t *output, size_t frames)
{
    DBOPL::Handler *chip_r = reinterpret_cast<DBOPL::Handler*>(chip);
    ssize_t left = (ssize_t)frames;
    while(left > 0)
    {
        ssize_t frames_i = left;
        chip_r->GenerateArrMix(output, &frames_i);
        output += (frames_i * 2);
        left -= frames_i;
    }
    return (int)frames;
}

const char *DosBoxOPL3::emulatorName()
{
    return "DosBox 0.74 OPL3";
}
