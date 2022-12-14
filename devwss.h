#pragma once

// Windows Sound System and GUS MAX/PnP codecs driver
// --wbcbz7 oloz7e5

#include "snddefs.h"
#ifdef SNDLIB_DEVICE_ENABLE_WSS

#include <stdint.h>
#include "snddev.h"
#include "convert.h"
#include "sndmisc.h"

#include "cli.h"
#include "irq.h"
#include "dma.h"
#include "sndioctl.h"

// WSS ioctl definitions
enum {
    SND_IOCTL_WSS_64KHZ_GET = SND_IOCTL_DEVICE_SPECIFIC,
    SND_IOCTL_WSS_64KHZ_SET,

    SND_IOCTL_WSS_VARIABLE_RATE_GET,
    SND_IOCTL_WSS_VARIABLE_RATE_SET,

    SND_IOCTL_WSS_FEATURELEVEL_GET,
    SND_IOCTL_WSS_FEATURELEVEL_SET,
};

enum {
    WSS_INDEX_INIT_STATE = (1 << 7),
    WSS_INDEX_MODE_CHANGE = (1 << 6),
    WSS_INDEX_TRD = (1 << 5),
};

enum {
    WSS_PORT_INDEX = 0,
    WSS_PORT_DATA,
    WSS_PORT_STATUS,
    WSS_PORT_PIO_DATA
};

enum {
    WSS_REG_LEFT_INPUT_CTRL = 0,
    WSS_REG_RIGHT_INPUT_CTRL,
    WSS_REG_LEFT_AUX0_CTRL,
    WSS_REG_RIGHT_AUX0_CTRL,
    WSS_REG_LEFT_AUX1_CTRL,
    WSS_REG_RIGHT_AUX1_CTRL,
    WSS_REG_LEFT_DAC_CTRL,
    WSS_REG_RIGHT_DAC_CTRL,
    WSS_REG_CLK_DATA_FORMAT,
    WSS_REG_INTERFACE_CONFIG,
    WSS_REG_PIN_CTRL,
    WSS_REG_TEST_INIT,
    WSS_REG_MODE_ID,
    WSS_REG_DIGITAL_MIX_CTRL,
    WSS_REG_DMA_COUNT_HIGH,
    WSS_REG_DMA_COUNT_LOW,

    // CS4231+ extended registers
    WSS_REG_ALT_FEATURE1,
    WSS_REG_ALT_FEATURE2,
    WSS_REG_LEFT_LINE_CTL,
    WSS_REG_RIGHT_LINET_CTL,
    WSS_REG_TIMER_HIGH,
    WSS_REG_TIMER_LOW,
    WSS_REG_ALTERNATE_SAMPLE_RATE,          // WSS_FEATURE_CS4236 only
    WSS_REG_ALT_FEATURE3 = 23,              // WSS_FEATURE_CS4231 only     
    WSS_REG_EXT_INDEX = 23,                 // WSS_FEATURE_CS4236 only
    WSS_REG_ALT_FEATURE_STATUS,
    WSS_REG_EXTENDED_ID,
    WSS_REG_MONO_CTRL,
    WSS_REG_LEFT_MASTER_OUT_CTRL,           // WSS_FEATURE_CS4236 only
    WSS_REG_CAPTURE_FORMAT,
    WSS_REG_RIGHT_MASTER_OUT_CTRL,          // WSS_FEATURE_CS4236 only
    WSS_REG_CAPTURE_COUNT_HIGH,
    WSS_REG_CAPTURE_COUNT_LOW,

    // AMD Interwave registers fixups
    IW_REG_LEFT_OUT_ATTENUATION = 25,
    IW_REG_RIGHT_OUT_ATTENUATION = 27,
    IW_REG_PLAYBACK_SAMPLE_RATE = 29,
};

enum {
    WSS_FEATURE_AD1848,             // 16 registers,         mode 1 only, half-duplex?
    WSS_FEATURE_CS4231,             // 32 registers,         mode 1/2, full-duplex
    WSS_FEATURE_CS4236,             // 32 + extra registers, mode 1/2/3, full-duplex, variable rate
    WSS_FEATURE_INTERWAVE,          // like CS4231 non-A,    mode 1/2 + own mode3, full-duplex, variable rate (22/32k limited)
};


// base WSS driver
class sndWindowsSoundSystem : public IsaDmaDevice {
    
public:
    // constructor (nothing fancy here)
    sndWindowsSoundSystem();

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
    //virtual int64_t  getPos();

    // ioctl
    virtual uint32_t ioctl(uint32_t function, void* data, uint32_t len);

    // stop playback
    virtual uint32_t stop();

    // close 
    virtual uint32_t close();

    // deinit device
    virtual uint32_t done();

protected:
    
    bool            isGus;                      // is GUS?
    bool            is64khz;
    bool            isVariableSampleRate;       // few codecs support this, i.e later Crystal codecs and AMD Interwave

    // detect WSS presence, fill deviceInfo, returns 1 if success
    bool            wssDetect(SoundDevice::deviceInfo* info, bool manualDetect = false);

    // reset codec
    virtual bool     wssReset(SoundDevice::deviceInfo* info, bool isGus);

    // set sample rate and format
    virtual uint32_t setFormat(SoundDevice::deviceInfo* info, uint32_t sampleRate, soundFormat format);

    // fill info according to DSP version
    virtual uint32_t fillCodecInfo(SoundDevice::deviceInfo* info);

    // get codec version
    virtual bool getCodecVersion(SoundDevice::deviceInfo* info);

    // probe environment
    bool    readEnvironment(SoundDevice::deviceInfo *info);

    // kickstart WSS to enable probing
    bool    kickstartProbingPlayback(SoundDevice::deviceInfo *info, uint32_t dmaChannel, ::dmaBlock &block, uint32_t probeLength, bool enableIrq);

    // identification info
    uint32_t        featureLevel;               // detected feature level
    uint32_t        oldId;                      // I12 aka WSS_REG_MODE_ID
    uint32_t        newId;                      // I25 aka WSS_REG_EXTENDED_ID
    uint32_t        extId;                      // X25

    // --------------------------- IRQ stuff --------------------

    virtual bool    irqProc();

    // used during IRQ discovery only
    static void __interrupt wssDetectIrqProc();
};

#endif
