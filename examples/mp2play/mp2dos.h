#pragma once

#include <stdint.h>

#include "mp2dec.h"
#include "mp2play.h"
#include <sndlib.h>


class mp2play_dos : public mp2play {

public:
    mp2play_dos() : isInitialized(false), isPlaying(false), isPaused(false), isDownsampled(false), isMono(false), mp2(NULL), pool(NULL), poolEntries(NULL), decoder() {}
    virtual ~mp2play_dos();

    // init player
    // sets up [decodedPool] decoded frames ring buffer
    // bufferSamples sets primary (aka DMA) buffer length
    // autosetup = false for manual sound device selection
    virtual bool init(uint32_t decodedPool, uint32_t bufferSamples, bool autosetup = true, bool mono = false, bool downsample2to1 = false);
    
    // load mp2 from file/memory, predecode samples to ring buffer
    virtual bool load(const char *filename);
    virtual bool loadmem(void * ptr, uint32_t size);

    // play (pos in frames == samples*1152)
    virtual bool play(uint64_t pos = -1);

    // render frames (0 - fill entire ringbuffer)
    virtual bool decode(uint32_t frames);

    // pause
    virtual bool pause();

    // resume
    virtual bool resume();

    // get position (+/- 20 samples accurate, i dunno, what about RTC interpolation?)
    virtual uint64_t getPos();

    // get sample rate
    virtual uint32_t getSampleRate() {
        return mp2header.sampleRate;
    }
    
    // stop
    virtual bool stop();

    // done
    virtual bool done();

protected:

    MP2Decoder              decoder;
    MP2Decoder::MPEGHeader  mp2header;
    
    uint8_t     *mp2;
    uint8_t     *mp2pos;
    uint32_t    mp2size;
    void        *pool;
    
    char        *stack;
    char        *stackTop;

#pragma pack(push, 16)
    struct poolEntry {
        poolEntry*  next;
        uint32_t    length;     // in BYTES
        void*       samples;
        bool        consumed;
    };
#pragma pack(pop)

    poolEntry* poolEntries;

    // pool
    struct poolState {
        uint32_t    poolSize;
        uint32_t    poolBytesPerEntry;
        uint32_t    bufferSamples;

        // head (fed to callback)
        poolEntry*  firstEntry;         // ptr
        uint32_t    firstIdx;
        uint32_t    firstIdxIsr;        // сохраняется коллбеком
        uint64_t    firstPos;           // frame pos
        uint32_t    firstRemainingPos;  // семпол с которого следующий колбек нарежет звук когда его попросят
        uint64_t    firstCurrentPos;

        // tail (fed by render()/decode())
        poolEntry*  lastEntry;
        uint32_t    lastIdx;
        uint64_t    lastPos;

        uint32_t    poolAvail;
        uint32_t    poolWatermark;      // если (poolAvail <= poolWatermark) && (!isModifying), то не дожидаясь юзера прямо из колбека нарезаем ceil(bufferSamples*2/1152 + 1) фреймов в пул
    };

    poolState       state;

    uint64_t        bufferPlay;
    uint64_t        bufferStart;        // updated each callback, used for getPos() interpolation
    uint64_t        bufferOldStart;
    uint64_t        bufferDelta;        // same

    bool            isMono;
    bool            isDownsampled;
    bool            isInitialized;
    bool            isPlaying;
    bool            isPaused;
    
    volatile bool   isIsrFinished;
    // "hey we're rendering more frames don't peek us much" flag
    volatile bool   isModifying;

    // sndlib stuff
    SoundDevice     *dev;
    soundFormatConverterInfo   convinfo;

    static  soundDeviceCallbackResult callbackBouncer(void* userPtr, void* buffer, uint32_t bufferSamples, soundFormatConverterInfo *fmt, uint64_t bufferPos);
public:
    soundDeviceCallbackResult   callback(void* buffer, uint32_t bufferSamples, soundFormatConverterInfo *fmt, uint64_t bufferPos);

    bool allocateBuffers();

    // downsampler

};