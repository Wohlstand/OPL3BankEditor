#include <QSysInfo>
#include <QMessageBox>
#include <cstring>
#include "win9x_opl_proxy.h"

#ifndef _WIN32
/* FAKE DUMMIES FOR ABILITY TO BUILD THIS CODE ON ANY OS */
#define _stdcall
typedef intptr_t HINSTANCE;
void *GetProcAddress(HINSTANCE, const char *)
{
    return nullptr;
}
const char *GetLastError()
{
    return "Unsupported operating system!";
}
HINSTANCE LoadLibraryA(const char*)
{
    return 0;
}
void FreeLibrary(HINSTANCE)
{}
#endif

extern "C"
{
    typedef void (_stdcall *opl_poke)(uint16_t index, uint16_t value);
    typedef void (_stdcall *opl_init)(void);
    typedef void (_stdcall *opl_unInit)(void);
}

struct OPLProxyDriver
{
    opl_poke     chip_oplPoke = nullptr;
    opl_init     chip_oplInit = nullptr;
    opl_unInit   chip_oplUninit = nullptr;
    HINSTANCE    chip_lib = 0;
};

template<class FunkPtr>
void initOplFunction(HINSTANCE &chip_lib, FunkPtr &ptr, const char *procName)
{
    ptr = (FunkPtr)GetProcAddress(chip_lib, procName);
    if(!ptr)
    {
        QMessageBox::warning(nullptr,
                             "liboplproxy.dll error",
                             QString("Oops... I have failed to load %1 function:\n"
                                     "Error %2\n"
                                     "Continuing without FM sound.")
                                    .arg(procName)
                                    .arg(GetLastError()));
    }
}

void Win9x_OPL_Proxy::initChip()
{
    OPLProxyDriver *chip_r = reinterpret_cast<OPLProxyDriver*>(m_chip);
    QSysInfo::WinVersion wver = QSysInfo::windowsVersion();
    bool m_enableProxy =    (wver == QSysInfo::WV_98) ||
                            (wver == QSysInfo::WV_Me);
    if(m_enableProxy && !chip_r->chip_lib)
    {
        chip_r->chip_lib = LoadLibraryA("liboplproxy.dll");
        if(!chip_r->chip_lib)
            QMessageBox::warning(nullptr, "liboplproxy.dll error", "Can't load liboplproxy.dll library");
        else
        {
            initOplFunction(chip_r->chip_lib, chip_r->chip_oplInit,   "_chipInit@0");
            initOplFunction(chip_r->chip_lib, chip_r->chip_oplPoke,   "_chipPoke@8");
            initOplFunction(chip_r->chip_lib, chip_r->chip_oplUninit, "_chipUnInit@0");
            if(chip_r->chip_oplInit)
                chip_r->chip_oplInit();
        }
    }
}

void Win9x_OPL_Proxy::closeChip()
{
    OPLProxyDriver *chip_r = reinterpret_cast<OPLProxyDriver*>(m_chip);
    if(chip_r->chip_lib)
    {
        if(chip_r->chip_oplUninit)
            chip_r->chip_oplUninit();
        FreeLibrary(chip_r->chip_lib);
        chip_r->chip_lib = 0;
        chip_r->chip_oplInit = nullptr;
        chip_r->chip_oplPoke = nullptr;
        chip_r->chip_oplUninit = nullptr;
    }
}

Win9x_OPL_Proxy::Win9x_OPL_Proxy():
    OPLChipBase()
{
    m_chip = new OPLProxyDriver;
    std::memset(m_chip, 0, sizeof(OPLProxyDriver));
    initChip();
}

Win9x_OPL_Proxy::Win9x_OPL_Proxy(const Win9x_OPL_Proxy &c):
    OPLChipBase(c)
{
    m_chip = new OPLProxyDriver;
    std::memset(m_chip, 0, sizeof(OPLProxyDriver));
    initChip();
}

Win9x_OPL_Proxy::~Win9x_OPL_Proxy()
{
    closeChip();
    OPLProxyDriver *chip_r = reinterpret_cast<OPLProxyDriver*>(m_chip);
    delete chip_r;
}

void Win9x_OPL_Proxy::setRate(uint32_t)
{
    /* Do nothing, as it's real chip, not an emulator */
}

void Win9x_OPL_Proxy::reset()
{}

void Win9x_OPL_Proxy::writeReg(uint16_t addr, uint8_t data)
{
    OPLProxyDriver *chip_r = reinterpret_cast<OPLProxyDriver*>(m_chip);
    if(chip_r->chip_oplPoke)
        chip_r->chip_oplPoke(addr, data);
}

int Win9x_OPL_Proxy::generate(int16_t *output, size_t frames)
{
    std::memset(output, 0, sizeof(int16_t) * frames * 2);
    return (int)frames;
}

int Win9x_OPL_Proxy::generateAndMix(int16_t * /*output*/, size_t frames)
{
    return (int)frames;
}

int Win9x_OPL_Proxy::generate32(int32_t *output, size_t frames)
{
    std::memset(output, 0, sizeof(int32_t) * frames * 2);
    return (int)frames;
}

int Win9x_OPL_Proxy::generateAndMix32(int32_t * /*output*/, size_t frames)
{
    return (int)frames;
}

const char *Win9x_OPL_Proxy::emulatorName()
{
    return "OPL3 Proxy Driver";
}
