#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#include "snddefs.h"
#include "snddev.h"
#include "sndfmt.h"
#include "convert.h"
#include "snderror.h"

// ---------------- device info methods -------------------
SoundDevice::deviceInfo::deviceInfo(uint32_t _privateBufSize) {
    privateBufSize = _privateBufSize;
    privateBuf = new char[privateBufSize];
    clear();
}

SoundDevice::deviceInfo::~deviceInfo() {
    if (privateBuf != NULL) delete[] privateBuf;
}

void SoundDevice::deviceInfo::clear() {
    memset(privateBuf, privateBufSize, 0);
    name = version = NULL; caps = NULL;
    iobase = iobase2 = irq = irq2 = dma = dma2 = pci.addr = -1;
    maxBufferSize = flags = capsLen = 0;
}

// fix private buffer pointers
void SoundDevice::deviceInfo::privFixup(const SoundDevice::deviceInfo& rhs) {
    if ((name >= rhs.privateBuf) && (name < rhs.privateBuf + privateBufSize)) 
        name += (privateBuf - rhs.privateBuf);
    if ((version >= rhs.privateBuf) && (version < rhs.privateBuf + privateBufSize)) 
        version += (privateBuf - rhs.privateBuf);
    if (((const char*)caps >= rhs.privateBuf) && ((const char*)caps < rhs.privateBuf + privateBufSize)) 
        caps = (const soundFormatCapability*)((const char*)caps + (privateBuf - rhs.privateBuf));
}

// copy constructor
SoundDevice::deviceInfo::deviceInfo(const SoundDevice::deviceInfo& rhs) {
    memcpy(this, &rhs, sizeof(deviceInfo));

    privateBuf = new char[rhs.privateBufSize];
    privateBufSize = rhs.privateBufSize;

    memcpy(privateBuf, rhs.privateBuf, rhs.privateBufSize);
    privFixup(rhs);
}

SoundDevice::deviceInfo& SoundDevice::deviceInfo::operator=(const SoundDevice::deviceInfo& rhs) {
    if (privateBuf != NULL) delete[] privateBuf;
    memcpy(this, &rhs, sizeof(deviceInfo));

    privateBuf = new char[rhs.privateBufSize];
    privateBufSize = rhs.privateBufSize;

    memcpy(privateBuf, rhs.privateBuf, rhs.privateBufSize);
    privFixup(rhs);
    return *this;
}

// -------------------------------------------------------

SoundDevice::SoundDevice(const char* _name, uint32_t _infoPrivateBufSize ) :
    name(_name), callback(NULL), userdata(NULL), devinfo(_infoPrivateBufSize) {
    memset(&irq,        0, sizeof(irqEntry));
    memset(&convinfo,   0, sizeof(convinfo));
    devinfo.clear();
    isDetected = isPaused = isPlaying = isInitialised = isOpened = false;
}

SoundDevice::~SoundDevice() {
    if (isPlaying) stop();
    if (isOpened) close();
    if (isInitialised) done();
};

const char * SoundDevice::getName()
{
    return name;
}

const uint32_t SoundDevice::getResourceInfo(const soundResourceInfo* info)
{
    if (info == NULL) return 0;
    return -1;
}

const uint32_t SoundDevice::getCaps(const soundFormatCapability* info)
{
    info = devinfo.caps;
    return devinfo.capsLen;
}

const SoundDevice::deviceInfo* SoundDevice::getDeviceInfo() {
    return &devinfo;
}

uint32_t SoundDevice::detect(SoundDevice::deviceInfo * info)
{
    return SND_ERR_UNSUPPORTED;
}

uint32_t SoundDevice::init(SoundDevice::deviceInfo * info)
{
    return SND_ERR_UNSUPPORTED;
}

