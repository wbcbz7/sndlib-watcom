#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "convert.h"
#include "sndmisc.h"
#include "snderror.h"
#include "devpas.h"
#include "logerror.h"

// define to enable logging
//#define DEBUG_LOG

#define arrayof(x) (sizeof(x) / sizeof(x[0]))

const size_t probeDataLength = 100;
const size_t probeIrqLength = 16;

const uint32_t pasIoBase[]  = { 0x388, 0x384, 0x38C, 0x288 }; 
const uint32_t pasIrq[]     = { 3, 4, 5, 7, 9, 10, 11, 12 };
const uint32_t pasDma[]     = { 0, 1, 3 };
const uint32_t pas16Dma[]   = { 0, 1, 3, 5, 6, 7 };

const uint32_t pasRates[]   = { 4000, 48000 };                  // officialll up to 44100hz, but i think 48k is doable as well

// PAS/PAS Plus resources
const soundResourceInfo pasRes[] = {
    {
        SND_RES_IOBASE,
        arrayof(pasIoBase),
        pasIoBase
    },
    {
        SND_RES_IRQ,
        arrayof(pasIrq),
        pasIrq
    },
    {
        SND_RES_DMA,
        arrayof(pasDma),
        pasDma
    },
};

// PAS16 resources
const soundResourceInfo pas16Res[] = {
    {
        SND_RES_IOBASE,
        arrayof(pasIoBase),
        pasIoBase
    },
    {
        SND_RES_IRQ,
        arrayof(pasIrq),
        pasIrq
    },
    {
        SND_RES_DMA,
        arrayof(pas16Dma),
        pas16Dma
    },
};

// PAS sound caps
const soundFormatCapability pasCaps[] = {
    {
        (SND_FMT_INT8  | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_UNSIGNED),
        -2,         // variable range
        pasRates,
    }
};

// PAS+/16 sound caps
const soundFormatCapability pas16Caps[] = {
    {
        (SND_FMT_INT8  | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_UNSIGNED),
        -2,         // variable range
        pasRates,
    },
    {
        (SND_FMT_INT16 | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_SIGNED),
        -2,         // variable range
        pasRates,
    }
};

// ---------------------------------------------
// PAS helper routines 

inline void pasRegWrite(uint32_t base, uint32_t reg, uint8_t data) {
    outp(base + reg, data);
#ifdef DEBUG_LOG
    logdebug("pas write Rx%02X = %02X\n", base + reg, data);
#endif
}

inline uint8_t pasRegRead(uint32_t base, uint32_t reg) {
    uint8_t data = inp(base + reg);
#ifdef DEBUG_LOG
    logdebug("pas read Rx%02X = %02X\n", base + reg, data);
#endif
    return data;
}

// round(a / b)
uint32_t udivRound(uint32_t a, uint32_t b);
#pragma aux udivRound = \
        "xor edx, edx"  "div ebx"       "shr ebx, 1" \
        "cmp edx, ebx"  "jb  _skip_inc" "inc eax" \
        "_skip_inc:" parm [eax] [ebx] value [eax] modify [eax ebx edx]

uint32_t pasGetDivisor(uint32_t rate) {
    if (rate < 4000) return 0; 
    return udivRound(0x1234DD, rate);
}

uint16_t isMvSoundInstalled();
#pragma aux isMvSoundInstalled = \
    "mov    ax, 0xBC00" \
    "mov    bx, 0x3F3F" \
    "xor    cx, cx" \
    "xor    dx, dx" \
    "int    0x2F" \
    "xor    bx, cx" \
    "xor    bx, dx" \
    modify [ax cx dx] value [bx]

MVSoundShadowRegisters *getMvShadowRegisters() {
    uint32_t  rawptr = 0;
    _asm {
        mov     ax, 0xBC02
        int     0x2F
        cmp     ax, 0x4D56
        jne     _out
        mov     word ptr [rawptr],     dx
        mov     word ptr [rawptr + 2], bx
        _out:
    }
    return (MVSoundShadowRegisters*)((rawptr & 0xFFFF0000 >> 12) | (rawptr & 0xFFFF));
}

