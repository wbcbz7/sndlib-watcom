#pragma once

// Pro Audio Spectrum original/Plus/16 driver
// --wbcbz7 l9o52o22

#include "snddefs.h"
#ifdef SNDLIB_DEVICE_ENABLE_PAS

#include <stdint.h>
#include "snddev.h"
#include "convert.h"
#include "sndmisc.h"

#include "cli.h"
#include "irq.h"
#include "dma.h"
#include "sndioctl.h"

// --------------------- MVSOUND.SYS related stuff ---------------------    
struct MVSoundShadowRegisters {
    uint8_t  _sysspkrtmr;   /*   42 System Speaker Timer Address */
    uint8_t  _systmrctlr;   /*   43 System Timer Control	    */
    uint8_t  _sysspkrreg;   /*   61 System Speaker Register	    */
    uint8_t  _joystick;     /*  201 Joystick Register	    */
    uint8_t  _lfmaddr;      /*  388 Left  FM Synth Address	    */
    uint8_t  _lfmdata;      /*  389 Left  FM Synth Data	    */
    uint8_t  _rfmaddr;      /*  38A Right FM Synth Address	    */
    uint8_t  _rfmdata;      /*  38B Right FM Synth Data	    */
    uint8_t  _dfmaddr;      /*  788 Dual  FM Synth Address	    */
    uint8_t  _dfmdata;      /*  789 Dual  FM Synth Data	    */
    uint8_t  _RESRVD1[1];   /*      reserved 		    */
    uint8_t  _paudiomixr;   /*  78B Paralllel Audio Mixer Control*/
    uint8_t  _audiomixr;    /*  B88 Audio Mixer Control          */
    uint8_t  _intrctlrst;   /*  B89 Interrupt Status 	    */
    uint8_t  _audiofilt;    /*  B8A Audio Filter Control	    */
    uint8_t  _intrctlr;     /*  B8B Interrupt Control	    */
    uint8_t  _pcmdata;      /*  F88 PCM Data I/O Register	    */
    uint8_t  _RESRVD2;      /*      reserved 		    */
    uint8_t  _crosschannel; /*  F8A Cross Channel		    */
    uint8_t  _RESRVD3;      /*      reserved 		    */
    uint16_t _samplerate;   /* 1388 Sample Rate Timer	    */
    uint16_t _samplecnt;    /* 1389 Sample Count Register	    */
    uint16_t _spkrtmr;      /* 138A Shadow Speaker Timer Count   */
    uint8_t  _tmrctlr;      /* 138B Shadow Speaker Timer Control */
    uint8_t  _mdirqvect;    /* 1788 MIDI IRQ Vector Register     */
    uint8_t  _mdsysctlr;    /* 1789 MIDI System Control Register */
    uint8_t  _mdsysstat;    /* 178A MIDI IRQ Status Register     */
    uint8_t  _mdirqclr;     /* 178B MIDI IRQ Clear Register	    */
    uint8_t  _mdgroup1;     /* 1B88 MIDI Group #1 Register	    */
    uint8_t  _mdgroup2;     /* 1B89 MIDI Group #2 Register	    */
    uint8_t  _mdgroup3;     /* 1B8A MIDI Group #3 Register	    */
    uint8_t  _mdgroup4;     /* 1B8B MIDI Group #4 Register	    */
};

// base PAS driver
class sndProAudioSpectrum : public IsaDmaDevice {

public:
    // constructor (nothing fancy here)
    sndProAudioSpectrum();

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

    // deinit all this shit
    virtual uint32_t done();

protected:

    // detection flags
    uint32_t            featureLevel;
    uint32_t            timeConstant;
    bool                isMvSoundPresent;

    // detect PAS presence, fill deviceInfo, returns 1 if success
    bool                pasDetect(SoundDevice::deviceInfo* info, bool manualDetect = false);

    // reset codec
    virtual bool        pasReset(SoundDevice::deviceInfo* info);
    
    // fill info according to DSP version
    virtual uint32_t    fillCodecInfo(SoundDevice::deviceInfo* info);

    // --------------------------- IRQ stuff --------------------

    virtual bool    irqProc();

    // --------------------- MVSOUND.SYS related stuff ---------------------    

    MVSoundShadowRegisters *shadowPtr;
    MVSoundShadowRegisters  localShadow;

};

#endif
