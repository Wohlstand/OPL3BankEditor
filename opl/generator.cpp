#include "generator.h"
#include <qendian.h>

const int DataSampleRateHz = 44100;
const int BufferSize      = 4096;

struct adldata
{
    unsigned int modulator_E862, carrier_E862;  // See below
    unsigned char modulator_40, carrier_40; // KSL/attenuation settings
    unsigned char feedconn; // Feedback/connection bits for the channel
    signed char finetune;
    bool diff;
};

struct adlinsdata
{
    enum { Flag_Pseudo4op = 0x01, Flag_NoSound = 0x02 };

    unsigned short adlno1, adlno2;
    unsigned char tone;
    unsigned char flags;
    unsigned short ms_sound_kon;  // Number of milliseconds it produces sound;
    unsigned short ms_sound_koff;
    double fine_tune;
};

adldata adl[] = {
{ 0x0F70700,0x0F70710, 0xFF,0xFF, 0x0, +0, false }, // 0: BisqP15; f15GP0; f15GP1; f15GP10; f15GP101; f15GP102; f15GP103; f15GP104; f15GP105; f15GP106; f15GP107; f15GP108; f15GP109; f15GP11; f15GP110; f15GP111; f15GP112; f15GP113; f15GP114; f15GP115; f15GP116; f15GP117; f15GP118; f15GP119; f15GP12; f15GP120; f15GP121; f15GP122; f15GP123; f15GP124; f15GP125; f15GP126; f15GP127; f15GP13; f15GP14; f15GP15; f15GP16; f15GP17; f15GP18; f15GP19; f15GP2; f15GP20; f15GP21; f15GP22; f15GP23; f15GP24; f15GP25; f15GP26; f15GP27; f15GP28; f15GP29; f15GP3; f15GP30; f15GP31; f15GP32; f15GP33; f15GP34; f15GP4; f15GP5; f15GP52; f15GP53; f15GP55; f15GP57; f15GP58; f15GP59; f15GP6; f15GP7; f15GP74; f15GP76; f15GP77; f15GP78; f15GP79; f15GP8; f15GP80; f15GP81; f15GP82; f15GP83; f15GP84; f15GP85; f15GP86; f15GP87; f15GP88; f15GP89; f15GP9; f15GP90; f15GP91; f15GP92; f15GP93; f15GP94; f15GP95; f15GP96; f15GP97; f15GP98; f15GP99; f26GP0; f26GP1; f26GP10; f26GP101; f26GP102; f26GP103; f26GP104; f26GP105; f26GP106; f26GP107; f26GP108; f26GP109; f26GP11; f26GP110; f26GP111; f26GP112; f26GP113; f26GP114; f26GP115; f26GP116; f26GP117; f26GP118; f26GP119; f26GP12; f26GP120; f26GP121; f26GP122; f26GP123; f26GP124; f26GP125; f26GP126; f26GP127; f26GP13; f26GP14; f26GP15; f26GP16; f26GP17; f26GP18; f26GP19; f26GP2; f26GP20; f26GP21; f26GP22; f26GP23; f26GP24; f26GP25; f26GP26; f26GP27; f26GP28; f26GP29; f26GP3; f26GP30; f26GP31; f26GP32; f26GP33; f26GP34; f26GP4; f26GP5; f26GP52; f26GP53; f26GP55; f26GP57; f26GP58; f26GP59; f26GP6; f26GP7; f26GP74; f26GP76; f26GP77; f26GP78; f26GP79; f26GP8; f26GP80; f26GP81; f26GP82; f26GP83; f26GP84; f26GP85; f26GP86; f26GP87; f26GP88; f26GP89; f26GP9; f26GP90; f26GP91; f26GP92; f26GP93; f26GP94; f26GP95; f26GP96; f26GP97; f26GP98; f26GP99; nosound; Bell Tree; Castanets; Chinese Cymbal; Crash Cymbal 2; High Wood Block; Jingle Bell; Long Guiro; Low Wood Block; Mute Cuica; Mute Surdu; Mute Triangle; Open Cuica; Open Surdu; Open Triangle; Ride Bell; Ride Cymbal 2; Shaker; Splash Cymbal; Vibraslap
{ 0x0F4F201,0x0F7F201, 0x8F,0x06, 0x8, +0, false }, // 1: BisqM0; GM0; b13M0; f29GM0; f30GM0; fat2M0; sGM0; AcouGrandPiano; am000
{ 0x0F4F201,0x0F7F201, 0x4B,0x00, 0x8, +0, false }, // 2: GM1; b13M1; f29GM1; f30GM1; fat2M1; sGM1; BrightAcouGrand; am001
{ 0x0F4F201,0x0F6F201, 0x49,0x00, 0x8, +0, false }, // 3: BisqM2; GM2; b13M2; f29GM2; f30GM2; f34GM0; f34GM1; f34GM2; fat2M2; sGM2; AcouGrandPiano; BrightAcouGrand; ElecGrandPiano; am002
{ 0x0F7F281,0x0F7F241, 0x12,0x00, 0x6, +0, false }, // 4: GM3; b13M3; f34GM3; fat2M3; sGM3; Honky-tonkPiano; am003
{ 0x0F7F101,0x0F7F201, 0x57,0x00, 0x0, +0, false }, // 5: GM4; b13M4; f34GM4; fat2M4; sGM4; Rhodes Piano; am004
{ 0x0F7F101,0x0F7F201, 0x93,0x00, 0x0, +0, false }, // 6: GM5; b13M5; f29GM6; f30GM6; f34GM5; fat2M5; sGM5; Chorused Piano; Harpsichord; am005
{ 0x0F2A101,0x0F5F216, 0x80,0x0E, 0x8, +0, false }, // 7: GM6; b13M6; f34GM6; fat2M6; Harpsichord; am006
{ 0x0F8C201,0x0F8C201, 0x92,0x00, 0xA, +0, false }, // 8: GM7; b13M7; b65MM7; f34GM7; fat2M7; sGM7; CLAVICHD
{ 0x0F4F60C,0x0F5F381, 0x5C,0x00, 0x0, +0, false }, // 9: GM8; b13M8; f34GM8; fat2M8; sGM8; Celesta; am008
{ 0x0F2F307,0x0F1F211, 0x97,0x80, 0x2, +0, false }, // 10: BisqM9; GM9; b13M9; b65MM9; f29GM101; f30GM101; f34GM9; fat2M9; FX 6 goblins; GLOCK
{ 0x0F45417,0x0F4F401, 0x21,0x00, 0x2, +0, false }, // 11: GM10; b13M10; b65MM10; f29GM100; f30GM100; f34GM10; fat2M10; sGM10; FX 5 brightness; MUSICBOX
{ 0x0F6F398,0x0F6F281, 0x62,0x00, 0x0, +0, false }, // 12: BisqM11; GM11; b13M11; f34GM11; fat2M11; sGM11; Vibraphone; am011
{ 0x0F6F618,0x0F7E701, 0x23,0x00, 0x0, +0, false }, // 13: GM12; b13M12; f29GM104; f29GM97; f30GM104; f30GM97; f34GM12; fat2M12; sGM12; FX 2 soundtrack; Marimba; Sitar; am012
{ 0x0F6F615,0x0F6F601, 0x91,0x00, 0x4, +0, false }, // 14: GM13; b13M13; f29GM103; f30GM103; f34GM13; fat2M13; sGM13; FX 8 sci-fi; Xylophone; am013
{ 0x0F3D345,0x0F3A381, 0x59,0x80, 0xC, +0, false }, // 15: GM14; b13M14; b65MM14; f34GM14; fat2M14; TUBEBELL
{ 0x1F57503,0x0F5B581, 0x49,0x80, 0x4, +0, false }, // 16: GM15; b13M15; b65MM15; f34GM15; fat2M15; sGM15; Dulcimer; PIANOBEL
{ 0x014F671,0x007F131, 0x92,0x00, 0x2, +0, false }, // 17: 3drm67M16; GM16; HMIGM16; b13M16; b7M16; f34GM16; fat2M16; sGM16; Hammond Organ; am016; am016.in
{ 0x058C772,0x008C730, 0x14,0x00, 0x2, +0, false }, // 18: 3drm67M17; GM17; HMIGM17; b13M17; b7M17; f34GM17; fat2M17; sGM17; Percussive Organ; am017; am017.in
{ 0x018AA70,0x0088AB1, 0x44,0x00, 0x4, +0, false }, // 19: 3drm67M18; GM18; HMIGM18; b13M18; b7M18; f34GM18; fat2M18; sGM18; Rock Organ; am018; am018.in
{ 0x1239723,0x01455B1, 0x93,0x00, 0x4, +0, false }, // 20: 3drm67M19; GM19; HMIGM19; b13M19; b7M19; f34GM19; fat2M19; Church Organ; am019; am019.in
{ 0x1049761,0x00455B1, 0x13,0x80, 0x0, +0, false }, // 21: 3drm67M20; BisqM20; GM20; HMIGM20; b13M20; b65MM20; b7M20; f34GM20; fat2M20; sGM20; REEDORGN
{ 0x12A9824,0x01A46B1, 0x48,0x00, 0xC, +0, false }, // 22: 3drm67M21; GM21; HMIGM21; b13M21; b7M21; f34GM21; fat2M21; sGM21; Accordion; am021; am021.in
{ 0x1069161,0x0076121, 0x13,0x00, 0xA, +0, false }, // 23: 3drm67M22; GM22; HMIGM22; b13M22; b7M22; f34GM22; fat2M22; sGM22; Harmonica; am022; am022.in
{ 0x0067121,0x00761A1, 0x13,0x89, 0x6, +0, false }, // 24: 3drm67M23; GM23; HMIGM23; b13M23; b7M23; f34GM23; fat2M23; sGM23; Tango Accordion; am023; am023.in
{ 0x194F302,0x0C8F341, 0x9C,0x80, 0xC, +0, false }, // 25: 3drm67M24; GM24; HMIGM24; b13M24; b7M24; f34GM24; fat2M24; Acoustic Guitar1; am024; am024.in
{ 0x19AF303,0x0E7F111, 0x54,0x00, 0xC, +0, false }, // 26: 3drm67M25; GM25; HMIGM25; b13M25; b7M25; f17GM25; f29GM60; f30GM60; f34GM25; fat2M25; mGM25; sGM25; Acoustic Guitar2; French Horn; am025; am025.in
{ 0x03AF123,0x0F8F221, 0x5F,0x00, 0x0, +0, false }, // 27: 3drm67M26; GM26; HMIGM26; b13M26; b7M26; b8M26; f17GM26; f34GM26; f35GM26; fat2M26; mGM26; sGM26; Electric Guitar1; am026; am026.in; jazzgtr
{ 0x122F603,0x0F8F321, 0x87,0x80, 0x6, +0, false }, // 28: GM27; b13M27; f30GM61; f34GM27; fat2M27; sGM27; Brass Section; Electric Guitar2; am027
{ 0x054F903,0x03AF621, 0x47,0x00, 0x0, +0, false }, // 29: 3drm67M28; GM28; HMIGM28; b13M28; b6M107; b6M3; b6M99; b7M28; b8M20; b8M28; f17GM28; f34GM28; f35GM28; fat2M28; hamM3; hamM60; intM3; mGM28; rickM3; sGM28; BPerc; BPerc.in; Electric Guitar3; RBPerc; Rmutegit; am028; am028.in; muteguit
{ 0x1419123,0x0198421, 0x4A,0x05, 0x8, +0, false }, // 30: GM29; b13M29; f34GM29; fat2M29; sGM29; Overdrive Guitar; am029
{ 0x1199523,0x0199421, 0x4A,0x00, 0x8, +0, false }, // 31: 3drm67M30; GM30; HMIGM30; b13M30; b6M116; b6M6; b7M30; f17GM30; f34GM30; f35GM30; fat2M30; hamM6; intM6; mGM30; rickM6; sGM30; Distorton Guitar; GDist; GDist.in; RGDist; am030; am030.in
{ 0x04F2009,0x0F8D184, 0xA1,0x80, 0x8, +0, false }, // 32: 3drm67M31; GM31; HMIGM31; b13M31; b6M104; b6M5; b7M31; b8M120; f34GM31; fat2M31; hamM5; intM5; rickM5; sGM31; Feedbck; GFeedbck; Guitar Harmonics; RFeedbck; am031; am031.in
{ 0x125B121,0x00872A2, 0x9B,0x01, 0xE, +0, false }, // 33: 3drm67M48; GM48; HMIGM48; b13M48; b7M48; f34GM48; fat2M48; String Ensemble1; am048; am048.in
{ 0x028F131,0x018F131, 0x12,0x00, 0xA, +0, false }, // 34: 3drm67M33; 3drm67M39; GM33; GM39; HMIGM33; HMIGM39; b13M33; b13M39; b7M33; b7M39; f15GM30; f17GM33; f17GM39; f26GM30; f29GM28; f29GM29; f30GM28; f30GM29; f34GM33; f34GM39; f35GM39; fat2M33; fat2M39; hamM68; mGM33; mGM39; sGM33; sGM39; Distorton Guitar; Electric Bass 1; Electric Guitar3; Overdrive Guitar; Synth Bass 2; am033; am033.in; am039; am039.in; synbass2
{ 0x871A7223,0x802A7221, 0xAC,0x83, 0x0, +0, false }, // 35: fat4M21; Accordion
{ 0x841A6223,0x802A62A1, 0x22,0x00, 0x1, +0, false }, // 36: fat4M21; Accordion
{ 0x104C060,0x10455B1, 0x51,0x80, 0x4, +12, false }, // 37: dMM48; hxMM48; musM48; raptM48; String Ensemble 1
{ 0x10490A0,0x1045531, 0x52,0x80, 0x6, +12, true }, // 38: dMM48; hxMM48; musM48; raptM48; String Ensemble 1
};

