#include "snddefs.h"
#ifdef SNDLIB_DEVICE_ENABLE_WSS

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "convert.h"
#include "sndmisc.h"
#include "snderror.h"
#include "devwss.h"

//define to enable logging
//#define DEBUG_LOG
#include "logerror.h"

#define arrayof(x) (sizeof(x) / sizeof(x[0]))

const size_t probeDataLength = 64;
const size_t probeIrqLength = 4;

const uint32_t wssgusIoBase[] = { 0x220, 0x230, 0x240, 0x250, 0x260, 0x270, 0x280 };
const uint32_t wssIoBase[]    = { 0x530, 0xE80, 0xF40, 0x680 };     // "standard" ports only
const uint32_t wssIrq[]       = { 3, 4, 5, 7, 9, 10, 11, 12 };
const uint32_t wssDma[]       = { 0, 1, 3, 5, 6, 7 };               // checkme!

const uint32_t wssRates[]     = { 8000, 5513, 16000, 11025, 27429, 18900, 32000, 22050, 0,     37800, 0,     44100, 48000, 33075, 9600, 6615 };
const uint32_t wssRates64k[]  = { 8000, 5513, 16000, 11025, 27429, 18900, 32000, 22050, 54857, 37800, 64000, 44100, 48000, 33075, 9600, 6615 }; // if 64khz supported
const uint32_t wssRatesVar[]  = { 4000, 48000 };

// Windows Sound System resources
const soundResourceInfo wssRes[] = {
    {
        SND_RES_IOBASE,
        arrayof(wssIoBase),
        wssIoBase
    },
    {
        SND_RES_IRQ,
        arrayof(wssIrq),
        wssIrq
    },
    {
        SND_RES_DMA,
        arrayof(wssDma),
        wssDma
    },
};

// GUS Max/PnP resources 
const soundResourceInfo wssgusRes[] = {
    {
        SND_RES_IOBASE,
        arrayof(wssgusIoBase),
        wssgusIoBase
    },
   {
        SND_RES_IRQ,
        arrayof(wssIrq),
        wssIrq
    },
    {
        SND_RES_DMA,
        arrayof(wssDma),
        wssDma
    },
};

// WSS sound caps
const soundFormatCapability wssCaps[] = {
    {
        (SND_FMT_INT8  | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_UNSIGNED),
        arrayof(wssRates), // fixed range
        wssRates,
    },
    {
        (SND_FMT_INT16 | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_SIGNED),
        arrayof(wssRates), // fixed range
        wssRates,
    },
};

// WSS sound caps w/ 64khz support
const soundFormatCapability wss64Caps[] = {
    {
        (SND_FMT_INT8 | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_UNSIGNED),
        arrayof(wssRates64k), // fixed range
        wssRates64k,
    },
    {
        (SND_FMT_INT16 | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_SIGNED),
        arrayof(wssRates64k), // fixed range
        wssRates64k,
    },
};

// WSS sound caps w/ variable rate
const soundFormatCapability wssVarCaps[] = {
    {
        (SND_FMT_INT8 | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_UNSIGNED),
        -2, // variable range
        wssRatesVar,
    },
    {
        (SND_FMT_INT16 | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_SIGNED),
        -2, // variable range
        wssRatesVar,
    },
};

// ---------------------------------------------
// WSS helper routines

#if 0

void cdecl wssRegWrite(uint32_t base, uint8_t reg, uint8_t data) {
    outp(base, reg); outp(base + 1, data);
#ifdef DEBUG_LOG
    logdebug("wss write Rx%02X = %02X\n", reg, data);
#endif
}

uint8_t cdecl wssRegRead(uint32_t base, uint8_t reg) {
    outp(base, reg); uint8_t data = inp(base + 1);
#ifdef DEBUG_LOG
    logdebug("wss read Rx%02X = %02X\n", reg, data);
#endif
    return data;
}

uint8_t cdecl wssStatus(uint32_t base) {
    uint8_t data = inp(base + 2);
#ifdef DEBUG_LOG
    logdebug("wss status =  %02X\n", data);
#endif
    return data;
}

#else
// TODO: do WSS codecs work fine with OUT DX, AX? technically they should, as data bus width is 8 bit and
//       ISA host controller would split word transfer by two adjacent byte transfers
inline void wssRegWrite(uint32_t base, uint8_t reg, uint8_t data);
#pragma aux wssRegWrite = "out dx, ax" parm [edx] [al] [ah]

