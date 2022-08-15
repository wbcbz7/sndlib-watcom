// Yamaha DS-1 PCI (YMF724/744/754) driver 
// wbcbz7 o7.o8.2o22

#include "snddefs.h"
#ifdef SNDLIB_DEVICE_ENABLE_DS1

/*
    notes:
        - stream positions are messed up
        - p44 slot is not used (seems like my YMF724 doesn't support it)
        - doesn't work for totally unknown reason 
*/

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "convert.h"
#include "sndmisc.h"
#include "snderror.h"
#include "devds1.h"
#include "logerror.h"
#include "tinypci.h"

// import DS1 reg definitions
#include "ds1defs.h"
using namespace sndlib;

//define to enable logging
//#define DEBUG_LOG

#define arrayof(x) (sizeof(x) / sizeof(x[0]))

const uint32_t ds1Rates[] = {4000, 48000};

// DS1 sound caps
const soundFormatCapability ds1Caps[] = {
    {
        (SND_FMT_INT8  | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_UNSIGNED),
        -2, // variable range
        ds1Rates,
    },
    {
        (SND_FMT_INT16 | SND_FMT_MONO | SND_FMT_STEREO | SND_FMT_SIGNED),
        -2, // variable range
        ds1Rates,
    }
};

// calculate sample delta
// ([sampleRate] << 28) / 48000)
uint32_t ds1_calcDelta(uint32_t sampleRate);
#pragma aux ds1_calcDelta = "xor edx, edx", "shld edx, eax, 28" "mov ebx, 48000" "div ebx" parm [eax] value [eax]

// rescale sample position by sample delta
// ([samples48k] * delta) >> 28
uint32_t ds1_rescale48ktoSampleRate(uint32_t posIn48kSamples);
#pragma aux ds1_rescale48ktoSampleRate = "mov ebx, 48000" "mul ebx" "shrd edx, eax, 28" parm [eax] value [eax]