// BH - IRQ, BL - DMA
uint16_t getMvIrqDma();
#pragma aux getMvIrqDma = \
    "mov    ax, 0xBC04" \
    "int    0x2F" \
    "mov    bh, cl" \
    modify [ax cx dx] value [bx]


// ----------------------------------------------

bool sndProAudioSpectrum::pasDetect(SoundDevice::deviceInfo* info, bool manualDetect)
{
#ifdef DEBUG_LOG
    fprintf(stderr, __func__": start detect...\n");
#endif

    if (info == NULL) return 0;

    // test if MVSOUND.SYS installed
    isMvSoundPresent = (isMvSoundInstalled() == 'MV');
    shadowPtr = &localShadow;

    if (isMvSoundPresent) {
#ifdef DEBUG_LOG
        logdebug("MVSOUND.SYS installed\n");
#endif
        // get IRQ/DMA settings
        uint16_t mvIrqDma = getMvIrqDma();
#ifdef DEBUG_LOG
        logdebug("MVSOUND.SYS IRQ = %d, DMA = %d\n", (mvIrqDma >> 8), (mvIrqDma & 0xFF));
#endif
        if (info->dma == -1)    info->dma = (mvIrqDma & 0xFF);
        if (info->irq == -1)    info->irq = (mvIrqDma >> 8); 

        // interesting enough, MVSOUND.SYS doesn't provides base port info...

        // get shadow register tables
        shadowPtr = getMvShadowRegisters();
    };

    // check if we have enough info for init
    // if not, probe io ports for codec + determine version

    featureLevel = PAS_FEATURELEVEL_ORIGINAL;
    if (info->iobase == -1) if (manualDetect == false) return false; else {
        bool isFound = false;
        for (const uint32_t* port = pasIoBase; port < (pasIoBase + arrayof(pasIoBase)); port++) {
            // get interrupt mask, bail out if floating bus
            uint8_t intmask = pasRegRead(*port, PAS_REG_INTRCTLR);
            if (intmask == 0xFF) continue;

            // test if revision bits are read-only
            pasRegWrite(*port, PAS_REG_INTRCTLR, intmask ^ 0xE0); inp(0x80); inp(0x80);
            uint8_t intmaskNew = pasRegRead(*port, PAS_REG_INTRCTLR);
            pasRegWrite(*port, PAS_REG_INTRCTLR, intmask);
            if (intmaskNew != intmask) continue;

            // found!
            isFound = true;
            info->iobase = *port;
            break;
        }
    }

#ifdef DEBUG_LOG
    logdebug("io = 0x%X, irq = %d, dma = %d\n", info->iobase, info->irq, info->dma);
#endif

    // if it's STILL not found....
    if (info->iobase == -1) return false;

#ifdef DEBUG_LOG
    logdebug("codec reset...\n");
#endif
    // reset codec
    if (pasReset(info) == false) return false;
#ifdef DEBUG_LOG
    logdebug("ok\n");
#endif

    // todo: irq/dma manual probing 
    if (info->dma == -1) return false;
    if (info->irq == -1) return false;

#ifdef DEBUG_LOG
    logdebug("io = 0x%X, irq = %d, dma = %d\n", info->iobase, info->irq, info->dma);
#endif

    // fill caps
    if (fillCodecInfo(info) != SND_ERR_OK) return false;

    // detected! :)
    isDetected = true;
    return true;
}

