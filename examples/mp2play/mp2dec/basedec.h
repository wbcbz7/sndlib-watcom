#pragma once
#include <stdint.h>

class BaseDecoder {
public:
    // constructor/destructor
    BaseDecoder() {};
    virtual ~BaseDecoder() {};

    // init
    virtual bool init() { return false; };

    // decode one frame
    virtual unsigned long decode(uint8_t * frame, int16_t * out) { return 0; }
};