// internal DS1 functions
namespace sndlib {

// memory allocation stuff
bool sndlib::ds1_allocBuffers(void *ds1regs, ymf_memoryAllocInfo *alloc, uint32_t playChannels) {
    if (alloc == NULL) return false;
    memset(alloc, 0, sizeof(ymf_memoryAllocInfo));

    // get buffer size
    alloc->playCtrlSize = YMF_REG_READ32(ds1regs, YMF_REG_AUDIO_PLAY_CONTROL_SIZE) << 2;
    alloc->recCtrlSize  = YMF_REG_READ32(ds1regs, YMF_REG_AUDIO_REC_CONTROL_SIZE) << 2;
    alloc->fxCtrlSize   = YMF_REG_READ32(ds1regs, YMF_REG_AUDIO_FX_CONTROL_SIZE) << 2;
    alloc->p44WorkSize  = 0;//YMF_REG_READ32(ds1regs, YMF_REG_AUDIO_WORK_SIZE) << 2;

    // alloc control arrays
    uint32_t workBlkSize =
        sizeof(ymf_playControlDataTable) + alloc->p44WorkSize + 
        (2 * 5 * alloc->fxCtrlSize);
    
    if (dmaAllocUnaligned(workBlkSize, &alloc->workBlk) == false) return false;
    memset(alloc->workBlk.ptr, 0, workBlkSize);

    // set control array pointers
    alloc->playCtrl     = (ymf_playControlDataTable*)alloc->workBlk.ptr;
    alloc->p44WorkBuf   = (uint8_t*)alloc->playCtrl + sizeof(ymf_playControlDataTable);
    alloc->fxCtrl       = (ymf_fxSlotControlData**)((uint8_t*)alloc->p44WorkBuf + alloc->p44WorkSize);

    // set effect stuff pointers
    uint32_t ofs = 0;
    for(size_t i = 0; i < 5; i++) {
        alloc->fxSlot[0][i] = (ymf_fxSlotControlData*)((uint8_t*)alloc->fxCtrl + ofs); ofs += alloc->fxCtrlSize;
        alloc->fxSlot[1][i] = (ymf_fxSlotControlData*)((uint8_t*)alloc->fxCtrl + ofs); ofs += alloc->fxCtrlSize;
    }

    // allocate slot data for each channel! (danger zone)
    alloc->playChannels = playChannels;
    alloc->playPtrBuf = new ymf_playSlotControlData*[playChannels * 2];
    alloc->playSlot[0] = alloc->playPtrBuf;
    alloc->playSlot[1] = alloc->playPtrBuf + playChannels*sizeof(ymf_playSlotControlData*);

    if (dmaAllocUnaligned((2 * playChannels * alloc->playCtrlSize), &alloc->playSlotBlk) == false)
        return false;
    memset(alloc->playSlotBlk.ptr, 0, (2 * playChannels * alloc->playCtrlSize));

    // set pointers to playback slot data
    ofs = 0;
    for(size_t i = 0; i < playChannels; i++) {
        alloc->playSlot[0][i] = (ymf_playSlotControlData*)((uint8_t*)alloc->playSlotBlk.ptr + ofs); ofs += alloc->playCtrlSize;
        alloc->playSlot[1][i] = (ymf_playSlotControlData*)((uint8_t*)alloc->playSlotBlk.ptr + ofs); ofs += alloc->playCtrlSize;
    }

    // fill play control data
    alloc->playCtrl->numOfPlay = playChannels;
    for(size_t i = 0; i < playChannels; i++) {
        alloc->playCtrl->slotBase[i] = alloc->playSlot[0][i];
    }

#ifdef DEBUG_LOG
    printf("work block ptr  - %08X\n", alloc->workBlk.ptr);
    printf("play ctl buffer - %08X\n", alloc->playCtrl);
    printf("play slot0 ptrs - %08X\n", alloc->playSlot[0]);
    printf("play slot1 ptrs - %08X\n", alloc->playSlot[1]);

    printf("play ch0   ptrs - %08X, %08X\n", alloc->playSlot[0][0], alloc->playSlot[1][0]);
    printf("play ch1   ptrs - %08X, %08X\n", alloc->playSlot[0][1], alloc->playSlot[1][1]);
#endif

    return true;
}

bool sndlib::ds1_freeBuffers (void *ds1regs, ymf_memoryAllocInfo *alloc) {
    if (alloc == NULL) return false;

    delete[] alloc->playPtrBuf;
    dmaFree(&alloc->workBlk);
    dmaFree(&alloc->playSlotBlk);

    return true;
}

void sndlib::ds1_setBuffers(void *ds1regs, ymf_memoryAllocInfo *alloc) {
    if (alloc == NULL) return;

    YMF_REG_WRITE32(ds1regs, YMF_REG_AUDIO_PLAY_CONTROL_BASE, alloc->playCtrl);
    YMF_REG_WRITE32(ds1regs, YMF_REG_AUDIO_REC_CONTROL_BASE,  0);
    YMF_REG_WRITE32(ds1regs, YMF_REG_AUDIO_FX_CONTROL_BASE,   0);
}

// disable DSP
void sndlib::ds1_dspDisable(void *ds1regs) {
    // put PCI audio into reset
    YMF_REG_WRITE32(ds1regs, YMF_REG_AUDIO_CONFIG, 0);

    // check if PCI audio is inactive
    uint32_t timeout = 250000;
    while (--timeout) if ((YMF_REG_FORCE_READ32(ds1regs, YMF_REG_AUDIO_STATUS) & 2) == 0) break;
}

// enable DSP
void sndlib::ds1_dspEnable(void *ds1regs) {
    // pull PCI audio from reset
    YMF_REG_WRITE32(ds1regs, YMF_REG_AUDIO_CONFIG, 1);
}

// reset DSP 
void sndlib::ds1_dspReset(void *ds1regs, bool enable) {
    YMF_REG_WRITE32(ds1regs, YMF_REG_VOL_AUDIO_DAC_OUT, 0);     // mute DAC output
    ds1_dspDisable(ds1regs);
    YMF_REG_WRITE32(ds1regs, YMF_REG_AUDIO_MODE, 0x00010000);   // reset PCI audio block
    YMF_REG_WRITE32(ds1regs, YMF_REG_AUDIO_MODE, 0);            // pull from reset

    // reset recording/fx mappings
    YMF_REG_WRITE32(ds1regs, YMF_REG_AUDIO_MAP_OF_FX, 0);
    YMF_REG_WRITE32(ds1regs, YMF_REG_AUDIO_MAP_OF_REC, 0);

    // reset control base
    YMF_REG_WRITE32(ds1regs, YMF_REG_AUDIO_PLAY_CONTROL_BASE, 0);
    YMF_REG_WRITE32(ds1regs, YMF_REG_AUDIO_REC_CONTROL_BASE, 0);
    YMF_REG_WRITE32(ds1regs, YMF_REG_AUDIO_FX_CONTROL_BASE, 0);

    // reset hadrware volume control?
    YMF_REG_WRITE16(ds1regs, YMF_REG_GLOBAL_CONTROL, (YMF_REG_READ16(ds1regs, YMF_REG_GLOBAL_CONTROL) & ~7));

    if (enable == true) ds1_dspEnable(ds1regs);
}

}