uint32_t sndProAudioSpectrum::fillCodecInfo(SoundDevice::deviceInfo* info) {
    info->maxBufferSize = 32768;  // BYTES
    switch (featureLevel) {
        case PAS_FEATURELEVEL_ORIGINAL:
            // original PAS
            info->caps      = pasCaps;
            info->capsLen   = arrayof(pasCaps);
            info->name      = "Pro Audio Spectrum";
            info->version   = "original";
            info->flags     = SND_DEVICE_CLOCKDRIFT;  // 44100hz MONO is fine (timer = 27, error = ~0.2%) but stereo gives timer = 13.5 which is OOOOF
            break;
        case PAS_FEATURELEVEL_PAS16:
            // PAS16
            info->caps      = pas16Caps;
            info->capsLen   = arrayof(pas16Caps);
            info->name      = "Pro Audio Spectrum 16";
            info->flags     = SND_DEVICE_CLOCKDRIFT;  // same for PAS16

            // put revision in private buffer
            snprintf(info->privateBuf, sizeof(info->privateBuf), "Rev. %d", pasRegRead(info->iobase, PAS_REG_INTRCTLR) >> 5);
            info->version = info->privateBuf;

            break;
        default:
            return SND_ERR_UNSUPPORTED;
    }
    return SND_ERR_OK;
}

bool sndProAudioSpectrum::pasReset(SoundDevice::deviceInfo* info)
{
    // TODO: is there anything to reset actually?

    // anyway, get feature level from the registers
    featureLevel = PAS_FEATURELEVEL_ORIGINAL;
    uint8_t intmask = pasRegRead(info->iobase, PAS_REG_INTRCTLR);
    if ((intmask & 0xE0) > 0) {
        uint8_t slaveMode = pasRegRead(info->iobase, PAS_REG_SLAVEMODRD);
        if (slaveMode & 0x08) featureLevel = PAS_FEATURELEVEL_PAS16;
    };

    // and init local shadow registers table
    memset(&localShadow, 0, sizeof(localShadow));
    localShadow._crosschannel = 0x09;
    localShadow._audiomixr    = 0x31;       // idk

    return true;
}

uint32_t sndProAudioSpectrum::detect(sndProAudioSpectrum::deviceInfo* info) {

    // clear and fill device info
    this->devinfo.clear();
    if (pasDetect(&this->devinfo, true) == false) return SND_ERR_NOTFOUND;

    // copy info if not NULL
    if (info != NULL) *info = devinfo;

    return SND_ERR_OK;
}

uint32_t sndProAudioSpectrum::init(SoundDevice::deviceInfo* info)
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
    if (pasReset(&this->devinfo) == false) return SND_ERR_NOTFOUND;

    return SND_ERR_OK;
}

