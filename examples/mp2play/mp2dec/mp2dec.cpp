// mp2dec - bitstream decoder
// --wbcbz7 o3.ll.zozo - 15.o8.2o22

// (3% cpu load on cel300a)

//#define DO_TESTS

//#include <iterator>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "mp2dec.h"
#include "transfrm.h"

using namespace std;

MP2Decoder::MP2Decoder() {}
MP2Decoder::~MP2Decoder() {

#if 0
    if (alignbuf != NULL) delete[] alignbuf;
#else
    if (polydata != NULL) delete polydata;
#endif
}

bool MP2Decoder::init() {   
    // init tables
    mp2dec_poly_premultiply();
    
#if 1
    // allocate polydata
    alignbuf = new char[sizeof(mp2dec_poly_private_data) + 0xFFF];
    
    // align to 4k boundary
    polydata = (mp2dec_poly_private_data*)(((size_t)alignbuf + 0xFFF) & ~0xFFF);
    memset(polydata, 0, sizeof(mp2dec_poly_private_data));
    
#else
    polydata = new mp2dec_poly_private_data;
#endif
            
    // fill scalefactor table
    for (size_t i = 0; i < 64; i++)
        scalefactor[i] = pow(2.0, 1.0 - (i / 3.0));
    
    // precalculate scale/bias for normalizaion
    for (size_t i = 1; quantizationClasses[i-1].steps != 65535; i++) {
        quantizationClasses[i].normScale = 2.0 / ((float)quantizationClasses[i].steps);
        quantizationClasses[i].normBias  = quantizationClasses[i].steps >> 1;
    }
    
    // fill grouped samples lookup
    {
        uint16_t *ptr = groupedLookup;
        
        for (size_t i = 1; i < 5; i++) if (quantizationClasses[i].grouping == false) continue; else {
            for (size_t smp3 = 0; smp3 < quantizationClasses[i].steps; smp3++)
                for (size_t smp2 = 0; smp2 < quantizationClasses[i].steps; smp2++)
                    for (size_t smp1 = 0; smp1 < quantizationClasses[i].steps; smp1++)
                        *ptr++ = (smp3 << 8) | (smp2 << 4) | (smp1);
        
        }
    }

    // init allocation table index
    currentFormatClass = 0;
    currentChannels    = 0;

    return false;
}

unsigned long MP2Decoder::decodeHeader(uint8_t * frame, MP2Decoder::MPEGHeader & header) {
    // set bit reader
    BitReader bits(frame);

    // additional data (e.g. IDv3 tags) length
    size_t tagLength = 0;
    
    // read header
    do {
        
        header.sync = bits.getbits(11);
        
        switch (header.sync) {
            case 0x7FF:
                // MPEG Audio frame - continue decoding
                break;
                
            case (('A' & 0x0F) << 8) | ('T'):
                // ID3v1, commonly occurs at end of file only
                // technically we can (and should!) return zero to mark end of stream
            case 0:
                return 0;
                
            case (('D' & 0x0F) << 8) | ('I'):
                // ID3v2, commonly occurs at start of file
                // get tag size and skip it
                
                // check if it's indeed ID3v2
                if ((frame[0] == 'I') && (frame[1] == 'D') && (frame[2] == '3') && (frame[3] != 0xFF) && (frame[4] != 0xFF)) {
                    
                    // get tag size and skip it
                    tagLength = (frame[6] << 21) | (frame[7] << 14) | (frame[8] << 7) | (frame[9] << 0);
                    
                    // restart bitreader
                    bits.init(frame + tagLength);
                    continue;
                }
                
                // whoops, not a ID3v2 - fallthrough!
                
            default:
                // unknown syncword!
                fprintf(stderr, "error - unknown syncword -> 0x%X", header.sync);
                return 0;
        }
        
    } while (false);

    // continue reading header
    header.versionId = bits.popbits(2);
    header.layerId = bits.popbits(2);
    header.crcPresent = bits.popbits(1);

    header.bitrate = bits.popbits(4);
    header.sampleRate = bits.popbits(2);
    header.padding = bits.popbits(1);
    header.privateBit = bits.popbits(1);

    header.stereoMode = bits.popbits(2);
    header.modeExtension = bits.popbits(2);
    header.copyright = bits.popbits(1);
    header.originalCopy = bits.popbits(1);
    header.emphasis = bits.popbits(2);
    bits.refill();
    
    // check sync/layer
    if ((header.sync != 0x7FF) || (header.versionId != 3) || (header.layerId != 2)) return 0;

    // skip CRC if any
    if (!header.crcPresent) bits.getbits(16);

    // get channels count
    unsigned long channels = (header.stereoMode == modeMono ? 1 : 2);

    // get bitrate
    header.bitrate = bitRates[header.bitrate];
    if (header.bitrate == 0) return 0;         // invalid bitrate

    // get samplerate
    header.sampleRate = sampleRates[header.sampleRate];
    if (header.sampleRate == 0) return 0;

    // data length (note bitrate counted as bps*1000, not in kbps!), also take tags length in account
    unsigned long dataLength = ((144 * 1000 * header.bitrate) / header.sampleRate) + header.padding + tagLength;
    return dataLength;
}

