/*
* inpout32.dll helper functions
* 2013 James Alan Nguyen
* http://www.codingchords.com
*
* Do whatever you want with it.  Made this while trying to port the OPL passthrough
* routines on VGMPlay, since PortTalk does not work under 64-bit Windows.
*/

#include "InpOut32Helper.h"

typedef void	(__stdcall *lpOut32)(short, short);
typedef short	(__stdcall *lpInp32)(short);
typedef BOOL	(__stdcall *lpIsInpOutDriverOpen)(void);
typedef BOOL	(__stdcall *lpIsXP64Bit)(void);

static lpOut32 gfpOut32 = 0;
static lpInp32 gfpInp32 = 0;
static lpIsInpOutDriverOpen gfpIsInpOutDriverOpen = 0;
static lpIsXP64Bit gfpIsXP64Bit = 0;

static HINSTANCE hInpOutDll ;
#ifdef _DEBUG
static FILE *hwlog;
#endif

/*
* Returns true if inpout32 failed to open (necessary for applications checking for non-zero failures)
*/
BOOL OpenInpOut32(void)
{
#if defined(_X86_)
	hInpOutDll = LoadLibraryW ( L"InpOut32.DLL" ) ;	//The 32bit DLL. If we are building x64 C++ 
	//applicaiton then use InpOutx64.dll
#elif defined(_AMD64_)
	hInpOutDll = LoadLibraryW ( L"InpOutX64.DLL" ) ;
#endif
	if ( hInpOutDll != NULL )
	{
		gfpOut32 = (lpOut32)GetProcAddress(hInpOutDll, "Out32");
		gfpInp32 = (lpInp32)GetProcAddress(hInpOutDll, "Inp32");
		gfpIsInpOutDriverOpen = (lpIsInpOutDriverOpen)GetProcAddress(hInpOutDll, "IsInpOutDriverOpen");
		gfpIsXP64Bit = (lpIsXP64Bit)GetProcAddress(hInpOutDll, "IsXP64Bit");

		if (gfpIsInpOutDriverOpen())
#ifdef _DEBUG
		hwlog = fopen("C:\\OPLSynth\\hwlog.log", "a");
		fprintf(hwlog, "Opened driver\n");
#endif
		return FALSE; 
	}

#if defined(_X86_)
	MessageBoxW(NULL, L"Cannot load inpout32.dll for hardware OPL playback.", L"OPL3_HW", MB_OK | MB_ICONEXCLAMATION);
#elif defined(_AMD64_)
	MessageBoxW(NULL, L"Cannot load inpoutx64.dll for hardware OPL playback.", L"OPL3_HW", MB_OK | MB_ICONEXCLAMATION);
#endif
	return TRUE;
}

void CloseInpOut32(void)
{
#ifdef _DEBUG
	fprintf(hwlog, "Closed driver\n");
	fclose(hwlog);
#endif
	FreeLibrary(hInpOutDll);
	gfpOut32 = NULL;
	gfpInp32 = NULL;
	gfpIsInpOutDriverOpen = NULL;
	gfpIsXP64Bit = NULL;
}

void outportb(unsigned short PortAddress, unsigned short Data)
{
	if (gfpOut32 == NULL)
	{
#ifdef _DEBUG
#if defined(_X86_)
		MessageBoxW(NULL, L"Cannot write to register; inpout32.dll not loaded.", L"DEBUG", MB_OK | MB_ICONERROR);
#elif defined(_AMD64_)
		MessageBoxW(NULL, L"Cannot write to register; inpoutx64.dll not loaded.", L"DEBUG", MB_OK | MB_ICONERROR);
#endif
#endif
		return;
	}
	gfpOut32(PortAddress, Data);
#ifdef _DEBUG
	fprintf(hwlog, "Write: %x, %x\n", PortAddress, Data);
#endif
}

unsigned char inportb(unsigned short PortAddress)
{
	if (gfpInp32 == NULL)
	{
#ifdef _DEBUG
#if defined(_X86_)
		MessageBoxW(NULL, L"Cannot read register; inpout32.dll not loaded.", L"DEBUG", MB_OK | MB_ICONERROR);
#elif defined(_AMD64_)
		MessageBoxW(NULL, L"Cannot read register; inpoutx64.dll not loaded.", L"DEBUG", MB_OK | MB_ICONERROR);
#endif
#endif
		return ~0;
	}
#ifdef _DEBUG
	fprintf(hwlog, "Read: %x\n", PortAddress);
#endif
	return (char)gfpInp32(PortAddress);
}