inline uint8_t wssRegRead(uint32_t base, uint8_t reg);
#pragma aux wssRegRead = "out dx, al" "inc edx" "in al, dx" parm [edx] [al] value [al] modify [edx eax]

inline uint8_t wssStatus(uint32_t base);
#pragma aux wssStatus = "add edx, 2" "in al, dx" parm [edx] value [al] modify [edx eax]

#endif

uint8_t csExtRegRead(uint32_t base, uint8_t reg) {
    outp(base,   WSS_REG_EXT_INDEX);
    outp(base+1, ((reg & 0xF) << 4) | ((reg & 0x10) >> 2) | 0x8);
    uint8_t data = inp(base + 1);
    outp(base, 0);      // reset to data
#ifdef DEBUG_LOG
    logwrite("CS432X ext read Rx%02X = %02X\n", data);
#endif
    return data;
}

void csExtRegWrite(uint32_t base, uint8_t reg, uint8_t data) {
    outp(base, WSS_REG_EXT_INDEX);
    outp(base + 1, ((reg & 0xF) << 4) | ((reg & 0x10) >> 2) | 0x8);
    outp(base + 1, data);
    outp(base, 0);      // reset to data
#ifdef DEBUG_LOG
    logwrite("CS432X ext write Rx%02X = %02X\n", data);
#endif
}

// ----------------------------------------------

sndWindowsSoundSystem::sndWindowsSoundSystem() : IsaDmaDevice("Windows Sound System") {
    // fill with defaults
    isGus = is64khz = isVariableSampleRate = isPaused = false;
    featureLevel = WSS_FEATURE_AD1848;
    oldId = newId = extId = 0;

    devinfo.name = getName();
    devinfo.version = NULL;
    devinfo.maxBufferSize = 32768;  // BYTES
}

bool sndWindowsSoundSystem::kickstartProbingPlayback(SoundDevice::deviceInfo *info, uint32_t dmaChannel, ::dmaBlock &block, uint32_t probeLength, bool enableIrq) {
    // ack interrupt
    if (inp(info->iobase + 2) & 1) outp(info->iobase + 2, 0);

    // do single cycle, 8 bit mono transfer at relatively low sample rate (8khz for example)
    if (setFormat(info, 8000, SND_FMT_INT8 | SND_FMT_UNSIGNED | SND_FMT_MONO)) return 0;
    dmaSetup(dmaChannel, &block, probeLength, dmaModeSingle | dmaModeRead | dmaModeAutoInit);
    
    // load block length
    wssRegWrite(info->iobase, WSS_REG_DMA_COUNT_LOW,    probeLength & 0xFF);
    wssRegWrite(info->iobase, WSS_REG_DMA_COUNT_HIGH,   probeLength >> 8);
    
    // disable/enable interrupts
    wssRegWrite(info->iobase, WSS_REG_PIN_CTRL,        (wssRegRead(info->iobase, WSS_REG_PIN_CTRL)  & ~2) | (enableIrq ? 2 : 0));
    
    // enable playback
    wssRegWrite(info->iobase, WSS_REG_INTERFACE_CONFIG, wssRegRead(info->iobase, WSS_REG_INTERFACE_CONFIG)  | 1);

    return true;
}