unsigned long MP2Decoder::decodeBitstream(uint8_t * frame, int16_t * out)
{
    MP2Decoder::MPEGHeader header;
    // set bit reader
    BitReader bits(frame);

    // additional data (e.g. IDv3 tags) length
    size_t tagLength = 0;

    // read header
    do {
        
        header.sync = bits.popbits(11);
        
        switch (header.sync) {
            case 0x7FF:
                // MPEG Audio frame - continue decoding
                break;
                
            case (('A' & 0x0F) << 8) | ('T'):
                // ID3v1, commonly occurs at end of file only
                // technically we can (and should!) return zero to mark end of stream
                return 0;
            case 0:
                return 0;    
                
            case (('D' & 0x0F) << 8) | ('I'):
                // ID3v2, commonly occurs at start of file
                // get tag size and skip it
                
                // check if it's indeed ID3v2
                if ((frame[0] == 'I') && (frame[1] == 'D') && (frame[2] == '3') && (frame[3] != 0xFF) && (frame[4] != 0xFF)) {
                    
                    // get tag size and skip it
                    tagLength = (frame[6] << 21) | (frame[7] << 14) | (frame[8] << 7) | (frame[9] << 0);
                    
                    // restart bitreader
                    bits.init(frame + tagLength);
                    continue;
                }
                
                // whoops, not a ID3v2 - fallthrough!
                
            default:
                // unknown syncword!
                fprintf(stderr, "error - unknown syncword -> 0x%X", header.sync);
                return 0;
        }
        
    } while (false);
    bits.refill();

    // continue reading header
    header.versionId = bits.popbits(2);
    header.layerId = bits.popbits(2);
    header.crcPresent = bits.popbits(1);

    header.bitrate = bits.popbits(4);
    header.sampleRate = bits.popbits(2);
    header.padding = bits.popbits(1);
    header.privateBit = bits.popbits(1);

    header.stereoMode = bits.popbits(2);
    header.modeExtension = bits.popbits(2);
    header.copyright = bits.popbits(1);
    header.originalCopy = bits.popbits(1);
    header.emphasis = bits.popbits(2);
    bits.refill();
    
    // check sync/layer
    if ((header.sync != 0x7FF) || (header.versionId != 3) || (header.layerId != 2)) return 0;

    // skip CRC if any
    if (!header.crcPresent) bits.getbits(16);

    // get channels count
    unsigned long channels = (header.stereoMode == modeMono ? 1 : 2);
    currentChannels = channels;

    // get bitrate
    unsigned long bitrate = bitRates[header.bitrate];
    if (bitrate == 0) return 0;         // invalid bitrate

    // get samplerate
    unsigned long sampleRate = sampleRates[header.sampleRate];
    if (sampleRate == 0) return 0;

    // data length (note bitrate counted as bps*1000, not in kbps!), also take tags length in account
    unsigned long dataLength = ((144 * 1000 * bitrate) / sampleRate) + header.padding + tagLength;

    // from that point we consider that frame as valid
    // clear output buffer
    if (out != NULL) memset(out, 0, 1152 * channels * sizeof(int16_t));

    // -------------------------
    // lookup trip

    // step 1 - get format class index
    size_t fmtClassIdx = formatTable[channels - 1][header.sampleRate][header.bitrate];
    if (fmtClassIdx == 0) return dataLength;         // invalid format combo

    // step 2 - get format class
    MP2Decoder::formatClass& fmtClass = formatClasses[fmtClassIdx];
    if (fmtClass.subbands == 0) return dataLength;   // invalid format

    // bound for intensity stereo
    unsigned long bound = (header.stereoMode == modeJointStereo ? ((header.modeExtension + 1) << 2) : (header.stereoMode == modeMono ? 0 : fmtClass.subbands));

    subband* bandptr = subbands;
    if (currentFormatClass != fmtClassIdx) {
        // reload format class
        currentFormatClass = fmtClassIdx;

        // step 3 - get subband class for each subband
        for (size_t sb = 0; sb < fmtClass.subbands; sb++) {
            for (size_t ch = 0; ch < channels; ch++, bandptr++) {
                bandptr->sbClass = &subbandClasses[fmtClass.subbandClass[sb]];
            }
        }

    }
    
    // read allocation data
    // step 4 - read allocation data
    bandptr = subbands;
    for (size_t sb = 0; sb < bound; sb++) {
        for (size_t ch = 0; ch < channels; ch++, bandptr++) {
            bandptr->quantization = &quantizationClasses[bandptr->sbClass->quantizationClassIdx[bits.getbits(bandptr->sbClass->bitAllocationLength)]];
        }
    }
    for (size_t sb = bound; sb < fmtClass.subbands; sb++, bandptr += channels) {
        bandptr[0].quantization = bandptr[1].quantization =
            &quantizationClasses[bandptr->sbClass->quantizationClassIdx[bits.getbits(bandptr->sbClass->bitAllocationLength)]];
    }

    bandptr = subbands;
    // read scalefactor selector
    for (size_t sb = 0; sb < fmtClass.subbands; sb++) {
        for (size_t ch = 0; ch < channels; ch++, bandptr++) {
            if (bandptr->quantization->steps != 0) bandptr->scfsi = bits.getbits(2);
        }
    }

    bandptr = subbands;
    // read scale factors
    for (size_t sb = 0; sb < fmtClass.subbands; sb++) {
        for (size_t ch = 0; ch < channels; ch++, bandptr++) {
            if (bandptr->quantization->steps != 0) switch (bandptr->scfsi) {
                case 0:
                    bandptr->scalefactor[0] = bits.popbits(6);
                    bandptr->scalefactor[1] = bits.popbits(6);
                    bandptr->scalefactor[2] = bits.popbits(6);
                    break;

                case 1:
                    bandptr->scalefactor[0] =
                    bandptr->scalefactor[1] = bits.popbits(6);
                    bandptr->scalefactor[2] = bits.popbits(6);
                    break;

                case 2:
                    bandptr->scalefactor[0] =
                    bandptr->scalefactor[1] =
                    bandptr->scalefactor[2] = bits.popbits(6);
                    break;

                case 3:
                    bandptr->scalefactor[0] = bits.popbits(6);
                    bandptr->scalefactor[1] =
                    bandptr->scalefactor[2] = bits.popbits(6);
                    break;

                default:
                    // invalid, should never happen
                    break;
            }
            bits.refill();
        }
    }

    // precalculate sample rescale factors
    {
        uint32_t sb = fmtClass.subbands * channels;
        bandptr = subbands;
        do { 
            if (bandptr->quantization->steps != 0) {
                bandptr->postscale[0] = scalefactor[bandptr->scalefactor[0]] * bandptr->quantization->normScale;
                bandptr->postscale[1] = scalefactor[bandptr->scalefactor[1]] * bandptr->quantization->normScale;
                bandptr->postscale[2] = scalefactor[bandptr->scalefactor[2]] * bandptr->quantization->normScale;
            } else {
                bandptr->postscale[0] = bandptr->postscale[1] = bandptr->postscale[2] = 0.0;
            }
            bandptr++;
        } while (--sb);
    }

    // read samples (separate routines for each channel mode)
    switch (header.stereoMode) {
        case modeMono:          readSamplesMono(bits, fmtClass.subbands); break;
        case modeStereo:
        case modeDualChannel:   readSamplesStereo(bits, fmtClass.subbands); break;
        case modeJointStereo:   readSamplesJointStereo(bits, fmtClass.subbands, bound); break;
    }
    return dataLength;
}


