#pragma once

#include "snddefs.h"
#if (defined(SNDLIB_DEVICE_ENABLE_SB)   ||  \
     defined(SNDLIB_DEVICE_ENABLE_SB16) ||  \
     defined(SNDLIB_DEVICE_ENABLE_ESS))


// Sound Blaster 1.x/2.x/Pro/16 and ESS AudioDrive driver
// --wbcbz7 oloz7e5

#include <stdint.h>
#include "snddev.h"
#include "convert.h"
#include "sndmisc.h"

#include "cli.h"
#include "irq.h"
#include "dma.h"
#include "sndioctl.h"

// base SB driver
class sndSBBase : public IsaDmaDevice {
public:
    // constructor (nothing fancy here)
    sndSBBase(const char *name);
    
    // get device name
    virtual const char *getName();
    
    /*
    // detect (0 if found + if (res != NULL) *res filled with current config)
    virtual uint32_t detect(SoundDevice::deviceInfo *info = NULL);
    */
    // select and init (use supplied *res info if (res != NULL))
    virtual uint32_t init(SoundDevice::deviceInfo *info = NULL);
    /*
    // check if format is supported (0 if supported + passes pointer to converter procedure)
    virtual uint32_t isFormatSupported(uint32_t sampleRate, soundFormat *fmt, soundFormatConverterInfo **conv);
    
    // return converter for current format
    soundFormatConverterInfo getFormatConverter(soundFormat *fmt);
    
    // init for playback
    virtual uint32_t open(uint32_t sampleRate, soundFormat *fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv);
    
    // start playback (won't return immediately, calls callback to fill DMA buffer)
    virtual uint32_t start();
    
    // pause playback (start() for resume)
    virtual uint32_t pause();
    */

    // resume playback
    //virtual uint32_t resume();

    // get playback position in samples
    //virtual uint64_t getPos();
    
    // ioctl
    virtual uint32_t ioctl(uint32_t function, void *data, uint32_t len);
    
    // stop playback
    //virtual uint32_t stop();
    
    // close playback
    virtual uint32_t close();

    // deinit device
    virtual uint32_t done();

protected:

    // DSP version
    uint32_t            dspVersion;

    // SB base common open functions
    uint32_t            openCommon(uint32_t sampleRate, soundFormat fmt, soundFormat newFormat, uint32_t bufferSize, soundDeviceCallback callback, void* userdata, soundFormatConverterInfo* conv);
    
    // detect SB presence, fill deviceInfo, returns dsp version
    uint32_t            sbDetect(SoundDevice::deviceInfo *info, bool manualDetect = false);
    
    // detect and init extended features (SB16?/ESS)
    virtual uint32_t    detectExt(SoundDevice::deviceInfo* info, uint32_t sbDspVersion);

    // get IRQ/DMA configuration
    virtual uint32_t    fillIrqDma(SoundDevice::deviceInfo* info, uint32_t sbDspVersion);

    // fill info according to DSP version
    virtual uint32_t    fillDspInfo(SoundDevice::deviceInfo *info, uint32_t sbDspVersion);
    
    // convert DSP version to string
    virtual const char* dspVerToString(SoundDevice::deviceInfo * info, uint32_t sbDspVersion);
    
    // --------------------------- IRQ stuff --------------------
    
    virtual bool    irqProc();
    
    // used during IRQ discovery only
    static void __interrupt sbDetectIrqProc();

};

#ifdef SNDLIB_DEVICE_ENABLE_SB

// SB 1.x/2.x/Pro driver
class sndSoundBlaster2Pro : public sndSBBase {

public:
    // constructor (nothing fancy here)
    sndSoundBlaster2Pro();
    
    // get device name
    //virtual const char *getName();
    
    // detect (0 if found + if (res != NULL) *res filled with current config)
    virtual uint32_t detect(SoundDevice::deviceInfo *info = NULL);
    
    /*
    // select and init (use supplied *res info if (res != NULL))
    virtual uint32_t init(SoundDevice::deviceInfo *info = NULL);
    
    // check if format is supported (0 if supported + passes pointer to converter procedure)
    //virtual uint32_t isFormatSupported(uint32_t sampleRate, soundFormat fmt, soundFormatConverterInfo *conv);
    
    // return converter for current format
    //virtual uint32_t getConverter(soundFormat srcfmt, soundFormat dstfmt, soundFormatConverterInfo *conv);
    */
    
    // init for playback
    virtual uint32_t open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv);    
    
    // start playback (won't return immediately, calls callback to fill DMA buffer)
    virtual uint32_t start();
    
    // pause playback (start() for resume)
    virtual uint32_t pause();
    
    // resume playback
    virtual uint32_t resume();

    // get playback position in samples
    //virtual uint64_t getPos();
    
    // ioctl
    virtual uint32_t ioctl(uint32_t function, void *data, uint32_t len);
    
    // stop playback
    virtual uint32_t stop();
    
    // deinit device
    //virtual uint32_t done();
    
