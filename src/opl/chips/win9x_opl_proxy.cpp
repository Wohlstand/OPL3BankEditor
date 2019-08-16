#include <QMessageBox>
#include <cstring>
#include "win9x_opl_proxy.h"

#ifdef _WIN32
#define OPLPROXY_LIBNAME "liboplproxy.dll"
#elif __APPLE__
#define OPLPROXY_LIBNAME "./liboplproxy.dylib"
#else
#define OPLPROXY_LIBNAME "./liboplproxy.so"
#endif


#ifdef _WIN32
#   include <windows.h>
#   define CALL_chipInit "_chipInit@0"
#   define CALL_chipPoke "_chipPoke@8"
#   define CALL_chipUnInit "_chipUnInit@0"
#   define CALL_chipSetPort "_chipSetPort@4"
#else
#   include <dlfcn.h>
#   define CALL_chipInit "chipInit"
#   define CALL_chipPoke "chipPoke"
#   define CALL_chipUnInit "chipUnInit"
#   define CALL_chipSetPort "chipSetPort"
#   define CALL_chipType "chipType"

/* Some simulation of LoadLibrary WinAPI */
#   define _stdcall
typedef void* HINSTANCE;

void *GetProcAddress(HINSTANCE lib, const char *call)
{
    return dlsym(lib, call);
}

const char *GetLastError()
{
    return "Library is incompatible or is not found!";
}

HINSTANCE LoadLibraryA(const char *path)
{
    return dlopen(path, RTLD_LAZY);
}

void FreeLibrary(HINSTANCE lib)
{
    dlclose(lib);
}
#endif //_WIN32

extern "C"
{
    typedef void (_stdcall *opl_poke)(uint16_t index, uint16_t value);
    typedef void (_stdcall *opl_init)(void);
    typedef void (_stdcall *opl_unInit)(void);
    typedef void (_stdcall *opl_setPort)(uint16_t port);
    typedef uint16_t (_stdcall *opl_type)(void);
}

struct OPLProxyDriver
{
    opl_poke     chip_oplPoke = nullptr;
    opl_init     chip_oplInit = nullptr;
    opl_unInit   chip_oplUninit = nullptr;
    opl_setPort  chip_oplSetPort = nullptr;
    opl_type     chip_oplType = nullptr;
    HINSTANCE    chip_lib = 0;
    OPLChipBase::ChipType chip_type = OPLChipBase::CHIPTYPE_OPL3;
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
                             OPLPROXY_LIBNAME " error",
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
        chip_r->chip_lib = LoadLibraryA(OPLPROXY_LIBNAME);
        if(!chip_r->chip_lib)
            QMessageBox::warning(nullptr, OPLPROXY_LIBNAME " error", "Can't load " OPLPROXY_LIBNAME " library");
        else
        {
            initOplFunction(chip_r->chip_lib, chip_r->chip_oplInit,   CALL_chipInit);
            initOplFunction(chip_r->chip_lib, chip_r->chip_oplPoke,   CALL_chipPoke);
            initOplFunction(chip_r->chip_lib, chip_r->chip_oplUninit, CALL_chipUnInit);
            initOplFunction(chip_r->chip_lib, chip_r->chip_oplSetPort, CALL_chipSetPort, false);
            initOplFunction(chip_r->chip_lib, chip_r->chip_oplType,   CALL_chipType, false);

            if(chip_r->chip_oplInit)
                chip_r->chip_oplInit();
            if(chip_r->chip_oplType)
                chip_r->chip_type = static_cast<OPLChipBase::ChipType>(chip_r->chip_oplType());
            else
                chip_r->chip_type = OPLChipBase::CHIPTYPE_OPL3;
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
        chip_r->chip_type = OPLChipBase::CHIPTYPE_OPL3;
        chip_r->chip_lib = 0;
        chip_r->chip_oplInit = nullptr;
        chip_r->chip_oplPoke = nullptr;
        chip_r->chip_oplUninit = nullptr;
        chip_r->chip_oplSetPort = nullptr;
        chip_r->chip_oplType = nullptr;
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

OPLChipBase::ChipType Win9x_OPL_Proxy::chipType()
{
    OPLProxyDriver *chip_r = reinterpret_cast<OPLProxyDriver*>(m_chip);
    return chip_r->chip_type;
}
