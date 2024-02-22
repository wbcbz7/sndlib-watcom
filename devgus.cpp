#include "snddefs.h"
#ifdef SNDLIB_DEVICE_ENABLE_GUS

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "convert.h"
#include "sndmisc.h"
#include "snderror.h"
#include "devgus.h"

//define to enable logging
//#define DEBUG_LOG
#include "logerror.h"

#define arrayof(x) (sizeof(x) / sizeof(x[0]))

static const uint32_t gusIoBase[]    = { 0x220, 0x230, 0x240, 0x250, 0x260, 0x270, 0x280 };
static const uint32_t gusIrq[]       = { 3, 4, 5, 7, 9, 10, 11, 12 };
static const uint32_t gusDma[]       = { 0, 1, 3, 5, 6, 7 };

enum {
    GUS_MIN_DRAM_SIZE = 256*1024,
};

// variable rates
static const uint32_t gusRatesVar[]  = { 4000, 48000 };

// Gravis Ultrasound resourses
const soundResourceInfo gusRes[] = {
    {
        SND_RES_IOBASE,
        arrayof(gusIoBase),
        gusIoBase
    },
   {
        SND_RES_IRQ,
        arrayof(gusIrq),
        gusIrq
    },
    {
        SND_RES_DMA,
        arrayof(gusDma),
        gusDma
    },
};

// GUS sound caps
const soundFormatCapability gusCaps[] = {
    {
        (SND_FMT_INT8 | SND_FMT_INT16 | SND_FMT_MONO | SND_FMT_UNSIGNED | SND_FMT_SIGNED),
        -2, // variable range
        gusRatesVar,
    },
    // TODO: stereo
};

// ---------------------------------------------
// GUS helper routines
// TODO: breaks PCem since it doesn't emulate ISA bus delays!!!
static void gf1_busdelay(uint32_t timeout) {do inp(0x80); while (--timeout);}

int gf1_mod_delay();
#pragma aux gf1_mod_delay = \
        "in al, 0x80" \
        "in al, 0x80" \
        "in al, 0x80" \
        "in al, 0x80" \
        "in al, 0x80" \
        "in al, 0x80" \
        "in al, 0x80" \
        value [eax]

static void gf1_write(uint32_t gusbase, int ch, int reg, uint32_t data) {
#ifdef DEBUG_GF1
    logdebug("GF1 write: base %03X, reg %02X, chan %02X = %02X\n", gusbase, reg, ch, data);
#endif
    if (ch != -1) outp(gusbase + 0x102, ch);
    outp(gusbase + 0x103, reg);
    outp(gusbase + 0x105, data);
}
static void gf1_writew(uint32_t gusbase, int ch, int reg, uint32_t data) {
#ifdef DEBUG_GF1
    logdebug("GF1 write: base %03X, reg %02X, chan %02X = %04X\n", gusbase, reg, ch, data);
#endif
    if (ch != -1) outp(gusbase + 0x102, ch);
    outp(gusbase + 0x103, reg);
    outpw(gusbase + 0x104, data);
}
static void gf1_write_mod(uint32_t gusbase, int ch, int reg, uint32_t data) {
#ifdef DEBUG_GF1
    logdebug("GF1 write: base %03X, reg %02X, chan %02X = %02X\n", gusbase, reg, ch, data);
#endif
    if (ch != -1) outp(gusbase + 0x102, ch);
    outp(gusbase + 0x103, reg);
    outp(gusbase + 0x105, data);
    gf1_mod_delay();
    outp(gusbase + 0x105, data);
}
static void gf1_writew_mod(uint32_t gusbase, int ch, int reg, uint32_t data) {
#ifdef DEBUG_GF1
    logdebug("GF1 write: base %03X, reg %02X, chan %02X = %04X\n", gusbase, reg, ch, data);
#endif
    if (ch != -1) outp(gusbase + 0x102, ch);
    outp(gusbase + 0x103, reg);
    outpw(gusbase + 0x104, data);
    gf1_mod_delay();
    outpw(gusbase + 0x104, data);
}

