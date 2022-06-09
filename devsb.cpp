#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "devsb.h"
#include "convert.h"
#include "sndmisc.h"
#include "snderror.h"

// define to enable logging
//#define DEBUG_LOG

#define arrayof(x) (sizeof(x) / sizeof(x[0]))

const uint32_t probeDataLength = 128;

const uint32_t sbIoBase[]  = { 0x220, 0x240, 0x260, 0x280 };
const uint32_t sbIrq[]     = { 3, 4, 5, 7, 9, 10, 11, 12 };
const uint32_t sbDma[]     = { 0, 1, 3 };
const uint32_t sbDmaHigh[] = { 0, 1, 3, 5, 6, 7 };            // SB16 can transfer 16bit audio over low DMA channels

// SB 2.0/Pro and ESS AudioDrive resources
const soundResourceInfo sbRes[] = {
    {
        SND_RES_IOBASE,
        arrayof(sbIoBase),
        sbIoBase
    },
    {
        SND_RES_IRQ,
        arrayof(sbIrq),
        sbIrq
    },
    {
        SND_RES_DMA,
        arrayof(sbDma),
        sbDma
    },
};

// SB16 resources
const soundResourceInfo sb16Res[] = {
    {
        SND_RES_IOBASE,
        arrayof(sbIoBase),
        sbIoBase
    },
    {
        SND_RES_IRQ,
        arrayof(sbIrq),
        sbIrq
    },
    {
        SND_RES_DMA,
        arrayof(sbDma),
        sbDma
    },
    {
        SND_RES_DMA2,
        arrayof(sbDmaHigh),
        sbDmaHigh
    }
};

const uint32_t sb2OldRates[] = {4000, 22222};
const uint32_t sb2Rates[] = {4000, 45454};
const uint32_t sb16Rates[] = {5000, 45454};         // yes, SB16 lowest limit is at ~5khz
const uint32_t sb16extRates[] = {5000, 48000};
const uint32_t essRates[] = {4000, 48000};          // officially up to 44100, but 48khz is fine (for 1868/1869 at least)

// SB 2.0 (DSP 2.0 w/o highspeed) sound caps
const soundFormatCapability sb2OldCaps[] = {
    {
        (SND_FMT_INT8 | SND_FMT_MONO   | SND_FMT_UNSIGNED),
        -2, // variable range
        sb2OldRates,
    }
};

// SB 2.0 (DSP >= 2.1) sound caps
const soundFormatCapability sb2Caps[] = {
    {
        (SND_FMT_INT8 | SND_FMT_MONO   | SND_FMT_UNSIGNED),
        -2, // variable range
        sb2Rates,
    }
};

// SB Pro sound caps
const soundFormatCapability sbProCaps[] = {
    {
        (SND_FMT_INT8 | SND_FMT_MONO   | SND_FMT_UNSIGNED),
        -2, // variable range
        sb2Rates,
    },
    {
        (SND_FMT_INT8 | SND_FMT_STEREO | SND_FMT_UNSIGNED),
        -2, // variable range
        sb2OldRates,
    }
};

// SB16 sound caps
const soundFormatCapability sb16Caps[] = {
    {
        (SND_FMT_INT8 | SND_FMT_INT16 | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_SIGNED | SND_FMT_UNSIGNED),
        -2, // variable range
        sb16Rates,
    }
};

// SB16 48khz sound caps (geninue Creative cards can go only up to 45454 Hz, some sources claim that SBLive! SB16 emulator can work up to 48kHz?)
const soundFormatCapability sb16extCaps[] = {
    {
        (SND_FMT_INT8 | SND_FMT_INT16 | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_SIGNED | SND_FMT_UNSIGNED),
        -2, // variable range
        sb16extRates,
    }
};


// ESS AudioDrive sound caps
const soundFormatCapability essCaps[] = {
    {
        (SND_FMT_INT8 | SND_FMT_INT16 | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_SIGNED | SND_FMT_UNSIGNED),
        -2, // variable range
        essRates,
    }
};


// ---------------------------------------------
// SB16 helper routines

void sbDspWrite(uint32_t base, uint8_t data) {
    int timeout = (1ULL << 10); while (((inp(base + 0xC) & 0x80) == 0x80) && (--timeout != 0));
    //while (inp(base + 0xC) & 0x80);
    outp(base + 0xC, data);
#ifdef DEBUG_LOG
    printf("dsp write = %02X%s\n", data, (timeout == 0 ? " (timeout!)" : ""));
#endif
}
#pragma aux sbDspWrite parm [edx] [eax]

uint32_t sbDspRead(uint32_t base) {
    int timeout = (1ULL << 10); while (((inp(base + 0xE) & 0x80) == 0) && (--timeout != 0));
    //while (inp(base + 0xE) & 0x80);
    uint8_t data = inp(base + 0xA);
#ifdef DEBUG_LOG
    printf("dsp read = %02X%s\n", data, (timeout == 0 ? " (timeout!)" : ""));
#endif
    return data;
}
#pragma aux sbDspRead parm [edx] value [eax]

void essRegWrite(uint32_t base, uint8_t reg, uint8_t data) {
    sbDspWrite(base, reg); sbDspWrite(base, data);
#ifdef DEBUG_LOG
    printf("ess write Rx%02X = %02X\n", reg, data);
#endif
}

uint32_t essRegRead(uint32_t base, uint8_t reg) {
    sbDspWrite(base, 0xC0); sbDspWrite(base, reg); uint32_t data = sbDspRead(base);
#ifdef DEBUG_LOG
    printf("ess read Rx%02X = %02X\n", reg, data);
#endif
    return data;
}

void sbMixerWrite(uint32_t base, uint8_t index, uint8_t data) {
    outp(base + 4, index); outp(base + 5, data);
#ifdef DEBUG_LOG
    printf("mixer write Rx%02X = %02X\n", index, data);
#endif
}

uint8_t sbMixerRead(uint32_t base, uint8_t index) {
    outp(base + 4, index); uint8_t data = inp(base + 5);
#ifdef DEBUG_LOG
    printf("mixer read Rx%02X = %02X\n", index, data);
#endif
    return data;
}

inline uint8_t sbGetInterruptMask(uint32_t base) {
    //return sbMixerRead(base, 0x82);
    outp(base + 4, 0x82); return inp(base + 5);     // isr+debug printout fix
}

inline void sbAck8Bit(uint32_t base) {
    inp(base + 0xE);
}

inline void sbAck16Bit(uint32_t base) {
    inp(base + 0xF);
}

uint32_t sbTimeConstant(int32_t rate) {
    if (rate < 4000) return 0; else return 256 - (1000000 / rate);
}

// round(a / b)
uint32_t udivRound(uint32_t a, uint32_t b);
#pragma aux udivRound = \
        "xor edx, edx"  "div ebx"       "shr ebx, 1" \
        "cmp edx, ebx"  "jb  _skip_inc" "inc eax" \
        "_skip_inc:" parm [eax] [ebx] value [eax] modify [eax ebx edx]

uint32_t sbTimeConstantAccurate(uint32_t rate) {
    if (rate < 4000) return 0; 
    uint32_t tc = udivRound(1000000, rate);
    
    // check which time constant is more accurate
    //if (abs(rate - (1000000 / tc)) > abs(rate - (1000000 / (tc + 1)))) tc++;

    return 256 - tc;
}

