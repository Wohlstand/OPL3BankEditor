#include <QMessageBox>
#include <cstring>
#include "win9x_opl_proxy.h"

#ifdef _WIN32
#include <windows.h>
#else
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
    typedef void (_stdcall *opl_setPort)(uint16_t port);
}

struct OPLProxyDriver
{
    opl_poke     chip_oplPoke = nullptr;
    opl_init     chip_oplInit = nullptr;
    opl_unInit   chip_oplUninit = nullptr;
    opl_setPort  chip_oplSetPort = nullptr;
    HINSTANCE    chip_lib = 0;
};

template<class FunkPtr>
void initOplFunction(HINSTANCE &chip_lib, FunkPtr &ptr, const char *procName, bool required = true)
{
    ptr = (FunkPtr)GetProcAddress(chip_lib, procName);

    if(!ptr && procName[0] == '_')
        ptr = (FunkPtr)GetProcAddress(chip_lib, procName + 1);

    static bool shownWarning = false;
    if(!ptr && required && !shownWarning)
    {
        shownWarning = true;
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
    if(!chip_r->chip_lib)
    {
        chip_r->chip_lib = LoadLibraryA("liboplproxy.dll");
        if(!chip_r->chip_lib)
            QMessageBox::warning(nullptr, "liboplproxy.dll error", "Can't load liboplproxy.dll library");
        else
        {
            initOplFunction(chip_r->chip_lib, chip_r->chip_oplInit,   "_chipInit@0");
            initOplFunction(chip_r->chip_lib, chip_r->chip_oplPoke,   "_chipPoke@8");
            initOplFunction(chip_r->chip_lib, chip_r->chip_oplUninit, "_chipUnInit@0");
            initOplFunction(chip_r->chip_lib, chip_r->chip_oplSetPort, "_chipSetPort@4", false);
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
        chip_r->chip_oplSetPort = nullptr;
    }
}

bool Win9x_OPL_Proxy::canSetOplAddress() const
{
    OPLProxyDriver *chip_r = reinterpret_cast<OPLProxyDriver*>(m_chip);
    return chip_r->chip_oplSetPort != nullptr;
}

void Win9x_OPL_Proxy::setOplAddress(uint16_t address)
{
    OPLProxyDriver *chip_r = reinterpret_cast<OPLProxyDriver*>(m_chip);
    if(chip_r->chip_oplSetPort)
        chip_r->chip_oplSetPort(address);
}

Win9x_OPL_Proxy::Win9x_OPL_Proxy()
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

void Win9x_OPL_Proxy::writeReg(uint16_t addr, uint8_t data)
{
    OPLProxyDriver *chip_r = reinterpret_cast<OPLProxyDriver*>(m_chip);
    if(chip_r->chip_oplPoke)
        chip_r->chip_oplPoke(addr, data);
}

void Win9x_OPL_Proxy::nativeGenerate(int16_t *frame)
{
    frame[0] = 0;
    frame[1] = 0;
}

const char *Win9x_OPL_Proxy::emulatorName()
{
    return "OPL3 Proxy Driver";
}
