#pragma once

// Intel High Definition Audio generic driver
// --wbcbz7 23.o6.2o22

#include "snddefs.h"
#ifdef SNDLIB_DEVICE_ENABLE_HDA

#include <stdint.h>
#include "snddev.h"
#include "convert.h"
#include "sndmisc.h"

#include "cli.h"
#include "irq.h"
#include "dma.h"
#include "sndioctl.h"

// HDA ioctl definitions
enum {
    SND_IOCTL_HDA_SPEAKEROUT_ENABLE_GET = SND_IOCTL_DEVICE_SPECIFIC,
    SND_IOCTL_HDA_SPEAKEROUT_ENABLE_SET,
};

#include "hdadefs.h"

// base HDA driver
class sndHDAudio : public DmaBufferDevice {
public:
    sndHDAudio();

    // ------------- common device methods --------------------------------
    // get device name
    //virtual const char *getName();
    
    // get available resources for manual config, return -1 if autoconfig only, 0 if error, else length of info array
    //virtual const uint32_t getResourceInfo(const soundResourceInfo *info);

    // get supported formats, 0 if error, else length of info array
    //virtual const uint32_t getCaps(const soundFormatCapability* info);

    // detect (0 if found + if (res != NULL) *res filled with current config)
    virtual uint32_t detect(SoundDevice::deviceInfo *info = NULL);
    
    // select and init (use supplied *res info if (res != NULL))
    virtual uint32_t init(SoundDevice::deviceInfo *info = NULL);
    
    // check if format is supported (0 if supported + passes pointer to converter procedure)
    //virtual uint32_t isFormatSupported(uint32_t sampleRate, soundFormat fmt, soundFormatConverterInfo *conv);
    
    // return converter for current format
    //virtual uint32_t getConverter(soundFormat srcfmt, soundFormat dstfmt, soundFormatConverterInfo *conv);
    
    // return bytes per sample count
    //virtual uint32_t getBytesPerSample(soundFormat fmt);
    
    // init for playback
    virtual uint32_t open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv);
    
    // start playback (won't return immediately, calls callback to fill DMA buffer)
    virtual uint32_t start();

    // pause playback (start() or resume() for resume)
    virtual uint32_t pause();
    
    // resume playback
    virtual uint32_t resume();

    // get playback position in samples
    //virtual uint64_t getPos();
    
    // ioctl
    virtual uint32_t ioctl(uint32_t function, void *data, uint32_t len);
    
    // stop playback
    virtual uint32_t stop();
    
    // close playback
    virtual uint32_t close();

    // deinit device
    virtual uint32_t done();

protected:

    // --------------------------- HDA stuff --------------------

    // stream format/tag
    sndlib::hda_streamFormat            hdaStreamFormat;
    uint32_t                            hdaStreamTag;
    uint32_t                            hdaStreamIndex;

    // HDA codec graph struct
    sndlib::hda_codecInfo               codecGraph;

    // PCI device info struct
    pciDeviceList                       pciInfo;

    // --------------------------- DMA stuff --------------------

    // buffer descriptor list pointers
    struct {
        _dpmi_ptr                       dpmi;   // allocated from DOS memory
        sndlib::hda_bufferDescriptor*   ptr;    // 128b aligned
    } bufferDescriptor;

    // init DMA buffer
    virtual uint32_t    dmaBufferInit(uint32_t bufferSize, soundFormatConverterInfo *conv);

    // free DMA buffer
    virtual uint32_t    dmaBufferFree();

    // --------------------------- IRQ stuff --------------------
    
    virtual bool    irqProc();

    // get play position in DMA buffer in bytes
    virtual uint32_t    getPlayPos();

    // advance buffer position
    virtual void        irqAdvancePos();

    // scan PCI configuration space, find suitable HDA codecs, fill info
    bool hdaDetect(SoundDevice::deviceInfo* info, bool manualDetect);
    
    // scan codec DACs for supported formats, returns format mask
    uint32_t getCodecCaps(SoundDevice::deviceInfo* info);

    // fill codec info, assumes info->membase is valid
    uint32_t fillCodecInfo(SoundDevice::deviceInfo* info);

    // reset codec and controller, get info
    uint32_t resetCodec(SoundDevice::deviceInfo* info, bool setupPCI);
};

#endif
