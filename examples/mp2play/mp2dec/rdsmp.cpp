// hack to keep $$%#%&#@Q watcom from inlining funcitons when it's absolutely not desired
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "mp2dec.h"
#include "getbits.h"

void MP2Decoder::readSamplesMono(BitReader& oldbits, uint32_t sbLimit) {
    BitReader bits = oldbits;

    // clear sample storage, only left channel!
    memset(&samples[0][0][0], 0, sizeof(float) * 32 * 36);

    // temp assigned variables
    MP2Decoder::subband*           bandptr;
    MP2Decoder::quantizationClass* bandclass;
    uint32_t gr = 0, sb; int32_t rawsamples[3];
    float   *os =  &samples[0][0][0];
    do {
        bandptr = subbands;
        sb = sbLimit;
        do {
            bandclass = bandptr->quantization;
            if (bandclass->steps != 0) {
                if (bandclass->grouping) {
                    // read grouped code
                    unsigned long code = bits.getbits(bandclass->bitsPerCodeword);
                    
                    // get packed samples from table
                    unsigned long samples = groupedLookup[groupedLookupIdx[(bandclass->steps - 3)] + code];
                    rawsamples[0] = (samples)   & 0xF; samples >>= 4;
                    rawsamples[1] = (samples)   & 0xF; samples >>= 4;
                    rawsamples[2] = (samples)   & 0xF;
                }
                else {
                    // ungrouped
                    rawsamples[0] = bits.getbits(bandclass->bitsPerCodeword);
                    rawsamples[1] = bits.getbits(bandclass->bitsPerCodeword);
                    rawsamples[2] = bits.getbits(bandclass->bitsPerCodeword);
                }
                
                // renormalize 
                signed long bias  = bandclass->normBias;
                
                *(float*)((uint8_t*)os + 0*36*32*4 + 0*32*4) = (bias - rawsamples[0]) * bandptr[0].postscale[gr >> 2];
                *(float*)((uint8_t*)os + 0*36*32*4 + 1*32*4) = (bias - rawsamples[1]) * bandptr[0].postscale[gr >> 2];
                *(float*)((uint8_t*)os + 0*36*32*4 + 2*32*4) = (bias - rawsamples[2]) * bandptr[0].postscale[gr >> 2];
            }
            bandptr++; os++;
        } while (--sb);
        os += 2*32 + 32 - sbLimit;
    } while (++gr != 12);

}

void MP2Decoder::readSamplesStereo(BitReader& oldbits, uint32_t sbLimit) {
    BitReader bits = oldbits;

    // clear sample storage
    memset(&samples[0][0][0], 0, sizeof(float) * 32 * 36 * 2);

    // temp assigned variables
    MP2Decoder::subband*           bandptr;
    MP2Decoder::quantizationClass* bandclass;
    uint32_t gr = 0, sb; int32_t rawsamples[3]; uint32_t sbLimit2 = (sbLimit << 1);
    float   *os =  &samples[0][0][0];
    uint32_t osofs = 0;                 // channel offset in sample buffer

    do {
        bandptr = subbands;
        sb = sbLimit2;
        do {
            // left channel
            bandclass = bandptr->quantization;
            if (bandclass->steps != 0) {
                if (bandclass->grouping) {
                    // read grouped code
                    unsigned long code = bits.getbits(bandclass->bitsPerCodeword);
                    
                    // get packed samples from table
                    unsigned long samples = groupedLookup[groupedLookupIdx[(bandclass->steps - 3)] + code];
                    rawsamples[0] = (samples)   & 0xF; samples >>= 4;
                    rawsamples[1] = (samples)   & 0xF; samples >>= 4;
                    rawsamples[2] = (samples)   & 0xF;
                }
                else {
                    // ungrouped
                    rawsamples[0] = bits.getbits(bandclass->bitsPerCodeword);
                    rawsamples[1] = bits.getbits(bandclass->bitsPerCodeword);
                    rawsamples[2] = bits.getbits(bandclass->bitsPerCodeword);
                }
                
                // renormalize 
                signed long bias  = bandclass->normBias;
                
                *(float*)((uint8_t*)os + osofs + 0*32*4) = (bias - rawsamples[0]) * bandptr->postscale[gr >> 2];
                *(float*)((uint8_t*)os + osofs + 1*32*4) = (bias - rawsamples[1]) * bandptr->postscale[gr >> 2];
                *(float*)((uint8_t*)os + osofs + 2*32*4) = (bias - rawsamples[2]) * bandptr->postscale[gr >> 2];
            }
            bandptr++; osofs ^= 1*36*32*4; if (sb & 1) os++;
        } while (--sb);
        os += 2*32 + 32 - sbLimit;
    } while (++gr != 12);

}

