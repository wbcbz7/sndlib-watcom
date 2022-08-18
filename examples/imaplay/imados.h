#pragma once

#include <stdint.h>
#include <sndlib.h>

#include "imaplay.h"
#include "wavehdr.h"

class imaplay_dos : public imaplay {

public:
    imaplay_dos(); 
    virtual ~imaplay_dos();

    // init player
    // sets up [decodedPool] decoded frames ring buffer
    // bufferSamples sets primary (aka DMA) buffer length
    // autosetup = false for manual sound device selection
    virtual bool init(uint32_t decodedPool, uint32_t bufferSamples, uint32_t resampleMode, bool autosetup = true);
    
    // load mp2 from file/memory, predecode samples to ring buffer
    virtual bool load(const char *filename, uint32_t filebufsize = -1);
    virtual bool loadmem(void * ptr, uint32_t size);

    // play (pos in samples)
    virtual bool play(uint64_t pos = -1);

    // render frames (0 - fill entire ringbuffer)
    virtual bool decode(uint32_t frames);

    // pause
    virtual bool pause();

    // resume
    virtual bool resume();

    // get position in samples
    virtual int64_t  getPos();

    // get sample rate
    virtual uint32_t getSampleRate() {
        return fmtHeader->nSamplesPerSec;
    }
    
    // stop
    virtual bool stop();

    // done
    virtual bool done();
    
private:
    // init tables
    bool    calculateTables();

    // allocate buffers
    bool    allocateBuffers();

    // wave file pointers
    fmt_ima_Header* fmtHeader;

    uint8_t         *wavBuffer;
    uint8_t         *wavBase;       // start of audio data
    uint8_t         *wavPos;        // current wave pos
    uint32_t         wavSize;       // total size in bytes
    uint32_t         wavTotalBlocks;
    uint32_t         wavSamplesPerBlock;

    // temp buffer for storing decoded samples before format conversion
    int16_t        *imaDecodedTempBuffer;

    // ring buffer stuff
#pragma pack(push, 16)
    struct poolEntry {
        poolEntry*  next;
        uint32_t    length;     // in BYTES
        void*       samples;
        bool        consumed;
    };
#pragma pack(pop)
    poolEntry      *poolEntries;
    uint8_t        *pool;                   // contains decoded samples

    struct PoolState {
        uint32_t    poolSize;
        uint32_t    poolAvail;
        uint32_t    poolWatermark;          // if (poolAvail <= poolWatermark) && (!isModifying), render blocks in callback
        uint32_t    poolBytesPerEntry;

        struct {
            uint32_t    index;
            uint32_t    cursor;
        } first, last;

        bool        isrFinished;
        bool        modifyLock;
    } state;

    // more states!
    bool            isInitialized;
    bool            isPlaying;
    bool            isPaused;
    uint32_t        resampleMode;
    uint32_t        dmaBufferSamples;

    // sample position
    uint64_t        bufferStart;

    // sndlib stuff
    SoundDevice     *dev;
    soundFormatConverterInfo   convinfo;

    // callback
    static soundDeviceCallbackResult callbackBouncer(void* userPtr, void* buffer, uint32_t bufferSamples, soundFormatConverterInfo *fmt, uint64_t bufferPos);
    soundDeviceCallbackResult callback(void* buffer, uint32_t bufferSamples, soundFormatConverterInfo *fmt, uint64_t bufferPos);

};