uint32_t SoundDevice::isFormatSupported(uint32_t sampleRate, soundFormat fmt, soundFormatConverterInfo *conv)
{
    if (sampleRate == 0) return SND_ERR_UNSUPPORTED;

    bool isFound = false; size_t rate = 0;
    for (int i = 0; i < devinfo.capsLen; i++) {
        // check format
        uint32_t formatMask = (devinfo.caps[i].format & fmt);
        if (((formatMask & SND_FMT_CHANNELS_MASK) == 0) || 
            ((formatMask & SND_FMT_DEPTH_MASK) == 0) || 
            ((formatMask & SND_FMT_SIGN_MASK) == 0)) continue;
        
        // check rate
        if (sampleRate == SND_ISFORMATSUPPORTED_MAXSAMPLERATE) {
            // get maximum supported sample rate for this format
            if (devinfo.caps[i].ratesLength == -2) sampleRate = devinfo.caps[i].rates[1]; else {
                // find maximum sample rate
                sampleRate = 0;
                for (uint32_t j = 0; j < devinfo.caps[i].ratesLength; j++) {
                    if (devinfo.caps[i].rates[j] > sampleRate) {
                        sampleRate = devinfo.caps[i].rates[j];
                        rate = j;
                    }
                }
            }
        } else {
            // check if rate is supported
            if (devinfo.caps[i].ratesLength == -2) {
                if ((sampleRate < devinfo.caps[i].rates[0]) || (sampleRate > devinfo.caps[i].rates[1])) {
                    continue;
                }
            }
            else {
                // iterate by hand, allow ~0.5% samplerate error
                for (rate = 0; rate < devinfo.caps[i].ratesLength; rate++) {
                    if (abs(sampleRate - devinfo.caps[i].rates[rate]) < (devinfo.caps[i].rates[rate] >> 8)) break;
                }
                // not found?
                if (rate >= devinfo.caps[i].ratesLength) continue;
            }
        }
        
        // target format is found! fill samplerate fields
        if (conv != NULL) {
            if (conv->format == SND_FMT_NULL) 
                conv->format = formatMask;
            if (conv->sourceSampleRate == 0)
                conv->sourceSampleRate = sampleRate;
            if (conv->sampleRate == 0) 
                conv->sampleRate = (devinfo.caps[i].ratesLength == -2) ? sampleRate : devinfo.caps[i].rates[rate];
        }
        return SND_ERR_OK;
    }
    return SND_ERR_UNSUPPORTED;
}

uint32_t SoundDevice::open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void * userdata, soundFormatConverterInfo * conv)
{
    return SND_ERR_UNSUPPORTED;
}

uint32_t SoundDevice::start()
{
    return SND_ERR_UNSUPPORTED;
}

uint32_t SoundDevice::resume()
{
    return SND_ERR_UNSUPPORTED;
}

uint32_t SoundDevice::pause()
{
    return SND_ERR_UNSUPPORTED;
}

int64_t SoundDevice::getPos()
{
    return 0;
}

uint32_t SoundDevice::ioctl(uint32_t function, void * data, uint32_t len)
{
    return SND_ERR_UNSUPPORTED;
}

uint32_t SoundDevice::stop()
{
    return SND_ERR_OK;
}

uint32_t SoundDevice::close()
{
    return SND_ERR_OK;
}


uint32_t SoundDevice::done()
{
    return SND_ERR_OK;
}

uint32_t SoundDevice::getBytesPerSample(soundFormat fmt) {
    uint32_t bytes = 0;
    
    switch(fmt & SND_FMT_DEPTH_MASK) {
        case SND_FMT_INT8:   
        case SND_FMT_XLAT8:  bytes = 1; break;
        case SND_FMT_INT16:  bytes = 2; break;   
        case SND_FMT_INT20:
        case SND_FMT_INT24:  bytes = 3; break; 
        case SND_FMT_FLOAT:  bytes = 4; break;   
        case SND_FMT_DOUBLE: bytes = 8; break;   
        default:             return 0;
    } 
    if (fmt & SND_FMT_STEREO) bytes <<= 1;
    
    return bytes;
}

// find converter procedure

