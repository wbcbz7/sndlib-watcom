// WE'RE GONNA SCREW EVERYTHING
#pragma once 

#include <stdint.h>
#include <string.h>
#include <i86.h>

#include "convert.h"
#include "irq.h"
#include "dma.h"

#define SNDDEV_IRQ_PER_DEVICE

// init sound library (called before any device using)
uint32_t sndlibInit();

// done sound library (called at exit)
uint32_t sndlibDone();


// deviceResources::flags
enum {
    SND_DEVICE_IRQ0                 = (1 << 0),         // timing based on IRQ0 (e.g. covox/pc speaker/sb non-dma)
    SND_DEVICE_CLOCKDRIFT           = (1 << 1),         // actual samplerate can differ from requested by >2%
    SND_DEVICE_CUSTOMFORMAT         = (1 << 2),         // custom format for samples (e.g. pc speaker)
};

// open::flags
enum {
    SND_OPEN_NOCONVERT              = (1 << 0),         // try setting direct format, don't perform conversion
    
};

enum {
    SND_RES_IOBASE,
    SND_RES_IOBASE2,
    SND_RES_IRQ,
    SND_RES_IRQ2,
    SND_RES_DMA,
    SND_RES_DMA2,
};

struct soundResourceInfo {
    uint32_t        resourceType;
    uint32_t        length;
    const uint32_t* data;
};


class SoundDevice {
    
    const char*         name;       // nothing fancy here
    
public:
    struct deviceInfo {
        const char*                     name;           // device name
        const char*                     version;        // device version
        
        uint32_t                        iobase;         // -1 if none
        uint32_t                        iobase2;        // secondary (hey gus), -1 if none
        uint32_t                        irq;            // -1 if none
        uint32_t                        irq2;           // -1 if none
        uint32_t                        dma;            // -1 if none
        uint32_t                        dma2;           // -1 if none
        union {
            struct {
            uint32_t                    function:8;
            uint32_t                    device:8;
            uint32_t                    bus:8;
            };
            uint32_t                    addr;
        }                               pci;            // PCI device address, -1 if none
        
        uint32_t                        maxBufferSize;  // maximum available buffer size in BYTES
        uint32_t                        flags;          // as usual
        
        uint32_t                        capsLen;        // caps list length
        const soundFormatCapability*    caps;           // caps list pointer

        // private buffer for copied strings/etc
        char                            privateBuf[64];
        
        // clear struct
        void clear();

        // fix private buffer pointers
        void privFixup(const deviceInfo& rhs);

        // regular constructor
        deviceInfo() { clear(); }

        // copy constructor
        deviceInfo(const deviceInfo& rhs);
        deviceInfo operator=(const deviceInfo& rhs);
    };
    
public:
    // constructor (nothing fancy here)
    SoundDevice(const char* _name) :
        name(_name), inIrq(false), currentFormat(SND_FMT_NULL), sampleRate(0),
        callback(NULL), userdata(NULL) {
        memset(&irq,        0, sizeof(irqEntry));
        memset(&convinfo,   0, sizeof(convinfo));
        devinfo.clear();
    }
    virtual ~SoundDevice() {
        if (isPlaying) stop();
        if (isOpened) close();
        if (isInitialised) done();
    };
    
    // get device name
    virtual const char *getName();
    
    // get available resources for manual config, return -1 if autoconfig only, 0 if error, else length of info array
    virtual const uint32_t getResourceInfo(const soundResourceInfo *info);

    // get supported formats, 0 if error, else length of info array
    virtual const uint32_t getCaps(const soundFormatCapability* info);

    // detect (0 if found + if (res != NULL) *res filled with current config)
    virtual uint32_t detect(SoundDevice::deviceInfo *info = NULL);
    
    // select and init (use supplied *res info if (res != NULL))
    virtual uint32_t init(SoundDevice::deviceInfo *info = NULL);
    
    // check if format is supported (0 if supported + passes pointer to converter procedure)
    virtual uint32_t isFormatSupported(uint32_t sampleRate, soundFormat fmt, soundFormatConverterInfo *conv);
    
    // return converter for current format
    virtual uint32_t getConverter(soundFormat srcfmt, soundFormat dstfmt, soundFormatConverterInfo *conv);
    
    // return bytes per sample count
    virtual uint32_t getBytesPerSample(soundFormat fmt);
    
