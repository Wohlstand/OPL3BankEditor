#include "bank.h"
#include <memory.h>

FmBank::FmBank()
{
    reset();
}

FmBank::FmBank(const FmBank &fb)
{
    int size=sizeof(Instrument)*128;
    memcpy(Ins_Melodic,    fb.Ins_Melodic,    size);
    memcpy(Ins_Percussion, fb.Ins_Percussion, size);
}

void FmBank::reset()
{
    int size=sizeof(Instrument)*128;
    memset(Ins_Melodic,    0, size);
    memset(Ins_Percussion, 0, size);
}