uint32_t SoundDevice::getConverter(soundFormat srcfmt, soundFormat dstfmt, soundFormatConverterInfo *conv) {
    if (conv == NULL) return SND_ERR_NULLPTR;
    
    if ((srcfmt == SND_FMT_NULL) || (dstfmt == SND_FMT_NULL)) return SND_ERR_UNKNOWN_FORMAT;
    
    if (srcfmt == dstfmt) {
        // same format - resolve shift for sndconv_memcpy()
        int32_t shift = 0;
        switch(srcfmt & SND_FMT_DEPTH_MASK) {
            case SND_FMT_INT8:
            case SND_FMT_XLAT8:  shift = 0; break;
            case SND_FMT_INT16:  shift = 1; break;   
            case SND_FMT_INT20:  return SND_ERR_UNKNOWN_FORMAT;  
            case SND_FMT_INT24:  return SND_ERR_UNKNOWN_FORMAT;
            case SND_FMT_FLOAT:  shift = 2; break;   
            case SND_FMT_DOUBLE: shift = 3; break;   
            default:             return SND_ERR_UNKNOWN_FORMAT;
        }
        if (srcfmt & SND_FMT_STEREO) shift++;
        
        conv->proc   = &sndconv_memcpy;
        conv->parm   = shift;
        conv->format = dstfmt;
        return SND_ERR_OK;
    }

#ifdef SNDLIB_CONVERT_ENABLE_PCSPEAKER
    // special xlat shit for PC speaker
    // note the xlat table (ptr at conv->parm2) is for SIGNED 8bit samples (thus offsets 0..255 -> -128..127)
    if (((dstfmt & SND_FMT_XLAT8) == SND_FMT_XLAT8) && ((dstfmt & SND_FMT_SIGN_MASK) == SND_FMT_UNSIGNED))  {
        // clear all other formats that xlat8
        dstfmt &= (~(SND_FMT_DEPTH_MASK & (~SND_FMT_XLAT8)));

        if ((srcfmt & SND_FMT_DEPTH_MASK) == SND_FMT_INT16) {
            // 16b -> xlat8
            if ((srcfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_STEREO) {
                // stereo->mono
                conv->proc   = &sndconv_16s_xlat;
                conv->parm   = ((srcfmt & SND_FMT_SIGN_MASK) == SND_FMT_UNSIGNED) ? 0x80008000 : 0;
                conv->format = dstfmt;
                return SND_ERR_OK;
            } else 
            if ((srcfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_MONO) {
                // stereo->mono
                conv->proc   = &sndconv_16m_xlat;
                conv->parm   = ((srcfmt & SND_FMT_SIGN_MASK) == SND_FMT_UNSIGNED) ? 0x80008000 : 0;
                conv->format = dstfmt;
                return SND_ERR_OK;
            }
        } else 
        if ((srcfmt & SND_FMT_DEPTH_MASK) == SND_FMT_INT8) {
            // 8b -> xlat8
            if ((srcfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_STEREO) {
                // stereo->mono
                conv->proc   = &sndconv_8s_xlat;
                conv->parm   = ((srcfmt & SND_FMT_SIGN_MASK) == SND_FMT_UNSIGNED) ? 0x80808080 : 0;
                conv->format = dstfmt;
                return SND_ERR_OK;
            } else 
            if ((srcfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_MONO) {
                // stereo->mono
                conv->proc   = &sndconv_8m_xlat;
                conv->parm   = ((srcfmt & SND_FMT_SIGN_MASK) == SND_FMT_UNSIGNED) ? 0x80808080 : 0;
                conv->format = dstfmt;
                return SND_ERR_OK;
            }
        }
    }
#endif  

#ifdef SNDLIB_CONVERT_ENABLE_ARBITRARY
    if (((srcfmt & SND_FMT_DEPTH_MASK) == SND_FMT_INT16) && ((dstfmt & SND_FMT_DEPTH_MASK) == SND_FMT_INT16)) {
        // 16b -> 16b 
        if (((srcfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_STEREO) && ((dstfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_MONO)) {
            // stereo->mono
            conv->proc   = &sndconv_16s_16m;
            conv->parm   = ((srcfmt & SND_FMT_SIGN_MASK) != (dstfmt & SND_FMT_SIGN_MASK)) ? 0x80008000 : 0;
            conv->format = dstfmt;
            return SND_ERR_OK;
        }
        else return SND_ERR_UNKNOWN_FORMAT; // handled by case above
    }
    
    if (((srcfmt & SND_FMT_DEPTH_MASK) == SND_FMT_INT8) && ((dstfmt & SND_FMT_DEPTH_MASK) == SND_FMT_INT16)) {
        // 8->16
        conv->proc   = &sndconv_8_16;
        conv->parm   = ((srcfmt & SND_FMT_SIGN_MASK) != (dstfmt & SND_FMT_SIGN_MASK)) ? 0x80808080 : 0;
        conv->parm2  = (srcfmt & SND_FMT_STEREO) ? 2 :1;
        conv->format = dstfmt;
        return SND_ERR_OK;
    }

    if (((srcfmt & SND_FMT_DEPTH_MASK) == SND_FMT_INT8) && ((dstfmt & SND_FMT_DEPTH_MASK) == SND_FMT_INT8)) {
        // 8b -> 8b 
        if (((srcfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_STEREO) && ((dstfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_MONO))
            if ((srcfmt & SND_FMT_SIGN_MASK) == SND_FMT_UNSIGNED)  {
                // stereo->mono signed
                conv->proc = &sndconv_8s_8m;
                conv->parm = ((srcfmt & SND_FMT_SIGN_MASK) != (dstfmt & SND_FMT_SIGN_MASK)) ? 0x80808080 : 0;
                conv->format = dstfmt;
                return SND_ERR_OK;
            }
            else if ((srcfmt & SND_FMT_SIGN_MASK) == SND_FMT_SIGNED) {
                // stereo->mono unsigned (bruh)
                conv->proc = &sndconv_8sus_8m;
                conv->parm = ((srcfmt & SND_FMT_SIGN_MASK) == (dstfmt & SND_FMT_SIGN_MASK)) ? 0x80808080 : 0;
                conv->format = dstfmt;
                return SND_ERR_OK;
            }
        else return SND_ERR_UNKNOWN_FORMAT; // handled by case above
    }
    
    if (((srcfmt & SND_FMT_DEPTH_MASK) == SND_FMT_INT16) && ((dstfmt & SND_FMT_DEPTH_MASK) == SND_FMT_INT8)) {
        // 16b -> 8b
        if (((srcfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_STEREO) && ((dstfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_MONO)) {
            // stereo->mono
            conv->proc   = &sndconv_16s_8m;
            conv->parm   = ((srcfmt & SND_FMT_SIGN_MASK) == (dstfmt & SND_FMT_SIGN_MASK)) ? 0x80808080 : 0;
            conv->format = dstfmt;
            return SND_ERR_OK;
        }
        else if (((srcfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_STEREO) && ((dstfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_STEREO)) {
            // stereo->stereo
            conv->proc   = &sndconv_16s_8s;
            conv->parm   = ((srcfmt & SND_FMT_SIGN_MASK) != (dstfmt & SND_FMT_SIGN_MASK)) ? 0x80808080 : 0;
            conv->format = dstfmt;
            return SND_ERR_OK;
        }
        else if (((srcfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_MONO) && ((dstfmt & SND_FMT_CHANNELS_MASK) == SND_FMT_MONO)) {
            // mono->mono
            conv->proc   = &sndconv_16m_8m;
            conv->parm   = ((srcfmt & SND_FMT_SIGN_MASK) != (dstfmt & SND_FMT_SIGN_MASK)) ? 0x80808080 : 0;
            conv->format = dstfmt;
            return SND_ERR_OK;
        }
        else return SND_ERR_UNKNOWN_FORMAT;
    }
#endif

    return SND_ERR_UNKNOWN_FORMAT;
}

// -----------------------------------------------
// DMA-like circular buffer device (both ISA DMA and PCI bus-master devices)
// -----------------------------------------------
DmaBufferDevice::DmaBufferDevice(const char* _name, uint32_t _infoPrivateBufSize) :
    SoundDevice(_name, _infoPrivateBufSize) {
    dmaChannel = -1;
    currentPos = irqs = 0;
    oldTotalPos = renderPos = 0;
    dmaBlockSize = dmaBufferCount = dmaBufferSize = dmaBufferSamples = dmaBlockSamples = dmaCurrentPtr = dmaRenderPtr = 0;
    dmaBlock.ptr = NULL; dmaBlock.dpmi.segment = dmaBlock.dpmi.selector = NULL;
}

uint32_t DmaBufferDevice::prefill() {
    if (isPaused) { resume(); return SND_ERR_RESUMED; }

    // stop if playing
    if (isPlaying) stop();

    isPaused = true;
    if (!isInitialised) return SND_ERR_UNINITIALIZED;

    // call callback to fill buffer with sound data
    {
        if (callback == NULL) return SND_ERR_NULLPTR;
#ifdef DEBUG_LOG
        printf("prefill...\n");
#endif
        soundDeviceCallbackResult rtn = callback(userdata, dmaBlock.ptr, dmaBlockSamples, &convinfo, renderPos); // fill entire buffer
        switch (rtn) {
            case callbackOk         : break;
            case callbackSkip       : 
            case callbackComplete   : 
            case callbackAbort      : 
            default : return SND_ERR_NO_DATA;
        }
        renderPos = dmaBufferSamples;
#ifdef DEBUG_LOG
        printf("done\n");
#endif
    }

    // reset vars
    currentPos = irqs = dmaCurrentPtr = 0; dmaRenderPtr = dmaBufferSize;

    return SND_ERR_OK;
}

uint32_t DmaBufferDevice::installIrq() {
    // install IRQ handler
    if (irq.hooked == false) {
        irq.flags = 0;
        irq.handler = snd_irqProcTable[devinfo.irq];
        if (irqHook(devinfo.irq, &irq, true) == true) return SND_ERR_INVALIDCONFIG;

        // set current active device
        snd_activeDevice[devinfo.irq] = this;

        return SND_ERR_OK;
    }
    else return SND_ERR_STUCK_IRQ;
}

uint32_t DmaBufferDevice::removeIrq() {
    // unhook irq if hooked
    if (irq.hooked) irqUnhook(&irq, false);

    return SND_ERR_OK;
}

static void sndlib_swapStacks();
#pragma aux sndlib_swapStacks = \
    " mov     dword ptr   [snddev_pm_old_stack + 4], ss   " \
    " mov     dword ptr   [snddev_pm_old_stack + 0], esp  " \
    " lss     esp, [snddev_pm_stack_top] "

static void sndlib_restoreStack();
#pragma aux sndlib_restoreStack = \
    " lss     esp, [snddev_pm_old_stack] "

int32_t DmaBufferDevice::getPlayPos() {
    return 0;
}

bool DmaBufferDevice::irqCallbackCaller() {
    soundDeviceCallbackResult rtn;

    // adjust playptr
    irqs++;

    // STACK POPIERDOLOLO
    if (snddev_pm_stack_in_use == 0) {
        snddev_pm_stack_in_use++;
        
        // enable interrupts
        _enable();

        // switch stack
        sndlib_swapStacks();
        
        // call callback
        // fill only previously played block
        rtn = callback(userdata, (uint8_t*)dmaBlock.ptr + dmaRenderPtr, dmaBufferSamples,
                &convinfo, renderPos);
        
        // switch back
        sndlib_restoreStack();

        // and disable interrupts again
        _disable();

        snddev_pm_old_stack = NULL;
        snddev_pm_stack_in_use--;
    }

    return rtn == callbackOk;
};

#define sndlib_min(a, b) ((a) < (b) ? (a) : (b))
#define sndlib_max(a, b) ((a) > (b) ? (a) : (b))
#define sndlib_clamp(a, l, h) (sndlib_max(sndlib_min(a, h), l))

void DmaBufferDevice::irqAdvancePos() {
    // recalc render ptr 
    // NOTE - can be optimized if buffer size/count is power of two, but i'm lazy :)
    int32_t playPos   = getPlayPos();

    // small fixup for quirky devices which fire IRQ before actually playing entire buffer
    int32_t playIdx   = (playPos + (dmaBufferSize >> 6)) / dmaBufferSize;
    if (playIdx >= dmaBufferCount) playIdx -= dmaBufferCount;
    dmaRenderPtr = (playIdx + 1) * dmaBufferSize;
    if (dmaRenderPtr >= dmaBlockSize) dmaRenderPtr = 0;

    renderPos += dmaBufferSamples;

    // adjust play position
#if 1
    int32_t playpos = playPos / convinfo.bytesPerSample;
    currentPos += (playpos < dmaCurrentPtr ? dmaBlockSamples + playpos - dmaCurrentPtr : playpos - dmaCurrentPtr);
    dmaCurrentPtr = playpos;
#else
    int32_t playpos = playPos / convinfo.bytesPerSample;
    if (playpos < dmaCurrentPtr) {
        currentPos += dmaBlockSamples;
    }
    dmaCurrentPtr = playpos;
#endif
}

uint32_t DmaBufferDevice::dmaBufferInit(uint32_t bufferSize, soundFormatConverterInfo *conv) {
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

uint32_t DmaBufferDevice::dmaBufferFree() {
    // deallocate DMA block
    if (dmaBlock.ptr != NULL) {
        dmaFree(&dmaBlock);
        dmaBlock.ptr = NULL;
    }

    // unlock DPMI memory for buffer
    dpmi_unlockmemory(dmaBlock.ptr, dmaBlockSize+64);

    return SND_ERR_OK;
}

int64_t DmaBufferDevice::getPos() {
    if (!isPlaying) return 0;
    
    uint32_t timeout = 5;
    uint64_t totalPos;
    do {
        volatile int32_t playpos = getPlayPos() / convinfo.bytesPerSample;
        volatile int32_t dmaptr  = dmaCurrentPtr;
        //playpos = sndlib_clamp(playpos, 0, dmaBlockSamples - 1);
        totalPos = currentPos + (playpos < dmaptr ? dmaBlockSamples + playpos - dmaptr : playpos - dmaptr);
    } while ((--timeout != 0) && ((totalPos < oldTotalPos) || ((totalPos - oldTotalPos) >= dmaBufferSamples)));
    if ((totalPos < oldTotalPos)) return oldTotalPos;   // give up but let the counter stop
    
    oldTotalPos = totalPos;
    return totalPos;
}

IsaDmaDevice::IsaDmaDevice(const char* _name) : DmaBufferDevice(_name) {}

int32_t IsaDmaDevice::getPlayPos() {
    int32_t pos = (((int32_t)(dmaGetCurrentAddress(dmaChannel, false) << (dmaChannel >= 4 ? 1 : 0)) & 0xFFFF) - (int32_t)(((uint32_t)dmaBlock.ptr) & 0xFFFF));
    //return pos;
    return sndlib_clamp(pos, 0, ((int32_t)dmaBlockSize - 1));
    //return dmaBlockSize - (((dmaGetCurrentCount(dmaChannel, false) + 1) & 0xFFFF) << (dmaChannel >= 4 ? 1 : 0));
}

#undef sndlib_min
#undef sndlib_max
#undef sndlib_clamp

// IRQ procedures

bool SoundDevice::irqProc() {
    return true;
}

extern "C" void __interrupt __far snd_irqStaticProc(INTPACK r);
void __interrupt __far snd_irqStaticProc(INTPACK r) {   
    SoundDevice *dev = snd_activeDevice[r.h.al];
    if (dev->irqProc() == false) return; 
    else _chain_intr(dev->irq.oldhandler);
}

// active device storage
SoundDevice *snd_activeDevice[16]; 

// device IRQ detection structure
volatile IrqDetectInfo snd_IrqDetectInfo;

#ifdef SNDDEV_IRQ_PER_DEVICE

#define EXTERN_IRQPROC(n) extern "C" void __interrupt __far snd_irqDispatch_##n()
#define IRQPROC(n) snd_irqDispatch_##n

//EXTERN_IRQPROC(0);
//EXTERN_IRQPROC(1);
//EXTERN_IRQPROC(2);
EXTERN_IRQPROC(3);
EXTERN_IRQPROC(4);
EXTERN_IRQPROC(5);
//EXTERN_IRQPROC(6);
EXTERN_IRQPROC(7);
//EXTERN_IRQPROC(8);
EXTERN_IRQPROC(9);
EXTERN_IRQPROC(10);
EXTERN_IRQPROC(11);
EXTERN_IRQPROC(12);
//EXTERN_IRQPROC(13);
//EXTERN_IRQPROC(14);
//EXTERN_IRQPROC(15);


void __interrupt __far (*snd_irqProcTable[16])() = {
    NULL,        NULL,        NULL,        IRQPROC(3),
    IRQPROC(4),  IRQPROC(5),  NULL,        IRQPROC(7),
    NULL,        IRQPROC(9),  IRQPROC(10), IRQPROC(11),
    IRQPROC(12), NULL,        NULL,        NULL
};

#endif