uint32_t sndProAudioSpectrum::open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv) {
    uint32_t result = SND_ERR_OK;
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
        uint32_t maxFormat = (featureLevel == PAS_FEATURELEVEL_ORIGINAL ? SND_FMT_INT8 : SND_FMT_INT16);
        if ((fmt & SND_FMT_DEPTH_MASK) > maxFormat) {
            newFormat = (fmt & (SND_FMT_CHANNELS_MASK | SND_FMT_SIGN_MASK)) | maxFormat;
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

    // we have all relevant info for opening sound device, do it now
    done();

    // allocate DMA buffer
    if (result = dmaBufferInit(bufferSize, conv) != SND_ERR_OK) return result;

    // install IRQ handler
    if (irq.hooked == false) {
        irq.flags = 0;
        irq.handler = snd_irqProcTable[devinfo.irq];
        if (irqHook(devinfo.irq, &irq, true) == true) return SND_ERR_MEMALLOC;

        // set current active device
        snd_activeDevice[devinfo.irq] = this;

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

    // debug output
#ifdef DEBUG_LOG
    fprintf(stderr, __func__": requested format 0x%X, opened format 0x%X, rate %d hz, buffer %d bytes, flags 0x%X\n", fmt, newFormat, sampleRate, bufferSize, flags);
#endif

    isInitialised = true;
    return SND_ERR_OK;
}

uint32_t sndProAudioSpectrum::done() {
    // stop playback
    if (isPlaying) stop();

    // stop DMA
    if (dmaChannel != -1) dmaStop(dmaChannel);
    
    // deallocate DMA block
    dmaBufferFree();

    // unhook irq if hooked
    if (irq.hooked) irqUnhook(&irq, false);

    // reset DSP (optional but just in cause)
    if (pasReset(&devinfo) == false) return SND_ERR_NOTFOUND;

    // fill with defaults
    isInitialised = isPlaying = false;
    currentPos = irqs = 0;
    dmaChannel = dmaBlockSize = dmaBufferCount = dmaBufferSize = dmaBufferSamples = dmaBlockSamples = dmaCurrentPtr = dmaRenderPtr = 0;
    sampleRate = 0;
    currentFormat = SND_FMT_NULL;

    return SND_ERR_OK;
}

uint32_t sndProAudioSpectrum::resume() {
    // resume playback
    pasRegWrite(devinfo.iobase, PAS_REG_AUDIOFILT, shadowPtr->_audiofilt |= 0xC0);

    isPaused = false;
    return SND_ERR_OK;
};

uint32_t sndProAudioSpectrum::start() {
    if (isPaused) return resume();

    // stop if playing
    if (isPlaying) stop();

    isPaused = true;
    if (!isInitialised) return SND_ERR_UNINITIALIZED;

    // call callback to fill buffer with sound data
    {
        if (callback == NULL) return SND_ERR_NULLPTR;
#ifdef DEBUG_LOG
        printf("prefill...\n");
#endif
        soundDeviceCallbackResult rtn = callback(dmaBlock.ptr, sampleRate, dmaBlockSamples, &convinfo, renderPos, userdata); // fill entire buffer
        switch (rtn) {
            case callbackOk         : break;
            case callbackSkip       : 
            case callbackComplete   : 
            case callbackAbort      : 
            default : return SND_ERR_NO_DATA;
        }
        renderPos += dmaBufferSamples;
#ifdef DEBUG_LOG
        printf("done\n");
#endif
    }

    // reset vars
    currentPos = irqs = dmaCurrentPtr = 0; dmaRenderPtr = dmaBufferSize;

    // ------------------------------------
    // device-specific code

    // check if 16 bit transfer
    dmaChannel = devinfo.dma;
    
    // acknowledge interrupt
    pasRegWrite(devinfo.iobase, PAS_REG_INTRCTLRST, 0);

    // set sample rate and format
    uint32_t rawRate = sampleRate;
    if ((convinfo.format & SND_FMT_STEREO) != 0) rawRate <<= 1;
    shadowPtr->_samplerate = pasGetDivisor(rawRate);
#ifdef DEBUG_LOG
        printf("sample rate = %d, i8253 divisor = %d\n", sampleRate, shadowPtr->_samplerate);
#endif

    // set bit depth mode for PAS+/16
    if (featureLevel > PAS_FEATURELEVEL_ORIGINAL) {
        uint8_t sysconfig2 = (pasRegRead(devinfo.iobase, PAS_REG_SYSCONFIG2) & ~0xC) | ((convinfo.format & SND_FMT_INT16) ? 4 : 0);
#ifdef DEBUG_LOG
        printf("PAS16 SYSCONFIG2(0x8389) = 0x%02X\n", sysconfig2);
        pasRegWrite(devinfo.iobase, PAS_REG_SYSCONFIG2, sysconfig2);
#endif
    }

    // set stereo mode, disable PCM state machine
    shadowPtr->_crosschannel = (shadowPtr->_crosschannel & 0x8F) | ((convinfo.format & SND_FMT_STEREO) ? 0x30 : 0x10);
    pasRegWrite(devinfo.iobase, PAS_REG_CROSSCHANNEL, shadowPtr->_crosschannel);

    // set sample rate
    pasRegWrite(devinfo.iobase, PAS_REG_TMRCTLR,    shadowPtr->_tmrctlr = 0x36);
    pasRegWrite(devinfo.iobase, PAS_REG_SAMPLERATE, shadowPtr->_samplerate & 0xFF);
    pasRegWrite(devinfo.iobase, PAS_REG_SAMPLERATE, shadowPtr->_samplerate >> 8);

    // enable PAS DMA (why?)
    pasRegWrite(devinfo.iobase, PAS_REG_CROSSCHANNEL, shadowPtr->_crosschannel |= 0x80);

    // program DMA controller for transfer
    if (dmaSetup(dmaChannel, &dmaBlock, dmaBlockSize, dmaModeSingle | dmaModeAutoInit | dmaModeRead))
        return SND_ERR_DMA;
#ifdef DEBUG_LOG
    logdebug("dma ready\n");
#endif

    uint32_t dmaBufferSamples = (dmaBufferSize / convinfo.bytesPerSample);  // testme
    if (convinfo.format & SND_FMT_STEREO)   dmaBufferSamples <<= 1;
    if (dmaChannel >= 4)                    dmaBufferSamples >>= 1;
    
    // start playback
    // load block length
    shadowPtr->_samplecnt = dmaBufferSamples;
    pasRegWrite(devinfo.iobase, PAS_REG_TMRCTLR,    shadowPtr->_tmrctlr = 0x74);
    pasRegWrite(devinfo.iobase, PAS_REG_SAMPLECNT,  shadowPtr->_samplecnt & 0xFF);
    pasRegWrite(devinfo.iobase, PAS_REG_SAMPLECNT,  shadowPtr->_samplecnt >> 8);
    
    // enable sample count timer interrupts (hope this won't trigger any spurious IRQs and over-advance buffer pointer)
    pasRegWrite(devinfo.iobase, PAS_REG_INTRCTLR, shadowPtr->_intrctlr = 0x08);

    // enable PCM state machine
    pasRegWrite(devinfo.iobase, PAS_REG_CROSSCHANNEL, shadowPtr->_crosschannel &= ~0x40);
    uint32_t timeout = 20; while (--timeout) inp(0x80);
    pasRegWrite(devinfo.iobase, PAS_REG_CROSSCHANNEL, shadowPtr->_crosschannel |=  0x40);

    // enable sample rate timer gate
    pasRegWrite(devinfo.iobase, PAS_REG_AUDIOFILT, shadowPtr->_audiofilt |= 0xC0);

#ifdef DEBUG_LOG
    printf("playback started\n");
#endif

    // done! we're playing sound :)
    isPaused = false; isPlaying = true;

    return SND_ERR_OK;
}

uint32_t sndProAudioSpectrum::pause()
{
    // disable sample rate timer gate
    pasRegWrite(devinfo.iobase, PAS_REG_AUDIOFILT, shadowPtr->_audiofilt &= ~0xC0);

    isPaused = true;
    return SND_ERR_OK;
}

uint32_t sndProAudioSpectrum::stop() {
    if (isPlaying) {
        // disable sample rate timer gate
        pasRegWrite(devinfo.iobase, PAS_REG_AUDIOFILT, shadowPtr->_audiofilt &= ~0xC0);

        // disable PCM state machine + PAS DMA
        pasRegWrite(devinfo.iobase, PAS_REG_CROSSCHANNEL, shadowPtr->_crosschannel = (shadowPtr->_crosschannel & ~0xC0) | 0x10);

        // reset to 8 bit mode
        if (featureLevel > PAS_FEATURELEVEL_ORIGINAL)
            pasRegWrite(devinfo.iobase, PAS_REG_SYSCONFIG2, pasRegRead(devinfo.iobase, PAS_REG_SYSCONFIG2) & ~0xC);

        // disable interrupts
        pasRegWrite(devinfo.iobase, PAS_REG_INTRCTLR, shadowPtr->_intrctlr = 0);

        // stop DMA controller
        dmaStop(dmaChannel);
    }

    isPlaying = false;

    // clear playing position
    currentPos = renderPos = irqs = 0;
    dmaCurrentPtr = dmaRenderPtr = 0;

    return SND_ERR_OK;
}

uint32_t sndProAudioSpectrum::ioctl(uint32_t function, void* data, uint32_t len)
{
    return SND_ERR_UNSUPPORTED;
}

// irq procedure
bool sndProAudioSpectrum::irqProc() {
    // advance play pointers
    irqAdvancePos();

    // acknowledge interrupt
    pasRegWrite(devinfo.iobase, PAS_REG_INTRCTLRST, 0);

    // acknowledge interrupt
    outp(irq.info->picbase, 0x20); if (irq.info->flags & IRQ_SECONDARYPIC) outp(0x20, 0x20);

    // call callback
    irqCallbackCaller();

    return false;   // we're handling EOI by itself
}
