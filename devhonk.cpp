#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <i86.h>

#include "convert.h"
#include "sndmisc.h"
#include "snderror.h"
#include "devhonk.h"
#include "logerror.h"

// define to enable logging
//#define DEBUG_LOG

#define arrayof(x) (sizeof(x) / sizeof(x[0]))

// pc speaker io ports
const uint32_t honkIoBase[]     = { 0x42 };

const uint32_t cvxIoBase[]      = { 0x378, 0x278, 0x3BC };  // covox "default" resources
      uint32_t cvxIoBaseBios[]  = { 0x378, 0x278, 0x3BC };  // this one is filled from BIOS data area

const uint32_t honkRates[]      = { 4000, 24000 };
const uint32_t honkRates48k[]   = { 4000, 48000 };  // >24k sounds noisy on pc honker, otherwise it's ok
const uint32_t cvxRates[]       = { 4000, 48000 };

// PC honker resources
const soundResourceInfo honkRes[] = {
    {
        SND_RES_IOBASE,
        arrayof(honkIoBase),
        honkIoBase
    }
};

// Covox resources
const soundResourceInfo covoxRes[] = {
    {
        SND_RES_IOBASE,
        arrayof(cvxIoBaseBios),
        cvxIoBaseBios
    }
};

// PC honker sound caps
const soundFormatCapability honkCaps[] = {
    {
        (SND_FMT_XLAT8 | SND_FMT_MONO | SND_FMT_UNSIGNED),
        -2,         // variable range
        honkRates,
    }
};
const soundFormatCapability honkCaps48k[] = {
    {
        (SND_FMT_XLAT8 | SND_FMT_MONO | SND_FMT_UNSIGNED),
        -2,         // variable range
        honkRates48k,
    }
};
const soundFormatCapability covoxCaps[] = {
    {
        (SND_FMT_INT8  | SND_FMT_MONO | SND_FMT_UNSIGNED),
        -2,         // variable range
        cvxRates,
    }
};

const soundFormatCapability covoxStereoCaps[] = {
    {
        (SND_FMT_INT8  | SND_FMT_STEREO | SND_FMT_UNSIGNED),
        -2,         // variable range
        cvxRates,
    }
};

extern "C" {

extern void cdecl __far     snddev_irq0_callback();                         // real-mode callback
extern snddev_irq0_struct  *snddev_irq0_callback_dataofs;                   // snddev_irq0_struct linear offset
extern _dpmi_rmregs         snddev_irq0_callback_registers;                 // callback registers

// protected mode lock positions
extern uint8_t*             snddev_rm_lock_start, snddev_rm_lock_end;       // start/end of locked range
}

uint32_t _DS();
#pragma aux _DS = "mov eax, ds" value [eax];

// round(a / b)
uint32_t udivRound(uint32_t a, uint32_t b);
#pragma aux udivRound = \
        "xor edx, edx"  "div ebx"       "shr ebx, 1" \
        "cmp edx, ebx"  "jb  _skip_inc" "inc eax" \
        "_skip_inc:" parm [eax] [ebx] value [eax] modify [eax ebx edx]

// get closest timer divisor
uint32_t honkGetDivisor(uint32_t rate) {
    if (rate < 4000) return 0; 
    return udivRound(0x1234DD, rate);
}

