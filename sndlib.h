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
    SND_CREATE_DEVICE_MANUAL_SELECT = 0,            // manual setup
    SND_CREATE_DEVICE_AUTO_DETECT   = 1,            // autodetect and select best available device
    SND_CREATE_DEVICE_SPECIFIC      = 0x1000,       // select specific device
    SND_CREATE_DEVICE_MASK          = 0x1FFF,       // 
    
    SND_CREATE_SKIP_NONDMA_DEVICES  = (1 << 24),    // skip non-DMA devices
    SND_CREATE_SKIP_DETECTION       = (1 << 25),    // skip device detection

    /// ------------------------------------------
    // device driver IDs
    // non-DMA devices...
    SND_CREATE_DEVICE_NONDMA_FIRST  = 0x1000,
    SND_CREATE_DEVICE_PC_SPEAKER    = SND_CREATE_DEVICE_NONDMA_FIRST,
    SND_CREATE_DEVICE_COVOX,
    SND_CREATE_DEVICE_DUAL_COVOX,
    SND_CREATE_DEVICE_STEREO_ON_1,
    SND_CREATE_DEVICE_NONDMA_LAST,

    // ISA DMA devices...
    SND_CREATE_DEVICE_ISA_DMA_FIRST = 0x1100,
    SND_CREATE_DEVICE_SB            = SND_CREATE_DEVICE_ISA_DMA_FIRST,
    SND_CREATE_DEVICE_SB16,
    SND_CREATE_DEVICE_WSS,
    SND_CREATE_DEVICE_ESS,
    SND_CREATE_DEVICE_PAS,
    SND_CREATE_DEVICE_ISA_DMA_LAST,

    // PCI/PCIe bus master devices...
    SND_CREATE_DEVICE_PCI_FIRST     = 0x1200,
    SND_CREATE_DEVICE_HDA           = SND_CREATE_DEVICE_PCI_FIRST,
    SND_CREATE_DEVICE_PCI_LAST,
};

// devices include
#include "devsb.h"
#include "devwss.h"
#include "devpas.h"
#include "devhonk.h"
#include "devhda.h"