uint32_t essGetDivisor(uint32_t rate) {    
    int tc = 0;
    if (rate > 22222) {
        tc = (795500 / rate);
        // check which time constant is more accurate
        if (abs(rate - (795500 / tc)) > abs(rate - (795500 / (tc + 1)))) tc++;
        tc = 256 - tc;
    } else {
        tc = (397700 / rate);
        // check which time constant is more accurate
        if (abs(rate - (397700 / tc)) > abs(rate - (397700 / (tc + 1)))) tc++;
        tc = 128 - tc;
    };
    
    return tc;
}

// ES1869 only: supports accurate clock rate for 48 kHz
uint32_t ess1869GetDivisor(int32_t rate) {
    int tc = 0;
    if ((rate % 8000) == 0) {
        tc = (768000 / rate);
        // check which time constant is more accurate
        if (abs(rate - (768000 / tc)) > abs(rate - (768000 / (tc + 1)))) tc++;
        tc = 256 - tc;
    } else {
        tc = (793800 / rate);
        // check which time constant is more accurate
        if (abs(rate - (793800 / tc)) > abs(rate - (793800 / (tc + 1)))) tc++;
        tc = 128 - tc;
    };
    
    return tc;
}

bool sbDspReset(uint32_t base) {
    uint32_t timeout;
    
    // trigger reset
    outp(base + 0x6, 1); 
    
    // wait a bit (i.e. read from unused port)
    timeout = 400; while (--timeout) inp(0x80); // should not screw everything up
    
    // remove reset and wait for 0xAA in read buffer 
    outp(base + 0x6, 0); 
    
    timeout = 20; bool detected = false;
    while((--timeout) && (!detected)) {
        if (sbDspRead(base) == 0xAA) detected = true;
    }

    return detected;
}

// -------------- devSBBase common stuff -----------------

uint32_t sndSBBase::sbDetect(SoundDevice::deviceInfo *info, bool manualDetect) {
    uint32_t sbType, sbDspVersion;
#ifdef DEBUG_LOG
    fprintf(stderr, __func__": start detect...\n");
#endif

    if (info == NULL) return 0;
    
    // query BLASTER variable
    char* blasterEnv = getenv("BLASTER");
    if (blasterEnv != NULL) {

#ifdef DEBUG_LOG
        printf("BLASTER variable = %s\n", blasterEnv);
#endif

        // copy variable to temporary buffer
        char envstr[256] = { 0 };
        strncpy(envstr, blasterEnv, sizeof(envstr));

        // tokenize
        char* p = strtok(envstr, " ");
        while (p != NULL) {
            switch (toupper(*p)) {
            case 'A': info->iobase = strtol(p + 1, NULL, 16); break;
            case 'I': info->irq = atoi(p + 1); break;
            case 'D': info->dma = atoi(p + 1); break;
            case 'H': info->dma2 = atoi(p + 1); break;
            case 'T': sbType = atoi(p + 1); break;
                /*
                case 'P' : res.midiPort = atoi(p+1); break;
                case 'E' : res.emuPort = atoi(p+1); break;
                */
            default: break;
            }
            p = strtok(NULL, " ");
        }
    }
    else if (manualDetect == false) return 0; else {
        info->iobase = info->dma = info->irq = info->dma2 = -1;
    }

    // check for port available
    if (info->iobase == -1)
        if (manualDetect == false) return 0; else {
            // probe each port
            bool isFound = false;
            for (const uint32_t* port = sbIoBase; port < (sbIoBase + arrayof(sbIoBase)); port++) {
                if (sbDspReset(*port) == true) {
                    // found!
                    isFound = true;
                    info->iobase = *port;
                    break;
                }
            }
            if (isFound == false) return 0;
        }

#ifdef DEBUG_LOG
    printf("io = 0x%X, irq = %d, dma = %d, highdma = %d\n", info->iobase, info->irq, info->dma, info->dma2);
#endif

    // test for SB presence by resetting DSP
    if (sbDspReset(info->iobase) == false) return 0;
#ifdef DEBUG_LOG
    printf("dsp reset ok\n");
#endif
    
    // get DSP version
    sbDspWrite(info->iobase, 0xE1);
    sbDspVersion = (sbDspRead(info->iobase) << 8); sbDspVersion |= sbDspRead(info->iobase);
#ifdef DEBUG_LOG
    printf("dsp version %d.%d\n", sbDspVersion >> 8, sbDspVersion & 0xff);
#endif
    
    // get extended detect info
    if (detectExt(info, sbDspVersion) != SND_ERR_OK) return 0;

    // fill IRQ/DMA info
    if (fillIrqDma(info, sbDspVersion) != SND_ERR_OK) return 0;

    // test for IRQ/DMA avail info
    if ((info->irq == -1) || (info->dma == -1)) 
        if (manualDetect == false) return 0; else {
            irqEntry irqstuff = { 0 };
            irqstuff.handler = &sbDetectIrqProc;
            snd_IrqDetectInfo.iobase = info->iobase;
            snd_IrqDetectInfo.irq    = &irqstuff;
            if (info->irq == -1) {
                // probe IRQ manually

                for (const uint32_t* irq = sbIrq; irq < (sbIrq + arrayof(sbIrq)); irq++) {
#ifdef DEBUG_LOG
                    printf("\nprobe irq %d...", *irq);
#endif
                    // hook
                    if (irqHook(*irq, &irqstuff, true)) continue;

                    // reset irq found flag
                    snd_IrqDetectInfo.found = false;

                    // trigger IRQ
                    sbDspWrite(info->iobase, 0xF2);
                    volatile uint32_t timeout = 16384;
                    do {
                        inp(0x80);
                        
                        if (snd_IrqDetectInfo.found) {
#ifdef DEBUG_LOG
                            printf("found\n");
#endif
                            info->irq = *irq;
                            break;
                        }
                    } while (--timeout);

                    irqUnhook(&irqstuff, true);
                    
                    // acknowledge interrupt if not found
                    if (snd_IrqDetectInfo.found) break; else sbAck8Bit(snd_IrqDetectInfo.iobase);
                }
                if (info->irq == -1) return 0;
            }

            // probe dma
            if (info->dma == -1) {
                //info->dma = 1;             // SB 2.0/Pro default

                ::dmaBlock testblk;
                if (dmaAlloc(probeDataLength, &testblk)) return 0;
                memset(testblk.ptr, 0x80, probeDataLength);

                for (const uint32_t* dma = sbDma; dma < (sbDma + arrayof(sbDma)); dma++) {
#ifdef DEBUG_LOG
                    printf("\nprobe dma %d...", *dma);
#endif
                    dmaSetup(*dma, &testblk, probeDataLength, dmaModeSingle | dmaModeRead | dmaModeNoAutoInit);

                    // do single-cycle transfer and track dma position change
                    sbDspReset(info->iobase);
                    sbDspWrite(info->iobase, 0xD1);
                    sbDspWrite(info->iobase, 0x40); sbDspWrite(info->iobase, sbTimeConstant(8000));
                    sbDspWrite(info->iobase, 0x14); sbDspWrite(info->iobase, probeDataLength & 0xFF); sbDspWrite(info->iobase, probeDataLength >> 8);
                    volatile uint32_t timeout = 16384;
                    volatile uint32_t dmapos = dmaGetPos(*dma);
                    do {
                        if (dmapos != dmaGetPos(*dma)) {
                            info->dma = *dma;
#ifdef DEBUG_LOG
                            printf("found\n");
#endif
                            break;
                        };
                    } while (--timeout);
                    dmaStop(*dma);
                    if (info->dma != -1) break;
                }
                dmaFree(&testblk);
                sbDspReset(info->iobase);

                if (info->dma == -1) return 0;
            }
        }

#ifdef DEBUG_LOG
    printf("io = 0x%X, irq = %d, dma = %d, highdma = %d\n", info->iobase, info->irq, info->dma, info->dma2);
#endif

    // fill caps
    if (fillDspInfo(info, sbDspVersion) == 0) return 0;

    // detected! :)
    isDetected = true;
    return sbDspVersion;
}

