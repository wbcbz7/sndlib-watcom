#pragma once

#include <stdint.h>
#include "sndfmt.h"
#include "snddefs.h"

// convert.asm imports
#pragma aux __sndconvcall "*_\\conv" parm caller [edi] [esi] [ecx] [edx] [ebx] \
                        value [eax] modify [eax ebx ecx edx esi edi]

#define __sndconvcall __declspec(__pragma("__sndconvcall"))

typedef int __sndconvcall (*soundFormatConverter) (void*, void*, uint32_t, uint32_t, uint32_t);

struct soundFormatConverterInfo {
    soundFormat             format;                 // current   sound format
    soundFormatConverter    proc;                   // converter procedure pointer
    uint32_t                parm;                   // passed in edx while calling proc
    uint32_t                parm2;                  // passed in ebx while calling proc
    uint32_t                bytesPerSample;         // bytes per each sample
    uint32_t                sourceSampleRate;       // requested sample rate
    uint32_t                sampleRate;             // actual    sample rate
};

// callback return codes
enum soundDeviceCallbackResult {
    callbackOk, callbackComplete, callbackSkip, callbackAbort
};

// callback definition
typedef soundDeviceCallbackResult(*soundDeviceCallback)(void* userPtr, void* buffer, uint32_t bufferSamples, soundFormatConverterInfo *fmt, uint64_t bufferPos);

int __sndconvcall sndconv_memcpy(void *dst, void *src, uint32_t length, uint32_t div, uint32_t);

#ifdef SNDLIB_CONVERT_ENABLE_ARBITRARY
int __sndconvcall sndconv_16s_16m(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t);
int __sndconvcall sndconv_8sus_8m(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t);
int __sndconvcall sndconv_8s_8m(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t);
int __sndconvcall sndconv_16s_8s(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t);
int __sndconvcall sndconv_16m_8m(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t);
int __sndconvcall sndconv_16s_8m(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t);
int __sndconvcall sndconv_8_16(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t lenmul);
#endif

#ifdef SNDLIB_CONVERT_ENABLE_PCSPEAKER
int __sndconvcall sndconv_16m_xlat(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t xlatPtr);
int __sndconvcall sndconv_8m_xlat (void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t xlatPtr);
int __sndconvcall sndconv_16s_xlat(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t xlatPtr);
int __sndconvcall sndconv_8s_xlat (void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t xlatPtr);
#endif
