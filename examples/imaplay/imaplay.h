#pragma once

#include <stdint.h>

class imaplay {

public:
    imaplay() {}; 
    virtual ~imaplay() {};

    enum {
        IMA_PLAY_DOWNSAMPLE_2TO1 = (1 << 0),        // i.e 44k -> 22k
        IMA_PLAY_UPSAMPLE_1TO2   = (1 << 1),        // i.e 22k -> 44k
        IMA_PLAY_RESAMPLE        = (1 << 2)
    };

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
    virtual uint32_t getSampleRate();
    
    // stop
    virtual bool stop();

    // done
    virtual bool done();
    
};
