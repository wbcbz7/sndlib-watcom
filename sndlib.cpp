#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#include "sndlib.h"
#include "snddefs.h"
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

// detect for SBEMU, returns ITN 2D multiplex or -1 if not found
static uint32_t sndlib_sbemu_detect() {
    uint8_t* amis_appstring;
    uint32_t ret_seg = 0, ret_ofs = 0;

    // scan all multiplexes of INT 2D
    for (int mx = 0; mx < 256; mx++) {
        _asm {
                mov     ah, byte ptr [mx]
                xor     al, al
                int     0x2d
                cmp     al, 0xFF        // is this a free multiplex?
                jz      _found
                xor     dx, dx          // it is, return NULL pointer
                xor     di, di
            _found:
                mov     word ptr [ret_seg], dx
                mov     word ptr [ret_ofs], di
        }

        // check for SBEMU application string
        amis_appstring = (uint8_t*)((ret_seg << 4) + ret_ofs);  // FIXME: assumes Watcom C zero-based flat model
        if ((amis_appstring != NULL) && (memcmp(amis_appstring+8, "SBEMU", 5) == 0)) return mx;
    }

    // TODO: implement
    return -1;
}

// this is SLOW (if not done via lookup), but it can't be helped right now
SoundDevice* sndlibCreateSpecificDevice(uint32_t id) {
    switch(id) {
        // non-DMA devices...
#ifdef SNDLIB_DEVICE_ENABLE_PC_SPEAKER
        case SND_CREATE_DEVICE_PC_SPEAKER:  return new sndPcSpeaker();
#endif
#ifdef SNDLIB_DEVICE_ENABLE_COVOX
        case SND_CREATE_DEVICE_COVOX     :  return new sndCovox();
#endif
#ifdef SNDLIB_DEVICE_ENABLE_DUAL_COVOX
        case SND_CREATE_DEVICE_DUAL_COVOX:  return new sndDualCovox();
#endif
#ifdef SNDLIB_DEVICE_ENABLE_STEREO_ON_1
        case SND_CREATE_DEVICE_STEREO_ON_1: return new sndStereoOn1();
#endif

        // ISA DMA devices...
#ifdef SNDLIB_DEVICE_ENABLE_SB
        case SND_CREATE_DEVICE_SB:          return new sndSoundBlaster();
#endif
#ifdef SNDLIB_DEVICE_ENABLE_SB16
        case SND_CREATE_DEVICE_SB16:        return new sndSoundBlaster16();
#endif
#ifdef SNDLIB_DEVICE_ENABLE_GUS
        case SND_CREATE_DEVICE_GUS:         return new sndGravisUltrasound();
#endif
#ifdef SNDLIB_DEVICE_ENABLE_WSS
        case SND_CREATE_DEVICE_WSS:         return new sndWindowsSoundSystem();
#endif
#ifdef SNDLIB_DEVICE_ENABLE_ESS
        case SND_CREATE_DEVICE_ESS:         return new sndESSAudioDrive();
#endif
#ifdef SNDLIB_DEVICE_ENABLE_PAS
        case SND_CREATE_DEVICE_PAS:         return new sndProAudioSpectrum();
#endif

        // PCI/PCIe bus master devices...
#ifdef SNDLIB_DEVICE_ENABLE_HDA
        case SND_CREATE_DEVICE_HDA:         return new sndHDAudio();
#endif
#ifdef SNDLIB_DEVICE_ENABLE_DS1
        case SND_CREATE_DEVICE_DS1:         return new sndYamahaDS1();
#endif

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

    // check for SBEMU running
    // in this case, do not probe PCI devices to avoid screwing things up
    uint32_t sbemu_multiplex = sndlib_sbemu_detect();
#ifdef DEBUG_LOG
    logdebug("SBEMU multiplex = %d\n", sbemu_multiplex);
#endif

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
        SND_CREATE_DEVICE_SB,
        SND_CREATE_DEVICE_GUS,
        SND_CREATE_DEVICE_PAS,

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

        // filter out PCI devices if SBEMU is running
        if ((sbemu_multiplex != -1) &&
            (id >= SND_CREATE_DEVICE_PCI_FIRST) &&
            (id <  SND_CREATE_DEVICE_PCI_LAST)) continue;

        // request specific device
        probeDevices[probeDeviceIdx] = sndlibCreateSpecificDevice(id);

        // check if such device class is available
        if (probeDevices[probeDeviceIdx] == NULL) continue;

        // detect if requested
        if (((flags & SND_CREATE_SKIP_DETECTION) == 0) && (probeDevices[probeDeviceIdx]->detect() != SND_ERR_OK))
            delete probeDevices[probeDeviceIdx];
        else probeDeviceIdx++;
    }

    // no devices?
    if (probeDeviceIdx == 0) return SND_ERR_NOTFOUND;

    // show selector
#ifdef SNDLIB_ENABLE_MANUAL_SETUP
    if ((flags & SND_CREATE_DEVICE_MASK) != SND_CREATE_DEVICE_AUTO_DETECT) {
        printf(" select available sound device: \n");
        for (size_t i = 0; i < probeDeviceIdx; i++) {
            printf(" %c - %-25s (%s)\n",
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
#endif

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