bool sndWindowsSoundSystem::readEnvironment(SoundDevice::deviceInfo *info) {
    SoundDevice::deviceInfo gusinfo;
    char envstr[64] = { 0 };

    // check for ULTRA16 variable
    // ULTRA16=iobase,dma,irq,1,0

    char* blasterEnv = getenv("ULTRA16");
    if (blasterEnv != NULL) {
#ifdef DEBUG_LOG
        logdebug("ULTRA16 variable = %s\n", blasterEnv);
#endif

        // copy variable to temporary buffer
        strncpy(envstr, blasterEnv, sizeof(envstr));

        // tokenize
        char* p = strtok(envstr, ",");
        if (info->iobase == -1) info->iobase = strtol(p, NULL, 16); p = strtok(NULL, ",");
        if (info->dma == -1)    info->dma = atoi(p);   p = strtok(NULL, ","); //if (info->dma == 0) info->dma = -1;
        if (info->irq == -1)    info->irq = atoi(p);   p = strtok(NULL, ","); if (info->irq == 0) info->irq = -1;

        // check if iobase points to WSS config/version register
        if ((inp(info->iobase + 3) & 0x3F) == 0x4) info->iobase += 4;
    
    }

    // check if gus codec
    char *ultravar = getenv("ULTRASND");
    if (ultravar != NULL) {
#ifdef DEBUG_LOG
        logdebug("ULTRASND variable = %s\n", ultravar);
#endif
        strncpy(envstr, ultravar, sizeof(envstr));
        char* p = strtok(envstr, ",");
        gusinfo.iobase = strtol(p, NULL, 16); p = strtok(NULL, ",");
        gusinfo.dma    = atoi(p); p = strtok(NULL, ",");
        gusinfo.irq    = atoi(p); p = strtok(NULL, ",");
        gusinfo.dma2   = atoi(p); p = strtok(NULL, ","); if (info->dma2 == 0) info->dma2 = info->dma;
        gusinfo.irq2   = atoi(p); p = strtok(NULL, ","); if (info->irq2 == 0) info->dma2 = info->irq;

        // test against GUS Max/PnP
        if ((info->iobase - 0x10C == gusinfo.iobase) || (inp(gusinfo.iobase + 0x106) == 0xB)) isGus = true;

        // get missing irq/dma fields
        if (isGus) {
            if (info->dma == -1) {
                if (gusinfo.dma2 != 0) info->dma = gusinfo.dma2; else if (gusinfo.dma != 0) info->dma = gusinfo.dma; else {
                    // read from gus reg
                }
            }
            if (info->irq == -1) {
                if (gusinfo.irq2 != 0) info->irq = gusinfo.irq2; else if (gusinfo.irq != 0) info->irq = gusinfo.irq;
            }
        }
    }

    return true;
}

