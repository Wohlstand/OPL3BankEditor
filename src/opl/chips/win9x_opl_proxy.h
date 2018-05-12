#ifndef WIN9X_OPL_PROXY_H
#define WIN9X_OPL_PROXY_H

#include "opl_chip_base.h"

class Win9x_OPL_Proxy : public OPLChipBase
{
    void *m_chip;
    void initChip();
    void closeChip();
public:
    Win9x_OPL_Proxy();
    Win9x_OPL_Proxy(const Win9x_OPL_Proxy &c);
    virtual ~Win9x_OPL_Proxy() override;

    virtual void setRate(uint32_t rate) override;
    virtual void reset() override;
    virtual void writeReg(uint16_t addr, uint8_t data) override;
    virtual int generate(int16_t *output, size_t frames) override;
    virtual int generateAndMix(int16_t *output, size_t frames) override;
    virtual int generate32(int32_t *output, size_t frames) override;
    virtual int generateAndMix32(int32_t *output, size_t frames) override;
    virtual const char *emulatorName() override;
};

#endif // WIN9X_OPL_PROXY_H
