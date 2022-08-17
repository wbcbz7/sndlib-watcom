#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <conio.h>
#include <sndlib.h>

/*
    advanced sine generator with manual format conversion - demonstrates simple sound synthesis in a callback
*/

// ------------------------------
// fixedpoint math helpers

#ifndef min
#define min(a, b)      ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b)      ((a) > (b) ? (a) : (b))
#endif
#ifndef sgn
#define sgn(a)         ((a) < (0) ? (-1) : ((a) > (0) ? (1) : (0)))
#endif
#ifndef clamp
#define clamp(a, l, h) ((a) > (h) ? (h) : ((a) < (l) ? (l) : (a)))
#endif

// (x * y) / z
int32_t imuldiv16(int32_t x, int32_t y, int32_t z);
#pragma aux imuldiv16 = \
    " imul  edx        "\
    " idiv  ebx        "\
    parm [eax] [edx] [ebx] modify exact [eax edx] value [eax]

// (x * y) >> 16
int32_t imul16(int32_t x, int32_t y);
#pragma aux imul16 = \
    " imul  edx        "\
    " shrd  eax,edx,16 "\
    parm [eax] [edx] value [eax]
    
// (x << 16) / y
int32_t idiv16(int32_t x, int32_t y);
#pragma aux idiv16 = \
    " mov   edx,eax    "\
    " sar   edx,16     "\
    " shl   eax,16     "\
    " idiv  ebx        "\
    parm [eax] [ebx] modify exact [eax edx] value [eax]

// (x * y) >> 8
int32_t imul8(int32_t x, int32_t y);
#pragma aux imul8 = \
    " imul  edx        "\
    " shrd  eax,edx,8  "\
    parm [eax] [edx] value [eax]
    
// (x << 8) / y
int32_t idiv8(int32_t x, int32_t y);
#pragma aux idiv8 = \
    " mov   edx,eax    "\
    " sar   edx,24     "\
    " shl   eax,8      "\
    " idiv  ebx        "\
    parm [eax] [ebx] modify exact [eax edx] value [eax]

// ------------------------------
// sine table 

const size_t SINTAB_SIZE = 256;

int32_t sintab[SINTAB_SIZE + 1];    // interpolation fixup

// iterative sine generator from https://www.musicdsp.org/en/latest/Synthesis/10-fast-sine-and-cosine-calculation.html
// c = (int)(65536.0*2.0*sin(PI/SINTAB_SIZE)), realculate it yourself for other SINTAB_SIZE
void makeWavetable() {
#if 1
    int32_t a = 32767, b = 0, c = 1608;
    for (size_t i = 0; i < SINTAB_SIZE; i++) {
        sintab[i] = b;
        a -= imul16(b,c);
        b += imul16(a,c);
    }
#else 
    for (size_t i = 0; i < SINTAB_SIZE; i++) {
        sintab[i] = ((65536 * i) / SINTAB_SIZE) - 32768;
    }
#endif
    sintab[SINTAB_SIZE] = sintab[0];
}

// ------------------------------
// callback context structure

struct callbackContext {
    uint32_t channels;

    // sine synth variables
    struct {
        int32_t vol;            // 16.16 fixedpoint
        int32_t pos;            // 24.8 fixedpoint
        int32_t delta;          // 24.8 fixedpoint
    } synth[2];

    // xor mask for flipping the sign
    uint32_t xorval;
};

// ------------------------------
// callback

void renderpc(void* buffer, uint32_t samples, callbackContext *ctx, soundFormatConverterInfo *info) {
    for (size_t j = 0; j < ctx->channels; j++) {
        uint8_t *p = (uint8_t*)buffer + j;
        for (uint32_t i = 0; i < samples; i++)  {
            int32_t intPos = ctx->synth[j].pos >> 8, fracPos = ctx->synth[j].pos & 0xFF;
            int32_t a = imul16(
                ((sintab[intPos] * (256 - fracPos)) + (sintab[intPos + 1] * (fracPos))),
                ctx->synth[j].vol) >> 16;
            a = clamp(a, -128, 127);
            *p = ((uint8_t*)info->parm2)[(uint8_t)a]; p += ctx->channels;
            ctx->synth[j].pos = (ctx->synth[j].pos + ctx->synth[j].delta) & (((SINTAB_SIZE - 1) << 8) + 0xFF);
        }
    }
}