bool sndWindowsSoundSystem::wssDetect(SoundDevice::deviceInfo* info, bool manualDetect)
{
#ifdef DEBUG_LOG
    fprintf(stderr, __func__": start detect...\n");
#endif

    if (info == NULL) return 0;
    
    // first, try to check environment variables
    readEnvironment(info);

    // check if we have enough info for init
    // if not, probe io ports for codec 

    // probing notes - afaik both AD and Crystal codecs have xxxx1010 in R12, but we'll accept any value other than xxxx1111 until further confirmation
    if (info->iobase == -1) if (manualDetect == false) return false; else {
        bool isFound = false;
        // probe WSS ports
        for (const uint32_t* port = wssIoBase; port < (wssIoBase + arrayof(wssIoBase)); port++) {
            if (((inp(info->iobase + 3) & 0x3F) == 0x4) || ((wssRegRead(*port + 0x4, WSS_REG_MODE_ID) & 0xF) == 0xA)) {
                // found!
                isFound = true; isGus = false;
                info->iobase = *port + 0x4;     // fixup
                break;
            }
        }
        // ...then probe GUS ports if not found
        if (isFound == false) for (const uint32_t* port = wssgusIoBase; port < (wssgusIoBase + arrayof(wssgusIoBase)); port++) {
            if ((inp(*port + 0x106) == 0xB) || ((wssRegRead(*port + 0x106, WSS_REG_MODE_ID) & 0xF) == 0xA)) {
            // found!
            isFound = true; isGus = true;
            info->iobase = *port + 0x10C;     // fixup
            break;
            }
        }
    }

#ifdef DEBUG_LOG
    logdebug("io = 0x%X, irq = %d, dma = %d, isGus = %d\n", info->iobase, info->irq, info->dma, isGus);
#endif

    // if it's STILL not found....
    if (info->iobase == -1) return false;

#ifdef DEBUG_LOG
    logdebug("codec reset...\n");
#endif
    // reset codec
    if (wssReset(info, isGus) == false) return false;
#ifdef DEBUG_LOG
    logdebug("ok\n");
#endif

    // init format caps for regular WSS (48khz max)
    info->caps = wssCaps;
    info->capsLen = arrayof(wssCaps);
    info->flags = 0;

    // probe DMA
    ::dmaBlock testblk;
    if (dmaAlloc(probeDataLength, &testblk) == false) return false;
    memset(testblk.ptr, 0x80, probeDataLength);

    // check if DMA channel has been found
    if (info->dma == -1) if (manualDetect == false) { dmaFree(&testblk); return false; } else {
        for (const uint32_t* dma = wssDma; dma < (wssDma + arrayof(wssDma)); dma++) {
            
#ifdef DEBUG_LOG
            logdebug("probe dma %d...\n", *dma);
#endif

            // start WSS playback
            kickstartProbingPlayback(info, *dma, testblk, probeDataLength, false);

            // wait until DMA current length counter changes
            uint32_t timeout = 0x1000;
            volatile uint32_t dmapos = dmaGetCurrentCount(*dma, false);
            while (--timeout) {
                inp(0x80);
                if (dmapos != dmaGetCurrentCount(*dma, false)) {
                    info->dma = *dma;
#ifdef DEBUG_LOG
                    logdebug("found\n");
#endif
                    break;
                };
            };
#ifdef DEBUG_LOG
            if (timeout == 0) logdebug("timeout!\n");
#endif

            // stop playback
            wssRegWrite(info->iobase, WSS_REG_INTERFACE_CONFIG, wssRegRead(info->iobase, WSS_REG_INTERFACE_CONFIG) & ~1);
            dmaStop(*dma);

            // ack interrupt
            if (inp(info->iobase + 2) & 1) outp(info->iobase + 2, 0);
            
            if (info->dma != -1) break;
        }
    }
    if (info->dma == -1) { dmaFree(&testblk); return false; }


#ifdef DEBUG_LOG
    logdebug("wss status = %02X\n", inp(info->iobase + 2));
#endif

    // acknowledge interrupt
    if (inp(info->iobase + 2) & 1) outp(info->iobase + 2, 0);

    // check if IRQ has been found
    if (info->irq == -1) if (manualDetect == false) return false; else {
        // probe IRQ      
        irqEntry irqstuff = { 0 };
        irqstuff.handler = &wssDetectIrqProc;
        snd_IrqDetectInfo.iobase = info->iobase;
        snd_IrqDetectInfo.irq    = &irqstuff;

        for (const uint32_t* irq = wssIrq; irq < (wssIrq + arrayof(wssIrq)); irq++) {

#ifdef DEBUG_LOG
            logdebug("probe irq %d...\n", *irq);
#endif
            // hook
            if (irqHook(*irq, &irqstuff, true)) continue;

            // reset irq found flag
            snd_IrqDetectInfo.found = false;

            // setup codec for playback
            kickstartProbingPlayback(info, info->dma, testblk, probeIrqLength, true);

            uint32_t timeout = 0x1000;
            while (--timeout) {
                inp(0x80);

                if (snd_IrqDetectInfo.found) {
#ifdef DEBUG_LOG
                    logdebug("found\n");
#endif
                    info->irq = *irq;
                    break;
                }
            };

            // stop playback
            wssRegWrite(info->iobase, WSS_REG_INTERFACE_CONFIG, wssRegRead(info->iobase, WSS_REG_INTERFACE_CONFIG) & ~1);         
            dmaStop(info->dma);

            irqUnhook(&irqstuff, true);

            // acknowledge interrupt if not found
            if (snd_IrqDetectInfo.found) break; else if (inp(info->iobase + 2) & 1) outp(info->iobase + 2, 0);   
        }
    }

    // free DMA block
    dmaFree(&testblk);

#ifdef DEBUG_LOG
    logdebug("wss status = %02X\n", inp(info->iobase + 2));
#endif

    if (info->irq == -1) return false;

#ifdef DEBUG_LOG
    logdebug("io = 0x%X, irq = %d, dma = %d, highdma = %d\n", info->iobase, info->irq, info->dma, info->dma2);
#endif

    // fill caps
    if (fillCodecInfo(info) != SND_ERR_OK) return false;
    getCodecVersion(&devinfo);

    // detected! :)
    isDetected = true;
    return true;
}

bool sndWindowsSoundSystem::wssReset(SoundDevice::deviceInfo* info, bool isGus)
{
    if (isGus) {
        // enable codec and relocate to iobase
        outp(info->iobase - (0x30C - 0x306), 0x40 | ((info->dma != -1) && (info->dma >= 4) ? 0x20 : 0) | ((info->iobase >> 4) & 0xF));
    }

    // check for codec alrady in reset/recalibration
    uint32_t timeout = 32768;
    do if ((inp(info->iobase) & 0x80) == 0) break; while (--timeout); if (timeout == 0) return false;

    // reset codec to legacy (AD1848/CS4248) mode
    wssRegWrite(info->iobase, WSS_REG_MODE_ID, 0);
    if ((wssRegRead(info->iobase, WSS_REG_MODE_ID) & 0xF) == 0xF) return false;

    // well cubic player sources don't help much :) falling back to datasheet...
    // disable playback, set single DMA channel, set DMA playback transfers
    wssRegWrite(info->iobase, WSS_REG_INTERFACE_CONFIG | WSS_INDEX_MODE_CHANGE, 0x4);
    
    // wait for reset
    timeout = 32768;
    do if ((inp(info->iobase) & 0x80) == 0) break; while (--timeout); if (timeout == 0) return false;
    
    outp(info->iobase, WSS_REG_INTERFACE_CONFIG);

    // another "reset"
    wssRegWrite(info->iobase, WSS_REG_MODE_ID, 0);

    // mute outputs
    wssRegWrite(info->iobase, WSS_REG_LEFT_DAC_CTRL,    (wssRegRead(info->iobase, WSS_REG_LEFT_DAC_CTRL)    |  0x80));
    wssRegWrite(info->iobase, WSS_REG_RIGHT_DAC_CTRL,   (wssRegRead(info->iobase, WSS_REG_RIGHT_DAC_CTRL)   |  0x80));
    wssRegWrite(info->iobase, WSS_REG_DIGITAL_MIX_CTRL, (wssRegRead(info->iobase, WSS_REG_DIGITAL_MIX_CTRL) & ~0x1));

    return true;
}

