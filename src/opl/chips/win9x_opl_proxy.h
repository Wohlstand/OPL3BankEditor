#ifndef WIN9X_OPL_PROXY_H
#define WIN9X_OPL_PROXY_H

#include "opl_chip_base.h"

class Win9x_OPL_Proxy : public OPLChipBaseT<Win9x_OPL_Proxy>
{
    void *m_chip;
    void initChip();
    void closeChip();

public:
    Win9x_OPL_Proxy();
    virtual ~Win9x_OPL_Proxy() override;

    void startChip();

    bool canSetOplAddress() const;
    void setOplAddress(uint16_t address);

    bool canRunAtPcmRate() const override { return false; }
    void setRate(uint32_t /*rate*/) override {}
    void reset() override {}
    void writeReg(uint16_t addr, uint8_t data) override;
    void nativePreGenerate() override {}
    void nativePostGenerate() override {}
    void nativeGenerate(int16_t *frame) override;
    const char *emulatorName() override;
    ChipType chipType() override;
    bool hasFullPanning() override;
};

#endif // WIN9X_OPL_PROXY_H