void render8(void* buffer, uint32_t samples, callbackContext *ctx) {
    for (size_t j = 0; j < ctx->channels; j++) {
        uint8_t *p = (uint8_t*)buffer + j;
        for (uint32_t i = 0; i < samples; i++) {
            int32_t intPos = ctx->synth[j].pos >> 8, fracPos = ctx->synth[j].pos & 0xFF;
            int32_t a = imul16(
                ((sintab[intPos] * (256 - fracPos)) + (sintab[intPos + 1] * (fracPos))),
                ctx->synth[j].vol) >> 16;
            a = clamp(a, -128, 127);
            *p = (uint16_t)(a ^ ctx->xorval); p += ctx->channels;
            ctx->synth[j].pos = (ctx->synth[j].pos + ctx->synth[j].delta) & (((SINTAB_SIZE - 1) << 8) + 0xFF);
        }
    }
}

void render16(void* buffer, uint32_t samples, callbackContext *ctx) {
    for (size_t j = 0; j < ctx->channels; j++) {
        uint16_t *p = (uint16_t*)buffer + j;
        for (uint32_t i = 0; i < samples; i++) {
            int32_t intPos = ctx->synth[j].pos >> 8, fracPos = ctx->synth[j].pos & 0xFF;
            int32_t a = imul16(
                ((sintab[intPos] * (256 - fracPos)) + (sintab[intPos + 1] * (fracPos))),
                ctx->synth[j].vol) >> 8;
            a = clamp(a, -32768, 32767);
            *p = (uint16_t)(a ^ ctx->xorval); p += ctx->channels;
            ctx->synth[j].pos = (ctx->synth[j].pos + ctx->synth[j].delta) & (((SINTAB_SIZE - 1) << 8) + 0xFF);
        }
    }
}

soundDeviceCallbackResult callback(void* userPtr, void* buffer, uint32_t bufferSamples, soundFormatConverterInfo *fmt, uint64_t bufferPos) {
    // cast userPtr to callbackContext 
    callbackContext *ctx = (callbackContext*)userPtr;

#if DEBUG_RASTER == 1
    outp(0x3c8, 0); outp(0x3c9, 0x1f); outp(0x3c9, 0x0); outp(0x3c9, 0x1f);
#endif

    // call appropriate synth
    switch (fmt->format & SND_FMT_DEPTH_MASK) {
        case SND_FMT_XLAT8: renderpc(buffer, bufferSamples, ctx, fmt); break;
        case SND_FMT_INT8:  render8 (buffer, bufferSamples, ctx); break;
        case SND_FMT_INT16: render16(buffer, bufferSamples, ctx); break;
        default: return callbackAbort;
    }

#if DEBUG_RASTER == 1
    outp(0x3c8, 0); outp(0x3c9, 0); outp(0x3c9, 0); outp(0x3c9, 0);
#endif

    // and exit callback
    return callbackOk;
}

// ------------------------------
// static data structures

SoundDevice     *dev = NULL;
callbackContext callbackContext;
soundFormatConverterInfo convinfo;

// ------------------------------
// initialization 

bool init(bool autosetup) {
    uint32_t rtn = SND_ERR_OK;

    // init sndlib
    if ((rtn = sndlibInit()) != SND_ERR_OK) {
        printf("error: unable to init sndlib!\n");
    }

    // query, detect, create sound device
    rtn = sndlibCreateDevice(&dev, autosetup ? SND_CREATE_DEVICE_AUTO_DETECT : SND_CREATE_DEVICE_MANUAL_SELECT);
    if (rtn == SND_ERR_USEREXIT) {
        printf("user exit\n");
        return false;
    }
    if (rtn != SND_ERR_OK) {
        printf("error: no available sound devices found!\n");
        return false;
    }

    // init device
    rtn = dev->init();
    if (rtn != SND_ERR_OK) {
        printf("error: unable to init sound device (rtn = %d)\n", rtn);
        sndlibDestroyDevice(dev);
        return false;
    }

    printf("%s init success\n", dev->getName());

    return true;
}

