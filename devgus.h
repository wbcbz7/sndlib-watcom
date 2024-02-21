#pragma once

// Gravis Ultrasound (GF1) driver
// --wbcbz7 21o27e8

#include "snddefs.h"
#ifdef SNDLIB_DEVICE_ENABLE_GUS

#include <stdint.h>
#include "snddev.h"
#include "convert.h"
#include "sndmisc.h"

#include "cli.h"
#include "irq.h"
#include "dma.h"
#include "sndioctl.h"

// register defines
enum {
    GF1_REG_DMA_CTRL                = 0x41, 
    GF1_REG_DMA_ADDR                = 0x42,
    GF1_REG_DRAM_ADDR_LOW           = 0x43,
    GF1_REG_DRAM_ADDR_HIGH          = 0x44,
    GF1_REG_TIMER_CTRL              = 0x45,
    GF1_REG_TIMER1_COUNT            = 0x46,
    GF1_REG_TIMER2_COUNT            = 0x47,
    GF1_REG_SAMPLING_FREQ           = 0x48,
    GF1_REG_SAMPLING_CTRL           = 0x49,
    GF1_REG_JOY_TRIMDAC             = 0x4B,
    GF1_REG_RESET                   = 0x4C,

    GF1_REG_CHAN_READ               = 0x80,

    GF1_REG_CHAN_CTRL               = 0x00,
    GF1_REG_CHAN_FREQ               = 0x01,
    GF1_REG_CHAN_START_HIGH         = 0x02,
    GF1_REG_CHAN_START_LOW          = 0x03,
    GF1_REG_CHAN_END_HIGH           = 0x04,
    GF1_REG_CHAN_END_LOW            = 0x05,
    GF1_REG_CHAN_RAMP_RATE          = 0x06,
    GF1_REG_CHAN_RAMP_START         = 0x07,
    GF1_REG_CHAN_RAMP_END           = 0x08,
    GF1_REG_CHAN_VOLUME             = 0x09,
    GF1_REG_CHAN_POS_HIGH           = 0x0A,
    GF1_REG_CHAN_POS_LOW            = 0x0B,
    GF1_REG_CHAN_PAN                = 0x0C,
    GF1_REG_CHAN_VOL_CTRL           = 0x0D,

    GF1_REG_ACTIVE_CHANS            = 0x0E,
    GF1_REG_IRQ_STATUS              = 0x0F,
};

// TODO: ioctls

// base GUS driver
class sndGravisUltrasound : public IsaDmaDevice {
    
public:
    // constructor (nothing fancy here)
    sndGravisUltrasound();

    // get device name
    // virtual const char    *getName();

    // detect (0 if found + if (res != NULL) *res filled with current config)
    virtual uint32_t detect(SoundDevice::deviceInfo *info = NULL);

    // select and init (use supplied *res info if (res != NULL))
    virtual uint32_t init(SoundDevice::deviceInfo* info = NULL);

    /*
    // check if format is supported (0 if supported + passes pointer to converter procedure)
    virtual uint32_t isFormatSupported(uint32_t sampleRate, soundFormat *fmt, soundFormatConverterInfo **conv);

    // return converter for current format
    soundFormatConverterInfo getFormatConverter(soundFormat *fmt);
    */

    // init for playback
    virtual uint32_t open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void* userdata, soundFormatConverterInfo* conv);

    // start playback (won't return immediately, calls callback to fill DMA buffer)
    virtual uint32_t start();

    // pause playback (start() for resume)
    virtual uint32_t pause();

    // resume playback
    virtual uint32_t resume();
    
    // get playback position in samples
    virtual int64_t  getPos();

    // ioctl
    virtual uint32_t ioctl(uint32_t function, void* data, uint32_t len);

    // stop playback
    virtual uint32_t stop();

    // close 
    virtual uint32_t close();

    // deinit device
    virtual uint32_t done();

protected:
    
    // detect GUS presence, fill deviceInfo, returns 1 if success
    bool             gusDetect(SoundDevice::deviceInfo* info, bool manualDetect = false);

    // reset GF1, get memory size in bytes or 0 if not detected or no memory available
    virtual uint32_t gusReset(uint32_t iobase);

    // get DMA control flags
    virtual uint8_t getDMACtrl(soundFormat format);

    // probe environment
    bool    readEnvironment(SoundDevice::deviceInfo *info);

    // convert and upload block of audio
    virtual uint32_t convertAndUpload(uint32_t samples, uint32_t gusofs, bool first);

    // returns play position within DMA buffer in bytes
    int32_t getPlayPos();

    // ------------------------- playback stuff -----------------

    // GF1 DRAM size in bytes
    uint32_t        dramsize;
    // offset of channel data in GF1 DRAM (in sample frames!)
    uint32_t        channel_offset[2];
    // last rendered sample count (for anticlick)
    uint32_t        sample_bytes_to_render_last;
    // DMA buffer offsets
    uint32_t        dmablk_offset[2];
    // sample pointers for converters
    uint8_t         *conv_buf_ptr[2];
    // channel pitch
    uint16_t        pitch;
    // DMACTRL (GF1 index 0x41) flags for format
    uint8_t         dmactrl;
    // current buffer half
    uint8_t         buffer_half;

    // --------------------------- IRQ stuff --------------------

    virtual bool    irqProc();
};

#endif