bool sndNonDmaBase::initIrq0() {
    if (isIrq0Initialised) return true;

    // get old ISRs
    oldIrq0RealMode = dpmi_getrmvect_ex(0x8);
    oldIrq0ProtectedMode = (void __interrupt (__far*)())dpmi_getpmvect(0x8);
#ifdef DEBUG_LOG
    logdebug("old IRQ0: RM %04X:%04X, PM %04X:%08X\n", oldIrq0RealMode.segment, oldIrq0RealMode.offset, FP_SEG(oldIrq0ProtectedMode), FP_OFF(oldIrq0ProtectedMode));
#endif

    // allocate memory for data struct
    dpmi_getdosmem((sizeof(snddev_irq0_struct) + 15) >> 4, &irq0structBlock);
    irq0struct = (snddev_irq0_struct *)(irq0structBlock.segment << 4);
    if (dpmi_status != 0) {
#ifdef DEBUG_LOG
        logerr("unable to allocate DOS memory for IRQ0 struct!\n")
#endif
        return false;
    }
#ifdef DEBUG_LOG
        logdebug("IRQ0 struct linear address: 0x%08X\n", irq0struct);
#endif

    // help watcom to allocate registers :)
    snddev_patch_table *patchTable = this->patchTable;
    snddev_irq0_struct *irq0struct = this->irq0struct;

    // allocate memory for rm code
    dpmi_getdosmem(((patchTable->rm_end - patchTable->rm_start) + 15) >> 4, &realModeISRBlock);
    realModeISREntry = (void*)(realModeISRBlock.segment << 4);
    if (dpmi_status != 0) {
#ifdef DEBUG_LOG
        logerr("unable to allocate DOS memory for real-mode ISR!\n")
#endif
        return false;
    }
#ifdef DEBUG_LOG
        logdebug("RM ISR linear address: 0x%08X\n", realModeISREntry);
#endif

    // patch rm code
    *patchTable->rm_patch_dataseg  = irq0structBlock.segment;
    *patchTable->rm_patch_dt       = 0;

    // copy rm code
    memcpy(realModeISREntry, patchTable->rm_start, (patchTable->rm_end - patchTable->rm_start));
    
    // patch pm code
    *patchTable->pm_patch_dataseg  = _DS();
    *patchTable->pm_patch_dataofs  = irq0struct;
    *patchTable->pm_patch_dt       = 0;

    // allocate real-mode callback routine
    irq0struct->rm_callback = dpmi_getcallback_ex(&snddev_irq0_callback, &snddev_irq0_callback_registers);
    if (dpmi_status != 0) {
#ifdef DEBUG_LOG
        logerr("unable to allocate real mode callback (DOS4GW MUST DIE!)\n")
#endif
        return false;
    }

#ifdef DEBUG_LOG
        logdebug("RM->PM callback = %04X:%04X\n", irq0struct->rm_callback.segment, irq0struct->rm_callback.offset);
#endif

    snddev_irq0_callback_dataofs = irq0struct;

    // fill structure
    irq0struct->chain_pm        = oldIrq0ProtectedMode;
    irq0struct->chain_rm        = oldIrq0RealMode;
    irq0struct->pm_callback     = &callbackBouncer;
    irq0struct->rmcount         = irq0struct->pmcount = 0;
    irq0struct->data_port       = devinfo.iobase;
    irq0struct->data_port2      = devinfo.iobase2;
    irq0struct->chain_acc       = 0;

    // other fields are filled after stream init

    // lock ISRs and irq0 struct
    dpmi_lockmemory(realModeISREntry,   patchTable->rm_end - patchTable->rm_start);
    dpmi_lockmemory(patchTable->pm_start, patchTable->pm_end - patchTable->pm_start);
    dpmi_lockmemory(irq0struct,         sizeof(snddev_irq0_struct));

    isIrq0Initialised = true;

    return true;
}