// ---------------------------------
// now for the usual stuff

sndYamahaDS1::sndYamahaDS1() : DmaBufferDevice("Yamaha DS-1 PCI", 512) {
    memset(&pciInfo, 0, sizeof(pciInfo));
    oldChanPlayPos      = 0;
    oldSample48Count    = 0;
}

bool sndYamahaDS1::ds1Detect(SoundDevice::deviceInfo* info, bool manualDetect) {
    if (info == NULL) return false;
    info->membase = NULL;
    
    // start PCI scan
    pciDeviceList ds1Devs[16];      // enough
    uint32_t ds1DevsCount = tinypci::enumerateByDeviceId(ds1Devs, arrayof(ds1Devs), 0x1073, -1);
    if (ds1DevsCount == 0) return false;

    // test if overriden (TODO)

    // select first found device lolololol
    info->pci = ds1Devs[0].address;
    info->irq = ds1Devs[0].interruptLine;
    pciInfo   = ds1Devs[0];
    return true;
};

// fill codec info, assumes info->membase is valid
uint32_t sndYamahaDS1::fillCodecInfo(SoundDevice::deviceInfo* info) {
    if ((info == NULL) || (info->membase == NULL)) return SND_ERR_NULLPTR;

    // fill info
    info->name = "Yamaha DS-1 PCI";
    info->maxBufferSize = 32768;  // BYTES
    info->flags = 0;

    uint32_t pciVendorDeviceId   = tinypci::configReadDword(info->pci, 0);
    uint8_t* privateBufPtr = (uint8_t*)info->privateBuf;

    // get vendor/device info
    int stringLength = snprintf(
        info->privateBuf, info->privateBufSize, "PCI %02d:%02d.%01d [%04X:%04X]",
        info->pci.bus, info->pci.device, info->pci.function,
        (pciVendorDeviceId & 0xFFFF), pciVendorDeviceId >> 16
    ) + 1;      // with trailing '\0'

    // link version string
    info->version = info->privateBuf; privateBufPtr += stringLength;

    info->capsLen   = arrayof(ds1Caps);
    info->caps      = ds1Caps;
    return SND_ERR_OK;
}

uint32_t sndYamahaDS1::detect(SoundDevice::deviceInfo* info) {

    // clear and fill device info
    this->devinfo.clear();
    if (ds1Detect(&this->devinfo, true) == false) return SND_ERR_NOTFOUND;

    // fill codec version
    fillCodecInfo(&this->devinfo);
    
    // copy info if not NULL
    if (info != NULL) *info = devinfo;

    return SND_ERR_OK;
}