static uint32_t gf1_read(uint32_t gusbase, int ch, int reg) {
    if (ch != -1) outp(gusbase + 0x102, ch);
    outp(gusbase + 0x103, reg);
    uint32_t data = inp(gusbase + 0x105);
#ifdef DEBUG_GF1
    logdebug("GF1 read:  reg %02X, chan %02X = %02X\n", reg, ch, data);
#endif
    return data;
}
static uint32_t gf1_readw(uint32_t gusbase, int ch, int reg) {
    if (ch != -1) outp(gusbase + 0x102, ch);
    outp(gusbase + 0x103, reg);
    uint32_t data = inpw(gusbase + 0x104);
#ifdef DEBUG_GF1
    logdebug("GF1 read:  reg %02X, chan %02X = %04X\n", reg, ch, data);
#endif
    return data;
}

static inline void gf1_dram_write(uint32_t gusbase, uint32_t pos, uint8_t data) {
    outp (gusbase + 0x103, 0x43);
    outpw(gusbase + 0x104, pos & 0xFFFF);
    outp (gusbase + 0x103, 0x44);
    outp (gusbase + 0x105, pos >> 16);
    outp(gusbase + 0x107, data);
}

static inline uint8_t gf1_dram_read(uint32_t gusbase, uint32_t pos) {
    outp (gusbase + 0x103, 0x43);
    outpw(gusbase + 0x104, pos & 0xFFFF);
    outp (gusbase + 0x103, 0x44);
    outp (gusbase + 0x105, pos >> 16);
    return inp(gusbase + 0x107);
}

static void gf1_clear_irq(uint32_t gusbase) {
    inp(gusbase + 0x6);
    gf1_read(gusbase, 0, 0x41);
    gf1_read(gusbase, 0, 0x49);
    gf1_read(gusbase, 0, 0x8F);
}

static void gf1_clear_channels(uint32_t gusbase) {
    // clear channels
    for (int ch = 0; ch < 32; ch++) {
        gf1_write (gusbase, ch, 0x0, 3);             // voice ctrl - stop channel
        gf1_write (gusbase, ch, 0xD, 3);             // volramp ctrl - stopped
        gf1_write (gusbase, ch, 0x6, 0x3F);          // volramp rate - fastest (afaik)
        gf1_write (gusbase, ch, 0xC, 7);             // pan position - center
        gf1_writew(gusbase, ch, 0x1, 0x0000);        // freq ctrl    - pitch 0.0
        gf1_writew(gusbase, ch, 0x9, 0x0000);        // current volume = 0
    }
}

static void gf1_setup_playback(uint32_t gusbase, uint32_t channel, uint32_t startOffset, uint32_t length, bool is16Bit) {
    uint32_t pcmstart = (startOffset + 0) << 9;
    uint32_t pcmloop  = (startOffset + length) << 9;
    
    // set address, do not start the channel for now
    gf1_writew(gusbase, channel, GF1_REG_CHAN_START_HIGH, pcmstart >> 16);
    gf1_writew(gusbase, channel, GF1_REG_CHAN_START_LOW,  pcmstart & 0xFFFF);
    gf1_writew(gusbase, channel, GF1_REG_CHAN_POS_HIGH,   pcmstart >> 16);
    gf1_writew(gusbase, channel, GF1_REG_CHAN_POS_LOW,    pcmstart & 0xFFFF);
    gf1_writew(gusbase, channel, GF1_REG_CHAN_END_HIGH,   pcmloop >> 16);
    gf1_writew(gusbase, channel, GF1_REG_CHAN_END_LOW,    pcmloop & 0xFFFF);
    gf1_writew(gusbase, channel, GF1_REG_CHAN_FREQ,       0);   // update pitch later
    gf1_write (gusbase, channel, GF1_REG_CHAN_CTRL,       is16Bit ? (1 << 2) : 0);
}