unsigned long MP2Decoder::decodeSound(int16_t * out, bool mono, bool downmix2to1)
{
    /*
        cases:
        1) frame is stereo, out in stereo  - out to stereo buffer
        2) frame is stereo, out in mono    - downmix subband samples, out to mono buffer
        3) frame is mono,   out in mono    - out to mono buffer
        4) frame is mono,   out in stereo  - out to stereo buffer, duplicate each sample
    */

    int16_t *p = out;

    if (mono) {
        if (currentChannels == 2) {
            // mix to mono
            float *left = &samples[0][0][0], *right = &samples[1][0][0];
            mp2dec_samples_monofy(left, right);
        }

        // decode!
        for (size_t smp = 0; smp < 12 * 3; smp++) {
            mp2dec_poly_fdct_p5(polydata, &samples[0][smp][0], 0);
            mp2dec_poly_window_mono_p5(polydata, p, 0, 1);
            mp2dec_poly_next_granule(polydata, 0);
            p += 32;
        }
    }
    else {
        for (size_t ch = 0; ch < currentChannels; ch++) {
            p = out + ch;
            for (size_t smp = 0; smp < 12 * 3; smp++) {
                mp2dec_poly_fdct_p5(polydata, &samples[ch][smp][0], ch);
                mp2dec_poly_window_mono_p5(polydata, p, ch, 2);
                mp2dec_poly_next_granule(polydata, ch);
                p += 64;
            }
        }

        if (currentChannels == 1) {
            // duplicate each sample
            mp2dec_samples_mono2stereo(out);
        }
    }

    return 0;
}

