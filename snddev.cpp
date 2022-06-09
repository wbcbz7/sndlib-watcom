#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#include "snddev.h"
#include "sndfmt.h"
#include "convert.h"
#include "snderror.h"

// init sound library internal resources
uint32_t sndlibInit() {
    // init PM ISR stack
    snddev_pm_stack_top = &snddev_pm_stack_top;

    // init snddev_pm_old_stack so it doesn't dangle in the void
    snddev_pm_old_stack = NULL;

    // clear in use flag
    snddev_pm_stack_in_use = 0;

    // lock memory for stack/RMCB data
    dpmi_lockmemory(&snddev_bss_lock_start, (&snddev_bss_lock_end - &snddev_bss_lock_start));

    // clear active device storage
    memset(snd_activeDevice, 0, sizeof(snd_activeDevice));

    return SND_ERR_OK;   
}

// close sound library and cleanup
uint32_t sndlibDone() {
    // unlock memory for stack/RMCB data
    dpmi_lockmemory(&snddev_bss_lock_start, (&snddev_bss_lock_end - &snddev_bss_lock_start));

    return SND_ERR_OK;   
}

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
    bool isFound = false;
    for (int i = 0; i < devinfo.capsLen; i++) {
        // check format
        if ((devinfo.caps[i].format & fmt) == 0) continue;
        
        // check rate
        if (devinfo.caps[i].ratesLength == -2) {
            if ((sampleRate < devinfo.caps[i].rates[0]) || (sampleRate > devinfo.caps[i].rates[1])) {
                continue;
            }
        }
        else {
            // iterate by hand
            size_t rate = 0;
            for (rate = 0; rate < devinfo.caps[i].ratesLength; rate++) {
                if (sampleRate == devinfo.caps[i].rates[rate]) break;
            }
            // not found?
            if (rate >= devinfo.caps[i].ratesLength) continue;
        }
        
        // target format is found!
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

uint32_t SoundDevice::pause()
{
    return SND_ERR_UNSUPPORTED;
}

uint64_t SoundDevice::getPos()
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
        
        // resolve proc by shift
        shift = 2 - shift;
        
        conv->proc   = (shift < 0) ? &sndconv_memcpy_shl : &sndconv_memcpy;
        conv->parm   = shift;
        conv->format = dstfmt;
        return SND_ERR_OK;
    }
    
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
    
    if (((srcfmt & SND_FMT_DEPTH_MASK) == SND_FMT_INT8) && ((dstfmt & SND_FMT_DEPTH_MASK) == SND_FMT_INT8)) {
        // 16b -> 16b 
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
    
    // special xlat shit for PC speaker
    // note the xlat table (ptr at conv->parm2) is for SIGNED 8bit samples (thus offsets 0..255 -> -128..127)
    if (((dstfmt & SND_FMT_DEPTH_MASK) == SND_FMT_XLAT8) && (dstfmt & SND_FMT_SIGN_MASK) == SND_FMT_UNSIGNED)  {
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
    
    return SND_ERR_UNKNOWN_FORMAT;
}

// IRQ procedures

bool SoundDevice::irqProc() {
    return true;
}

// STACK POPIERDOLOLO
void __interrupt __far snd_irqStaticProc() {   
    // reentrancy test
    if ((snddev_pm_stack_in_use == 0) && (snddev_pm_old_stack == NULL)) {
        snddev_pm_stack_in_use++;
        
        // switch stack
        _asm {
            mov     word  ptr   [snddev_pm_old_stack + 4], ss
            mov     dword ptr   [snddev_pm_old_stack + 0], esp
            lss     esp, [snddev_pm_stack_top]
        }

        // duplicate code for atomicity
        if (snd_activeDevice[0]->irqProc() == false) {
            // EOI handled by us, return from interrupt
            // switch it back
            _asm {
                lss     esp, [snddev_pm_old_stack]
            }

            snddev_pm_old_stack = NULL;
            snddev_pm_stack_in_use--;
            return;

        } else {
            // switch it back
            _asm {
                lss     esp, [snddev_pm_old_stack]
            }

            snddev_pm_old_stack = NULL;
            snddev_pm_stack_in_use--;
        }
    }
    _chain_intr(snd_activeDevice[0]->irq.oldhandler); 
};     

// active device storage
SoundDevice *snd_activeDevice[16]; 

// device IRQ detection structure
volatile IrqDetectInfo snd_IrqDetectInfo;

#ifdef SNDDEV_IRQ_PER_DEVICE

#define MAKE_IRQPROC(num)                                               \
void __interrupt __far snd_irqStaticProc##num () {                      \
    if (snd_activeDevice[##num] == NULL) {                             \
        return;                                                         \
    }                                                                   \
    if (!snd_activeDevice[##num]->inIrq) {                             \
        snd_activeDevice[##num]->inIrq = true;                         \
        bool rtn = snd_activeDevice[##num]->irqProc();                 \
        snd_activeDevice[##num]->inIrq = false;                        \
        if (rtn) _chain_intr(snd_activeDevice[##num]->irq.oldhandler); \
    } else _chain_intr(snd_activeDevice[##num]->irq.oldhandler);       \
};                                                                      \

MAKE_IRQPROC(0);
MAKE_IRQPROC(1);
MAKE_IRQPROC(2);
MAKE_IRQPROC(3);
MAKE_IRQPROC(4);
MAKE_IRQPROC(5);
MAKE_IRQPROC(6);
MAKE_IRQPROC(7);
MAKE_IRQPROC(8);
MAKE_IRQPROC(9);
MAKE_IRQPROC(10);
MAKE_IRQPROC(11);
MAKE_IRQPROC(12);
MAKE_IRQPROC(13);
MAKE_IRQPROC(14);
MAKE_IRQPROC(15);

#define IRQPROC(n) snd_irqStaticProc##n

void __interrupt __far (*snd_irqProcTable[16])() = {
    IRQPROC(0),  IRQPROC(1),  IRQPROC(2),  IRQPROC(3),
    IRQPROC(4),  IRQPROC(5),  IRQPROC(6),  IRQPROC(7),
    IRQPROC(8),  IRQPROC(9),  IRQPROC(10), IRQPROC(11),
    IRQPROC(12), IRQPROC(13), IRQPROC(14), IRQPROC(15)
};

#endif
