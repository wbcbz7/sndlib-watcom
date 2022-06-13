#pragma once

// --wbcbz7 22o52o22

// PC Honker and Covox routines (ugh)

#include <stdint.h>
#include "snddev.h"
#include "convert.h"
#include "sndmisc.h"

#include "cli.h"
#include "irq.h"
#include "dma.h"
#include "dpmi.h"
#include "sndioctl.h"


#pragma pack(push, 1)
// non-DMA sound device IRQ0 structure
struct snddev_irq0_struct {
    _dpmi_rmpointer             chain_rm;    // real      mode IRQ0 old routine
    void __interrupt __far    (*chain_pm)(); // protected mode ----//---
    
    uint16_t        data_port;      // data port
    
    _dpmi_rmpointer rm_callback;    // rm->pm callback routine
    void __near   (*pm_callback)(); // pm     callback routine (NEAR pointer -> same code segment!)
    
    uint32_t        rmcount;        // rm handler invocations
    uint32_t        pmcount;        // pm handler invocations
    
    uint16_t        data_port2;     // data port #2 (for dual covox)
    uint16_t        bufferseg;      // sound buffer segment      (for rm hander) - rounded down to 4k paras (0x0000, 0x1000 up to 0xF000)   ]
    uint8_t*        bufferofs;      // sound buffer linear start (pm handler uses it as-is, rm handler uses only least 16bits + .bufferseg) ] seg:ofs pair for rm handler
    uint32_t        bufferpos;      // current buffer position, relative to buffer start
    uint32_t        bufferlencur;   // current buffer length
    uint32_t        bufferlen;      // buffer length
    uint32_t        bufferlentotal; // total buffer length
    
    uint32_t        chain_acc;      // IRQ0 old routine invocation accumulator

};

// driver patch table
struct snddev_patch_table {
    // real-mode section
    uint8_t              *rm_start;         // code/data start       ] copy and lock range
    uint8_t              *rm_end;           // code/data end         ] 
    void                (*rm_entry)();      // ISR entrypoint

    // protected-mode section
    uint8_t              *pm_start;         // code/data start       ] copy and lock range
    uint8_t              *pm_end;           // code/data end         ] 
    void                (*pm_entry)();      // ISR entrypoint

    // patch table
    uint16_t             *rm_patch_dataseg; // pointer to  WORD snddev_irq0_struct segment
    uint32_t             *rm_patch_dt;      // pointer to DWORD chain_acc increment

    uint32_t             *pm_patch_dataseg; // pointer to DWORD flat 4G data selector
    snddev_irq0_struct  **pm_patch_dataofs; // pointer to DWORD snddev_irq0_struct linear offset
    uint32_t             *pm_patch_dt;      // pointer to DWORD chain_acc increment
};
#pragma pack(pop)

extern "C" {
    // driver patch table
    extern snddev_patch_table   snddev_irq0_patch_pcspeaker;    // PC Speaker/Covox mono
    extern snddev_patch_table   snddev_irq0_patch_stereo1;      // Stereo-On-1
    extern snddev_patch_table   snddev_irq0_patch_stereo1_fast; // Stereo-On-1, fast protocol (from FT2)
    extern snddev_patch_table   snddev_irq0_patch_dualcovox;    // Dual Covox
    
};

// base non-DMA device driver
class sndNonDmaBase : public DmaBufferDevice {
    
public:
    // constructor (nothing fancy here)
    sndNonDmaBase(const char *name) : DmaBufferDevice(name) {
        isDetected = isOpened = isIrq0Initialised = isInitialised = isPlaying = isPaused = false;
        timerDivisor = 0;

        dmaBlock.ptr = NULL; dmaBlock.dpmi.segment = dmaBlock.dpmi.selector = NULL; convtab = NULL;
        irq0struct = NULL; rmCallback.ptr = NULL; patchTable = NULL;
        irq0structBlock.segment  = irq0structBlock.selector  = NULL;
        realModeISRBlock.segment = realModeISRBlock.selector = NULL;
        realModeISREntry = NULL;
        oldIrq0RealMode.ptr = NULL;
        oldIrq0ProtectedMode = NULL;

        devinfo.name = getName();
        devinfo.version = NULL;
        devinfo.maxBufferSize = 32768;  // BYTES
        
        devinfo.caps      = NULL;
        devinfo.capsLen   = 0;
        devinfo.flags     = 0;
    }
    virtual ~sndNonDmaBase() {}
    
    /*
    // get device name
    virtual char    *getName();
    
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
    */

