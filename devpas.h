#pragma once

// --wbcbz7 l9o52o22

// Pro Audio Spectrum original/Plus/16 driver

#include <stdint.h>
#include "snddev.h"
#include "convert.h"
#include "sndmisc.h"

#include "cli.h"
#include "irq.h"
#include "dma.h"
#include "sndioctl.h"


enum {
    PAS_REG_AUDIOMIXR    =    0x0B88 - 0x388,        // Audio Mixer Control Register
    PAS_REG_AUDIOFILT    =    0x0B8A - 0x388,        // Audio Filter Control Register
    PAS_REG_INTRCTLRST   =    0x0B89 - 0x388,        // Interrupt Control Status Register
    PAS_REG_INTRCTLR     =    0x0B8B - 0x388,        // Interrupt Control Register write
    PAS_REG_INTRCTLRRB   =    0x0B8B - 0x388,        // Interrupt Control Register read back
    PAS_REG_PCMDATA      =    0x0F88 - 0x388,        // PCM data I/O register (low byte)
    PAS_REG_PCMDATAH     =    0x0F89 - 0x388,        // PCM data I/O register (high byte)
    PAS_REG_CROSSCHANNEL =    0x0F8A - 0x388,        // Cross Channel Register
    PAS_REG_SAMPLERATE   =    0x1388 - 0x388,        // (t0) Sample Rate Timer Register
    PAS_REG_SAMPLECNT    =    0x1389 - 0x388,        // (t1) Sample Count Register
    PAS_REG_SPKRTMR      =    0x138A - 0x388,        // (t2) Local Speaker Timer Address
    PAS_REG_TMRCTLR      =    0x138B - 0x388,        // Local Timer Control Register

    // hardware specific equates for the MV101 (digital ASIC)

    PAS_REG_MASTERADDRP  =    0x9a01 - 0x388,        // Master Address Pointer      (w)
    PAS_REG_MASTERCHIPR  =    0xff88 - 0x388,        // Master Chip Rev             (r)
    PAS_REG_SLAVECHIPR   =    0xef88 - 0x388,        // Slave Chip Rev              (r)
    PAS_REG_SYSCONFIG1   =    0x8388 - 0x388,        // System Config 1           (r/w)
    PAS_REG_SYSCONFIG2   =    0x8389 - 0x388,        // System Config 2           (r/w)
    PAS_REG_SYSCONFIG3   =    0x838a - 0x388,        // System Config 3           (r/w)
    PAS_REG_SYSCONFIG4   =    0x838b - 0x388,        // System Config 4           (r/w)
    PAS_REG_IOCONFIG1    =    0xf388 - 0x388,        // I/O Config 1              (r/w)
    PAS_REG_IOCONFIG2    =    0xf389 - 0x388,        // I/O Config 2              (r/w)
    PAS_REG_IOCONFIG3    =    0xf38a - 0x388,        // I/O Config 3              (r/w)
    PAS_REG_IOCONFIG4    =    0xf38b - 0x388,        // I/O Config 4              (r/w)
    PAS_REG_COMPATREGE   =    0xf788 - 0x388,        // Compatible Rgister Enable (r/w)
    PAS_REG_EMULADDRP    =    0xf789 - 0x388,        // Emulation Address Pointer (r/w)
    PAS_REG_WAITSTATE    =    0xbf88 - 0x388,        // Wait State                (r/w)
    PAS_REG_MASTERMODRD  =    0xff8b - 0x388,        // Master Mode Read            (r)
    PAS_REG_SLAVEMODRD   =    0xef8b - 0x388,        // Slave Mode Read             (r)
};

enum {
    PAS_FEATURELEVEL_ORIGINAL = 0,      // original PAS (8 bit stereo max)
    PAS_FEATURELEVEL_PLUS     = 1,      // +16bit playback over 8-bit DMA?
    PAS_FEATURELEVEL_PAS16    = 2,      // +16bit playback over 8/16-bit DMA
};

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
    sndProAudioSpectrum() : IsaDmaDevice("Pro Audio Spectrum") {
        memset(&localShadow, 0, sizeof(localShadow));
        shadowPtr = &localShadow;

        featureLevel = PAS_FEATURELEVEL_ORIGINAL;
        isMvSoundPresent = false;
        
        devinfo.name = getName();
        devinfo.version = NULL;
        devinfo.maxBufferSize = 32768;  // BYTES

        // inherit caps from SB2.0 non-highspeed (changed at detect)
        devinfo.caps = NULL;
        devinfo.capsLen = 0;
        devinfo.flags = 0;
    }
    virtual ~sndProAudioSpectrum() {

    }

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
    //virtual uint64_t getPos();

    // ioctl
    virtual uint32_t ioctl(uint32_t function, void* data, uint32_t len);

    // stop playback
    virtual uint32_t stop();

    // deinit all this shit
    virtual uint32_t done();

protected:

    // detection flags
    uint32_t            featureLevel;
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