uint32_t sndWindowsSoundSystem::fillCodecInfo(SoundDevice::deviceInfo* info)
{
    // fill info
    info->maxBufferSize = 32768;  // BYTES
    if (isGus) {
        info->caps = wssCaps;
        info->capsLen = arrayof(wssCaps);
        info->name = "Gravis Ultrasound MAX/PnP";
        info->flags = 0;
    }
    else {
        info->caps = wssCaps;
        info->capsLen = arrayof(wssCaps);
        info->name = "Windows Sound System";
        info->flags = 0;
    }

    return SND_ERR_OK;
}

bool sndWindowsSoundSystem::getCodecVersion(SoundDevice::deviceInfo* info)
{
    // read old ID 
    oldId = wssRegRead(info->iobase, WSS_REG_MODE_ID);
    newId = 0;
    extId = 0;

    // read new ID
    if (oldId & 0x80) {
        wssRegWrite(info->iobase, WSS_REG_MODE_ID, wssRegRead(info->iobase, WSS_REG_MODE_ID) | 0x40);
        newId = wssRegRead(info->iobase, WSS_REG_EXTENDED_ID);
        wssRegWrite(info->iobase, WSS_REG_MODE_ID, wssRegRead(info->iobase, WSS_REG_MODE_ID) & ~0x40);
    }

    switch (oldId & 0x8F) {
    case 0xA:
        info->version = "AD1848K/CS4248 or compatible";
        featureLevel = WSS_FEATURE_AD1848;
        break;

    case 0x8A:
        // check new ID
        switch (newId & 7) {
        case 0:
            if ((newId & 0xE0) == 0xA0) {
                info->version = "CS4231A";
                featureLevel = WSS_FEATURE_CS4231;
            }
            else {
                // test for AMD Interwave

                // enable IW mode 3
                wssRegWrite(info->iobase, WSS_REG_MODE_ID, 0x6C);

                // try to change WSS_REG_EXTENDED_ID (it's Left Output Attenuation register on Interwave mode 3)
                uint32_t oldReg25 = wssRegRead(info->iobase, IW_REG_LEFT_OUT_ATTENUATION);
                // try 0x55
                wssRegWrite(info->iobase, IW_REG_LEFT_OUT_ATTENUATION, 0x55);
                if (wssRegRead(info->iobase, IW_REG_LEFT_OUT_ATTENUATION) == 0x55) {
                    // try 0xAA
                    wssRegWrite(info->iobase, IW_REG_LEFT_OUT_ATTENUATION, 0xAA);
                    if (wssRegRead(info->iobase, IW_REG_LEFT_OUT_ATTENUATION) == 0xAA) {
                        // Rx25 is indeed changeable, it's definitely AMD Interwave
                        // restore old Rx25 content and return to mode 0
                        wssRegWrite(info->iobase, IW_REG_LEFT_OUT_ATTENUATION, oldReg25);
                        wssRegWrite(info->iobase, WSS_REG_MODE_ID, 0);

                        info->version = "AMD Interwave";
                        featureLevel = WSS_FEATURE_INTERWAVE;
                        break;
                    }
                }

                // not an Interwave - it's CS4231 non-A or compatible
                info->version = "CS4231 or compatible";
                featureLevel = WSS_FEATURE_CS4231;

            }
            break;

        case 2:
            info->version = ((newId & 0xE0) == 0xA0 ? "CS4232" : "CS4232 pre-release");
            featureLevel = WSS_FEATURE_CS4231;
            break;

        case 3:
#ifdef SNDLIB_DEVICE_WSS_EXTENDED_ID_PROBING
        {
            // mnogonozhka ebanaya
            // cirrus engineers are really weird

            // enable mode 3
            wssRegWrite(info->iobase, WSS_REG_MODE_ID, wssRegRead(info->iobase, WSS_REG_MODE_ID) | 0x60);

            // X25 register - extended ID
            extId = csExtRegRead(info->iobase, 25);

            // disable mode 3
            wssRegWrite(info->iobase, WSS_REG_MODE_ID, wssRegRead(info->iobase, WSS_REG_MODE_ID) | ~0x60);

            char* chipname = NULL;

            switch (extId & 0xF) {
            case 0x8:
                chipname = "CS4237B";
                break;

            case 0xB:
                chipname = "CS4236B";
                break;

            case 0x1D:
                chipname = "CS4235";
                break;

            case 0x1E:
                chipname = "CS4239";
                break;

            case 0x9:
                chipname = "CS4238B";
                break;

            default:
                snprintf(info->privateBuf, info->privateBufSize, "unknown extended id 0x%X", extId);
                break;
            }

            if (chipname != NULL) snprintf(info->privateBuf, info->privateBufSize, "%s Rev. %c", chipname, "AAAAABCE"[extId >> 5]);

            info->version = info->privateBuf;
            featureLevel = WSS_FEATURE_CS4236;
            break;
        }
#else
        info->version = "CS4235/6/8/9";
        featureLevel = WSS_FEATURE_CS4231;
#endif
        break;
        default:
            snprintf(info->privateBuf, info->privateBufSize, "unknown new id 0x%X", newId);
            info->version = info->privateBuf;
            featureLevel = WSS_FEATURE_CS4231;
            break;
        }
        break;
    default:
        snprintf(info->privateBuf, info->privateBufSize, "unknown old id 0x%X", oldId);
        info->version = info->privateBuf;
        featureLevel = WSS_FEATURE_AD1848;
        break;
    }

    return true;
}