    // init for playback
    virtual uint32_t open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv);

    // start playback (won't return immediately, calls callback to fill DMA buffer)
    virtual uint32_t start();
    
    // pause playback (start() for resume)
    virtual uint32_t pause();
    
    // get playback position in samples
    virtual uint64_t getPos();
    
    // ioctl
    virtual uint32_t ioctl(uint32_t function, void *data, uint32_t len);
    
    // stop playback
    virtual uint32_t stop();

    // close playback
    virtual uint32_t close();

    // deinit device
    virtual uint32_t done();
    
protected:
    
    // init IRQ0 stuff
    bool initIrq0();

    // install IRQ0 handler
    bool setupIrq0();

    // uninstall IRQ0
    bool removeIrq0();

    // done IRQ0 stuff
    bool doneIrq0();

    // init port for playback
    virtual bool initPort() { return true; }

    // deinit port
    virtual bool donePort() { return true; }

    // init conversion table
    virtual bool initConversionTab() { return true; }

    // done conversion table
    virtual bool doneConversionTab() { return true; }

    // flags
    bool            isIrq0Initialised;

    // --------------------- IRQ0 handler stuff -----------------------

    // patch table pointer
    snddev_patch_table     *patchTable;

    // instead of irqEntry i'll use separate structures
    snddev_irq0_struct     *irq0struct; 
    _dpmi_rmpointer         rmCallback;                 // RM->PM callback
    _dpmi_ptr               irq0structBlock;
    _dpmi_ptr               realModeISRBlock;
    void*                   realModeISREntry;
    _dpmi_rmpointer         oldIrq0RealMode;
    void __interrupt __far (*oldIrq0ProtectedMode)();
    uint32_t                timerDivisor;
    
    // --------------------------- IRQ stuff --------------------
    
    static  void    callbackBouncer();
    virtual bool    irqProc();

    // ---------------------- conversion table ------------------
    uint8_t        *convtab;
};

// PC speaker sound driver
class sndPcSpeaker : public sndNonDmaBase {
    
public:
    // constructor (nothing fancy here)
    sndPcSpeaker() : sndNonDmaBase("PC Speaker") {
        patchTable = &snddev_irq0_patch_pcspeaker;
    }
    virtual ~sndPcSpeaker() {};
    
    /*
    // get device name
    virtual char    *getName();
    
    */
    // detect (0 if found + if (res != NULL) *res filled with current config)
    virtual uint32_t detect(SoundDevice::deviceInfo *info = NULL);

    // select and init (use supplied *res info if (res != NULL))
    //virtual uint32_t init(SoundDevice::deviceInfo *info = NULL);
    /*
    // check if format is supported (0 if supported + passes pointer to converter procedure)
    virtual uint32_t isFormatSupported(uint32_t sampleRate, soundFormat *fmt, soundFormatConverterInfo **conv);
    
    // return converter for current format
    soundFormatConverterInfo getFormatConverter(soundFormat *fmt);
    */
    // init for playback
    //virtual uint32_t open(uint32_t sampleRate, soundFormat *fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv);
    
    // start playback (won't return immediately, calls callback to fill DMA buffer)
    //virtual uint32_t start();
    
    // pause playback (start() for resume)
    //virtual uint32_t pause();
    
    // get playback position in samples
    //virtual uint64_t getPos();
    
    // ioctl
    //virtual uint32_t ioctl(uint32_t function, void *data, uint32_t len);
    
    // stop playback
    //virtual uint32_t stop();

    // close playback
    //virtual uint32_t close();

    // deinit device
    //virtual uint32_t done();

protected:
    // init port for playback
    virtual bool initPort();

    // deinit port
    virtual bool donePort();

    // init conversion table
    virtual bool initConversionTab();

    // done conversion table
    virtual bool doneConversionTab();
};


// Covox driver
class sndCovox : public sndNonDmaBase {
    
public:
    // constructor (nothing fancy here)
    sndCovox(const char* name = "Covox LPT DAC") : sndNonDmaBase(name) {
        patchTable = &snddev_irq0_patch_pcspeaker;
        // scan BIOS data area and fixup BIOS LPT resources
        scanBDA();
    };
    virtual ~sndCovox() {};
    
    /*
    // get device name
    virtual char    *getName();
    
    */
    // detect (0 if found + if (res != NULL) *res filled with current config)
    virtual uint32_t detect(SoundDevice::deviceInfo *info = NULL);

    // select and init (use supplied *res info if (res != NULL))
    //virtual uint32_t init(SoundDevice::deviceInfo *info = NULL);
    /*
    // check if format is supported (0 if supported + passes pointer to converter procedure)
    virtual uint32_t isFormatSupported(uint32_t sampleRate, soundFormat *fmt, soundFormatConverterInfo **conv);
    
    // return converter for current format
    soundFormatConverterInfo getFormatConverter(soundFormat *fmt);
    */
    // init for playback
    //virtual uint32_t open(uint32_t sampleRate, soundFormat *fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv);
    