adlinsdata adlins[] =
{
    // Amplitude begins at    0.0,
    // fades to 20% at 0.0s, keyoff fades to 20% in 0.0s.
    {   0,   0,  0, 2,      0,     0,0.000000 }, // 0: BisqP15; f15GP0; f15GP1; f15GP10; f15GP101; f15GP102; f15GP103; f15GP104; f15GP105; f15GP106; f15GP107; f15GP108; f15GP109; f15GP11; f15GP110; f15GP111; f15GP112; f15GP113; f15GP114; f15GP115; f15GP116; f15GP117; f15GP118; f15GP119; f15GP12; f15GP120; f15GP121; f15GP122; f15GP123; f15GP124; f15GP125; f15GP126; f15GP127; f15GP13; f15GP14; f15GP15; f15GP16; f15GP17; f15GP18; f15GP19; f15GP2; f15GP20; f15GP21; f15GP22; f15GP23; f15GP24; f15GP25; f15GP26; f15GP27; f15GP28; f15GP29; f15GP3; f15GP30; f15GP31; f15GP32; f15GP33; f15GP34; f15GP4; f15GP5; f15GP52; f15GP53; f15GP55; f15GP57; f15GP58; f15GP59; f15GP6; f15GP7; f15GP74; f15GP76; f15GP77; f15GP78; f15GP79; f15GP8; f15GP80; f15GP81; f15GP82; f15GP83; f15GP84; f15GP85; f15GP86; f15GP87; f15GP88; f15GP89; f15GP9; f15GP90; f15GP91; f15GP92; f15GP93; f15GP94; f15GP95; f15GP96; f15GP97; f15GP98; f15GP99; f26GP0; f26GP1; f26GP10; f26GP101; f26GP102; f26GP103; f26GP104; f26GP105; f26GP106; f26GP107; f26GP108; f26GP109; f26GP11; f26GP110; f26GP111; f26GP112; f26GP113; f26GP114; f26GP115; f26GP116; f26GP117; f26GP118; f26GP119; f26GP12; f26GP120; f26GP121; f26GP122; f26GP123; f26GP124; f26GP125; f26GP126; f26GP127; f26GP13; f26GP14; f26GP15; f26GP16; f26GP17; f26GP18; f26GP19; f26GP2; f26GP20; f26GP21; f26GP22; f26GP23; f26GP24; f26GP25; f26GP26; f26GP27; f26GP28; f26GP29; f26GP3; f26GP30; f26GP31; f26GP32; f26GP33; f26GP34; f26GP4; f26GP5; f26GP52; f26GP53; f26GP55; f26GP57; f26GP58; f26GP59; f26GP6; f26GP7; f26GP74; f26GP76; f26GP77; f26GP78; f26GP79; f26GP8; f26GP80; f26GP81; f26GP82; f26GP83; f26GP84; f26GP85; f26GP86; f26GP87; f26GP88; f26GP89; f26GP9; f26GP90; f26GP91; f26GP92; f26GP93; f26GP94; f26GP95; f26GP96; f26GP97; f26GP98; f26GP99; nosound; Bell Tree; Castanets; Chinese Cymbal; Crash Cymbal 2; High Wood Block; Jingle Bell; Long Guiro; Low Wood Block; Mute Cuica; Mute Surdu; Mute Triangle; Open Cuica; Open Surdu; Open Triangle; Ride Bell; Ride Cymbal 2; Shaker; Splash Cymbal; Vibraslap

    // Amplitude begins at 1540.5, peaks 1600.4 at 0.0s,
    // fades to 20% at 1.6s, keyoff fades to 20% in 1.6s.
    {   1,   1,  0, 0,   1633,  1633,0.000000 }, // 1: BisqM0; GM0; b13M0; f29GM0; f30GM0; fat2M0; sGM0; AcouGrandPiano; am000

    // Amplitude begins at 2838.4, peaks 3325.4 at 0.0s,
    // fades to 20% at 1.7s, keyoff fades to 20% in 1.7s.
    {   2,   2,  0, 0,   1720,  1720,0.000000 }, // 2: GM1; b13M1; f29GM1; f30GM1; fat2M1; sGM1; BrightAcouGrand; am001

    // Amplitude begins at 2666.9, peaks 3006.2 at 0.2s,
    // fades to 20% at 2.0s, keyoff fades to 20% in 2.0s.
    {   3,   3,  0, 0,   2000,  2000,0.000000 }, // 3: BisqM2; GM2; b13M2; f29GM2; f30GM2; f34GM0; f34GM1; f34GM2; fat2M2; sGM2; AcouGrandPiano; BrightAcouGrand; ElecGrandPiano; am002

    // Amplitude begins at 2531.7, peaks 2708.0 at 0.1s,
    // fades to 20% at 1.5s, keyoff fades to 20% in 1.5s.
    {   4,   4,  0, 0,   1546,  1546,0.000000 }, // 4: GM3; b13M3; f34GM3; fat2M3; sGM3; Honky-tonkPiano; am003

    // Amplitude begins at 2712.0, peaks 2812.8 at 0.0s,
    // fades to 20% at 1.3s, keyoff fades to 20% in 1.3s.
    {   5,   5,  0, 0,   1266,  1266,0.000000 }, // 5: GM4; b13M4; f34GM4; fat2M4; sGM4; Rhodes Piano; am004

    // Amplitude begins at 2990.4, peaks 3029.6 at 0.0s,
    // fades to 20% at 2.0s, keyoff fades to 20% in 2.0s.
    {   6,   6,  0, 0,   2000,  2000,0.000000 }, // 6: GM5; b13M5; f29GM6; f30GM6; f34GM5; fat2M5; sGM5; Chorused Piano; Harpsichord; am005

    // Amplitude begins at  884.4, peaks  996.5 at 0.0s,
    // fades to 20% at 1.1s, keyoff fades to 20% in 1.1s.
    {   7,   7,  0, 0,   1066,  1066,0.000000 }, // 7: GM6; b13M6; f34GM6; fat2M6; Harpsichord; am006

    // Amplitude begins at 2855.1, peaks 3242.8 at 0.1s,
    // fades to 20% at 1.2s, keyoff fades to 20% in 1.2s.
    {   8,   8,  0, 0,   1246,  1246,0.000000 }, // 8: GM7; b13M7; b65MM7; f34GM7; fat2M7; sGM7; CLAVICHD

    // Amplitude begins at 2810.5,
    // fades to 20% at 0.9s, keyoff fades to 20% in 0.9s.
    {   9,   9,  0, 0,    906,   906,0.000000 }, // 9: GM8; b13M8; f34GM8; fat2M8; sGM8; Celesta; am008

    // Amplitude begins at 2420.2, peaks 2448.1 at 0.0s,
    // fades to 20% at 1.1s, keyoff fades to 20% in 1.1s.
    {  10,  10,  0, 0,   1066,  1066,0.000000 }, // 10: BisqM9; GM9; b13M9; b65MM9; f29GM101; f30GM101; f34GM9; fat2M9; FX 6 goblins; GLOCK

    // Amplitude begins at 2465.6, peaks 3080.6 at 0.0s,
    // fades to 20% at 0.5s, keyoff fades to 20% in 0.5s.
    {  11,  11,  0, 0,    486,   486,0.000000 }, // 11: GM10; b13M10; b65MM10; f29GM100; f30GM100; f34GM10; fat2M10; sGM10; FX 5 brightness; MUSICBOX

    // Amplitude begins at 2678.3, peaks 2947.0 at 0.0s,
    // fades to 20% at 1.7s, keyoff fades to 20% in 1.7s.
    {  12,  12,  0, 0,   1746,  1746,0.000000 }, // 12: BisqM11; GM11; b13M11; f34GM11; fat2M11; sGM11; Vibraphone; am011

    // Amplitude begins at 2584.6,
    // fades to 20% at 0.1s, keyoff fades to 20% in 0.1s.
    {  13,  13,  0, 0,     66,    66,0.000000 }, // 13: GM12; b13M12; f29GM104; f29GM97; f30GM104; f30GM97; f34GM12; fat2M12; sGM12; FX 2 soundtrack; Marimba; Sitar; am012

    // Amplitude begins at 2790.2,
    // fades to 20% at 0.1s, keyoff fades to 20% in 0.1s.
    {  14,  14,  0, 0,    140,   140,0.000000 }, // 14: GM13; b13M13; f29GM103; f30GM103; f34GM13; fat2M13; sGM13; FX 8 sci-fi; Xylophone; am013

    // Amplitude begins at 1839.2, peaks 2527.2 at 0.0s,
    // fades to 20% at 1.0s, keyoff fades to 20% in 1.0s.
    {  15,  15,  0, 0,    960,   960,0.000000 }, // 15: GM14; b13M14; b65MM14; f34GM14; fat2M14; TUBEBELL

    // Amplitude begins at 1984.6, peaks 2599.0 at 0.0s,
    // fades to 20% at 0.3s, keyoff fades to 20% in 0.3s.
    {  16,  16,  0, 0,    273,   273,0.000000 }, // 16: GM15; b13M15; b65MM15; f34GM15; fat2M15; sGM15; Dulcimer; PIANOBEL

    // Amplitude begins at 2950.9, peaks 3494.4 at 32.1s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.0s.
    {  17,  17,  0, 0,  40000,    33,0.000000 }, // 17: 3drm67M16; GM16; HMIGM16; b13M16; b7M16; f34GM16; fat2M16; sGM16; Hammond Organ; am016; am016.in

    // Amplitude begins at 2577.0, peaks 3096.4 at 0.0s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.0s.
    {  18,  18,  0, 0,  40000,     6,0.000000 }, // 18: 3drm67M17; GM17; HMIGM17; b13M17; b7M17; f34GM17; fat2M17; sGM17; Percussive Organ; am017; am017.in

    // Amplitude begins at  817.2, peaks 3053.5 at 38.0s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.0s.
    {  19,  19,  0, 0,  40000,    20,0.000000 }, // 19: 3drm67M18; GM18; HMIGM18; b13M18; b7M18; f34GM18; fat2M18; sGM18; Rock Organ; am018; am018.in

    // Amplitude begins at    0.8, peaks 2671.9 at 0.1s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.2s.
    {  20,  20,  0, 0,  40000,   193,0.000000 }, // 20: 3drm67M19; GM19; HMIGM19; b13M19; b7M19; f34GM19; fat2M19; Church Organ; am019; am019.in

    // Amplitude begins at    0.6, peaks 1932.1 at 38.0s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.2s.
    {  21,  21,  0, 0,  40000,   220,0.000000 }, // 21: 3drm67M20; BisqM20; GM20; HMIGM20; b13M20; b65MM20; b7M20; f34GM20; fat2M20; sGM20; REEDORGN

    // Amplitude begins at    0.0, peaks 2568.0 at 0.1s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.0s.
    {  22,  22,  0, 0,  40000,     6,0.000000 }, // 22: 3drm67M21; GM21; HMIGM21; b13M21; b7M21; f34GM21; fat2M21; sGM21; Accordion; am021; am021.in

    // Amplitude begins at    0.8, peaks 1842.8 at 30.6s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.1s.
    {  23,  23,  0, 0,  40000,    66,0.000000 }, // 23: 3drm67M22; GM22; HMIGM22; b13M22; b7M22; f34GM22; fat2M22; sGM22; Harmonica; am022; am022.in

    // Amplitude begins at    0.0, peaks 1038.5 at 39.6s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.1s.
    {  24,  24,  0, 0,  40000,    73,0.000000 }, // 24: 3drm67M23; GM23; HMIGM23; b13M23; b7M23; f34GM23; fat2M23; sGM23; Tango Accordion; am023; am023.in

    // Amplitude begins at 2281.3, peaks 2659.2 at 0.0s,
    // fades to 20% at 1.0s, keyoff fades to 20% in 1.0s.
    {  25,  25,  0, 0,    993,   993,0.000000 }, // 25: 3drm67M24; GM24; HMIGM24; b13M24; b7M24; f34GM24; fat2M24; Acoustic Guitar1; am024; am024.in

    // Amplitude begins at 3021.2, peaks 3142.7 at 0.0s,
    // fades to 20% at 1.7s, keyoff fades to 20% in 1.7s.
    {  26,  26,  0, 0,   1693,  1693,0.000000 }, // 26: 3drm67M25; GM25; HMIGM25; b13M25; b7M25; f17GM25; f29GM60; f30GM60; f34GM25; fat2M25; mGM25; sGM25; Acoustic Guitar2; French Horn; am025; am025.in

    // Amplitude begins at 2413.7, peaks 3118.0 at 0.0s,
    // fades to 20% at 1.8s, keyoff fades to 20% in 1.8s.
    {  27,  27,  0, 0,   1780,  1780,0.000000 }, // 27: 3drm67M26; GM26; HMIGM26; b13M26; b7M26; b8M26; f17GM26; f34GM26; f35GM26; fat2M26; mGM26; sGM26; Electric Guitar1; am026; am026.in; jazzgtr

    // Amplitude begins at 2547.4,
    // fades to 20% at 1.1s, keyoff fades to 20% in 1.1s.
    {  28,  28,  0, 0,   1066,  1066,0.000000 }, // 28: GM27; b13M27; f30GM61; f34GM27; fat2M27; sGM27; Brass Section; Electric Guitar2; am027

    // Amplitude begins at 2653.1, peaks 2768.8 at 0.0s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.0s.
    {  29,  29,  0, 0,  40000,     0,0.000000 }, // 29: 3drm67M28; GM28; HMIGM28; b13M28; b6M107; b6M3; b6M99; b7M28; b8M20; b8M28; f17GM28; f34GM28; f35GM28; fat2M28; hamM3; hamM60; intM3; mGM28; rickM3; sGM28; BPerc; BPerc.in; Electric Guitar3; RBPerc; Rmutegit; am028; am028.in; muteguit

    // Amplitude begins at   71.0, peaks 1929.9 at 0.0s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.0s.
    {  30,  30,  0, 0,  40000,    13,0.000000 }, // 30: GM29; b13M29; f34GM29; fat2M29; sGM29; Overdrive Guitar; am029

    // Amplitude begins at  801.9, peaks 2954.2 at 0.0s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.0s.
    {  31,  31,  0, 0,  40000,    13,0.000000 }, // 31: 3drm67M30; GM30; HMIGM30; b13M30; b6M116; b6M6; b7M30; f17GM30; f34GM30; f35GM30; fat2M30; hamM6; intM6; mGM30; rickM6; sGM30; Distorton Guitar; GDist; GDist.in; RGDist; am030; am030.in

    // Amplitude begins at 2395.3, peaks 2449.6 at 0.0s,
    // fades to 20% at 4.2s, keyoff fades to 20% in 4.2s.
    {  32,  32,  0, 0,   4153,  4153,0.000000 }, // 32: 3drm67M31; GM31; HMIGM31; b13M31; b6M104; b6M5; b7M31; b8M120; f34GM31; fat2M31; hamM5; intM5; rickM5; sGM31; Feedbck; GFeedbck; Guitar Harmonics; RFeedbck; am031; am031.in

    // Amplitude begins at    7.0, peaks 2702.0 at 1.6s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.0s.
    {  33,  33,  0, 0,  40000,    33,0.000000 }, // 48: 3drm67M48; GM48; HMIGM48; b13M48; b7M48; f34GM48; fat2M48; String Ensemble1; am048; am048.in

    // Amplitude begins at 2705.3, peaks 2718.0 at 0.4s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.0s.
    {  34,  34,  0, 0,  40000,    20,0.000000 }, // 34: 3drm67M33; 3drm67M39; GM33; GM39; HMIGM33; HMIGM39; b13M33; b13M39; b7M33; b7M39; f15GM30; f17GM33; f17GM39; f26GM30; f29GM28; f29GM29; f30GM28; f30GM29; f34GM33; f34GM39; f35GM39; fat2M33; fat2M39; hamM68; mGM33; mGM39; sGM33; sGM39; Distorton Guitar; Electric Bass 1; Electric Guitar3; Overdrive Guitar; Synth Bass 2; am033; am033.in; am039; am039.in; synbass2

    // Amplitude begins at    0.8, peaks 2846.8 at 0.1s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.0s.
    {  35,  36,  0, 0,  40000,     6,0.000000 }, // 35: fat4M21; Accordion

    // Amplitude begins at    2.4, peaks 2270.8 at 4.3s,
    // fades to 20% at 40.0s, keyoff fades to 20% in 0.0s.
    {  37,  38,  0, 1,  40000,     0,-0.125000 }, //36: dMM48; hxMM48; musM48; raptM48; String Ensemble 1

};