    // init for playback
    virtual uint32_t open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv);
    
    // start playback (won't return immediately, calls callback to fill DMA buffer)
    virtual uint32_t start();

    // pause playback (start() or resume() for resume)
    virtual uint32_t pause();
    
    // resume playback
    virtual uint32_t resume();

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

    // ---------------- interrupt stuff ------------------
    
    // IRQ stuff info
    irqEntry        irq;
    
    // nested interrupt flag;
    bool            inIrq;
    
    // irq procedure (returns true if needs to chain to old handler)
    virtual bool    irqProc();
    
protected:
    // ---------- internal running state flags ----------
    bool            isDetected;         // set by detect()
    bool            isInitialised;      // init() ................................ done()
    bool            isOpened;           //        open() ................. close()
    bool            isPlaying;          //               play() ... stop()
    bool            isPaused;           //               pause() .. play()


    // ------------------ device info --------------------
    SoundDevice::deviceInfo     devinfo;
    
    // ------------ format and callback info -------------
    soundFormat                 currentFormat;
    uint32_t                    sampleRate;

    // callback info
    soundDeviceCallback         callback;
    void*                       userdata;
    soundFormatConverterInfo    convinfo;

};

// -----------------------------------------------
// DMA-like circular buffer device (both ISA DMA and PCI bus-master devices)
class DmaBufferDevice : public SoundDevice {
public:
    DmaBufferDevice(const char* _name) : SoundDevice(_name) {
        dmaChannel = -1;
        currentPos = irqs = 0;
        oldTotalPos = renderPos = 0;
        dmaBlockSize = dmaBufferCount = dmaBufferSize = dmaBufferSamples = dmaBlockSamples = dmaCurrentPtr = dmaRenderPtr = 0;
        dmaBlock.ptr = NULL; dmaBlock.dpmi.segment = dmaBlock.dpmi.selector = NULL;
    }
    virtual ~DmaBufferDevice() {};

protected:

    // -------------------------- DMA stuff ------------------------
    
    // getPos() previous values
    uint64_t        oldTotalPos;                // previous total playback pos

    uint64_t        currentPos;                 // total playback  pos
    uint64_t        renderPos;                  // total rendering pos
    uint64_t        irqs;                       // total IRQs count
    
    // each block contains one or more buffers (2 in our case)
    
    uint32_t        dmaChannel;                 // sometimes different formats require different DMA channels (hey SB16)
    
    dmaBlock        dmaBlock;
    uint32_t        dmaBlockSize;
    uint32_t        dmaBlockSamples;            // size in samples
    
    uint32_t        dmaBufferCount;             // num of buffers inside one block
    uint32_t        dmaBufferSize;              // size of each buffer
    uint32_t        dmaBufferSamples;           // size of each buffer (in samples)

    uint32_t        dmaCurrentPtr;              // points to current playing(!) buffer
    uint32_t        dmaRenderPtr;               // points to current rendering buffer

    // init DMA buffer
    virtual uint32_t    dmaBufferInit(uint32_t bufferSize, soundFormatConverterInfo *conv);

    // free DMA buffer
    virtual uint32_t    dmaBufferFree();

    // install IRQ routine
    virtual uint32_t    installIrq();

    // remove IRQ routine
    virtual uint32_t    removeIrq();

    // advance play/render pointers
    virtual void        irqAdvancePos();

    // IRQ->callback caller
    virtual bool        irqCallbackCaller();
};

// ISA DMA device
class IsaDmaDevice : public DmaBufferDevice {
public:
    IsaDmaDevice(const char* _name) : DmaBufferDevice(_name) {}
    virtual ~IsaDmaDevice() {};

    // get playback position in samples
    virtual uint64_t getPos();

protected:

};


// IRQ handling stuff
extern SoundDevice              *snd_activeDevice[16];
#ifdef SNDDEV_IRQ_PER_DEVICE
extern void __interrupt __far  (*snd_irqProcTable[16])();
#endif
extern "C" void __interrupt __far snd_irqStaticProc(INTPACK r);

// device IRQ detection stuff
struct IrqDetectInfo {
    bool     found;
    uint32_t iobase;
    irqEntry *irq;
};
extern volatile IrqDetectInfo snd_IrqDetectInfo;

// protected mode ISR stack
extern "C" {
    extern uint8_t      snddev_bss_lock_start,  snddev_bss_lock_end;    // BSS lock start/end (not a variable)
    extern uint8_t      snddev_pm_stack_in_use;                         // PM stack "in use" flag (0 - free)

    extern void __far  *snddev_pm_stack_top;                            // PM stack top
    extern void __far  *snddev_pm_stack;                                // PM stack bottom
    extern void __far  *snddev_pm_old_stack;                            // PM saved stack pointer
}