bool open(uint32_t sampleRate, uint32_t bufferSamples, bool mono, bool use8bit) {
    soundFormat format = (mono ? SND_FMT_MONO : SND_FMT_STEREO) | (use8bit ? SND_FMT_INT8 : SND_FMT_INT16);
    memset(&convinfo, 0, sizeof(convinfo));

    // check if format is supported
    if (dev->isFormatSupported(sampleRate == -1 ? SND_ISFORMATSUPPORTED_MAXSAMPLERATE : sampleRate, format | SND_FMT_SIGN_MASK, &convinfo) != SND_ERR_OK) {
        // downgrade the format
        format = (format & ~SND_FMT_DEPTH_MASK) | SND_FMT_INT8;
        if (dev->isFormatSupported(sampleRate == -1 ? SND_ISFORMATSUPPORTED_MAXSAMPLERATE : sampleRate, format | SND_FMT_SIGN_MASK, &convinfo) != SND_ERR_OK) {
            // downgrade it more :)
            format = (format & ~SND_FMT_CHANNELS_MASK) | SND_FMT_MONO;
            if (dev->isFormatSupported(sampleRate == -1 ? SND_ISFORMATSUPPORTED_MAXSAMPLERATE : sampleRate, format | SND_FMT_SIGN_MASK, &convinfo) != SND_ERR_OK) {
                // downgrade it EVEN more :) for PC speaker
                format = (format & ~SND_FMT_DEPTH_MASK) | SND_FMT_XLAT8;
                if (dev->isFormatSupported(sampleRate == -1 ? SND_ISFORMATSUPPORTED_MAXSAMPLERATE : sampleRate, format | SND_FMT_SIGN_MASK, &convinfo) != SND_ERR_OK) {
                    // oops!
                    printf("error: unsupported sample rate or format!\n");
                    return false;
                }
            }
        }
    }
        
    // get sample rate
    if (sampleRate == -1) sampleRate = convinfo.sampleRate;

    // get format sign
    format = (format & ~SND_FMT_SIGN_MASK) | (convinfo.format & SND_FMT_SIGNED ? SND_FMT_SIGNED : SND_FMT_UNSIGNED);

    // finally open the device!
    // note SND_OPEN_NOCONVERT flag, that tells to use exact requested format
    uint32_t rtn = dev->open(sampleRate, format, bufferSamples, SND_OPEN_NOCONVERT, callback, &callbackContext, &convinfo);
    if (rtn != SND_ERR_OK) {
        printf("error: unable to open sound device (rtn = %d)\n", rtn);
        return false;
    }

    printf("stream open success: %d->%d hz %sbit %s %s\n",
        convinfo.sourceSampleRate,  convinfo.sampleRate,
        convinfo.format & SND_FMT_INT8  ? "8" : (convinfo.format & SND_FMT_INT16 ? "16" : "unk"),
        convinfo.format & SND_FMT_MONO ? "mono" : "stereo",
        convinfo.format & SND_FMT_SIGNED ? "signed" : "unsigned"
        );

    // init synth
    // generate 500hz tone in left/mono channel and 1000hz in right channel
    for (size_t i = 0; i < 2; i++) {
        callbackContext.synth[i].vol = 0xC000;       // 0.75
        callbackContext.synth[i].pos = 0;
        callbackContext.synth[i].delta = idiv8((SINTAB_SIZE * 500 * (i + 1)), convinfo.sampleRate);
    }
    callbackContext.channels = (convinfo.format & SND_FMT_STEREO ? 2 : 1);
    callbackContext.xorval   = (convinfo.format & SND_FMT_UNSIGNED ? 0x80 : 0) << (convinfo.format & SND_FMT_INT16 ? 8 : 0);

    printf("synth init complete\n");

    return true;
}