static const unsigned short Operators[23*2] =
    {0x000,0x003,0x001,0x004,0x002,0x005, // operators  0, 3,  1, 4,  2, 5
     0x008,0x00B,0x009,0x00C,0x00A,0x00D, // operators  6, 9,  7,10,  8,11
     0x010,0x013,0x011,0x014,0x012,0x015, // operators 12,15, 13,16, 14,17
     0x100,0x103,0x101,0x104,0x102,0x105, // operators 18,21, 19,22, 20,23
     0x108,0x10B,0x109,0x10C,0x10A,0x10D, // operators 24,27, 25,28, 26,29
     0x110,0x113,0x111,0x114,0x112,0x115, // operators 30,33, 31,34, 32,35
     0x010,0x013,   // operators 12,15
     0x014,0xFFF,   // operator 16
     0x012,0xFFF,   // operator 14
     0x015,0xFFF,   // operator 17
     0x011,0xFFF }; // operator 13

static const unsigned short Channels[23] =
    {0x000,0x001,0x002, 0x003,0x004,0x005, 0x006,0x007,0x008, // 0..8
     0x100,0x101,0x102, 0x103,0x104,0x105, 0x106,0x107,0x108, // 9..17 (secondary set)
     0x006,0x007,0x008,0xFFF,0xFFF }; // <- hw percussions, 0xFFF = no support for pitch/pan