static void gf1_pcm_set_rollover(uint32_t gusbase, uint32_t channel, bool rollover) {
    // set rollover bit
    int volctrl = gf1_read(gusbase, channel, 0x8D);
    (rollover) ? volctrl |= (1 << 2) : volctrl &= ~(1 << 2);
    gf1_write_mod(gusbase, channel, 0x0D, volctrl);

    // set loop bit
    int chanctrl = gf1_read(gusbase, channel, 0x80);
    (rollover) ? chanctrl &= ~(1 << 3) : chanctrl |= (1 << 3);
    gf1_write_mod(gusbase, channel, 0x00, chanctrl);
}

static void gf1_pcm_start(uint32_t gusbase, uint32_t channel, bool irq) {
    int chanctrl = gf1_read(gusbase, channel, 0x80);
    chanctrl = (chanctrl & ~3) | (irq ? (1 << 5) : 0); // enable voice IRQ if requested
    gf1_write_mod(gusbase, channel, 0, chanctrl);

    // start volume ramp to max (TODO: volume control)
    int volctrl = gf1_read(gusbase, channel, 0x8D);
    volctrl &= ~3;
    gf1_write_mod(gusbase, channel, 0xD, volctrl);
}

static void gf1_pcm_stop(uint32_t gusbase, uint32_t channel) {
    int chanctrl = gf1_read(gusbase, channel, 0x80);
    chanctrl = (chanctrl | 3) & ~(1 << 5); // disable voice IRQ
    gf1_write_mod(gusbase, channel, 0, chanctrl);

    // start volume ramp to silence
    gf1_write(gusbase, channel, 0x7, 0);
    int volctrl = gf1_read(gusbase, channel, 0x8D);
    volctrl &= ~3;
    volctrl |= (1 << 6);    // ramp down
    gf1_write_mod(gusbase, channel, 0xD, volctrl);
}


// probe DRAM installed, returns memory size in bytes
static uint32_t gf1_getMemSize(uint32_t iobase) {
    gf1_dram_write(iobase, 0, 0x55);
    gf1_dram_write(iobase, 1, 0xAA);

    if (gf1_dram_read(iobase, 0) != 0x55) return 0;
    if (gf1_dram_read(iobase, 1) != 0xAA) return 0;

    uint32_t offset = 0;
    while (offset < 1*1024*1024) {
        gf1_dram_write(iobase, offset+0, 0xF0);
        gf1_dram_write(iobase, offset+1, 0x0D);

        if (gf1_dram_read(iobase, offset+0) != 0xF0) break;
        if (gf1_dram_read(iobase, offset+1) != 0x0D) break;

        // else increment offset
        offset += 0x10000;    // 64k bytes
    }

    return offset; // in bytes!
}

// ----------------------------------------------

sndGravisUltrasound::sndGravisUltrasound() : IsaDmaDevice("Gravis Ultrasound") {
    // fill with defaults
    dramsize = channel_offset[0] = channel_offset[1] = sample_bytes_to_render_last = 0;
    pitch = 0;
    dmactrl = 0;

    devinfo.name = getName();
    devinfo.version = NULL;
    devinfo.maxBufferSize = 32768;  // BYTES
}

bool sndGravisUltrasound::readEnvironment(SoundDevice::deviceInfo *info) {
    SoundDevice::deviceInfo gusinfo;
    char envstr[64] = { 0 };

    // check for ULTRASND variable
    // ULTRASND=iobase,playdma,recdma,playirq,recirq

    // check if gus codec
    char *ultravar = getenv("ULTRASND");
    if (ultravar != NULL) {
#ifdef DEBUG_LOG
        logdebug("ULTRASND variable = %s\n", ultravar);
#endif
        strncpy(envstr, ultravar, sizeof(envstr));
        char* p = strtok(envstr, ",");
        if (info->iobase == -1) info->iobase = strtol(p, NULL, 16); p = strtok(NULL, ",");
        if (info->dma == -1)    info->dma    = atoi(p); p = strtok(NULL, ",");
        if (info->dma2 == -1)   info->dma2   = atoi(p); p = strtok(NULL, ",");
        if (info->irq == -1)    info->irq    = atoi(p); p = strtok(NULL, ",");
        if (info->irq2 == -1)   info->irq2   = atoi(p);
    }

    return true;
}