uint32_t sndWindowsSoundSystem::setFormat(SoundDevice::deviceInfo* info, uint32_t sampleRate, soundFormat format)
{
    if (isVariableSampleRate) {
        return SND_ERR_UNSUPPORTED;
    }
    else {
        // check format
        if (((format & SND_FMT_INT8)  && !(format & SND_FMT_UNSIGNED)) ||
            ((format & SND_FMT_INT16) && !(format & SND_FMT_SIGNED)) ||
            ((format & (SND_FMT_INT8 | SND_FMT_INT16)) == 0) || (sampleRate == 0) || (sampleRate == -1)) return SND_ERR_UNSUPPORTED;

        // build format
        uint32_t rawFormat = (format & SND_FMT_STEREO ? 0x10 : 0) | (format & SND_FMT_INT16 ? 0x40 : 0) | 0x00;

        // find matching sample rate
        size_t rateIdx = 0;
        for (rateIdx = 0; rateIdx < devinfo.caps->ratesLength; rateIdx++) {
            if (abs(sampleRate - devinfo.caps->rates[rateIdx]) < (devinfo.caps->rates[rateIdx] >> 8)) break;
        }
        // not found?
        if (rateIdx >= devinfo.caps->ratesLength) SND_ERR_UNSUPPORTED;

#ifdef DEBUG_LOG
        logdebug("wss format = %02X\n", rawFormat | rateIdx);
#endif

        // set new rate
        wssRegWrite(info->iobase, WSS_REG_CLK_DATA_FORMAT | WSS_INDEX_MODE_CHANGE, rawFormat | rateIdx);

        // timeout
        uint32_t timeout = 32768;
        do if ((inp(info->iobase) & 0x80) == 0) break; while (--timeout); if (timeout == 0) return SND_ERR_UNSUPPORTED;

        // reset MCE bit
        outp(info->iobase, 0);

        // another timeout (just in case)
        timeout = 32768; do inp(info->iobase); while (--timeout);

#ifdef DEBUG_LOG
        logdebug("set format ok\n");
#endif

        return SND_ERR_OK;
    }

    return SND_ERR_UNSUPPORTED;
}

uint32_t sndWindowsSoundSystem::detect(SoundDevice::deviceInfo* info) {

    // clear and fill device info
    this->devinfo.clear();

    if (wssDetect(&this->devinfo, true) == false) return SND_ERR_NOTFOUND;

    // fill codec version
    getCodecVersion(&this->devinfo);

    // copy info if not NULL
    if (info != NULL) *info = devinfo;

    return SND_ERR_OK;
}