// FUCK WATCOM AND C++03!
// so here we using anonymous namespace to init static fields (kids don't try this at home, keep you mind clean!)

namespace {
    // decoding tables
    // scale factor table (scalefactor[idx] = pow(2.0, 1 - (idx / 3.0)))
    static float MP2Decoder::scalefactor[64];
    
    // ----------------------------------------------
    // bit rates table
    static unsigned long MP2Decoder::bitRates[] = { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0 };

    // sample rates table
    static unsigned long MP2Decoder::sampleRates[] = { 44100, 48000, 32000, 0 };

    // valid format class table   [isNotMono][samplerate][bitrate]
    static unsigned long MP2Decoder::formatTable[2][4][16] = {
        // mono
        {
            // free  32   48   56   64   80   96  112  128  160  192  224  256  320  384  bad
            {   2,   3,   3,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   2,   0 },   // 44100 kHz
            {   1,   3,   3,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0 },   // 48000 kHz
            {   2,   4,   4,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   2,   0 },   // 32000 kHz
            {0},
        },
        // stereo/dual/joint stereo
        // note that technically 32/48/56/80kbps are invalid for non-mono, but we'll suppport it
        {
            // free  32   48   56   64   80   96  112  128  160  192  224  256  320  384  bad 
            {   2,   3,   3,   3,   3,   3,   3,   1,   1,   1,   2,   2,   2,   2,   2,   0 },   // 44100 kHz
            {   1,   3,   3,   3,   3,   3,   3,   1,   1,   1,   1,   1,   1,   1,   1,   0 },   // 48000 kHz
            {   2,   4,   4,   4,   4,   4,   4,   1,   1,   1,   2,   2,   2,   2,   2,   0 },   // 32000 kHz
            {0},
        },
    };

    // Layer II subband classes index table (unused subbands are zero))
    static MP2Decoder::formatClass MP2Decoder::formatClasses[] = {
        { 0 }, // illegal format 
        { 27, { 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 6, 6 } },
        { 30, { 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 6, 6, 6, 6, 6 } },
        { 8,  { 3, 3, 5, 5, 5, 5, 5, 5 } },
        { 12, { 3, 3, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 } },
    };

    static MP2Decoder::subbandClass MP2Decoder::subbandClasses[] = {
        { 0 }, // empty subband, no allocation
        { 4, { 0, 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 } },
        { 4, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17 } },
        { 4, { 0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 } },
        { 3, { 0, 1, 2, 3, 4, 5, 6, 17 } },
        { 3, { 0, 1, 2, 4, 5, 6, 7, 8 } },
        { 2, { 0, 1, 2, 17 } },
    };

    static MP2Decoder::quantizationClass MP2Decoder::quantizationClasses[] = {
        { 0 },      // empty, zero allocation
        { 3,        true,     5 },
        { 5,        true,     7 },
        { 7,        false,    3 },
        { 9,        true,     10 },
        { 15,       false,    4 },
        { 31,       false,    5 },
        { 63,       false,    6 },
        { 127,      false,    7 },
        { 255,      false,    8 },
        { 511,      false,    9 },
        { 1023,     false,    10 },
        { 2047,     false,    11 },
        { 4095,     false,    12 },
        { 8191,     false,    13 },
        { 16383,    false,    14 },
        { 32767,    false,    15 },
        { 65535,    false,    16 },
    };
    
    // scalefactor index lookup (faster than div by 12)
    static const size_t MP2Decoder::scalefactorLookup[] = {0, 0, 0, 1, 1, 1, 2, 2, 2};

    // grouped samples lookup
    static const size_t   MP2Decoder::groupedLookupIdx[] = {0, 0, 3*3*3, 0, 0, 0, 3*3*3 + 5*5*5};
    static       uint16_t MP2Decoder::groupedLookup[3*3*3 + 5*5*5 + 9*9*9];
 
    
};