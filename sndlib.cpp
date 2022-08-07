#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#include "sndlib.h"
#include "snddev.h"
#include "sndfmt.h"
#include "convert.h"
#include "snderror.h"

// init sound library internal resources
uint32_t sndlibInit() {
    // init PM ISR stack
    snddev_pm_stack_top = &snddev_pm_stack_top;

    // init snddev_pm_old_stack so it doesn't dangle in the void
    snddev_pm_old_stack = NULL;

    // clear in use flag
    snddev_pm_stack_in_use = 0;

    // lock memory for stack/RMCB data
    dpmi_lockmemory(&snddev_bss_lock_start, (&snddev_bss_lock_end - &snddev_bss_lock_start));

    // clear active device storage
    memset(snd_activeDevice, 0, sizeof(snd_activeDevice));

    return SND_ERR_OK;   
}

// close sound library and cleanup
uint32_t sndlibDone() {
    // unlock memory for stack/RMCB data
    dpmi_lockmemory(&snddev_bss_lock_start, (&snddev_bss_lock_end - &snddev_bss_lock_start));

    return SND_ERR_OK;   
}

// this is SLOW (if not done via lookup), but it can't be helped right now
SoundDevice* sndlibCreateSpecificDevice(uint32_t id) {
    switch(id) {
        // non-DMA devices...
        case SND_CREATE_DEVICE_PC_SPEAKER:  return new sndPcSpeaker();
        case SND_CREATE_DEVICE_COVOX     :  return new sndCovox();
        case SND_CREATE_DEVICE_DUAL_COVOX:  return new sndDualCovox();
        case SND_CREATE_DEVICE_STEREO_ON_1: return new sndStereoOn1();

        // ISA DMA devices...
        case SND_CREATE_DEVICE_SB:          return new sndSoundBlaster2Pro();
        case SND_CREATE_DEVICE_SB16:        return new sndSoundBlaster16();
        case SND_CREATE_DEVICE_WSS:         return new sndWindowsSoundSystem();
        case SND_CREATE_DEVICE_ESS:         return new sndESSAudioDrive();
        case SND_CREATE_DEVICE_PAS:         return new sndProAudioSpectrum();

        // PCI/PCIe bus master devices...
        case SND_CREATE_DEVICE_HDA:         return new sndHDAudio();
        case SND_CREATE_DEVICE_DS1:         return new sndYamahaDS1();

        default:  return NULL;
    }
}

#define arrayof(a) (sizeof(a) / sizeof(a[0]))

const uint32_t SND_TOTAL_DEVICES = 
    (SND_CREATE_DEVICE_NONDMA_LAST - SND_CREATE_DEVICE_NONDMA_FIRST) + 
    (SND_CREATE_DEVICE_ISA_DMA_LAST - SND_CREATE_DEVICE_ISA_DMA_FIRST) +
    (SND_CREATE_DEVICE_PCI_LAST - SND_CREATE_DEVICE_PCI_FIRST);

// detect, create device object and other stuff, y'know
uint32_t sndlibCreateDevice(SoundDevice **device, uint32_t flags) {
    if (device == NULL) return SND_ERR_NULLPTR;
    
    // return status
    uint32_t rtn = SND_ERR_OK;

    // device flags
    uint32_t deviceFlags = flags & SND_CREATE_DEVICE_SPECIFIC;

    if (deviceFlags >= SND_CREATE_DEVICE_SPECIFIC) {
        // request specific device
        SoundDevice* dev = sndlibCreateSpecificDevice(deviceFlags);
        if (dev == NULL) return SND_ERR_MEMALLOC;

        // detect if requested
        if (((flags & SND_CREATE_SKIP_DETECTION) != 0) && ((rtn = dev->detect()) != SND_ERR_OK)) return rtn;

        // pass pointer to device and return success
        *device = dev;
        return SND_ERR_OK;
    }

    // else we have to query each driver for supported devices
    uint32_t selection = ((flags & SND_CREATE_DEVICE_MASK) == SND_CREATE_DEVICE_AUTO_DETECT) ? 0 : -1;

    // probe devices array
    SoundDevice * probeDevices[SND_TOTAL_DEVICES];
    uint32_t probeDeviceIdx = 0;

    // device priority list
    static const uint32_t priorityList[] = {
        SND_CREATE_DEVICE_HDA,
        SND_CREATE_DEVICE_DS1,

        SND_CREATE_DEVICE_SB16,
        SND_CREATE_DEVICE_WSS,
        SND_CREATE_DEVICE_ESS,
        SND_CREATE_DEVICE_PAS,
        SND_CREATE_DEVICE_SB,

        SND_CREATE_DEVICE_STEREO_ON_1,
        SND_CREATE_DEVICE_DUAL_COVOX,
        SND_CREATE_DEVICE_COVOX,
        SND_CREATE_DEVICE_PC_SPEAKER,
    };

    // probe each device
    for (uint32_t i = 0; i < arrayof(priorityList); i++) {
        uint32_t id = priorityList[i];

        // filter out non-DMA devices if requested
        if ((flags & SND_CREATE_SKIP_NONDMA_DEVICES) &&
            (id <  SND_CREATE_DEVICE_NONDMA_LAST) && 
            (id >= SND_CREATE_DEVICE_NONDMA_FIRST)) continue;
        
        // request specific device
        probeDevices[probeDeviceIdx] = sndlibCreateSpecificDevice(id);

        // detect if requested
        if (((flags & SND_CREATE_SKIP_DETECTION) == 0) && (probeDevices[probeDeviceIdx]->detect() != SND_ERR_OK))
            delete probeDevices[probeDeviceIdx];
        else probeDeviceIdx++;
    }

    // no devices?
    if (probeDeviceIdx == 0) return SND_ERR_NOTFOUND;

    // show selector
    if ((flags & SND_CREATE_DEVICE_MASK) != SND_CREATE_DEVICE_AUTO_DETECT) {
        printf(" select available sound device: \n");
        for (size_t i = 0; i < probeDeviceIdx; i++) {
            printf(" %c - %s (%s)\n",
                (i >= 10 ? (i - 10 + 'A') : (i + '0')),
                probeDevices[i]->getDeviceInfo()->name,
                probeDevices[i]->getDeviceInfo()->version
            );
        }
        printf(" ------------------------------------------\n");
        printf(" press [0 - %c] to select device, [ESC] to exit... \n", (probeDeviceIdx >= 11 ? (probeDeviceIdx - 11 + 'A') : (probeDeviceIdx - 1 + '0')));

        // messy keyscan routine
        do {
            char ch = getch();
            if (ch == 0x1B) {
                // user exit
                rtn = SND_ERR_USEREXIT;
                break;
            }
            if (ch < '0') continue;
            if ((ch <= 'z') && (ch >= 'a')) ch -= 32;
            selection = (ch >= 'A' ? (ch + 10 - 'A') : (ch - '0'));
        } while (selection >= probeDeviceIdx);
    }

    // free unused devices
    for (uint32_t i = 0; i < probeDeviceIdx; i++) if (i != selection) delete probeDevices[i];

    // fill user pointer with current device
    if (selection < probeDeviceIdx) *device = probeDevices[selection];

    // done :)
    return rtn;
}

uint32_t sndlibDestroyDevice(SoundDevice *dev) {
    if (dev == NULL) return SND_ERR_NULLPTR;
    delete dev;
    return SND_ERR_OK;
}