uint32_t sndYamahaDS1::init(SoundDevice::deviceInfo *info) {
    uint32_t rtn = SND_ERR_OK;

    // deinit
    if (isInitialised) done();

    // validate fields
    SoundDevice::deviceInfo* p = (info != NULL ? info : &this->devinfo);

    // validate resources
    if ((p->pci.bus == -1) || (p->pci.device == -1) || (p->pci.function == -1)) return SND_ERR_INVALIDCONFIG;

    // copy resource fields
    if (info != NULL) {
        this->devinfo.pci = p->pci;
    }

    // extract PCI info
    if (tinypci::enumerateByAddress(&pciInfo, 1, devinfo.pci) == 0) return SND_ERR_INVALIDCONFIG;
    if (pciInfo.vendorId != 0x1073) return SND_ERR_INVALIDCONFIG;
    if ((pciInfo.bar0 & 1) != 0) return SND_ERR_INVALIDCONFIG;

    // map device registers
    devinfo.membase = dpmi_mapphysical(16384, (void*)(pciInfo.bar0 & ~0xF)); if (devinfo.membase == 0) return SND_ERR_NULLPTR;
    devinfo.irq = pciInfo.interruptLine;

    // reset controller and codec
    ds1_dspReset(&devinfo, true);

    // allocate memory for DS1 intermediate buffers
    if (ds1_allocBuffers(devinfo.membase, &ds1alloc, 2) == false) return SND_ERR_MEMALLOC;

    isInitialised = true;
    return SND_ERR_OK;
}

uint32_t sndYamahaDS1::done() {
    if (isOpened) close();

    // unmap device
    if (devinfo.membase != NULL) dpmi_unmapphysical(devinfo.membase); devinfo.membase = NULL;

    // reset DSP
    ds1_dspReset(&devinfo, true);

    // deallocate buffers
    ds1_freeBuffers(devinfo.membase, &ds1alloc);

    isInitialised = false;
    return SND_ERR_OK;
}

uint32_t sndYamahaDS1::open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv) {
    uint32_t result = SND_ERR_OK;
    if ((conv == NULL) || (callback == NULL)) return SND_ERR_NULLPTR;

    // stooop!
    if (isOpened) close();

    // clear converter info
    memset(conv, 0, sizeof(soundFormatConverterInfo));

    // test for conversion
    soundFormat newFormat = fmt;
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

    // allocate DMA buffer
    if ((result = dmaBufferInit(bufferSize, conv)) != SND_ERR_OK) return result;

    // install IRQ handler
    if ((result = installIrq()) != SND_ERR_OK) return result;

    // save callback info
    this->callback = callback;
    this->userdata = userdata;

    // pass coverter info
    memcpy(&convinfo, conv, sizeof(convinfo));

    this->currentFormat = newFormat;
    this->sampleRate = sampleRate;
    this->sampleDelta = ds1_calcDelta(sampleRate);
    
    // debug output
#ifdef DEBUG_LOG
    fprintf(stderr, __func__": requested format 0x%X, opened format 0x%X, rate %d hz, buffer %d bytes, flags 0x%X\n", fmt, newFormat, sampleRate, bufferSize, flags);
#endif

    isOpened = true;
    return SND_ERR_OK;
}

uint32_t sndYamahaDS1::dmaBufferInit(uint32_t bufferSize, soundFormatConverterInfo *conv) {
    // clamp min buffer size to 512 samples for reliable position tracking
    // (see irqProc() comments for more info)
    if (bufferSize < 512) bufferSize = 512;

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
    if (dmaAlloc(dmaBlockSize, &dmaBlock) == false) return SND_ERR_MEMALLOC;
    
    // lock DPMI memory for buffer
    dpmi_lockmemory(dmaBlock.ptr, dmaBlockSize+64);

    return SND_ERR_OK;
}

