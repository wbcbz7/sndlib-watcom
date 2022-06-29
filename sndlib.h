#pragma once

/*
    sndlib main include
    --wbcbz7 28.o6.2o22
*/

#include "convert.h"
#include "snderror.h"
#include "sndfmt.h"
#include "snddev.h"
#include "sndmisc.h"

// init sound library (called before any device using)
uint32_t sndlibInit();

// done sound library (called at exit)
uint32_t sndlibDone();

// create device object, optionally autodetect and select best device, request specific device driver, etc...
uint32_t sndlibCreateDevice(SoundDevice **dev, uint32_t flags = 0);

// destroy device
uint32_t sndlibDestroyDevice(SoundDevice *dev);

// sndlibCreateDevice() enums
enum {
    SND_CREATE_SHOW_DEVICE_SELECTOR = 0,            // manual setup
    SND_CREATE_AUTO_DETECT          = 0x1FFF,       // autoselect best device
    
    SND_CREATE_SKIP_NONDMA_DEVICES  = (1 << 24),    // skip non-DMA devices
    SND_CREATE_SKIP_DETECTION       = (1 << 25),    // skip device detection

    /// ------------------------------------------
    // device driver IDs
    // non-DMA devices...
    SND_CREATE_DEVICE_PC_SPEAKER    = 0x1000,
    SND_CREATE_DEVICE_COVOX         = 0x1001,
    SND_CREATE_DEVICE_DUAL_COVOX    = 0x1002,
    SND_CREATE_DEVICE_STEREO_ON_1   = 0x1003,

    // ISA DMA devices...
    SND_CREATE_DEVICE_SB            = 0x1100,
    SND_CREATE_DEVICE_SB16          = 0x1101,
    SND_CREATE_DEVICE_WSS           = 0x1102,
    SND_CREATE_DEVICE_ESS           = 0x1103,
    SND_CREATE_DEVICE_PAS           = 0x1104,

    // PCI/PCIe bus master devices...
    SND_CREATE_DEVICE_HDA           = 0x1205,
};

// devices include
#include "devsb.h"
#include "devwss.h"
#include "devpas.h"
#include "devhonk.h"
#include "devhda.h"