bool sndNonDmaBase::setupIrq0() {
#if 0
    printf("chain_rm - %08X\n", irq0struct->chain_rm.ptr);
    printf("chain_pm - %04X:%08X\n", FP_SEG(irq0struct->chain_pm), FP_OFF(irq0struct->chain_pm));
    printf("data_port - %08X\n", irq0struct->data_port);
    printf("rm_callback - %08X\n", irq0struct->rm_callback.ptr);
    printf("pm_callback - %08X\n", irq0struct->pm_callback);
    printf("rmcount - %08X\n", irq0struct->rmcount);
    printf("pmcount - %08X\n", irq0struct->pmcount);
    printf("align1 - %08X\n", irq0struct->align1);
    printf("bufferseg - %08X\n", irq0struct->bufferseg);
    printf("bufferofs - %08X\n", irq0struct->bufferofs);
    printf("bufferpos - %08X\n", irq0struct->bufferpos);
    printf("bufferlencur - %08X\n", irq0struct->bufferlencur);
    printf("bufferlen - %08X\n", irq0struct->bufferlen);
    printf("bufferlentotal - %08X\n", irq0struct->bufferlentotal);
    printf("chain_acc - %08X\n", irq0struct->chain_acc);

    for (size_t i = 0; i < 128; i++) {
        if ((i & 15) == 0) printf("\n");
        printf("%02X ", ((uint8_t*)snddev_pcspeaker_irq0_proc_pm)[i]);
    }
    for (size_t i = 0; i < 128; i++) {
        if ((i & 15) == 0) printf("\n");
        printf("%02X ", ((uint8_t*)realModeISREntry)[i]);
    }
    fflush(stdout);
#endif

    _disable();

#ifdef DEBUG_LOG
        logerr("set PM ISR...\n");
#endif
    // set protected mode ISR
    dpmi_setpmvect(0x8, (void __far (*)())patchTable->pm_entry);
    if (dpmi_status != 0) {
#ifdef DEBUG_LOG
        logerr("unable to set protected mode ISR!\n");
#endif
        return false;
    }
    
#ifdef DEBUG_LOG
        logerr("set RM ISR...\n");
#endif
    // set real-mode ISR
    dpmi_setrmvect(0x8, realModeISREntry);
    if (dpmi_status != 0) {
#ifdef DEBUG_LOG
        logerr("unable to set real mode ISR!\n");
#endif
        return false;
    }
    
    // set ch0 to desired sample rate
#ifdef DEBUG_LOG
    logdebug("sample rate = %d, IRQ0 divisor = %d\n", sampleRate, timerDivisor);
#endif
    outp(0x43, 0x34);       // ch0, mode2, lsb+msb
    outp(0x40, (timerDivisor     ));
    outp(0x40, (timerDivisor >> 8));

    _enable();

    return true;
}

// remove IRQ0, don't deallocate memory
bool sndNonDmaBase::removeIrq0() {
    _disable();
    
    // restore interrupt vectors
    dpmi_setpmvect(0x8,     oldIrq0ProtectedMode);
    if (dpmi_status) return false;
    dpmi_setrmvect_ex(0x8,  oldIrq0RealMode);
    if (dpmi_status) return false;

    // reset ch0 to 18.2hz (actually we need to reset to old rate which is not always 18.2hz!)
    outp(0x43, 0x34);       // ch0, mode2, lsb+msb
    outp(0x40, 0);
    outp(0x40, 0);          // -> 18.2hz

    _enable();

    return true;
}

bool sndNonDmaBase::doneIrq0() {
    // unlock memory
    dpmi_unlockmemory(realModeISREntry,   patchTable->rm_end - patchTable->rm_start);
    dpmi_unlockmemory(patchTable->pm_start, patchTable->pm_end - patchTable->pm_start);
    dpmi_unlockmemory(irq0struct,         sizeof(snddev_irq0_struct));

    // remove callback
    dpmi_freecallback_ex(irq0struct->rm_callback);

    // free DOS allocated memory
    dpmi_freedosmem(&irq0structBlock);
    dpmi_freedosmem(&realModeISRBlock);

    return true;
}

uint32_t sndNonDmaBase::init(SoundDevice::deviceInfo* info)
{
    // deinit
    if (isInitialised) done();

    // validate fields
    SoundDevice::deviceInfo* p = (info != NULL ? info : &this->devinfo);

    // validate resources
    if ((p->iobase == -1) || (p->iobase == 0)) return SND_ERR_INVALIDCONFIG;

    // copy resource fields
    if (info != NULL) {
        this->devinfo.iobase = p->iobase;
    }

    // init IRQ0 structures
    if (initIrq0() == false) return SND_ERR_NOTFOUND;

    // phew :)
    isInitialised = true;

    return SND_ERR_OK;
}