uint32_t sndSBBase::detectExt(SoundDevice::deviceInfo* info, uint32_t sbDspVersion)
{
    return SND_ERR_OK;
}

uint32_t sndSBBase::fillIrqDma(SoundDevice::deviceInfo* info, uint32_t sbDspVersion)
{
    return SND_ERR_OK;
}

uint32_t sndSBBase::fillDspInfo(SoundDevice::deviceInfo * info, uint32_t sbDspVersion)
{
    return 0;
}

const char* sndSBBase::dspVerToString(SoundDevice::deviceInfo * info, uint32_t sbDspVersion) {  
    snprintf(info->privateBuf, sizeof(info->privateBuf), "DSP v.%d.%02d\0", (sbDspVersion >> 8) & 0xFF, sbDspVersion & 0xFF);
    info->version = info->privateBuf;
    return info->version;
}

bool sndSBBase::irqProc()
{
    return true;
}

void __far __interrupt sndSBBase::sbDetectIrqProc()
{
    // signal IRQ is found
    snd_IrqDetectInfo.found = true;
    sbAck8Bit(snd_IrqDetectInfo.iobase);
    outp(snd_IrqDetectInfo.irq->info->picbase, 0x20); if (snd_IrqDetectInfo.irq->info->flags & IRQ_SECONDARYPIC) outp(0x20, 0x20);
}

uint32_t sndSBBase::init(SoundDevice::deviceInfo* info)
{
    // deinit
    if (isInitialised) done();
    
    // validate fields
    SoundDevice::deviceInfo *p = (info != NULL ? info : &this->devinfo);
    
    // validate resources
    if ((p->iobase == -1) || (p->iobase == 0) || (p->irq == -1) || (p->dma == -1)) return SND_ERR_INVALIDCONFIG;
    
    // copy resource fields
    if (info != NULL) {
        this->devinfo.iobase = p->iobase;
        this->devinfo.irq = p->irq;
        this->devinfo.dma = p->dma;
        this->devinfo.dma2 = p->dma2;
    }
    
    // TODO: redetect
    
    // reset DSP
    if (sbDspReset(this->devinfo.iobase) == false) return SND_ERR_NOTFOUND;
    
    // disable speaker
    sbDspWrite(this->devinfo.iobase, 0xD3);
    
    return SND_ERR_OK;
}

uint32_t sndSBBase::done() {
    // stop playback
    if (isPlaying) stop();

    // stop DMA and deallocate DMA block
    if (dmaChannel != -1) dmaStop(dmaChannel);
    if (dmaBlock.ptr != NULL) {
        dmaFree(&dmaBlock);
        dmaBlock.ptr = NULL;
    }

    // unlock DPMI memory for buffer
    dpmi_unlockmemory(dmaBlock.ptr, dmaBlockSize+64);

    // unhook irq if hooked
    if (irq.hooked) irqUnhook(&irq, false);

    // reset DSP (optional but just in cause)
    if (sbDspReset(devinfo.iobase) == false) return SND_ERR_NOTFOUND;

    // fill with defaults
    isInitialised = isPlaying = false;
    currentPos = irqs = 0;
    dmaChannel = dmaBlockSize = dmaBufferCount = dmaBufferSize = dmaBufferSamples = dmaCurrentPtr = dmaBufferPtr = 0;
    sampleRate = bytesPerSample = 0;
    currentFormat = SND_FMT_NULL;

    return SND_ERR_OK;
}

const char* sndSBBase::getName()
{
    return devinfo.name;
}

uint64_t sndSBBase::getPos() {
    if (isPlaying) {
        volatile uint64_t totalPos = 0; uint32_t timeout = 300;
        // quick and dirty rewind bug fix :D
        do {
            totalPos = currentPos + ((dmaBlockSize - (dmaGetPos(dmaChannel, false) << (dmaChannel >= 4 ? 1 : 0))) / convinfo.bytesPerSample);
        } while ((totalPos < oldTotalPos) && (--timeout != 0));
        oldTotalPos = totalPos;
        return totalPos;
    }
    else return 0;
}

uint32_t sndSBBase::ioctl(uint32_t function, void* data, uint32_t len)
{
    // stub
    return SND_ERR_UNSUPPORTED;
}

// ----------- SB 2.0/Pro common stuff -------------------------

uint32_t sndSoundBlaster2Pro::fillDspInfo(SoundDevice::deviceInfo *info, uint32_t sbDspVersion) {
    // fill info
    info->maxBufferSize = 32768;  // BYTES
    switch (sbDspVersion >> 8) {
        case 1:
            // SB 1.x w/o autoinit - not supported!
            return 0;
        
        case 2:
        case 4: // SB16 doesn't support SBPro stereo mode!            
            if (sbDspVersion == 0x200) {
                // old SB 2.0 w/o highspeed mode
                info->caps      = sb2OldCaps;
                info->capsLen   = arrayof(sb2OldCaps);
                info->name      = "Sound Blaster 2.0 (non-highspeed)";
                info->flags     = SND_DEVICE_CLOCKDRIFT;
                
            } else {
                // new SB 2.0 w/ highspeed mode
                info->caps      = sb2Caps;
                info->capsLen   = arrayof(sb2Caps);
                info->name      = "Sound Blaster 2.0";
                info->flags     = SND_DEVICE_CLOCKDRIFT;
            }
            break;
            
        case 3:
            info->caps      = sbProCaps;
            info->capsLen   = arrayof(sbProCaps);
            info->name      = "Sound Blaster Pro";
            info->flags     = SND_DEVICE_CLOCKDRIFT;
            break;
            
        /*
        case 4:
            info->caps      = sb16Caps;
            info->capsLen   = arrayof(sb16Caps);
            info->name      = "Sound Blaster 16";
            info->flags     = 0;
            break;
        */
        default:
            return 0;
    }
    return sbDspVersion;
}


uint32_t sndSoundBlaster2Pro::detect(SoundDevice::deviceInfo *info) {

    // clear and fill device info
    this->devinfo.clear();
    this->dspVersion = sbDetect(&this->devinfo, true);
    
    // filter out no SB + SB 1.0 (w/o autoinit)
    if (this->dspVersion < 0x200) return SND_ERR_NOTFOUND;
    
    // fill DSP version
    dspVerToString(&this->devinfo, this->dspVersion);
    
    // copy info if not NULL
    if (info != NULL) *info = devinfo;
    
    return SND_ERR_OK;
}