unsigned char regBD = 0;
//unsigned char pit = 0;

const unsigned NumChannels = 23;

char four_op_category[NumChannels];
                            // 1 = quad-master, 2 = quad-slave, 0 = regular
                            // 3 = percussion BassDrum
                            // 4 = percussion Snare
                            // 5 = percussion Tom
                            // 6 = percussion Crash cymbal
                            // 7 = percussion Hihat
                            // 8 = percussion slave

//unsigned short ins[NumChannels];
unsigned short g_ins[NumChannels]; // index to adl[], cached, needed by Touch()
unsigned char  pit[NumChannels];  // value poked to B0, cached, needed by NoteOff)(

Generator::Generator(int sampleRate,
                     QObject *parent)
    :   QIODevice(parent)
{
    note = 57;
    metainstr = 36;
    m_buffer.fill(0, 512);
    //g_ins.resize(NumChannels,     17);
    memset(g_ins, 17, sizeof(unsigned short)*NumChannels);
    //pit.resize(NumChannels,      0);
    memset(pit, 0, sizeof(unsigned char)*NumChannels);
    memset(four_op_category, 0, NumChannels);
    unsigned p=0;
    for(unsigned b=0; b<18; ++b) four_op_category[p++] = 0;
    for(unsigned b=0; b< 5; ++b) four_op_category[p++] = 8;

    static const short data[] =
    { 0x004,96, 0x004,128,        // Pulse timer
      0x105, 0, 0x105,1, 0x105,0, // Pulse OPL3 enable
      0x001,32, 0x105,1           // Enable wave, OPL3 extensions
    };

    chip.Init(sampleRate);

    unsigned char HighTremoloMode   = 0;
    unsigned char HighVibratoMode   = 0;
    unsigned char AdlPercussionMode = 0;
    unsigned fours = 7;

    for(unsigned a=0; a< 18; ++a) chip.WriteReg(0xB0+Channels[a], 0x00);
    for(unsigned a=0; a< sizeof(data)/sizeof(*data); a+=2)
        chip.WriteReg(data[a], data[a+1]);
    chip.WriteReg(0x0BD, regBD = (HighTremoloMode*0x80
                                + HighVibratoMode*0x40
                                + AdlPercussionMode*0x20) );
    unsigned fours_this_card = std::min(fours, 6u);
    chip.WriteReg(0x104, (1 << fours_this_card) - 1);

    unsigned nextfour = 0;
    for(unsigned a=0; a<fours; ++a)
    {
        four_op_category[nextfour  ] = 1;
        four_op_category[nextfour+3] = 2;
        switch(a % 6)
        {
            case 0: case 1: nextfour += 1; break;
            case 2:         nextfour += 9-2; break;
            case 3: case 4: nextfour += 1; break;
            case 5:         nextfour += 23-9-2; break;
        }
    }

    //Shutup!
    for(unsigned c=0; c<NumChannels; ++c) { NoteOff(c); Touch_Real(c, 0); }

}