    // start playback (won't return immediately, calls callback to fill DMA buffer)
    //virtual uint32_t start();
    
    // pause playback (start() for resume)
    //virtual uint32_t pause();
    
    // get playback position in samples
    //virtual uint64_t getPos();
    
    // ioctl
    //virtual uint32_t ioctl(uint32_t function, void *data, uint32_t len);
    
    // stop playback
    //virtual uint32_t stop();

    // close playback
    //virtual uint32_t close();

    // deinit device
    //virtual uint32_t done();

protected:

    // scan BIOS data area and fixup BIOS LPT resources
    virtual void scanBDA();

    // fill covox stuff
    virtual uint32_t fillCodecInfo(SoundDevice::deviceInfo* info);

};

// Dual Covox driver
class sndDualCovox : public sndCovox {
    
public:
    // constructor (nothing fancy here)
    sndDualCovox() : sndCovox("Dual Covox LPT DAC") {
        patchTable = &snddev_irq0_patch_dualcovox;
        // sndCovox() scans BDA by itself
    };
    virtual ~sndDualCovox() {};
    
    /*
    // get device name
    virtual char    *getName();
    
    */
    // detect (0 if found + if (res != NULL) *res filled with current config)
    virtual uint32_t detect(SoundDevice::deviceInfo *info = NULL);

    // select and init (use supplied *res info if (res != NULL))
    //virtual uint32_t init(SoundDevice::deviceInfo *info = NULL);
    /*
    // check if format is supported (0 if supported + passes pointer to converter procedure)
    virtual uint32_t isFormatSupported(uint32_t sampleRate, soundFormat *fmt, soundFormatConverterInfo **conv);
    
    // return converter for current format
    soundFormatConverterInfo getFormatConverter(soundFormat *fmt);
    */
    // init for playback
    //virtual uint32_t open(uint32_t sampleRate, soundFormat *fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv);
    
    // start playback (won't return immediately, calls callback to fill DMA buffer)
    //virtual uint32_t start();
    
    // pause playback (start() for resume)
    //virtual uint32_t pause();
    
    // get playback position in samples
    //virtual uint64_t getPos();
    
    // ioctl
    //virtual uint32_t ioctl(uint32_t function, void *data, uint32_t len);
    
    // stop playback
    //virtual uint32_t stop();

    // close playback
    //virtual uint32_t close();

    // deinit device
    //virtual uint32_t done();

protected:

    // fill covox stuff
    virtual uint32_t fillCodecInfo(SoundDevice::deviceInfo* info);

};

// Stereo ioctl definitions
enum {
    SND_IOCTL_STEREO_ON_1_FAST_PROTOCOL_GET = SND_IOCTL_DEVICE_SPECIFIC,
    SND_IOCTL_STEREO_ON_1_FAST_PROTOCOL_SET,
};

// Stereo-On-1 driver
class sndStereoOn1 : public sndCovox {
    
public:
    // constructor (nothing fancy here)
    sndStereoOn1() : sndCovox("Stereo-On-1 LPT DAC") {
        patchTable = &snddev_irq0_patch_stereo1;
        // sndCovox() scans BDA by itself
    };
    virtual ~sndStereoOn1() {};
    
    /*
    // get device name
    virtual char    *getName();
    
    */
    // detect (0 if found + if (res != NULL) *res filled with current config)
    virtual uint32_t detect(SoundDevice::deviceInfo *info = NULL);

    // select and init (use supplied *res info if (res != NULL))
    //virtual uint32_t init(SoundDevice::deviceInfo *info = NULL);
    /*
    // check if format is supported (0 if supported + passes pointer to converter procedure)
    virtual uint32_t isFormatSupported(uint32_t sampleRate, soundFormat *fmt, soundFormatConverterInfo **conv);
    
    // return converter for current format
    soundFormatConverterInfo getFormatConverter(soundFormat *fmt);
    */
    // init for playback
    //virtual uint32_t open(uint32_t sampleRate, soundFormat *fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv);
    
    // start playback (won't return immediately, calls callback to fill DMA buffer)
    //virtual uint32_t start();
    
    // pause playback (start() for resume)
    //virtual uint32_t pause();
    
    // get playback position in samples
    //virtual uint64_t getPos();
    
    // ioctl
    virtual uint32_t ioctl(uint32_t function, void *data, uint32_t len);
    
    // stop playback
    //virtual uint32_t stop();

    // close playback
    //virtual uint32_t close();

    // deinit device
    //virtual uint32_t done();

protected:

    // init port for playback
    virtual bool initPort();

    // deinit port
    virtual bool donePort();

    // fill covox stuff
    virtual uint32_t fillCodecInfo(SoundDevice::deviceInfo* info);
};