uint32_t sndSoundBlaster2Pro::open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv) {
    if ((conv == NULL) || (callback == NULL)) return SND_ERR_NULLPTR;
    
    // stooop!
    if (isPlaying) stop();

    // clear converter info
    memset(&convinfo, 0, sizeof(convinfo));

    soundFormat newFormat = fmt;
    // check if format is supported
    if (flags & SND_OPEN_NOCONVERT) {
        // no conversion if performed
        if (isFormatSupported(sampleRate, fmt, &convinfo) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;
        
    } else {
        // conversion is allowed
        
        switch (dspVersion >> 8) {
            case 3:
                    // convert 16bit to 8bit unsigned, stereo->mono if (rate > 22222) else stereo
                    if (((fmt & SND_FMT_CHANNELS_MASK) == SND_FMT_STEREO) && (sampleRate <= 22222))
                        newFormat = SND_FMT_UNSIGNED | SND_FMT_INT8 | SND_FMT_STEREO;
                    
                    else 
                        // samplerate > 22222hz, fallthrough here
            case 2:
            case 4: // SB16 doesn't support SBPro stereo mode!
                    // convert 16bit to 8bit unsigned, stereo to mono, check for rate limit
                    newFormat = SND_FMT_UNSIGNED | SND_FMT_INT8 | SND_FMT_MONO;
                    
                    // check if format supported
                    if (isFormatSupported(sampleRate, newFormat, &convinfo) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;
                    
                    break;
                    
                    
            default:
                    return SND_ERR_UNSUPPORTED;
        }
    }
    
    // pass converter info
#ifdef DEBUG_LOG
    printf("src = 0x%x, dst = 0x%x\n", fmt, newFormat);
#endif
    if (getConverter(fmt, newFormat, &convinfo) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;
    convinfo.bytesPerSample = getBytesPerSample(newFormat);

    // premultiply bufferSize by bytesPerSample
    bufferSize *= convinfo.bytesPerSample;
    
    // check for bufsize
    if (bufferSize > devinfo.maxBufferSize) bufferSize = devinfo.maxBufferSize;
    
    // we have all relevant info for opening sound device, do it now
    done();
    
    // save dma info
    dmaChannel = devinfo.dma;
    dmaBufferCount = 2;
    dmaBufferSize = bufferSize;
    dmaBlockSize = dmaBufferSize * 2;
    dmaCurrentPtr = dmaBufferPtr = 0;
    dmaBufferSamples = dmaBufferSize / convinfo.bytesPerSample;
    dmaBlockSamples  = dmaBlockSize  / convinfo.bytesPerSample;

    // allocate DMA buffer
    if (dmaBlock.ptr != NULL) if (dmaFree(&dmaBlock) != 0) return SND_ERR_MEMALLOC;
    if (dmaAlloc(dmaBlockSize, &dmaBlock) != 0) return SND_ERR_MEMALLOC;
    
    // lock DPMI memory for buffer
    dpmi_lockmemory(dmaBlock.ptr, dmaBlockSize+64);
    
    // install IRQ handler
    if (irq.hooked == false) {
        irq.flags = 0;
        //irq.handler = snd_irqProcTable[devinfo.irq];
        irq.handler = snd_irqStaticProc;
        if (irqHook(devinfo.irq, &irq, true) == true) return SND_ERR_MEMALLOC;
        
        // set current active device
        snd_activeDevice[0] = this;
        
        inIrq = false;
    }
    else return SND_ERR_STUCK_IRQ;

    // save callback info
    this->callback = callback;
    this->userdata = userdata;

    // pass coverter info
    memcpy(conv, &convinfo, sizeof(convinfo));
    
    this->currentFormat  = newFormat;
    this->sampleRate     = sampleRate;
    this->bytesPerSample = convinfo.bytesPerSample;

    // debug output
#ifdef DEBUG_LOG
    fprintf(stderr, __func__": requested format 0x%X, opened format 0x%X, rate %d hz, buffer %d bytes, flags 0x%X\n", fmt, newFormat, sampleRate, bufferSize, flags);
#endif
    
    isInitialised = true;
    return SND_ERR_OK;
}

uint32_t sndSoundBlaster2Pro::start() {
    if (isPaused) {
        // resume playback
        
        // enable speaker
        sbDspWrite(devinfo.iobase, 0xD1);
        
        // resume DMA
        // select normal or hispeed mode
        if (isHighspeed) {
            // idk
            sbDspWrite(devinfo.iobase, 0x90);       // UNRELIABLE
        } else {
            sbDspWrite(devinfo.iobase, 0xD4);
        }

        isPaused = false;
        return SND_ERR_OK;
    };
    
    // stop if playing
    if (isPlaying) stop();
    
    isPaused = true;
    
    // call callback to fill buffer with sound data
    {
        if (callback == NULL) return SND_ERR_NULLPTR;
#ifdef DEBUG_LOG
        printf("prefill...\n");
#endif
        soundDeviceCallbackResult rtn = callback(dmaBlock.ptr, dmaBlockSize / convinfo.bytesPerSample, &convinfo, sampleRate, userdata); // fill entire buffer
        switch (rtn) {
            case callbackOk         : break;
            case callbackSkip       : 
            case callbackComplete   : 
            case callbackAbort      : 
            default : return SND_ERR_NO_DATA;
        }
#ifdef DEBUG_LOG
        printf("done\n");
#endif
    }
    
    // reset vars
    currentPos = irqs = dmaCurrentPtr = dmaBufferPtr = 0;
    
    // acknowledge stuck IRQs
    sbAck8Bit(devinfo.iobase);
    
    // enable speaker
    sbDspWrite(devinfo.iobase, 0xD1);
    
    // set time constant
    sbDspWrite(devinfo.iobase, 0x40);
    sbDspWrite(devinfo.iobase, sbTimeConstantAccurate(sampleRate << (currentFormat & SND_FMT_STEREO ? 1 : 0)));      // doubled for sbpro stereo
    
    // set block size
    sbDspWrite(devinfo.iobase, 0x48);
    sbDspWrite(devinfo.iobase, (dmaBufferSize - 1) & 0xFF);
    sbDspWrite(devinfo.iobase, (dmaBufferSize - 1) >> 8);
    
    // program DMA controller for transfer
    if (dmaSetup(dmaChannel, &dmaBlock, dmaBlockSize, dmaModeSingle | dmaModeAutoInit | dmaModeRead))
        return SND_ERR_DMA;
#ifdef DEBUG_LOG
    printf("dma ready\n");
#endif

    // set stereo mode (sbpro only)
    // warning - creative official doc asks you to do 1-byte single cycle transfer (to avoid reversed stereo?) then do actual transfers, we'll omit it now
    if (currentFormat & SND_FMT_STEREO) {
        sbMixerWrite(devinfo.iobase, 0xE, sbMixerRead(devinfo.iobase, 0xE) | 0x22); // stereo out + disable lowpass filter
    }
    
    // select normal or hispeed mode and enable DMA playback
    if ((sampleRate << (currentFormat & SND_FMT_STEREO ? 1 : 0)) > 22222) {
        isHighspeed = true;
        sbDspWrite(devinfo.iobase, 0x90);
#ifdef DEBUG_LOG
        printf("highspeed mode\n");
#endif
    } else {
        isHighspeed = false;
        sbDspWrite(devinfo.iobase, 0x1C);
#ifdef DEBUG_LOG
        printf("normal mode\n");
#endif
    }
#ifdef DEBUG_LOG
    printf("playback started\n");
#endif

    // done! we're playing sound :)
    isPaused = false; isPlaying = true;
    
    return SND_ERR_OK;
}

// pause
uint32_t sndSoundBlaster2Pro::pause() {    
    
    // select normal or hispeed mode
    if (isHighspeed) {
        // totally unreliable, at least on dosbox-x
        sbDspReset(devinfo.iobase);
    } else {
        sbDspWrite(devinfo.iobase, 0xD0);
    }

    // disable speaker
    sbDspWrite(devinfo.iobase, 0xD3);

    isPaused = true;
    return SND_ERR_OK;
}

uint32_t sndSoundBlaster2Pro::ioctl(uint32_t function, void * data, uint32_t len)
{
    return sndSBBase::ioctl(function, data, len);
}

uint32_t sndSoundBlaster2Pro::stop() {
    if (isPlaying) {
        // stop dma
        // select normal or hispeed mode
        if (isHighspeed) {
#if SB2_HISPEED_FAST_RESUME
            sbDspReset(devinfo.iobase);
#else

#endif
        } else {
            sbDspWrite(devinfo.iobase, 0xDA);
        }
        
        dmaStop(dmaChannel);
    }
    
    isPlaying = false;
    
    // clear playing position
    currentPos = irqs = 0;
    dmaCurrentPtr = dmaCurrentBuffer = dmaBufferPtr = 0;
    
    return SND_ERR_OK;
}

// irq procedure
bool sndSoundBlaster2Pro::irqProc() {
    // adjust playptr
    irqs++;
    dmaCurrentPtr += dmaBufferSize; if (dmaCurrentPtr >= dmaBlockSize) {
        dmaCurrentPtr = 0;
        currentPos += dmaBlockSamples;
    }

    // acknowledge dma interrupt
    sbAck8Bit(devinfo.iobase);

    // acknowledge interrupt
    outp(irq.info->picbase, 0x20); if (irq.info->flags & IRQ_SECONDARYPIC) outp(0x20, 0x20);
    
    // get address of block to fill
    unsigned char* p = (unsigned char*)dmaBlock.ptr + dmaBufferPtr;
    
    // adjust dmabuffer
    dmaCurrentBuffer++; if (dmaCurrentBuffer >= dmaBufferCount) dmaCurrentBuffer = 0;
    dmaBufferPtr += dmaBufferSize; if (dmaBufferPtr >= dmaBlockSize) dmaBufferPtr = 0;

    // call callback
    soundDeviceCallbackResult rtn = callback(p, dmaBufferSamples, &convinfo, sampleRate, userdata); // fill only previously played block
    switch (rtn) {
        case callbackOk: break;
        case callbackSkip:
        case callbackComplete:
        case callbackAbort:
        default: stop();                   // race condition?
    }

    return false;   // we're handling EOI by itself
}

// ----------- SB16 common stuff -------------------------
// SO FUCKING MUCH DUPLICATED CODE

uint32_t sndSoundBlaster16::fillDspInfo(SoundDevice::deviceInfo *info, uint32_t sbDspVersion) {
    // fill info
    info->maxBufferSize = 32768;  // BYTES
    if (sbDspVersion >= 0x400) {
        info->caps = sb16Caps;
        info->capsLen = arrayof(sb16Caps);
        info->name = "Sound Blaster 16";
        info->flags = 0;
    } else 
        return 0;               // not a SB16!
    return sbDspVersion;
}

uint32_t sndSoundBlaster16::detect(SoundDevice::deviceInfo *info) {

    // clear and fill device info
    this->devinfo.clear();
    this->dspVersion = sbDetect(&this->devinfo, true);

    // filter out no SB + SB 1.0 (w/o autoinit)
    if (this->dspVersion < 0x400) return SND_ERR_NOTFOUND;

    // fill DSP version
    dspVerToString(&this->devinfo, this->dspVersion);

    // copy info if not NULL
    if (info != NULL) *info = devinfo;
    
    return SND_ERR_OK;
}

uint32_t sndSoundBlaster16::open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv) {
    if ((conv == NULL) || (callback == NULL)) return SND_ERR_NULLPTR;

    // stooop!
    if (isPlaying) stop();

    // clear converter info
    memset(&convinfo, 0, sizeof(convinfo));

    soundFormat newFormat = fmt;
    // check if format is supported
    if (flags & SND_OPEN_NOCONVERT) {
        // no conversion if performed
        if (isFormatSupported(sampleRate, fmt, &convinfo) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;

    }
    else {
        // conversion is allowed
        // suggest 16bit mono/stereo, leave orig format for 8/16bit
        if ((fmt & SND_FMT_DEPTH_MASK) > SND_FMT_INT16) {
            newFormat = (fmt & (SND_FMT_CHANNELS_MASK | SND_FMT_SIGN_MASK)) | SND_FMT_INT16;
            if (isFormatSupported(sampleRate, newFormat, &convinfo) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;
        } else 
            if (isFormatSupported(sampleRate, fmt, &convinfo) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;
    }

    // pass converter info
#ifdef DEBUG_LOG
    printf("src = 0x%x, dst = 0x%x\n", fmt, newFormat);
#endif
    if (getConverter(fmt, newFormat, &convinfo) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;
    convinfo.bytesPerSample = getBytesPerSample(newFormat);

    // premultiply bufferSize by bytesPerSample
    bufferSize *= convinfo.bytesPerSample;

    // check for bufsize
    if (bufferSize > devinfo.maxBufferSize) bufferSize = devinfo.maxBufferSize;

    // we have all relevant info for opening sound device, do it now
    done();

    // save dma info
    dmaChannel = devinfo.dma;
    dmaBufferCount = 2;
    dmaBufferSize = bufferSize;
    dmaBlockSize = dmaBufferSize * 2;
    dmaCurrentPtr = dmaBufferPtr = 0;
    dmaBufferSamples = dmaBufferSize / convinfo.bytesPerSample;
    dmaBlockSamples  = dmaBlockSize  / convinfo.bytesPerSample;

    // allocate DMA buffer
    if (dmaBlock.ptr != NULL) if (dmaFree(&dmaBlock) != 0) return SND_ERR_MEMALLOC;
    if (dmaAlloc(dmaBlockSize, &dmaBlock) != 0) return SND_ERR_MEMALLOC;

    // lock DPMI memory for buffer
    dpmi_lockmemory(dmaBlock.ptr, dmaBlockSize+64);

    // install IRQ handler
    if (irq.hooked == false) {
        irq.flags = 0;
        //irq.handler = snd_irqProcTable[devinfo.irq];
        irq.handler = snd_irqStaticProc;
        if (irqHook(devinfo.irq, &irq, true) == true) return SND_ERR_MEMALLOC;

        // set current active device
        snd_activeDevice[0] = this;

        inIrq = false;
    }
    else return SND_ERR_STUCK_IRQ;

    // save callback info
    this->callback = callback;
    this->userdata = userdata;

    // pass coverter info
    memcpy(conv, &convinfo, sizeof(convinfo));

    this->currentFormat = newFormat;
    this->sampleRate = sampleRate;
    this->bytesPerSample = convinfo.bytesPerSample;

    // debug output
#ifdef DEBUG_LOG
    fprintf(stderr, __func__": requested format 0x%X, opened format 0x%X, rate %d hz, buffer %d bytes, flags 0x%X\n", fmt, newFormat, sampleRate, bufferSize, flags);
#endif

    isInitialised = true;
    return SND_ERR_OK;
}

uint32_t sndSoundBlaster16::start() {
    if (isPaused) {
        // resume playback
        sbDspWrite(devinfo.iobase, (is16Bit ? 0xD6 : 0xD4));

        isPaused = false;
        return SND_ERR_OK;
    };

    // stop if playing
    if (isPlaying) stop();

    isPaused = true;

    // call callback to fill buffer with sound data
    {
        if (callback == NULL) return SND_ERR_NULLPTR;
#ifdef DEBUG_LOG
        printf("prefill...\n");
#endif
        soundDeviceCallbackResult rtn = callback(dmaBlock.ptr, dmaBlockSize / convinfo.bytesPerSample, &convinfo, sampleRate, userdata); // fill entire buffer
        switch (rtn) {
            case callbackOk: break;
            case callbackSkip:
            case callbackComplete:
            case callbackAbort:
            default: return SND_ERR_NO_DATA;
        }
#ifdef DEBUG_LOG
        printf("done\n");
#endif
    }

    // reset vars
    currentPos = irqs = dmaCurrentPtr = dmaBufferPtr = 0;

    // reset DSP
    if (sbDspReset(devinfo.iobase) == false) return SND_ERR_INVALIDCONFIG;

    // check if 16 bit transfer
    is16Bit = (currentFormat & SND_FMT_INT16) == SND_FMT_INT16;
    dmaChannel = (is16Bit ? devinfo.dma2 : devinfo.dma);

    // set sample rate
    sbDspWrite(devinfo.iobase, 0x41);
    sbDspWrite(devinfo.iobase, sampleRate >> 8); sbDspWrite(devinfo.iobase, sampleRate & 0xFF);

    // program DMA controller for transfer
    if (dmaSetup(dmaChannel, &dmaBlock, dmaBlockSize, dmaModeSingle | dmaModeAutoInit | dmaModeRead))
        return SND_ERR_DMA;
#ifdef DEBUG_LOG
    printf("dma ready\n");
#endif
    
    // start playback
    uint32_t datalen = (dmaBufferSamples << (currentFormat & SND_FMT_STEREO ? 1 : 0)) - 1;
    uint32_t cmd = getStartCommand(convinfo);
    if (cmd == 0) return SND_ERR_INVALIDCONFIG;
#ifdef DEBUG_LOG
    printf("sb16 cmd = 0x%X\n", cmd);
#endif

    sbDspWrite(devinfo.iobase, cmd & 0xFF);
    sbDspWrite(devinfo.iobase, cmd >> 8);
    sbDspWrite(devinfo.iobase, datalen & 0xFF);
    sbDspWrite(devinfo.iobase, datalen >> 8);
#ifdef DEBUG_LOG
    printf("playback started\n");
#endif

    // done! we're playing sound :)
    isPaused = false; isPlaying = true;

    return SND_ERR_OK;
}

uint32_t sndSoundBlaster16::pause() {
    // pause
    sbDspWrite(devinfo.iobase, (is16Bit ? 0xD5 : 0xD0));
    
    isPaused = true;
    return SND_ERR_OK;
}


uint32_t sndSoundBlaster16::ioctl(uint32_t function, void * data, uint32_t len)
{
    return sndSBBase::ioctl(function, data, len);
}

uint32_t sndSoundBlaster16::stop() {
    if (isPlaying) {
        // stop dma
        sbDspWrite(devinfo.iobase, (is16Bit ? 0xD9 : 0xDA));

        dmaStop(dmaChannel);
    }

    isPlaying = false;

    // clear playing position
    currentPos = irqs = 0;
    dmaCurrentPtr = dmaCurrentBuffer = dmaBufferPtr = 0;

    return SND_ERR_OK;
}

uint32_t sndSoundBlaster16::fillIrqDma(SoundDevice::deviceInfo* info, uint32_t sbDspVersion)
{
    // extract IRQ\DMA settings from mixer for SB16
    if (sbDspVersion >= 0x400) {
        uint8_t irqSetup = sbMixerRead(info->iobase, 0x80);
        uint8_t dmaSetup = sbMixerRead(info->iobase, 0x81);

        info->irq  = (irqSetup & 0x8 ? 10 : (irqSetup & 0x4 ?  7 : (irqSetup & 0x2  ? 5 : (irqSetup & 0x1 ? 9 : info->dma))));
        info->dma  = (dmaSetup & 0x8 ?  3 : (dmaSetup & 0x2 ?  1 : (dmaSetup & 0x1  ? 0 : info->dma)));
        info->dma2 = (dmaSetup & 0x80 ? 7 : (dmaSetup & 0x40 ? 6 : (dmaSetup & 0x20 ? 5 : info->dma)));
    }
    return SND_ERR_OK;
}


// irq procedure
bool sndSoundBlaster16::irqProc() {
    // check IRQ cause
    uint8_t intMask = sbGetInterruptMask(devinfo.iobase);

    // check and acknowledge sb interrupt
    if (intMask & (is16Bit ? 0x2 : 0x1) == 0) return true; else (is16Bit ? sbAck16Bit(devinfo.iobase) : sbAck8Bit(devinfo.iobase));

    // adjust playptr
    irqs++;
    dmaCurrentPtr += dmaBufferSize; if (dmaCurrentPtr >= dmaBlockSize) {
        dmaCurrentPtr = 0;
        currentPos += dmaBlockSamples;
    }

    // acknowledge interrupt
    outp(irq.info->picbase, 0x20); if (irq.info->flags & IRQ_SECONDARYPIC) outp(0x20, 0x20);

    // get address of block to fill
    unsigned char* p = (unsigned char*)dmaBlock.ptr + dmaBufferPtr;
    
    // adjust dmabuffer
    dmaCurrentBuffer++; if (dmaCurrentBuffer >= dmaBufferCount) dmaCurrentBuffer = 0;
    dmaBufferPtr += dmaBufferSize; if (dmaBufferPtr >= dmaBlockSize) dmaBufferPtr = 0;

    // call callback
    soundDeviceCallbackResult rtn = callback(p, dmaBufferSamples, &convinfo, sampleRate, userdata); // fill only previously played block
    switch (rtn) {
        case callbackOk: break;
        case callbackSkip:
        case callbackComplete:
        case callbackAbort:
        default: stop();                   // race condition?
    }

    return false;   // we're handling EOI by itself
}

uint32_t sndSoundBlaster16::getStartCommand(soundFormatConverterInfo & conv)
{
    uint32_t cmd = 0x6; // sound out, auto-init, fifo on
    switch (conv.format & SND_FMT_DEPTH_MASK) {
        case SND_FMT_INT8:  cmd |= 0xC0; break;
        case SND_FMT_INT16: cmd |= 0xB0; break;
        default: return 0;
    };

    uint32_t mode = ((conv.format & SND_FMT_STEREO) ? 0x20 : 0x0) | ((conv.format & SND_FMT_UNSIGNED) ? 0 : 0x10);

    return (uint16_t)((mode << 8) | cmd);
}


// TODO: ESS AudioDrive support

#if 1

uint32_t sndESSAudioDrive::fillDspInfo(SoundDevice::deviceInfo* info, uint32_t sbDspVersion) {
    // fill info
    info->maxBufferSize = 32768;  // BYTES
    info->caps = essCaps;
    info->capsLen = arrayof(essCaps);
    info->name = "ESS AudioDrive";
    info->flags = 0;
    return sbDspVersion;
}

uint32_t sndESSAudioDrive::detectExt(SoundDevice::deviceInfo* info, uint32_t sbDspVersion)
{
    modelNumber = modelId = 0;

    // limit possible DSP versions
    if ((sbDspVersion < 0x200) || (sbDspVersion >= 0x400)) return SND_ERR_NOTFOUND;

    // check id string (probably will fail on PCI cards, fixme or ignore)
    sbDspWrite(info->iobase, 0xE7);
    volatile uint32_t id = sbDspRead(info->iobase) << 8; id |= sbDspRead(info->iobase);

#ifdef DEBUG_LOG
    printf("ESS id = 0x%X, rev %d\n", id, (id & 0xF));
#endif

    if ((id & 0xFFF0) != 0x6880) return SND_ERR_NOTFOUND;
    
    // enable extended commands
    sbDspWrite(info->iobase, 0xC6);
    
    modelId = id;

    if ((id & 0x8) == 0x8) {
        // ESS1868/1869, get new model info
        // get high byte
        volatile uint16_t extid = sbMixerRead(info->iobase, 0x40);
        // get low byte
        extid  = (extid << 8);
        extid |= inp(info->iobase + 5);     // FIX: write to mixer index reg resets ID sequence

        // next two reads return config register base, ignore it
        
#ifdef DEBUG_LOG
        printf("ESS extended id = 0x%X\n", extid);
#endif

        // convert form BCD to hex
        while (extid != 0) {
            modelNumber = modelNumber * 10 + ((extid & 0xF000) >> 12); extid <<= 4;
        }
    }
    else {
        modelNumber = 688;
    }

    return SND_ERR_OK;
}

uint32_t sndESSAudioDrive::fillIrqDma(SoundDevice::deviceInfo* info, uint32_t sbDspVersion)
{
    if (modelId == 0) return SND_ERR_NOTFOUND;

    // read IRQ config
    uint8_t irq = essRegRead(info->iobase, 0xB1), dma = essRegRead(info->iobase, 0xB2);
#ifdef DEBUG
    printf("irq = 0x%02X, dma = 0x%02X\n", irq, dma);
#endif
    // filter out other bits
    irq &= 0xF; dma &= 0xF;
    info->irq = (irq == 0x5 ? 5 : (irq == 0xA ? 7 : (irq == 0xF ? 10 : info->irq)));
    info->dma = (dma == 0x5 ? 0 : (dma == 0xA ? 1 : (dma == 0xF ? 3  : info->dma)));

    return SND_ERR_OK;
}

uint32_t sndESSAudioDrive::detect(SoundDevice::deviceInfo* info) {

    // clear and fill device info
    this->devinfo.clear();
    this->dspVersion = sbDetect(&this->devinfo, true);

    // check DSP version
    if ((this->dspVersion < 0x200) || (this->dspVersion >= 0x400)) return SND_ERR_NOTFOUND;

    // fill DSP version
    dspVerToString(&this->devinfo, this->dspVersion);

    // copy info if not NULL
    if (info != NULL) *info = devinfo;

    return SND_ERR_OK;
}

uint32_t sndESSAudioDrive::open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void* userdata, soundFormatConverterInfo* conv) {
    if ((conv == NULL) || (callback == NULL)) return SND_ERR_NULLPTR;

    // stooop!
    if (isPlaying) stop();

    // clear converter info
    memset(&convinfo, 0, sizeof(convinfo));

    soundFormat newFormat = fmt;
    // check if format is supported
    if (flags & SND_OPEN_NOCONVERT) {
        // no conversion if performed
        if (isFormatSupported(sampleRate, fmt, &convinfo) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;

    }
    else {
        // conversion is allowed
        // suggest 16bit mono/stereo, leave orig format for 8/16bit
        if ((fmt & SND_FMT_DEPTH_MASK) > SND_FMT_INT16) {
            newFormat = (fmt & (SND_FMT_CHANNELS_MASK | SND_FMT_SIGN_MASK)) | SND_FMT_INT16;
            if (isFormatSupported(sampleRate, newFormat, &convinfo) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;
        }
    }

    // pass converter info
#ifdef DEBUG_LOG
    printf("src = 0x%x, dst = 0x%x\n", fmt, newFormat);
#endif
    if (getConverter(fmt, newFormat, &convinfo) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;
    convinfo.bytesPerSample = getBytesPerSample(newFormat);

    // premultiply bufferSize by bytesPerSample
    bufferSize *= convinfo.bytesPerSample;

    // check for bufsize
    if (bufferSize > devinfo.maxBufferSize) bufferSize = devinfo.maxBufferSize;

    // we have all relevant info for opening sound device, do it now
    done();

    // save dma info
    dmaChannel = devinfo.dma;
    dmaBufferCount = 2;
    dmaBufferSize = bufferSize;
    dmaBlockSize = dmaBufferSize * 2;
    dmaCurrentPtr = dmaBufferPtr = 0;
    dmaBufferSamples = dmaBufferSize / convinfo.bytesPerSample;
    dmaBlockSamples  = dmaBlockSize  / convinfo.bytesPerSample;

    // allocate DMA buffer
    if (dmaBlock.ptr != NULL) if (dmaFree(&dmaBlock) != 0) return SND_ERR_MEMALLOC;
    if (dmaAlloc(dmaBlockSize, &dmaBlock) != 0) return SND_ERR_MEMALLOC;

    // lock DPMI memory for buffer
    dpmi_lockmemory(dmaBlock.ptr, dmaBlockSize+64);

    // install IRQ handler
    if (irq.hooked == false) {
        irq.flags = 0;
        //irq.handler = snd_irqProcTable[devinfo.irq];
        irq.handler = snd_irqStaticProc;
        if (irqHook(devinfo.irq, &irq, true) == true) return SND_ERR_MEMALLOC;

        // set current active device
        snd_activeDevice[0] = this;

        inIrq = false;
    }
    else return SND_ERR_STUCK_IRQ;

    // save callback info
    this->callback = callback;
    this->userdata = userdata;

    // pass coverter info
    memcpy(conv, &convinfo, sizeof(convinfo));

    this->currentFormat = newFormat;
    this->sampleRate = sampleRate;
    this->bytesPerSample = convinfo.bytesPerSample;

    // debug output
#ifdef DEBUG_LOG
    fprintf(stderr, __func__": requested format 0x%X, opened format 0x%X, rate %d hz, buffer %d bytes, flags 0x%X\n", fmt, newFormat, sampleRate, bufferSize, flags);
#endif

    isInitialised = true;
    return SND_ERR_OK;
}

uint32_t sndESSAudioDrive::start() {
    if (isPaused) {
        // resume playback
        essRegWrite(devinfo.iobase, 0xB8, (essRegRead(devinfo.iobase, 0xB8) & ~0x1) | 0x1);

        isPaused = false;
        return SND_ERR_OK;
    };

    // stop if playing
    if (isPlaying) stop();

    isPaused = true;

    // call callback to fill buffer with sound data
    {
        if (callback == NULL) return SND_ERR_NULLPTR;
#ifdef DEBUG_LOG
        printf("prefill...\n");
#endif
        soundDeviceCallbackResult rtn = callback(dmaBlock.ptr, dmaBlockSize / convinfo.bytesPerSample, &convinfo, sampleRate, userdata); // fill entire buffer
        switch (rtn) {
        case callbackOk: break;
        case callbackSkip:
        case callbackComplete:
        case callbackAbort:
        default: return SND_ERR_NO_DATA;
        }
#ifdef DEBUG_LOG
        printf("done\n");
#endif
    }

    // reset vars
    currentPos = irqs = dmaCurrentPtr = dmaBufferPtr = 0;

    // acknowledge stuck IRQs
    sbAck8Bit(devinfo.iobase);
    
    // reset DSP
#ifdef DEBUG_LOG
    printf("reset dsp...\n");
#endif
    if (!sbDspReset(devinfo.iobase)) return SND_ERR_NOTFOUND;

    // enable enhanced mode
#ifdef DEBUG_LOG
    printf("enter extended mode...\n");
#endif
    sbDspWrite(devinfo.iobase, 0xC6);

    // set autoinit DMA playback transfer
    essRegWrite(devinfo.iobase, 0xB8, 0x04);

    // set mono/stereo
    essRegWrite(devinfo.iobase, 0xA8, (essRegRead(devinfo.iobase, 0xA8) & ~3) | (currentFormat & SND_FMT_STEREO ? 1 : 3));

    // set single transfer DMA (TODO: demand mode for bus offloading?)
    if ((demandModeEnable) && ((dmaBufferSize & 3) == 0)) {
#ifdef DEBUG_LOG
        printf("enable demand mode transfer...\n");
#endif
        essRegWrite(devinfo.iobase, 0xB9, 2);
    } else essRegWrite(devinfo.iobase, 0xB9, 0);

    // set sample/filter rate
    uint32_t divisor = 0;
    if ((modelNumber == 1869) || (modelNumber == 1879)) {
#ifdef DEBUG_LOG
        printf("ESS1869/1879 -> enable 768khz clock\n");
#endif
        // enable 768khz clock
        sbMixerWrite(devinfo.iobase, 0x71, (sbMixerRead(devinfo.iobase, 0x71) | 0x20));
        divisor = ess1869GetDivisor(sampleRate);
    } else {
        // use older codepath
        divisor = essGetDivisor(sampleRate);
    }
    uint32_t filterRate = 256 - (7160000 * 20) / (8 * 82 * sampleRate); // 80% of half of samplerate
    essRegWrite(devinfo.iobase, 0xA1, divisor);
    essRegWrite(devinfo.iobase, 0xA2, filterRate);

    // set 11/22/44 khz rate fixup (ESS1868 and higher only?)
    if (((sampleRate % 11025) == 0) && (modelNumber >= 1868)) {
#ifdef DEBUG_LOG
        printf("sample rate = %d -> enable rate fixup\n", sampleRate);
#endif
        essRegWrite(devinfo.iobase, 0xBA, (essRegRead(devinfo.iobase, 0xBA) & ~0x40) | 0x40);
    }

#ifdef DEBUG_LOG
    printf("divisor = 0x%X, filter rate = 0x%X\n", divisor, filterRate);
#endif

    dmaChannel = devinfo.dma;

    // set transfer length 
    uint32_t datalen = -(dmaBufferSize);                    // fifo counter is 2's complement and counts upwards
    essRegWrite(devinfo.iobase, 0xA4, (datalen) & 0xFF);
    essRegWrite(devinfo.iobase, 0xA5, (datalen) >> 8);

    // more init!
    essRegWrite(devinfo.iobase, 0xB6, currentFormat & SND_FMT_SIGNED ? 0x00 : 0x80);
    essRegWrite(devinfo.iobase, 0xB7, currentFormat & SND_FMT_SIGNED ? 0x71 : 0x51);
    essRegWrite(devinfo.iobase, 0xB7,
        0x90 | (currentFormat & SND_FMT_SIGNED ? 0x20 : 0x00) |
        (currentFormat & SND_FMT_INT16 ? 0x04 : 0x00) |
        (currentFormat & SND_FMT_STEREO ? 0x08 : 0x40));     // wtf

    // enable DMA/IRQ
    essRegWrite(devinfo.iobase, 0xB1, (essRegRead(devinfo.iobase, 0xB1) & ~0xA0) | 0x50);
    essRegWrite(devinfo.iobase, 0xB2, (essRegRead(devinfo.iobase, 0xB2) & ~0xA0) | 0x50);

    // delay(?) loop
#ifdef DEBUG_LOG
    printf("delay...\n");
#endif
    for (size_t i = 0; i < 12000; i++) inp(0x80);

    // enable speaker
    sbDspWrite(devinfo.iobase, 0xD1);

    // program DMA controller for transfer
    if (dmaSetup(dmaChannel, &dmaBlock, dmaBlockSize, ((demandModeEnable) && ((dmaBufferSize & 3) == 0) ? dmaModeDemand : dmaModeSingle) | dmaModeAutoInit | dmaModeRead))
        return SND_ERR_DMA;
#ifdef DEBUG_LOG
    printf("dma ready\n");
#endif

    // start playback
    essRegWrite(devinfo.iobase, 0xB8, (essRegRead(devinfo.iobase, 0xB8) | 0x1));

#ifdef DEBUG_LOG
    printf("playback started\n");
#endif

    // done! we're playing sound :)
    isPaused = false; isPlaying = true;

    return SND_ERR_OK;
}

uint32_t sndESSAudioDrive::pause() {
    // pause
    essRegWrite(devinfo.iobase, 0xB8, (essRegRead(devinfo.iobase, 0xB8) & ~0x1));

    isPaused = true;
    return SND_ERR_OK;
}

uint32_t sndESSAudioDrive::ioctl(uint32_t function, void* data, uint32_t len)
{
    
    if (len == 0) return SND_ERR_UNSUPPORTED;

    switch (function) {
    case SND_IOCTL_ESS_DEMANDMODE_SET:
        demandModeEnable = ((uint32_t*)data != 0);
        break;

    case SND_IOCTL_ESS_DEMANDMODE_GET:
        (*(uint32_t*)data) = (demandModeEnable ? 1 : 0);
        break;

    default:
        return sndSBBase::ioctl(function, data, len);
    }

    return SND_ERR_OK;
}

uint32_t sndESSAudioDrive::stop() {
    if (isPlaying) {
        // stop dma
        essRegWrite(devinfo.iobase, 0xB8, (essRegRead(devinfo.iobase, 0xB8) & ~0x1));

        dmaStop(dmaChannel);
    }

    isPlaying = false;

    // clear playing position
    currentPos = irqs = 0;
    dmaCurrentPtr = dmaCurrentBuffer = dmaBufferPtr = 0;

    return SND_ERR_OK;
}

// irq procedure
bool sndESSAudioDrive::irqProc() {
    // adjust playptr
    irqs++;
    dmaCurrentPtr += dmaBufferSize; if (dmaCurrentPtr >= dmaBlockSize) {
        dmaCurrentPtr = 0;
        currentPos += dmaBlockSamples;
    }

    // acknowledge interrupt
    sbAck8Bit(devinfo.iobase);

    // acknowledge interrupt
    outp(irq.info->picbase, 0x20); if (irq.info->flags & IRQ_SECONDARYPIC) outp(0x20, 0x20);

    // get address of block to fill
    unsigned char* p = (unsigned char*)dmaBlock.ptr + dmaBufferPtr;

    // adjust dmabuffer
    dmaCurrentBuffer++; if (dmaCurrentBuffer >= dmaBufferCount) dmaCurrentBuffer = 0;
    dmaBufferPtr += dmaBufferSize; if (dmaBufferPtr >= dmaBlockSize) dmaBufferPtr = 0;

    // call callback
    soundDeviceCallbackResult rtn = callback(p, dmaBufferSamples, &convinfo, sampleRate, userdata); // fill only previously played block
    switch (rtn) {
        case callbackOk: break;
        case callbackSkip:
        case callbackComplete:
        case callbackAbort:
        default: stop();                   // race condition?
    }

    return false;   // we're handling EOI by itself
}

const char* sndESSAudioDrive::dspVerToString(SoundDevice::deviceInfo * info, uint32_t ver)
{
    snprintf(info->privateBuf, sizeof(info->privateBuf), "ESS%d, rev. %d", modelNumber, modelId & 0xF);
    info->version = info->privateBuf;
    return info->version;
}

#endif
