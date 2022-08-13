#pragma once

/*
    sndlib options setup include
    --wbcbz7 28.o6.2o22
*/

// ------------------------------
// main sndlib defines

// enable manual setup
#define SNDLIB_ENABLE_MANUAL_SETUP

// enable debug logging
#define SNDLIB_ENABLE_DEBUG_LOG

// ------------------------------
// sound device defines

// enable non-DMA devices
#define SNDLIB_DEVICE_ENABLE_NONDMA

// enable ISA DMA devices
#define SNDLIB_DEVICE_ENABLE_ISADMA

// enable PCI/PCIe bus master devices
#define SNDLIB_DEVICE_ENABLE_PCI

// individually enable devices
#define SNDLIB_DEVICE_ENABLE_PC_SPEAKER
#define SNDLIB_DEVICE_ENABLE_COVOX
#define SNDLIB_DEVICE_ENABLE_DUAL_COVOX
#define SNDLIB_DEVICE_ENABLE_STEREO_ON_1
#define SNDLIB_DEVICE_ENABLE_SB
#define SNDLIB_DEVICE_ENABLE_SB16
#define SNDLIB_DEVICE_ENABLE_WSS
#define SNDLIB_DEVICE_ENABLE_ESS
#define SNDLIB_DEVICE_ENABLE_PAS
#define SNDLIB_DEVICE_ENABLE_HDA
//#define SNDLIB_DEVICE_ENABLE_DS1      // disabled until i make the driver work as intended

// ------------------------------
// sound device custom properties

// WSS: extended ID probing (CS4232 or newer, informational only, as sndlib uses AD1848 subset only)
//#define SNDLIB_DEVICE_WSS_EXTENDED_ID_PROBING

// HDA: enable buffer position workaround (still buggy :D)
#define SNDLIB_DEVICE_HDA_BUFFER_POS_WORKAROUND

// ------------------------------
// sound format converter defines

// enable arbitrary format converters
// if not defined, source format must match one of device supported formats
#define SNDLIB_CONVERT_ENABLE_ARBITRARY

// enable 8/16bit to PC-Speaker format converters
// automatically disabled if PC Speaker device is not enabled
#define SNDLIB_CONVERT_ENABLE_PCSPEAKER


// -----------------------------
// autogenerated defines

#ifndef SNDLIB_DEVICE_ENABLE_NONDMA
    #undef SNDLIB_DEVICE_ENABLE_PC_SPEAKER
    #undef SNDLIB_DEVICE_ENABLE_COVOX
    #undef SNDLIB_DEVICE_ENABLE_DUAL_COVOX
    #undef SNDLIB_DEVICE_ENABLE_STEREO_ON_1
#endif
#ifndef SNDLIB_DEVICE_ENABLE_ISADMA
    #undef SNDLIB_DEVICE_ENABLE_SB
    #undef SNDLIB_DEVICE_ENABLE_SB16
    #undef SNDLIB_DEVICE_ENABLE_WSS
    #undef SNDLIB_DEVICE_ENABLE_ESS
    #undef SNDLIB_DEVICE_ENABLE_PAS
#endif
#ifndef SNDLIB_DEVICE_ENABLE_PCI
    #undef SNDLIB_DEVICE_ENABLE_HDA
    #undef SNDLIB_DEVICE_ENABLE_DS1
#endif

#ifndef SNDLIB_DEVICE_ENABLE_PC_SPEAKER
    #undef SNDLIB_CONVERT_ENABLE_PCSPEAKER
#endif
