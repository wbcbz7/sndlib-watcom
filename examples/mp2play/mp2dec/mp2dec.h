#pragma once
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "getbits.h"
#include "basedec.h"
#include "transfrm.h"

class MP2Decoder : public BaseDecoder {

public:
    
    // constructor/destructor
    MP2Decoder();
    virtual ~MP2Decoder();
    
    // init
    virtual bool init();

    // MPEG header struct
    struct MPEGHeader {
            unsigned long   sync;
            unsigned long   versionId;
            unsigned long   layerId;
            unsigned long   crcPresent;

            unsigned long   bitrate;
            unsigned long   sampleRate;
            unsigned long   padding;
            unsigned long   privateBit;

            unsigned long   stereoMode;
            unsigned long   modeExtension;
            unsigned long   copyright;
            unsigned long   originalCopy;
            unsigned long   emphasis;
    };

    // mode enums
    enum {
        modeStereo = 0, modeJointStereo, modeDualChannel, modeMono,

    };
    
    // decode one frame (ASSUMES you're passing valid buffer), returns frame length in bytes, tags are skipped, 0 if end of stream
    // note - can return either empty buffer or zero length for invalid bitrate/samplerate frames
    virtual unsigned long decode(uint8_t * frame, int16_t * out, bool mono = false, bool downmix2to1 = false) {

        unsigned long rtn = decodeBitstream(frame, out);
        decodeSound(out, mono, downmix2to1);

        return rtn;
    };    
    
    // decoe header and get frame info/length, returns frame length in bytes, tags are skipped, 0 if end of stream
    virtual unsigned long decodeHeader(uint8_t * frame, MPEGHeader & out);
    
    // decode frame bitstream only and store inside decoder data
    virtual unsigned long decodeBitstream(uint8_t * frame, int16_t * out);

    // decode sound data using decodeBitstream() data; pass (1152 * (mono ? 1 : 2)) / (downmix2to1 ? 2 : 1) int16_t's buffer
    virtual unsigned long decodeSound(int16_t * out, bool mono = false, bool downmix2to1 = false);

private:

    // optimized sample readers
    // read samples, mono
    void readSamplesMono(BitReader& bits, uint32_t sbLimit);
    void readSamplesStereo(BitReader& bits, uint32_t sbLimit);
    void readSamplesJointStereo(BitReader& bits, uint32_t sbLimit,  uint32_t bound);

protected:

    // bit reader
    BitReader bits;

    // scale factor table (scalefactor[idx] = pow(2.0, 1 - (idx / 3.0)))
    static float scalefactor[64];

    // grouped samples lookup
    static const size_t   groupedLookupIdx[];
    static       uint16_t groupedLookup[3*3*3 + 5*5*5 + 9*9*9]; // packed as ((s1 << 8) | (s2 << 4) | s3)
    
    // ----------------------------------------------
    // bit rates table
    static unsigned long bitRates[16];

    // sample rates table
    static unsigned long sampleRates[4];

    // valid format class table            isNotMono samplerate bitrate
    static unsigned long formatTable[2]        [4]     [16];

    // Layer II subband classes index table (unused subbands are zero))
    struct formatClass {
        unsigned long  subbands;
        unsigned long  subbandClass[32];
    };

    static formatClass formatClasses[5];

    // Layer II subband classes
    struct subbandClass {
        unsigned long   bitAllocationLength;
        unsigned long   quantizationClassIdx[16];
    };

    static subbandClass subbandClasses[7];

    // Layer II quantization classes
    struct quantizationClass {
        unsigned long   steps;
        bool            grouping;
        unsigned long   bitsPerCodeword;
        
        // calculated in constructor
        float           normScale;
        signed long     normBias;
    };

    static quantizationClass quantizationClasses[18];
    
    // scalefactor index lookup (faster than div by 12)
    static const size_t scalefactorLookup[];

    // --------------------------------------
    // subband stuff
    struct subband {
        quantizationClass   *quantization;
        subbandClass        *sbClass;
        union {
            unsigned long   scalefactor[3];     // for each part of 12 samples
            float           postscale[3];
        };
        unsigned long       scfsi;
    };

    // current format class index
    uint32_t currentFormatClass;
    uint32_t currentChannels;

    // channel data struct
    float   samples[2][36][32];
    subband subbands[32 * 2];   // separate

    // polydata
    char                        *alignbuf;
    mp2dec_poly_private_data    *polydata;
    
};