bool sndGravisUltrasound::gusDetect(SoundDevice::deviceInfo* info, bool manualDetect)
{
#ifdef DEBUG_LOG
    fprintf(stderr, __func__": start detect...\n");
#endif

    if (info == NULL) return 0;
    
    // first, try to check environment variables
    readEnvironment(info);

    // check if we have enough info for init
    // if not, probe io ports for codec 
    if (info->iobase == -1) if (manualDetect == false) return false; else {
        bool isFound = false;
        for (const uint32_t* port = gusIoBase; port < (gusIoBase + arrayof(gusIoBase)); port++) {
            if ((this->dramsize = gusReset(*port)) >= GUS_MIN_DRAM_SIZE) {
                // found!
                isFound = true;
                info->iobase = *port;
                break;
            }
        }
    }

#ifdef DEBUG_LOG
    logdebug("io = 0x%X, irq = %d, dma = %d\n", info->iobase, info->irq, info->dma);
#endif

    // if it's STILL not found....
    if (info->iobase == -1) return false;

#ifdef DEBUG_LOG
    logdebug("GF1 reset...\n");
#endif
    // reset codec
    if ((this->dramsize = gusReset(info->iobase)) < GUS_MIN_DRAM_SIZE) return false;
#ifdef DEBUG_LOG
    logdebug("ok, %d bytes available\n", this->dramsize);
#endif

    // get IRQ and DMA settings from GF1 registers
    if (info->dma == -1) {
        static uint8_t gusdma[] = {-1, 1, 3, 5, 6, 7, 0, -1};
        outp(info->iobase + 0, inp(info->iobase + 0) & ~(1 << 6));

        // read DMA settings
        info->dma = gusdma[(inp(info->iobase + 0xB) >> 3) & 7];
    }
    if (info->irq == -1) {
        static uint8_t gusdma[] = {-1, 9, 5, 3, 7, 11, 12, 15};
        outp(info->iobase + 0, inp(info->iobase + 0) |  (1 << 6));

        // read IRQ settings
        info->dma = gusdma[(inp(info->iobase + 0xB)) & 7];
    }

    if ((info->dma == -1) && (info->irq == -1)) return false;

    // init format caps for GUS
    info->caps = gusCaps;
    info->maxBufferSize = 32768;    // BYTES
    info->capsLen = arrayof(gusCaps);
    info->name = "Gravis Ultrasound";
    snprintf(
        info->privateBuf,
        info->privateBufSize,
        "A%03X/I%d/D%d, %d kb DRAM",
        info->iobase, info->irq, info->dma, dramsize >> 10
    );
    info->version = info->privateBuf;
    info->flags = 0;

    // detected! :)
    isDetected = true;
    return true;
}

uint32_t sndGravisUltrasound::gusReset(uint32_t iobase) {
    // test GUS reset reaction
    gf1_write(iobase, -1, 0x4C, 0);
    gf1_busdelay(1 << 12);
    if (gf1_read(iobase, -1, 0x4C) != 0) return 0;
    gf1_write(iobase, -1, 0x4C, 1);
    gf1_busdelay(1 << 12);
    gf1_write(iobase, -1, 0x4C, 3);
    gf1_busdelay(1 << 12);
    if (gf1_read(iobase, -1, 0x4C) != 3) return 0;

    // clear pending interrupts
    gf1_clear_irq(iobase);

    // clear DMA and sampling control registers
    gf1_write(iobase, -1, GF1_REG_DMA_CTRL, 0);
    gf1_write(iobase, -1, GF1_REG_DMA_ADDR, 0);
    gf1_write(iobase, -1, GF1_REG_SAMPLING_CTRL, 0);
    gf1_write(iobase, -1, GF1_REG_SAMPLING_FREQ, 0);

    // as GUS is detected now, time to check how much DRAM is installed
    gf1_dram_write(iobase, 0, 0x55);
    gf1_dram_write(iobase, 1, 0xAA);

    if (gf1_dram_read(iobase, 0) != 0x55) return 0;
    if (gf1_dram_read(iobase, 1) != 0xAA) return 0;

    uint32_t offset = 0;
    while (offset < 1*1024*1024) {
        gf1_dram_write(iobase, offset+0, 0xF0);
        gf1_dram_write(iobase, offset+1, 0x0D);

        if (gf1_dram_read(iobase, offset+0) != 0xF0) break;
        if (gf1_dram_read(iobase, offset+1) != 0x0D) break;

        // else increment offset
        offset += 0x10000;    // 64k bytes
    }

    return offset; // in bytes!
}

