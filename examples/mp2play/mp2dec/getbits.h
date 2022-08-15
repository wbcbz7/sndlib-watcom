#pragma once
#include <stdint.h>

// bit stream reader
class BitReader {
protected:
    // buffer position
    uint8_t *bufferPos;

    // window buffer
    uint32_t window;

    // bits in window
    uint32_t bitsInWindow;

public:

    BitReader() : bufferPos(NULL), window(0), bitsInWindow(0) {}
    ~BitReader() {};

    // init constructor
    BitReader(uint8_t *buffer) {
        init(buffer);
    }

    // init
    BitReader& init(uint8_t *buffer);

    // get count bits
    uint32_t getbits(uint32_t count) {
        if (count == 0) return 0;

        // extract bits from window
        uint32_t result = (window >> (bitsInWindow - count)) & ((1 << count) - 1);
        bitsInWindow -= count;

        // fill window with new bits
        while (bitsInWindow <= 24) {
            window = (window << 8) | *bufferPos++;
            bitsInWindow += 8;
        }

        return result;
    }
    
    // get count pits, do not refill
    uint32_t popbits(uint32_t count) {
        if (count == 0) return 0;

        // extract bits from window
        uint32_t result = (window >> (bitsInWindow - count)) & ((1 << count) - 1);
        bitsInWindow -= count;

        return result;
    }

    // refill window (gurantees at least 24 bits available)
    void refill() {
        // fill window with new bits
        while (bitsInWindow <= 24) {
            window = (window << 8) | *bufferPos++;
            bitsInWindow += 8;
        }
    }
};