uint32_t sndNonDmaBase::open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv) {
    uint32_t result = SND_ERR_OK;
    if ((conv == NULL) || (callback == NULL)) return SND_ERR_NULLPTR;

    // stooop!
    if (isPlaying) stop();

    // clear converter info
    memset(conv, 0, sizeof(soundFormatConverterInfo));

    soundFormat newFormat = fmt;
    // check if format is supported
    if (flags & SND_OPEN_NOCONVERT) {
        // no conversion if performed
        if (isFormatSupported(sampleRate, fmt, conv) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;

    }
    else {
        // conversion is allowed

        // suggest mono if mono only
        if ((fmt & SND_FMT_CHANNELS_MASK) >= (devinfo.caps->format & SND_FMT_CHANNELS_MASK))
            newFormat = (devinfo.caps->format & SND_FMT_CHANNELS_MASK) | (newFormat & ~SND_FMT_CHANNELS_MASK);
        else return SND_ERR_UNKNOWN_FORMAT;
        
        // test if acceptable depth
        if ((fmt & SND_FMT_DEPTH_MASK) <= SND_FMT_INT16)
            newFormat = (devinfo.caps->format & SND_FMT_DEPTH_MASK) | (newFormat & ~SND_FMT_DEPTH_MASK);

        newFormat = (devinfo.caps->format & SND_FMT_SIGN_MASK) | (newFormat & ~SND_FMT_SIGN_MASK);

        if (isFormatSupported(sampleRate, newFormat, conv) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;
    }

    // pass converter info
#ifdef DEBUG_LOG
    printf("devinfo.caps->format = 0x%x, src = 0x%x, dst = 0x%x\n", devinfo.caps->format, fmt, newFormat);
#endif

    if (getConverter(fmt, newFormat, conv) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;
    conv->bytesPerSample = getBytesPerSample(newFormat);

    // we have all relevant info for opening sound device, do it now
    if (isOpened) close();

    // allocate DMA buffer
    if (result = dmaBufferInit(bufferSize, conv) != SND_ERR_OK) return result;

    // help watcom to allocate registers :)
    snddev_irq0_struct *irq0struct = this->irq0struct;

    // set callback variables
    irq0struct->bufferseg       = (uint16_t)(((uint32_t)dmaBlock.ptr & 0xF0000) >> 4);
    irq0struct->bufferofs       = (uint8_t*)dmaBlock.ptr;
    irq0struct->bufferpos       = 0;
    irq0struct->bufferlen       = irq0struct->bufferlencur = dmaBufferSamples;
    irq0struct->bufferlentotal  = dmaBlockSize;

    // set irqproc
    snd_activeDevice[0] = this;
    inIrq = false;

    // fill conversion table, pass and link it to convinfo
    timerDivisor = honkGetDivisor(sampleRate);
#ifdef DEBUG_LOG
    printf("sample rate = %d hz, timer divisor = %d\n", sampleRate, timerDivisor);
#endif
    if (initConversionTab() == false) {
#ifdef DEBUG_LOG
        logerr("unable to init conversion table!\n");
#endif
        return SND_ERR_UNINITIALIZED;
    }
#ifdef DEBUG_LOG
    printf("xlat table = %08X\n", convtab);
#endif
    conv->parm2 = (uint32_t)convtab;

    // save callback info
    this->callback = callback;
    this->userdata = userdata;

    // pass coverter info
    memcpy(&convinfo, conv, sizeof(convinfo));

    this->currentFormat = newFormat;
    this->sampleRate = sampleRate;

    // debug output
#ifdef DEBUG_LOG
    fprintf(stderr, __func__": requested format 0x%X, opened format 0x%X, rate %d hz, buffer %d bytes, flags 0x%X\n", fmt, newFormat, sampleRate, bufferSize, flags);
#endif

    isOpened = true;
    return SND_ERR_OK;
}

uint64_t sndNonDmaBase::getPos() {
    // read info from irq0 structure
    if (isPlaying) {
        return currentPos + irq0struct->bufferpos / convinfo.bytesPerSample;
    }
    else return 0;
}

void sndNonDmaBase::callbackBouncer() {
    snd_activeDevice[0]->irqProc();
}

uint32_t sndNonDmaBase::ioctl(uint32_t function, void *data, uint32_t len) {
    return SND_ERR_UNSUPPORTED;
}

// irq procedure
bool sndNonDmaBase::irqProc() {
    // advance play pointers
    irqAdvancePos();

    // call callback
    irqCallbackCaller();

    return false;   // we're handling EOI by itself
}

uint32_t sndNonDmaBase::pause()
{
    // uninstall IRQ0
    removeIrq0();

    isPaused = true;
    return SND_ERR_OK;
}

uint32_t sndNonDmaBase::resume()
{
    // resume playback
    setupIrq0();

    isPaused = false;
    return SND_ERR_RESUMED;
}


uint32_t sndNonDmaBase::start() {
    uint32_t rtn = SND_ERR_OK;
    if (rtn = prefill() != SND_ERR_OK) return rtn;

    // ------------------------------------
    // device-specific code

    // set patching variables
    uint32_t chain_acc = (timerDivisor << 16);
    *((uint32_t*)(((uint8_t*)patchTable->rm_patch_dt - patchTable->rm_start) + (uint8_t*)realModeISREntry)) = chain_acc;
    *patchTable->pm_patch_dt = chain_acc;

    // install IRQ0 and start playback
    if (initPort() == false) {
        return SND_ERR_UNINITIALIZED;
    }
    if (setupIrq0() == false) {
        return SND_ERR_UNINITIALIZED;
    }

#ifdef DEBUG_LOG
    printf("playback started\n");
#endif

    // ------------------------------------

    // done! we're playing sound :)
    isPaused = false; isPlaying = true;

    return SND_ERR_OK;
}

uint32_t sndNonDmaBase::stop() {
    if (isPlaying) {
        // uninstall IRQ0
        donePort();
        removeIrq0();
    }

    isPlaying = false;

    // clear playing position
    currentPos = renderPos = irqs = 0;
    dmaCurrentPtr = dmaRenderPtr = 0;

    return SND_ERR_OK;
}

uint32_t sndNonDmaBase::close() {
    // stop playback
    if (isPlaying) stop();
    
    // deallocate DMA block
    dmaBufferFree();

    // deinit conversion table
    if (doneConversionTab() == false) {
#ifdef DEBUG_LOG
        logerr("unable to remove conversion table!\n");
#endif
        return SND_ERR_UNINITIALIZED;
    }

    // fill with defaults
    isOpened = false;
    dmaBlockSize = dmaBufferSize = dmaBufferSamples = dmaBlockSamples = 0;
    sampleRate = timerDivisor = 0;
    currentFormat = SND_FMT_NULL;

    return SND_ERR_OK;
}

uint32_t sndNonDmaBase::done() {
    if (isOpened) close();

    // remove all irq0 stuff
    if (isIrq0Initialised) doneIrq0();
    isInitialised = isIrq0Initialised = false;

    return SND_ERR_OK;
}

// --------------------------------------
// (the almighty) PC Honker driver


uint32_t sndPcSpeaker::detect(SoundDevice::deviceInfo *info) {
    // clear and fill device info
    this->devinfo.clear();

    // always present :P
    this->devinfo.iobase = 0x0042;
    this->devinfo.caps = honkCaps48k;
    this->devinfo.capsLen = arrayof(honkCaps48k);
    this->devinfo.maxBufferSize = 32768;        // BYTES
    this->devinfo.name = "PC Speaker";
    this->devinfo.version = "PWM";
    this->devinfo.flags = SND_DEVICE_CUSTOMFORMAT | SND_DEVICE_IRQ0 | SND_DEVICE_CLOCKDRIFT;

    // copy info if not NULL
    if (info != NULL) *info = devinfo;

    return SND_ERR_OK;
}

bool sndPcSpeaker::initPort() {
    // program PIT
    outp(0x43, 0x90);       // ch2, mode0, lsb only
    outp(0x42, 0);          // shut it up
    
    // enable speaker
    outp(0x61, inp(0x61) | 3);

    return true;
}

bool sndPcSpeaker::donePort() {
    // shut the speaker up
    outp(0x61, inp(0x61) & ~3);

    return true;
}

bool sndPcSpeaker::initConversionTab() {
    convtab = new uint8_t[256]; if (convtab == NULL) return false;
    
    // signed 8bit -> i8253 pwm timer value
    uint32_t maxValue = (timerDivisor > 256 ? 256 : timerDivisor) - 1;

    // linear
    for (int32_t i = 0; i < 256; i++) {
        convtab[i ^ 128] = ((maxValue * i) >> 8) + 1;   // fix i8253 rate 0==65536
    }

    return true;
}

bool sndPcSpeaker::doneConversionTab() {

    if (convtab != NULL) {
        delete[] convtab;
        convtab = NULL;
    }

    return true;
}

// --------------------------------------
// Covox Speech Thing aka LPT DAC, mono

void sndCovox::scanBDA() {
    uint16_t *bdaLpt = (uint16_t*)(0x408);
    for (size_t i = 0; i < 3; i++) {
        cvxIoBaseBios[i] = ((bdaLpt[i] != 0) && (bdaLpt[i] != -1)) ? bdaLpt[i] : -1;
    }
}

uint32_t sndCovox::detect(SoundDevice::deviceInfo *info) {
    // clear and fill device info
    this->devinfo.clear();

    if (info == NULL) return 0;

    // not a user-supplied info
    if (devinfo.iobase == -1) {
        // use data from BDA, scan backwards (so user can add own custom ports)
        for (int i = 2; i >= 0; i--) if (cvxIoBaseBios[i] != -1) { devinfo.iobase = cvxIoBaseBios[i]; break; }
    }

    // if still not found..
    if (devinfo.iobase == -1) return SND_ERR_NOTFOUND;

#ifdef DEBUG_LOG
    logdebug("io = 0x%X\n", devinfo.iobase);
#endif

    // fill caps
    if (fillCodecInfo(&this->devinfo) != SND_ERR_OK) return false;

    // copy info if not NULL
    if (info != NULL) *info = devinfo;

    return SND_ERR_OK;
}

uint32_t sndCovox::fillCodecInfo(SoundDevice::deviceInfo* info) {
    info->maxBufferSize = 32768;
    info->caps          = covoxCaps;
    info->capsLen       = arrayof(covoxCaps);
    info->name          = "Covox LPT DAC";
    info->flags         = SND_DEVICE_IRQ0 | SND_DEVICE_CLOCKDRIFT;

    // put IO base  address in private buffer
    snprintf(info->privateBuf, sizeof(info->privateBuf), "port 0x%03X", info->iobase);
    info->version = info->privateBuf;

    return SND_ERR_OK;
}

// --------------------------------------
// Dual Covox aka Dual LPT DAC, stereo

uint32_t sndDualCovox::detect(SoundDevice::deviceInfo *info) {
    // clear and fill device info
    this->devinfo.clear();

    if (info == NULL) return 0;

    // not a user-supplied info
    if (devinfo.iobase == -1) {
        // use data from BDA, scan backwards (so user can add own custom ports)
        for (int i = 1; i >= 0; i--) if ((cvxIoBaseBios[i] != -1) && (cvxIoBaseBios[i+1] != -1)) {
            devinfo.iobase  = cvxIoBaseBios[i];
            devinfo.iobase2 = cvxIoBaseBios[i+1];
            break;
        }
    }

    // if still not found..
    if ((devinfo.iobase == -1) || (devinfo.iobase2 == -1)) return SND_ERR_NOTFOUND;

#ifdef DEBUG_LOG
    logdebug("io = 0x%X\n", devinfo.iobase);
#endif

    // fill caps
    if (fillCodecInfo(&this->devinfo) != SND_ERR_OK) return false;

    // copy info if not NULL
    if (info != NULL) *info = devinfo;

    return SND_ERR_OK;
}

uint32_t sndDualCovox::fillCodecInfo(SoundDevice::deviceInfo* info) {
    info->maxBufferSize = 32768;
    info->caps          = covoxStereoCaps;
    info->capsLen       = arrayof(covoxStereoCaps);
    info->name          = "Dual Covox DAC";
    info->flags         = SND_DEVICE_IRQ0 | SND_DEVICE_CLOCKDRIFT;

    // put IO base  address in private buffer
    snprintf(info->privateBuf, sizeof(info->privateBuf), "port 0x%03X/0x%03X", info->iobase, info->iobase2);
    info->version = info->privateBuf;

    return SND_ERR_OK;
}

// --------------------------------------
// Stereo-On-1 LPT DAC, stereo :)

uint32_t sndStereoOn1::detect(SoundDevice::deviceInfo *info) {
    // clear and fill device info
    this->devinfo.clear();

    if (info == NULL) return 0;

    // not a user-supplied info
    if (devinfo.iobase == -1) {
        // use data from BDA, scan backwards (so user can add own custom ports)
        for (int i = 2; i >= 0; i--) if (cvxIoBaseBios[i] != -1) {
            // test for stereo-on-1 presence (pins 9 and 11 are bridged on LPT port)
            outp(cvxIoBaseBios[i], 0x00);
            if ((inp(cvxIoBaseBios[i] + 1) & 0x80) == 0x80) {
                outp(cvxIoBaseBios[i], 0x80);
                if ((inp(cvxIoBaseBios[i] + 1) & 0x80) == 0x00) {
                    // found!
                    devinfo.iobase = cvxIoBaseBios[i];
                    break;
                }
            }
        }
    }

    // if still not found..
    if (devinfo.iobase == -1) return SND_ERR_NOTFOUND;

#ifdef DEBUG_LOG
    logdebug("io = 0x%X\n", devinfo.iobase);
#endif

    // fill caps
    if (fillCodecInfo(&this->devinfo) != SND_ERR_OK) return false;

    // copy info if not NULL
    if (info != NULL) *info = devinfo;

    return SND_ERR_OK;
}

uint32_t sndStereoOn1::fillCodecInfo(SoundDevice::deviceInfo* info) {
    info->maxBufferSize = 32768;
    info->caps          = covoxStereoCaps;
    info->capsLen       = arrayof(covoxStereoCaps);
    info->name          = "Stereo-On-1 LPT DAC";
    info->flags         = SND_DEVICE_IRQ0 | SND_DEVICE_CLOCKDRIFT;

    // put IO base address in private buffer
    snprintf(info->privateBuf, sizeof(info->privateBuf), "port 0x%03X", info->iobase);
    info->version = info->privateBuf;

    return SND_ERR_OK;
}

bool sndStereoOn1::initPort() {
    // enable both channels if mono
    if ((currentFormat & SND_FMT_CHANNELS_MASK) == SND_FMT_MONO) 
        outp(devinfo.iobase + 2, 3);

    return true;
}

bool sndStereoOn1::donePort() {
    // play nice and switch to mono mode at exit :)
    outp(devinfo.iobase + 2, 3);

    return true;
}

uint32_t sndStereoOn1::ioctl(uint32_t function, void *data, uint32_t len) {
    if (data == NULL) return SND_ERR_NULLPTR;

    switch (function) {
        case SND_IOCTL_STEREO_ON_1_FAST_PROTOCOL_SET:
            patchTable = ((uint32_t*)data != 0) ? &snddev_irq0_patch_stereo1_fast : &snddev_irq0_patch_stereo1;
            break;

        case SND_IOCTL_STEREO_ON_1_FAST_PROTOCOL_GET:
            *(uint32_t*)data = (patchTable == &snddev_irq0_patch_stereo1_fast) ? 1 : 0;
            break;

        default:
            return sndNonDmaBase::ioctl(function, data, len);
    }
    return SND_ERR_UNSUPPORTED;
}