uint32_t sndWindowsSoundSystem::init(SoundDevice::deviceInfo* info)
{
    // deinit
    if (isInitialised) done();

    // validate fields
    SoundDevice::deviceInfo* p = (info != NULL ? info : &this->devinfo);

    // validate resources
    if ((p->iobase == -1) || (p->iobase == 0) || (p->irq == -1) || (p->dma == -1)) return SND_ERR_INVALIDCONFIG;

    // copy resource fields
    if (info != NULL) {
        this->devinfo.iobase = p->iobase;
        this->devinfo.irq = p->irq;
        this->devinfo.dma = p->dma;
    }

    // reset codec
    if (wssReset(&this->devinfo, isGus) == false) return SND_ERR_NOTFOUND;

    isInitialised = true;
    return SND_ERR_OK;
}

uint32_t sndWindowsSoundSystem::open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void* userdata, soundFormatConverterInfo* conv) {
    uint32_t result = SND_ERR_OK;
    if ((conv == NULL) || (callback == NULL)) return SND_ERR_NULLPTR;

    // stooop!
    if (isOpened) close();

    // clear converter info
    memset(conv, 0, sizeof(soundFormatConverterInfo));

    soundFormat newFormat = fmt;
    // check if format is supported
    if ((flags & SND_OPEN_NOCONVERT) == 0) {
        // conversion is allowed
        // suggest 16bit mono/stereo, leave orig format for 8/16bit
        if ((fmt & SND_FMT_DEPTH_MASK) > SND_FMT_INT16) {
            newFormat = (fmt & (SND_FMT_CHANNELS_MASK)) | SND_FMT_INT16 | SND_FMT_SIGNED;
        }
    }
    if (isFormatSupported(sampleRate, newFormat, conv) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;

    // pass converter info
#ifdef DEBUG_LOG
    logdebug("src = 0x%x, dst = 0x%x\n", fmt, newFormat);
#endif
    if (getConverter(fmt, newFormat, conv) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;
    conv->bytesPerSample = getBytesPerSample(newFormat);

    // we have all relevant info for opening sound device, do it now

    // allocate DMA buffer
    if ((result = dmaBufferInit(bufferSize, conv)) != SND_ERR_OK) return result;

    // install IRQ handler
    if ((result = installIrq()) != SND_ERR_OK) return result;

    // save callback info
    this->callback = callback;
    this->userdata = userdata;

    // pass coverter info
    memcpy(&convinfo, conv, sizeof(convinfo));
    
    // debug output
#ifdef DEBUG_LOG
    fprintf(stderr, __func__": requested format 0x%X, opened format 0x%X, rate %d hz, buffer %d bytes, flags 0x%X\n", fmt, newFormat, sampleRate, bufferSize, flags);
#endif

    isOpened = true;
    return SND_ERR_OK;
}

uint32_t sndWindowsSoundSystem::done() {
    if (isOpened) close();

    isInitialised = false;
    return SND_ERR_OK;
}

uint32_t sndWindowsSoundSystem::close() {
    // stop playback
    if (isPlaying) stop();

    // stop DMA
    if (dmaChannel != -1) dmaStop(dmaChannel);
    
    // deallocate DMA block
    dmaBufferFree();

    // unhook irq if hooked
    if (irq.hooked) irqUnhook(&irq, false);

    // reset DSP (optinal but just in cause)
    if (wssReset(&devinfo, isGus) == false) return SND_ERR_NOTFOUND;

    // fill with defaults
    isOpened = isPlaying = false;
    currentPos = irqs = 0;
    dmaChannel = dmaBlockSize = dmaBufferCount = dmaBufferSize = dmaBufferSamples = dmaBlockSamples = dmaCurrentPtr = dmaRenderPtr = 0;

    return SND_ERR_OK;
}

uint32_t sndWindowsSoundSystem::resume()  {
    // resume playback
    wssRegWrite(devinfo.iobase, WSS_REG_INTERFACE_CONFIG, wssRegRead(devinfo.iobase, WSS_REG_INTERFACE_CONFIG) | 1);

    isPaused = false;
    return SND_ERR_RESUMED;
}

uint32_t sndWindowsSoundSystem::start() {
    uint32_t rtn = SND_ERR_OK;
    if ((rtn = prefill()) != SND_ERR_OK) return rtn;

    // check if 16 bit transfer
    dmaChannel = devinfo.dma;
    
    // acknowledge interrupt
    outp(devinfo.iobase + 2, 0);

    // set sample rate and format
    if (setFormat(&devinfo, convinfo.sampleRate, convinfo.format) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;

    // program DMA controller for transfer
    if (dmaSetup(dmaChannel, &dmaBlock, dmaBlockSize, dmaModeSingle | dmaModeAutoInit | dmaModeRead) == false)
        return SND_ERR_DMA;
#ifdef DEBUG_LOG
    logdebug("dma ready\n");
#endif

    // start playback
    
    // load block length
    wssRegWrite(devinfo.iobase, WSS_REG_DMA_COUNT_LOW,  (dmaBufferSamples - 1) & 0xFF);
    wssRegWrite(devinfo.iobase, WSS_REG_DMA_COUNT_HIGH, (dmaBufferSamples - 1) >> 8);
    
    // ENABLE interrupts
    wssRegWrite(devinfo.iobase, WSS_REG_PIN_CTRL,         wssRegRead(devinfo.iobase, WSS_REG_PIN_CTRL)         | 2);
    
    // enable playback
    wssRegWrite(devinfo.iobase, WSS_REG_INTERFACE_CONFIG, wssRegRead(devinfo.iobase, WSS_REG_INTERFACE_CONFIG) | 1);

    // unmute
    wssRegWrite(devinfo.iobase, WSS_REG_LEFT_DAC_CTRL,  (wssRegRead(devinfo.iobase, WSS_REG_LEFT_DAC_CTRL)  & ~0x80));
    wssRegWrite(devinfo.iobase, WSS_REG_RIGHT_DAC_CTRL, (wssRegRead(devinfo.iobase, WSS_REG_RIGHT_DAC_CTRL) & ~0x80));
    if (featureLevel >= WSS_FEATURE_CS4231) {
        // unmute mono output
        wssRegWrite(devinfo.iobase, WSS_REG_MONO_CTRL, (wssRegRead(devinfo.iobase, WSS_REG_MONO_CTRL) & ~0x40));
    }

#ifdef DEBUG_LOG
    printf("playback started\n");
#endif

    // done! we're playing sound :)
    isPaused = false; isPlaying = true;

    return SND_ERR_OK;
}

uint32_t sndWindowsSoundSystem::pause()
{
    // pause
    wssRegWrite(devinfo.iobase, WSS_REG_INTERFACE_CONFIG, wssRegRead(devinfo.iobase, WSS_REG_INTERFACE_CONFIG) & ~1);

    isPaused = true;
    return SND_ERR_OK;
}

uint32_t sndWindowsSoundSystem::ioctl(uint32_t function, void* data, uint32_t len)
{

    switch (function) {
    case SND_IOCTL_WSS_64KHZ_GET:
        is64khz = ((uint32_t*)data != 0);
        break;

    case SND_IOCTL_WSS_64KHZ_SET:
        (*(uint32_t*)data) = (is64khz ? 1 : 0);
        break;

    default:
        return SND_ERR_UNSUPPORTED;
    }

    return SND_ERR_OK;
}

uint32_t sndWindowsSoundSystem::stop() {
    if (isPlaying) {
        // stop dma
        wssRegWrite(devinfo.iobase, 9, wssRegRead(devinfo.iobase, 9) & ~1);

        dmaStop(dmaChannel);
    }

    isPlaying = false;

    // clear playing position
    currentPos = renderPos = irqs = 0;
    dmaCurrentPtr = dmaRenderPtr = 0;

    return SND_ERR_OK;
}

// irq procedure
bool sndWindowsSoundSystem::irqProc() {
    // test if WSS interrupt
    if ((inp(devinfo.iobase + 2) & 0x1) == 0) return true;      // else chain to previous ISR
    
    // advance play pointers
    irqAdvancePos();

    // acknowledge interrupt
    outp(devinfo.iobase + 2, 0);

    // acknowledge interrupt
    outp(irq.info->picbase, 0x20); if (irq.info->flags & IRQ_SECONDARYPIC) outp(0x20, 0x20);

    // call callback
    irqCallbackCaller();
    
    return false;   // we're handling EOI by itself
}

void __interrupt sndWindowsSoundSystem::wssDetectIrqProc()
{
    // signal IRQ is found
    snd_IrqDetectInfo.found = true;
    outp(snd_IrqDetectInfo.iobase + 2, 0);
    outp(snd_IrqDetectInfo.irq->info->picbase, 0x20); if (snd_IrqDetectInfo.irq->info->flags & IRQ_SECONDARYPIC) outp(0x20, 0x20);
    return;
}

#endif