void Generator::NoteOff(unsigned c)
{
    unsigned cc = c%23;
    if(cc >= 18)
    {
        regBD &= ~(0x10 >> (cc-18));
        chip.WriteReg(0xBD, regBD);
        return;
    }
    chip.WriteReg(0xB0 + Channels[cc], pit[c] & 0xDF);
}

void Generator::NoteOn(unsigned c, double hertz) // Hertz range: 0..131071
{
    unsigned cc = c%23;
    unsigned x = 0x2000;
    if(hertz < 0 || hertz > 131071) // Avoid infinite loop
        return;
    while(hertz >= 1023.5) { hertz /= 2.0; x += 0x400; } // Calculate octave
    x += (int)(hertz + 0.5);
    unsigned chn = Channels[cc];
    if(cc >= 18)
    {
        regBD |= (0x10 >> (cc-18));
        chip.WriteReg(0x0BD, regBD);
        x &= ~0x2000;
        //x |= 0x800; // for test
    }
    if(chn != 0xFFF)
    {
        chip.WriteReg(0xA0 + chn, x & 0xFF);
        chip.WriteReg(0xB0 + chn, pit[c] = x >> 8);
    }
}

void Generator::Touch_Real(unsigned c, unsigned volume)
{
    if(volume > 63) volume = 63;
    unsigned /*card = c/23,*/ cc = c%23;
    unsigned i = g_ins[c], o1 = Operators[cc*2], o2 = Operators[cc*2+1];
    unsigned x = adl[i].modulator_40, y = adl[i].carrier_40;
    bool do_modulator;
    bool do_carrier;

    unsigned mode = 1; // 2-op AM
    if(four_op_category[c] == 0 || four_op_category[c] == 3)
    {
        mode = adl[i].feedconn & 1; // 2-op FM or 2-op AM
    }
    else if(four_op_category[c] == 1 || four_op_category[c] == 2)
    {
        unsigned i0, i1;
        if ( four_op_category[c] == 1 )
        {
            i0 = i;
            i1 = g_ins[c + 3];
            mode = 2; // 4-op xx-xx ops 1&2
        }
        else
        {
            i0 = g_ins[c - 3];
            i1 = i;
            mode = 6; // 4-op xx-xx ops 3&4
        }
        mode += (adl[i0].feedconn & 1) + (adl[i1].feedconn & 1) * 2;
    }
    static const bool do_ops[10][2] =
      { { false, true },  /* 2 op FM */
        { true,  true },  /* 2 op AM */
        { false, false }, /* 4 op FM-FM ops 1&2 */
        { true,  false }, /* 4 op AM-FM ops 1&2 */
        { false, true  }, /* 4 op FM-AM ops 1&2 */
        { true,  false }, /* 4 op AM-AM ops 1&2 */
        { false, true  }, /* 4 op FM-FM ops 3&4 */
        { false, true  }, /* 4 op AM-FM ops 3&4 */
        { false, true  }, /* 4 op FM-AM ops 3&4 */
        { true,  true  }  /* 4 op AM-AM ops 3&4 */
      };

    do_modulator = do_ops[ mode ][ 0 ];
    do_carrier   = do_ops[ mode ][ 1 ];

    chip.WriteReg(0x40+o1, do_modulator ? (x|63) - volume + volume*(x&63)/63 : x);
    if(o2 != 0xFFF)
    chip.WriteReg(0x40+o2, do_carrier   ? (y|63) - volume + volume*(y&63)/63 : y);
    // Correct formula (ST3, AdPlug):
    //   63-((63-(instrvol))/63)*chanvol
    // Reduces to (tested identical):
    //   63 - chanvol + chanvol*instrvol/63
    // Also (slower, floats):
    //   63 + chanvol * (instrvol / 63.0 - 1)
}