void MP2Decoder::readSamplesJointStereo(BitReader& oldbits, uint32_t sbLimit, uint32_t bound) {
    BitReader bits = oldbits;
    
    // clear sample storage
    memset(&samples[0][0][0], 0, sizeof(float) * 32 * 36 * 2);

    // temp assigned variables
    MP2Decoder::subband*           bandptr;
    MP2Decoder::quantizationClass* bandclass;
    uint32_t gr = 0, sb; int32_t rawsamples[3]; uint32_t sbLimit2 = (sbLimit << 1), bound2 = (bound << 1);
    float   *os =  &samples[0][0][0];
    uint32_t osofs = 0;                 // channel offset in sample buffer

    do {
        // process bound
        bandptr = subbands;
        sb = bound2; 
        do {
            bandclass = bandptr->quantization;
            if (bandclass->steps != 0) {
                if (bandclass->grouping) {
                    // read grouped code
                    unsigned long code = bits.getbits(bandclass->bitsPerCodeword);
                    
                    // get packed samples from table
                    unsigned long samples = groupedLookup[groupedLookupIdx[(bandclass->steps - 3)] + code];
                    rawsamples[0] = (samples)   & 0xF; samples >>= 4;
                    rawsamples[1] = (samples)   & 0xF; samples >>= 4;
                    rawsamples[2] = (samples)   & 0xF;
                }
                else {
                    // ungrouped
                    rawsamples[0] = bits.getbits(bandclass->bitsPerCodeword);
                    rawsamples[1] = bits.getbits(bandclass->bitsPerCodeword);
                    rawsamples[2] = bits.getbits(bandclass->bitsPerCodeword);
                }
                
                // renormalize 
                signed long bias  = bandclass->normBias;
                
                *(float*)((uint8_t*)os + osofs + 0*32*4) = (bias - rawsamples[0]) * bandptr->postscale[gr >> 2];
                *(float*)((uint8_t*)os + osofs + 1*32*4) = (bias - rawsamples[1]) * bandptr->postscale[gr >> 2];
                *(float*)((uint8_t*)os + osofs + 2*32*4) = (bias - rawsamples[2]) * bandptr->postscale[gr >> 2];
            }
            bandptr++; osofs ^= 1*36*32*4; if (sb & 1) os++;
        } while (--sb);

        // process after bound
        sb = (sbLimit - bound);
        do {
            bandclass = bandptr->quantization;
            if (bandclass->steps != 0) {
                if (bandclass->grouping) {
                    // read grouped code
                    unsigned long code = bits.getbits(bandclass->bitsPerCodeword);
                    
                    // get packed samples from table
                    unsigned long samples = groupedLookup[groupedLookupIdx[(bandclass->steps - 3)] + code];
                    rawsamples[0] = (samples)   & 0xF; samples >>= 4;
                    rawsamples[1] = (samples)   & 0xF; samples >>= 4;
                    rawsamples[2] = (samples)   & 0xF;
                }
                else {
                    // ungrouped
                    rawsamples[0] = bits.getbits(bandclass->bitsPerCodeword);
                    rawsamples[1] = bits.getbits(bandclass->bitsPerCodeword);
                    rawsamples[2] = bits.getbits(bandclass->bitsPerCodeword);
                }
                
                // renormalize 
                signed long bias  = bandclass->normBias;
                
                *(float*)((uint8_t*)os + 0*36*32*4 + 0*32*4) = (bias - rawsamples[0]) * bandptr[0].postscale[gr >> 2];
                *(float*)((uint8_t*)os + 0*36*32*4 + 1*32*4) = (bias - rawsamples[1]) * bandptr[0].postscale[gr >> 2];
                *(float*)((uint8_t*)os + 0*36*32*4 + 2*32*4) = (bias - rawsamples[2]) * bandptr[0].postscale[gr >> 2];
                *(float*)((uint8_t*)os + 1*36*32*4 + 0*32*4) = (bias - rawsamples[0]) * bandptr[1].postscale[gr >> 2];
                *(float*)((uint8_t*)os + 1*36*32*4 + 1*32*4) = (bias - rawsamples[1]) * bandptr[1].postscale[gr >> 2];
                *(float*)((uint8_t*)os + 1*36*32*4 + 2*32*4) = (bias - rawsamples[2]) * bandptr[1].postscale[gr >> 2];
            }
            bandptr += 2; os++;
        } while (--sb);

        os += 2*32 + 32 - sbLimit;
    } while (++gr != 12);

}