void close() {
    // stop and close device
    dev->stop();
    dev->close();

    // destroy device
    sndlibDestroyDevice(dev);

    // cleanup sndlib
    sndlibDone();
}


void showHelp() {
    printf("usage: sine.exe [sampleRate] [mono] [8bit] [autosetup]\n");
}

int main(int argc, char* argv[]) {
    // init sine table
    makeWavetable();

    // options
    bool downmix = false, mono = false, use8bit = false, autosetup = false;
    uint32_t sampleRate = -1, rtn;

    for (size_t i = 1; i < argc; i++) {
        // first try to convert to sample rate
        uint32_t d = strtol(argv[i], NULL, 10); if (d >= 4000) {sampleRate = d; continue;}

        // else parse as option
        char a = toupper(*argv[i]);

        switch(a) {
            case '?': showHelp(); return 0;
            case 'A': autosetup = true; break;
            case 'M': mono = true; break;
            case '8': use8bit = true; break;
            default: break;
        }
    }

    // init and open sound device
    if (init(autosetup) == false) return 1;
    if (open(sampleRate, 1024, mono, use8bit) == false) {close(); return 1;}
    
    // start!
    if ((rtn = dev->start()) != SND_ERR_OK) {
        printf("error: unable to start sound device (rtn = %d)\n", rtn);
        close();
        return 1;
    }

    bool isRunnning = true, isPause = false;
    printf("space to pause/resume, esc to exit\n");
    printf("[q/w] to adjust left channel delta, [a/s] - right, [z/x] - volume\n");

    // loop
    while (isRunnning) {
        
        // keyboard control
        if (kbhit()) {
            char a = toupper(getch());
            switch (a) {
                case 'Q' : callbackContext.synth[0].delta = clamp(callbackContext.synth[0].delta - 0x10, 0, 0xFFFFFF); break;
                case 'W' : callbackContext.synth[0].delta = clamp(callbackContext.synth[0].delta + 0x10, 0, 0xFFFFFF); break;
                case 'A' : callbackContext.synth[1].delta = clamp(callbackContext.synth[1].delta - 0x10, 0, 0xFFFFFF); break;
                case 'S' : callbackContext.synth[1].delta = clamp(callbackContext.synth[1].delta + 0x10, 0, 0xFFFFFF); break;
                case 'Z' : callbackContext.synth[0].vol = clamp(callbackContext.synth[0].vol - 0x100, 0, 0xFFFF);
                           callbackContext.synth[1].vol = clamp(callbackContext.synth[1].vol - 0x100, 0, 0xFFFF); break;
                case 'X' : callbackContext.synth[0].vol = clamp(callbackContext.synth[0].vol + 0x100, 0, 0xFFFF);
                           callbackContext.synth[1].vol = clamp(callbackContext.synth[1].vol + 0x100, 0, 0xFFFF); break;
                case ' ' :
                    isPause = !isPause;
                    if (isPause) dev->pause(); else dev->resume();
                    break;
                case 0x1B: isRunnning = false; break;
                default:  break;
            }
        }
        
        // get current position in samples, convert to mm:ss:ms
        uint64_t pos                = dev->getPos();
        uint64_t posInMilliseconds  = (pos * 1000 / convinfo.sampleRate);
        uint32_t minutes = (posInMilliseconds / (1000 * 60));
        uint32_t seconds = (posInMilliseconds / 1000) % 60;
        uint32_t ms      = (posInMilliseconds % 1000);

        printf("\rplaypos = %8llu samples, %02d:%02d.%03d, synth delta: %04X/%04X, volume %04X/%04X", 
            pos, minutes, seconds, ms,
            callbackContext.synth[0].delta, callbackContext.synth[1].delta,
            callbackContext.synth[0].vol,   callbackContext.synth[1].vol); fflush(stdout);
        
        // wait for vertical retrace
        while (inp(0x3da) & 8); while (!(inp(0x3da) & 8));

    };

    // done :) cleanup
    close();

    return 0;
}