void Generator::Touch(unsigned c, unsigned volume) // Volume maxes at 127*127*127
{
    // The formula below: SOLVE(V=127^3 * 2^( (A-63.49999) / 8), A)
    Touch_Real(c, volume>8725  ? std::log(volume)*11.541561 + (0.5 - 104.22845) : 0);
    // The incorrect formula below: SOLVE(V=127^3 * (2^(A/63)-1), A)
    //Touch_Real(c, volume>11210 ? 91.61112 * std::log(4.8819E-7*volume + 1.0)+0.5 : 0);
}

void Generator::Patch(unsigned c, unsigned i)
{
    unsigned cc = c%23;
    static const unsigned char data[4] = {0x20,0x60,0x80,0xE0};
    g_ins[c] = i;
    unsigned o1 = Operators[cc*2+0], o2 = Operators[cc*2+1];
    unsigned x = adl[i].modulator_E862, y = adl[i].carrier_E862;
    for(unsigned a=0; a<4; ++a)
    {
        chip.WriteReg(data[a]+o1, x&0xFF); x>>=8;
        if(o2 != 0xFFF)
        chip.WriteReg(data[a]+o2, y&0xFF); y>>=8;
    }
}

void Generator::Pan(unsigned c, unsigned value)
{
    unsigned cc = c%23;
    if(Channels[cc] != 0xFFF)
        chip.WriteReg(0xC0 + Channels[cc], adl[g_ins[c]].feedconn | value);
}