uint32_t sndYamahaDS1::close() {
    // stop playback
    if (isPlaying) stop();
    
    // deallocate DMA block
    dmaBufferFree();

    // unhook irq if hooked
    if (irq.hooked) irqUnhook(&irq, false);

    // fill with defaults
    isOpened = isPlaying = false;
    currentPos = irqs = 0;
    dmaChannel = dmaBlockSize = dmaBufferCount = dmaBufferSize = dmaBufferSamples = dmaBlockSamples = dmaCurrentPtr = dmaRenderPtr = 0;
    sampleRate = 0;
    currentFormat = SND_FMT_NULL;

    return SND_ERR_OK;
}

uint32_t sndYamahaDS1::resume()  {
    // resume playback
    YMF_REG_WRITE32(devinfo.membase, YMF_REG_AUDIO_MODE, 3);

    isPaused = false;
    return SND_ERR_RESUMED;
}

uint32_t sndYamahaDS1::pause()
{
    // save current stream position
    // TODO: extend sample48Count to 64 bits
    YMF_REG_WRITE32(devinfo.membase, YMF_REG_AUDIO_SAMPLE_COUNT, 0);
    uint32_t sample48Count = YMF_REG_READ32(devinfo.membase, YMF_REG_AUDIO_SAMPLE_COUNT);
    currentPos = ds1_rescale48ktoSampleRate(sample48Count);

    // pause
    YMF_REG_WRITE32(devinfo.membase, YMF_REG_AUDIO_MODE, 0);

    isPaused = true;
    return SND_ERR_OK;
}

uint32_t sndYamahaDS1::ioctl(uint32_t function, void* data, uint32_t len)
{
    return SND_ERR_UNSUPPORTED;
}

uint32_t sndYamahaDS1::stop() {
    if (isPlaying) {
        // stop stream
        YMF_REG_WRITE32(devinfo.membase, YMF_REG_AUDIO_MODE, 0);

        // disable pci audio out in mixer
        YMF_REG_WRITE32(devinfo.membase, YMF_REG_VOL_AUDIO_DAC_OUT, 0);
    }

    isPlaying = false;

    // clear playing position
    currentPos = renderPos = irqs = 0;
    dmaCurrentPtr = dmaRenderPtr = 0;
    oldChanPlayPos = oldSample48Count = 0;

    return SND_ERR_OK;
}

uint32_t sndYamahaDS1::start() {
    uint32_t rtn = SND_ERR_OK;
    if ((rtn = prefill()) != SND_ERR_OK) return rtn;

    // --------------------------------
    // device specific right now

    // fill channel slot descriptors
    ymf_playSlotControlData* chan = ds1alloc.playSlot[0][0];
    memset(chan, 0, ds1alloc.playCtrlSize * 4);
    // fill common stuff
    chan->pgBase    = dmaBlock.ptr;
    chan->pgStart   = 0;
    chan->pgLoop    = 0;
    chan->pgLoopEnd = dmaBlockSamples;
    chan->pgDelta   = chan->pgDeltaEnd = sampleDelta;
    chan->egGain    = chan->egGainEnd  = 0x40000000;
    chan->lpfK      = chan->lpfKEnd    = 0x40000000;
    chan->lpfQ      = 0x40000000;
    chan->format    =   (currentFormat & SND_FMT_STEREO ? (1 << 16) : 0) |
                        (currentFormat & SND_FMT_INT8   ? (1 << 31) : 0);

    if (currentFormat & SND_FMT_STEREO) {
        // stereo
        ds1alloc.playCtrl->numOfPlay = 2;
        memcpy(ds1alloc.playSlot[0][1], chan, ds1alloc.playCtrlSize);

        // left channel
        ds1alloc.playSlot[0][0]->leftGain       = 0x40000000;
        ds1alloc.playSlot[0][0]->leftGainEnd    = 0x40000000;

        // right channel
        ds1alloc.playSlot[0][1]->format        |= 0x00000001;
        ds1alloc.playSlot[0][1]->rightGain      = 0x40000000;
        ds1alloc.playSlot[0][1]->rightGainEnd   = 0x40000000;

    } else {
        // mono
        ds1alloc.playCtrl->numOfPlay = 1;

        ds1alloc.playSlot[0][0]->leftGain       = 0x40000000;
        ds1alloc.playSlot[0][0]->leftGainEnd    = 0x40000000;
        ds1alloc.playSlot[0][0]->rightGain      = 0x40000000;
        ds1alloc.playSlot[0][0]->rightGainEnd   = 0x40000000;
    }

    // and finally fill other bank
    memcpy(ds1alloc.playSlot[1][0], ds1alloc.playSlot[0][0], ds1alloc.playCtrlSize);
    memcpy(ds1alloc.playSlot[1][1], ds1alloc.playSlot[0][1], ds1alloc.playCtrlSize);

    // set buffers
    ds1_setBuffers(devinfo.membase, &ds1alloc);

    // enable PCI audio
    YMF_REG_WRITE32(devinfo.membase, YMF_REG_AUDIO_MODE, 3);

    // enable pci audio out in mixer
    YMF_REG_WRITE32(devinfo.membase, YMF_REG_VOL_AUDIO_DAC_OUT, 0x3FFF3FFF);

    // done! we're playing sound :)
    isPaused = false; isPlaying = true;

    return SND_ERR_OK;
}

