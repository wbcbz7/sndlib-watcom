#pragma once

#include <stdint.h>

class mp2play {

public:
    mp2play() {}; 
    virtual ~mp2play() {};

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
    virtual int64_t  getPos();
    virtual bool     setPos(double pos);

    // get sample rate
    virtual uint32_t getSampleRate();
    
    // stop
    virtual bool stop();

    // done
    virtual bool done();
    
};