void Generator::PlayNoteF(int noteID, int patch, int chan2op1, int chan2op2, int chan4op1, int chan4op2)
{
    int meta = patch;
    int tone = noteID;
    if(adlins[meta].tone)
    {
        if(adlins[meta].tone < 20)
            tone += adlins[meta].tone;
        else if(adlins[meta].tone < 128)
            tone = adlins[meta].tone;
        else
            tone -= adlins[meta].tone-128;
    }
    int i[2] = { adlins[meta].adlno1, adlins[meta].adlno2 };
    bool pseudo_4op = adlins[meta].flags & adlinsdata::Flag_Pseudo4op;

    int  adlchannel[2] = { chan4op1, chan4op2 };
    if(i[0] == i[1] || pseudo_4op)
    {
        adlchannel[0] = chan2op1;
        adlchannel[1] = chan2op2;
    }

    g_ins[adlchannel[0]] = i[0];
    g_ins[adlchannel[1]] = i[1];

    //NoteOff(adlchannel[0]);
    //NoteOff(adlchannel[1]);

    //0x0014F671,0x0007F131, 0x92,0x00, 0x2, +0
    double bend = 0.0;
    double phase = 0.0;

    Patch(adlchannel[0], i[0]);
    Patch(adlchannel[1], i[1]);

    Pan(adlchannel[0], 0x30);
    Pan(adlchannel[1], 0x30);

    Touch_Real(adlchannel[0], 63);
    Touch_Real(adlchannel[1], 63);

    bend  = 0.0 + adl[i[0]].finetune;
    NoteOn(adlchannel[0], 172.00093 * std::exp(0.057762265 * (tone + bend + phase)));

    if( pseudo_4op )
    {
        bend  = 0.0 + adl[i[1]].finetune + adlins[meta].fine_tune;
        NoteOn(adlchannel[1], 172.00093 * std::exp(0.057762265 * (tone + bend + phase)));
    }
}