// irq procedure
bool sndYamahaDS1::irqProc() {
    // test if DS-1 PCI audio interrupt
    if ((YMF_REG_FORCE_READ32(devinfo.membase, YMF_REG_AUDIO_STATUS) & 0x80000000) == 0)
        return true; // else chain to previous ISR

    // acknowledge it
    YMF_REG_WRITE32(devinfo.membase, YMF_REG_AUDIO_STATUS, 0x80000000);

    // acknowledge frame completion
    YMF_REG_MODIFY32(devinfo.membase, YMF_REG_AUDIO_MODE, 2, 2);

    // acknowledge interrupt
    outp(irq.info->picbase, 0x20); if (irq.info->flags & IRQ_SECONDARYPIC) outp(0x20, 0x20);

    // now for the hard part. DS-1 triggers audio frame interrupt every 256 48khz samples, so we
    // have to determine if current channel has switched to next DMA buffer, and in that case,
    // call callback to render more audio data
    // this is also the reason the buffer sample count must be 512 or mode
    uint32_t bank = YMF_REG_READ32(devinfo.membase, YMF_REG_AUDIO_CONTROLSELECT) & 1;
    uint32_t chanPlayPos = ds1alloc.playSlot[bank][0]->pgStart;
    if (((oldChanPlayPos - chanPlayPos) >= dmaBufferSamples) && (chanPlayPos < oldChanPlayPos)) {
        // DMA buffer boundary is crossed
        oldChanPlayPos = chanPlayPos;
        dmaRenderPtr = ((chanPlayPos / dmaBufferSamples) + 1) * dmaBufferSize;
        if (dmaRenderPtr >= dmaBlockSize) dmaRenderPtr = 0;

        // current pos calculation uses separate path (get current number of 48khz frames processed,
        // then rescale to channel sample rate)

        // call callback
        irqCallbackCaller();
    }

    return false;   // we're handling EOI by itself
} 

uint64_t sndYamahaDS1::getPos() {
    if (isPlaying) {
        // TODO: fix 32bit wraparound (after ~24.8 hours)

        // get current 48khz sample count, rescale to target samplerate
        YMF_REG_WRITE32(devinfo.membase, YMF_REG_AUDIO_SAMPLE_COUNT, 0);
        uint32_t sample48Count = YMF_REG_READ32(devinfo.membase, YMF_REG_AUDIO_SAMPLE_COUNT);
        return (currentPos + ds1_rescale48ktoSampleRate(sample48Count));
    }
    else return 0;
}

#endif