uint32_t sndGravisUltrasound::detect(SoundDevice::deviceInfo* info) {

    // clear and fill device info
    this->devinfo.clear();

    if (gusDetect(&this->devinfo, true) == false) return SND_ERR_NOTFOUND;

    // copy info if not NULL
    if (info != NULL) *info = devinfo;

    return SND_ERR_OK;
}

uint32_t sndGravisUltrasound::init(SoundDevice::deviceInfo* info)
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

    // reset codec (THRICE!), check if there is at least 256kb installed
    if ((this->dramsize = gusReset(this->devinfo.iobase)) < GUS_MIN_DRAM_SIZE) return SND_ERR_NOTFOUND;

    isInitialised = true;
    return SND_ERR_OK;
}

uint32_t sndGravisUltrasound::open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void* userdata, soundFormatConverterInfo* conv) {
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

    // allocate DMA buffer (overcommit for anticlick)
    {
        // premultiply bufferSize by bytesPerSample
        bufferSize *= conv->bytesPerSample;
        
        // check for bufsize
        if (bufferSize > devinfo.maxBufferSize) bufferSize = devinfo.maxBufferSize;
        
        // save dma info
        dmaChannel = devinfo.dma;
        dmaBufferCount = 2;
        dmaBufferSize = bufferSize;
        dmaBlockSize = dmaBufferSize * 2;
        dmaCurrentPtr = dmaRenderPtr = 0;
        dmaBufferSamples = dmaBufferSize / conv->bytesPerSample;
        dmaBlockSamples  = dmaBlockSize  / conv->bytesPerSample;

        // allocate DMA buffer
        if (dmaBlock.ptr != NULL) if (dmaFree(&dmaBlock) == false) return SND_ERR_MEMALLOC;
        if (dmaAlloc(dmaBlockSize + 128, &dmaBlock) == false) return SND_ERR_MEMALLOC;
        
        // lock DPMI memory for buffer
        dpmi_lockmemory(dmaBlock.ptr, dmaBlockSize+128+64);
    }

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

uint32_t sndGravisUltrasound::done() {
    if (isOpened) close();

    isInitialised = false;
    return SND_ERR_OK;
}

uint32_t sndGravisUltrasound::close() {
    // stop playback
    if (isPlaying) stop();

    // stop DMA
    if (dmaChannel != -1) dmaStop(dmaChannel);
    
    // deallocate DMA block
    dmaBufferFree();

    // unhook irq if hooked
    if (irq.hooked) irqUnhook(&irq, false);

    // reset GF1 (optinal but just in cause)
    if ((this->dramsize = gusReset(this->devinfo.iobase)) < GUS_MIN_DRAM_SIZE) return SND_ERR_NOTFOUND;

    // fill with defaults
    isOpened = isPlaying = false;
    currentPos = irqs = 0;
    dmaChannel = dmaBlockSize = dmaBufferCount = dmaBufferSize = dmaBufferSamples = dmaBlockSamples = dmaCurrentPtr = dmaRenderPtr = 0;

    return SND_ERR_OK;
}

uint32_t sndGravisUltrasound::resume() {
    // resume playback
    gf1_writew(devinfo.iobase, 0, GF1_REG_CHAN_FREQ, pitch);
    gf1_writew(devinfo.iobase, 1, GF1_REG_CHAN_FREQ, pitch);

    isPaused = false;
    return SND_ERR_OK;
}

uint32_t sndGravisUltrasound::pause() {
    // pause playback
    // TODO: pausing at loop end boundary may result in voice IRQ storm!
    gf1_writew(devinfo.iobase, 0, GF1_REG_CHAN_FREQ, 0);
    gf1_writew(devinfo.iobase, 1, GF1_REG_CHAN_FREQ, 0);

    isPaused = true;
    return SND_ERR_OK;
}

uint32_t sndGravisUltrasound::ioctl(uint32_t function, void* data, uint32_t len)
{
    return SND_ERR_UNSUPPORTED;
}

uint8_t sndGravisUltrasound::getDMACtrl(soundFormat format) {
    uint8_t dmactrl = 0;
    if (format & SND_FMT_UNSIGNED) {
        dmactrl |= (1 << 7);
        if (format & SND_FMT_INT16) {
            dmactrl |= (1 << 6);
        }
    }
    return dmactrl;
}

uint32_t sndGravisUltrasound::convertAndUpload(uint32_t samples, uint32_t gusofs, bool first) {
    // TODO: mono only!!!!!!!!!!
    uint32_t bytes_to_render = samples * convinfo.bytesPerSample;

    // copy overflow samples
    if (!first)
        memcpy( conv_buf_ptr[0] - convinfo.bytesPerSample,
                conv_buf_ptr[0] + sample_bytes_to_render_last - convinfo.bytesPerSample,
                convinfo.bytesPerSample);
    
    // call callback to fill buffer with sound data
    {
        if (callback == NULL) return SND_ERR_NULLPTR;
        soundDeviceCallbackResult rtn = callback(userdata, conv_buf_ptr[0], samples, &convinfo, renderPos); // fill entire buffer
        switch (rtn) {
            case callbackOk         : break;
            case callbackSkip       : 
            case callbackComplete   : 
            case callbackAbort      : 
            default : return SND_ERR_NO_DATA;
        }
        renderPos += samples;
        sample_bytes_to_render_last = bytes_to_render;
    }

    // setup and start GF1 DMA transfer
    dmaSetup(dmaChannel, &dmaBlock, bytes_to_render + convinfo.bytesPerSample, dmaModeRead | dmaModeSingle | dmaModeNoAutoInit, dmablk_offset[0]);
    uint8_t dmactrl = this->dmactrl | (dmaChannel >= 4 ? 4 : 0);
    gf1_write (devinfo.iobase, -1, 0x41, dmactrl);
    gf1_writew(devinfo.iobase, -1, 0x42, (channel_offset[0] + gusofs) >> (dmaChannel >= 4 ? 5 : 4));
    gf1_write (devinfo.iobase, -1, 0x41, dmactrl | 1);   // start DMA transfer!

    return SND_ERR_OK;
}

uint32_t sndGravisUltrasound::start() {
    uint32_t rtn = SND_ERR_OK;
    if ((rtn = prefill()) != SND_ERR_OK) return rtn;

    // check if 16 bit transfer
    dmaChannel = devinfo.dma;
    
    // clear GUS interrupts
    gf1_clear_irq(devinfo.iobase);
    
    // enable GF1 voice interrupts
    gf1_write(devinfo.iobase, -1, GF1_REG_RESET, 7);

    // set 14 channels running
    gf1_write(0, -1, 0xE, 0xC0 | (14 - 1));

    // clear channels
    gf1_clear_channels(devinfo.iobase);

    // get DMA control
    dmactrl = getDMACtrl(convinfo.format);

    // get channel pitch
    pitch = (convinfo.sampleRate << 10) / 44100;
    pitch = (pitch + 1) & ~1;   // fixup for LSB forced to 0

    // fill internal GUS driver info
    channel_offset[0]           = 0;
    channel_offset[1]           = dmaBlockSize * 2;    // leave enough room for anticlick samples
    conv_buf_ptr[0]             = (uint8_t*)dmaBlock.ptr + 16;
    conv_buf_ptr[1]             = (uint8_t*)dmaBlock.ptr + 16 + dmaBufferSize + 16;
    dmablk_offset[0]            = (conv_buf_ptr[0] - (uint8_t*)dmaBlock.ptr) - convinfo.bytesPerSample;
    dmablk_offset[1]            = (conv_buf_ptr[1] - (uint8_t*)dmaBlock.ptr) - convinfo.bytesPerSample;
    sample_bytes_to_render_last = 0;
    buffer_half = 0;

    // set channels for PCM playback
    gf1_setup_playback(devinfo.iobase, 0, channel_offset[0], dmaBufferSamples, convinfo.format & SND_FMT_INT16);
    gf1_setup_playback(devinfo.iobase, 1, channel_offset[1], dmaBufferSamples, convinfo.format & SND_FMT_INT16);
    gf1_pcm_set_rollover(devinfo.iobase, 0, true);
    gf1_pcm_set_rollover(devinfo.iobase, 1, true);

    // set volume/panning
    if (convinfo.format & SND_FMT_STEREO) {
        gf1_write(devinfo.iobase, 0, GF1_REG_CHAN_RAMP_END, 0xF0);  // volume ramp target
        gf1_write(devinfo.iobase, 1, GF1_REG_CHAN_RAMP_END, 0xF0);  // volume ramp target
        gf1_write(devinfo.iobase, 0, GF1_REG_CHAN_PAN, 0);          // left
        gf1_write(devinfo.iobase, 1, GF1_REG_CHAN_PAN, 15);         // right
    } else {
        gf1_write(devinfo.iobase, 0, GF1_REG_CHAN_RAMP_END, 0xFF);
        gf1_write(devinfo.iobase, 0, GF1_REG_CHAN_PAN, 7);          // center (kind of)
    }

    // upload first two blocks of sound data
    convertAndUpload(dmaBlockSamples, 0, true);

    // reset vars
    currentPos = irqs = dmaCurrentPtr = 0; dmaRenderPtr = dmaBufferSize;

    // start PCM playback!
    gf1_pcm_start(devinfo.iobase, 0, true);
    if (convinfo.format & SND_FMT_STEREO) gf1_pcm_start(devinfo.iobase, 1, false);
    // synchronoulsy put channels on play
    gf1_writew(devinfo.iobase, 0, GF1_REG_CHAN_FREQ, pitch);
    gf1_writew(devinfo.iobase, 1, GF1_REG_CHAN_FREQ, pitch);

    // done! we're playing sound :)
    isPaused = false; isPlaying = true;

    return SND_ERR_OK;
}

uint32_t sndGravisUltrasound::stop() {
    if (isPlaying) {
        // stop channel playback
        gf1_pcm_stop(devinfo.iobase, 0);
        gf1_pcm_stop(devinfo.iobase, 1);
    }

    isPlaying = false;

    // clear playing position
    currentPos = renderPos = irqs = 0;
    dmaCurrentPtr = dmaRenderPtr = 0;

    return SND_ERR_OK;
}

// returns play position in bytes
int32_t sndGravisUltrasound::getPlayPos() {
    uint32_t high = 0;
    uint32_t low  = 0;
    do {
        high = gf1_readw(devinfo.iobase, 0, 0x8A);
        low = gf1_readw(devinfo.iobase, 0, 0x8B);
    } while (high != gf1_readw(devinfo.iobase, 0, 0x8A));
    
    int32_t pos = ((high << 16) | low) >> 9;
    return pos * convinfo.bytesPerSample;
}

#if 0
int64_t sndGravisUltrasound::getPos() {
    uint64_t totalPos;

    // anti-rewind stuff removed since GF1 is probably not prone of it (unlike i8237!)
    int32_t playpos = getPlayPos() / convinfo.bytesPerSample;
    int32_t dmaptr  = dmaCurrentPtr;
    totalPos = currentPos + (playpos < dmaptr ? dmaBlockSamples + playpos - dmaptr : playpos - dmaptr);

    if ((totalPos < oldTotalPos)) return oldTotalPos;   // give up but let the counter stop
    oldTotalPos = totalPos;

    return totalPos;
}

void sndGravisUltrasound::irqAdvancePos() {
    // unlike i8237, GF1 is predictable enough to not have any of these annoying position quirks
    int32_t playPos   = getPlayPos();   // in bytes!
    int32_t playIdx   = (playPos) / dmaBufferSize;
    if (playIdx >= dmaBufferCount) playIdx -= dmaBufferCount;
    dmaRenderPtr = (playIdx + 1) * dmaBufferSize;
    if (dmaRenderPtr >= dmaBlockSize) dmaRenderPtr = 0;
    renderPos += dmaBufferSamples;

    // adjust play position
    int32_t playpos = playPos / convinfo.bytesPerSample;
    currentPos += (playpos < dmaCurrentPtr ? dmaBlockSamples + playpos - dmaCurrentPtr : playpos - dmaCurrentPtr);
    dmaCurrentPtr = playpos;
}
#endif

static void sndlib_swapStacks();
#pragma aux sndlib_swapStacks = \
    " mov     dword ptr   [snddev_pm_old_stack + 4], ss   " \
    " mov     dword ptr   [snddev_pm_old_stack + 0], esp  " \
    " lss     esp, [snddev_pm_stack_top] "

static void sndlib_restoreStack();
#pragma aux sndlib_restoreStack = \
    " lss     esp, [snddev_pm_old_stack] "

// irq procedure
bool sndGravisUltrasound::irqProc() {
    // get GUS interrupt status
    int irqstatus = inp(devinfo.iobase + 6);
    if (irqstatus == 0) return true;        // chain to previous ISR

    // acknowledge interrupt
    outp(irq.info->picbase, 0x20); if (irq.info->flags & IRQ_SECONDARYPIC) outp(0x20, 0x20);

    // save GF1 chan/reg index
    uint16_t regIndex = inpw(devinfo.iobase + 2);

    // check IRQ reason
    if (irqstatus & (1 << 5)) {
        // voice interrupt

        // advance play pointers
        irqAdvancePos();

        // clear IRQ (TODO: proper handling)
        gf1_read(devinfo.iobase, -1, 0x8F);

        // switch rollover and loop point
        buffer_half = dmaRenderPtr >= dmaBufferSize ? 0 : 1;
        uint32_t pcmloop = (channel_offset[0] + (dmaBufferSamples << (buffer_half))) << 9;
        gf1_writew(devinfo.iobase, 0, 0x4, pcmloop >> 16);
        gf1_writew(devinfo.iobase, 0, 0x5, pcmloop & 0xFFFF);
        gf1_pcm_set_rollover(devinfo.iobase, 0, buffer_half == 0);

#if 1
        // STACK POPIERDOLOLO
        if (snddev_pm_stack_in_use == 0) {
            snddev_pm_stack_in_use++;

            // work around possible SS:EBP addressing, as we're switching stacks now
            static sndGravisUltrasound *staticSelf;
            static uint32_t samplesToRender;
            static uint32_t gusDst;
            staticSelf = this;
            samplesToRender = dmaBufferSamples;
            gusDst = (buffer_half ? 0 : dmaBufferSize);

            // enable interrupts
            _enable();

            // switch stack
            sndlib_swapStacks();

            // render more sound data
            staticSelf->convertAndUpload(samplesToRender, gusDst, false);

            // switch back
            sndlib_restoreStack();

            // and disable interrupts again
            _disable();

            snddev_pm_old_stack = NULL;
            snddev_pm_stack_in_use--;
        }
#endif
    }
    if (irqstatus & (1 << 7)) {
        // DMA TC interrupt
        // clear it
        gf1_write(devinfo.iobase, -1, 0x41, gf1_read(devinfo.iobase, -1, 0x41) & ~1);
    }
    
    // restore GF1 chan/reg index
    outpw(devinfo.iobase + 2, regIndex);

    return false;   // we're handling EOI by itself
}


#endif