void Generator::MuteNote()
{
    for(unsigned c=0; c<NumChannels; ++c) { NoteOff(c); }
}

void Generator::PlayNote()
{
    PlayNoteF(note,   metainstr, 7,  6,    1,  4);
}

void Generator::PlayMajorChord()
{
    PlayNoteF(note,   metainstr, 7,  6,    1,  4);
    PlayNoteF(note+4, metainstr, 15, 8,   2,  5);
    PlayNoteF(note-5, metainstr, 17, 16,  9,  12);
}

void Generator::PlayMinorChord()
{
    PlayNoteF(note,   metainstr, 7,  6,    1,  4);
    PlayNoteF(note+3, metainstr, 15, 8,   2,  5);
    PlayNoteF(note-5, metainstr, 17, 16,  9,  12);
}

void Generator::changePatch(int patch)
{
    for(unsigned c=0; c<NumChannels; ++c) { NoteOff(c); }
    metainstr = patch;
}

void Generator::changeNote(int newnote)
{
    note = newnote;
}

Generator::~Generator()
{}

void Generator::start()
{
    open(QIODevice::ReadOnly);
    saySomething("Meow");
}

void Generator::stop()
{
    m_pos = 0;
    close();
}

qint64 Generator::readData(char *data, qint64 len)
{
    short* _out = (short*)data;
    qint64 total = 0, lenS = (len/4);
    int samples[4096];
    if(lenS > BufferSize/4)
        lenS = BufferSize/4;
    unsigned long lenL = 512;
    //unsigned long lenBytes = lenL*4;
    //FILE* shit = fopen("C:/_Repos/OPL3BankEditor/_AudioOutputQtExample/shit.raw", "ab");
    while( (total+512) < lenS )
    {
        chip.GenerateArr(samples, &lenL);
        short out;
        int offset;
        for(unsigned long p = 0; p < lenL; ++p)
        {
            for(unsigned w=0; w<2; ++w)
            {
                out    = samples[p*2+w];
                offset = total+p*2+w;
                _out[offset] = out;
                //fwrite(&out, 1, 2, shit);
            }
        }
        total += lenL;
    };

    saySomething(QString::number(total*4));
    return total*4;
}

qint64 Generator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

qint64 Generator::bytesAvailable() const
{
    return 2048;// + QIODevice::bytesAvailable();
}



//________________________Old_code________________________
//  adlchannel[ccount] = c;
//    for(unsigned ccount = 0; ccount < 2; ++ccount)
//    {
//        if(ccount == 1)
//        {
//            if(i[0] == i[1]) {/*twoChans=false;*/ break; } // No secondary channel
//            if(adlchannel[0] == -1) { /*twoChans=false;*/ break;} // No secondary if primary failed
//        }

//        int c = -1;
//        //long bs = -0x7FFFFFFFl;
//        for(int a = adlchannel[0]; a <= adlchannel[1]/*(int)NumChannels*/; a += 3)
//        {
//            if(ccount == 1 && a == adlchannel[0]) continue;
//            // ^ Don't use the same channel for primary&secondary

//            if(i[0] == i[1] || pseudo_4op)
//            {
//                // Only use regular channels
//                int expected_mode = 0;
//                if(four_op_category[a] != expected_mode)
//                    continue;
//            }
//            else
//            {
//                if(ccount == 0)
//                {
//                    // Only use four-op master channels
//                    if(four_op_category[a] != 1)
//                        continue;
//                }
//                else
//                {
//                    // The secondary must be played on a specific channel.
//                    if(a != adlchannel[0] + 3)
//                        continue;
//                }
//            }
//            //long s = CalculateAdlChannelGoodness(a, i[ccount], MidCh);
//            //if(s > bs) { bs=s; c = a; } // Best candidate wins
//            c = a;
//        }

//        if(c < 0)
//        {
//            //UI.PrintLn("ignored unplaceable note");
//            continue; // Could not play this note. Ignore it.
//        }
//        //PrepareAdlChannelForNewNote(c, i[ccount]);
//        g_ins[c] = i[ccount];
//        adlchannel[ccount] = c;
//    }
//    if(adlchannel[0] < 0 && adlchannel[1] < 0)
//    {
//        saySomething("crap");
//        // The note could not be played, at all.
//        //return;
//    }