private:

    enum playbackType {
        SingleCycle = 0,        // SB 1.x only
        AutoInit,               // SB 2.00+/Pro up to 11m/22s
        HighSpeed               // SB 2.01+/Pro above 11m/22s
    };

    // is highspeed? flag
    playbackType    playbackType;
    
    // fill info according to DSP version
    virtual uint32_t fillDspInfo(SoundDevice::deviceInfo *info, uint32_t sbDspVersion);

    virtual bool    irqProc();
};

#endif

#ifdef SNDLIB_DEVICE_ENABLE_SB16

// SB16 driver
class sndSoundBlaster16 : public sndSBBase {
    
public:
    // constructor (nothing fancy here)
    sndSoundBlaster16();
    
    // get device name
    //virtual const char *getName();
    
    // detect (0 if found + if (res != NULL) *res filled with current config)
    virtual uint32_t detect(SoundDevice::deviceInfo *info = NULL);
    
    /*
    // select and init (use supplied *res info if (res != NULL))
    virtual uint32_t init(SoundDevice::deviceInfo *info = NULL);
    
    // check if format is supported (0 if supported + passes pointer to converter procedure)
    virtual uint32_t isFormatSupported(uint32_t sampleRate, soundFormat fmt, soundFormatConverterInfo *conv);
    
    // return converter for current format
    uint32_t getConverter(soundFormat srcfmt, soundFormat dstfmt, soundFormatConverterInfo *conv);
    */

    // init for playback
    virtual uint32_t open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv);
    
    // start playback (won't return immediately, calls callback to fill DMA buffer)
    virtual uint32_t start();
    
    // pause playback (start() for resume)
    virtual uint32_t pause();
    
    // resume playback
    virtual uint32_t resume();

    // ioctl
    virtual uint32_t ioctl(uint32_t function, void *data, uint32_t len);
    
    // stop playback
    virtual uint32_t stop();
    
    // deinit all this shit
    //virtual uint32_t done();
    
private:
    
    bool            is16Bit;

    // fill info according to DSP version
    virtual uint32_t fillDspInfo(SoundDevice::deviceInfo *info, uint32_t sbDspVersion);

    // get IRQ/DMA configuration
    virtual uint32_t fillIrqDma(SoundDevice::deviceInfo* info, uint32_t sbDspVersion);

    virtual bool    irqProc();

    // get start command
    virtual uint32_t getStartCommand(soundFormatConverterInfo &conv);
};

#endif

#ifdef SNDLIB_DEVICE_ENABLE_ESS

// ESS ioctl definitions
enum {
    SND_IOCTL_ESS_DEMANDMODE_GET = SND_IOCTL_DEVICE_SPECIFIC,
    SND_IOCTL_ESS_DEMANDMODE_SET,
};

// ESS AudioDrive driver
class sndESSAudioDrive : public sndSBBase {

public:
    // constructor (nothing fancy here)
    sndESSAudioDrive();

    // get device name
    //virtual const char* getName();

    // detect (0 if found + if (res != NULL) *res filled with current config)
    virtual uint32_t detect(SoundDevice::deviceInfo* info = NULL);

    /*
    // select and init (use supplied *res info if (res != NULL))
    virtual uint32_t init(SoundDevice::deviceInfo* info = NULL);

    // check if format is supported (0 if supported + passes pointer to converter procedure)
    virtual uint32_t isFormatSupported(uint32_t sampleRate, soundFormat fmt, soundFormatConverterInfo *conv);

    // return converter for current format
    uint32_t getConverter(soundFormat srcfmt, soundFormat dstfmt, soundFormatConverterInfo *conv);
    */

    // init for playback
    virtual uint32_t open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void* userdata, soundFormatConverterInfo* conv);

    // start playback (won't return immediately, calls callback to fill DMA buffer)
    virtual uint32_t start();

    // pause playback (start() for resume)
    virtual uint32_t pause();

    // resume playback
    virtual uint32_t resume();

    // ioctl
    virtual uint32_t ioctl(uint32_t function, void* data, uint32_t len);

    // stop playback
    virtual uint32_t stop();

    // deinit all this shit
    //virtual uint32_t done();

private:

    uint32_t        modelId;            // read by DSP command 0xE7
    uint32_t        modelNumber;        // e.g 688/1868/1869

    // fill info according to DSP version
    virtual uint32_t fillDspInfo(SoundDevice::deviceInfo* info, uint32_t sbDspVersion);

    // detect and init extended features (SB16?/ESS)
    virtual uint32_t detectExt(SoundDevice::deviceInfo* info, uint32_t sbDspVersion);

    // get IRQ/DMA configuration
    virtual uint32_t fillIrqDma(SoundDevice::deviceInfo* info, uint32_t sbDspVersion);

    // convert DSP version to string
    virtual const char* dspVerToString(SoundDevice::deviceInfo * info, uint32_t sbDspVersion);

    virtual bool    irqProc();

    // enable demand mode
    bool            demandModeEnable;
    uint32_t        demandModeBurstLength;
};

#endif

#endif
