#pragma once
#include <stdint.h>

#pragma pack(push, 1)

// RIFF header
struct RIFF_Header {
    char        id[4];      // "RIFF"
    uint32_t    size;
    char        fourcc[4];  // "WAVE"
};

struct chunk_Header {
    char        id[4];
    uint32_t     size;
};

// wave format header
struct fmt_Header {
    char        id[4];              // "fmt "
    uint32_t    size;               // size of chunk!

    uint16_t    wFormatTag;         // Format code
    uint16_t    nChannels;          // Number of interleaved channels
    uint32_t    nSamplesPerSec;     // Sampling rate (blocks per second)
    uint32_t    nAvgBytesPerSec;    // Data rate
    uint16_t    nBlockAlign;        // Data block size (bytes)
    uint16_t    wBitsPerSample;     // Bits per sample

    // enough :)
};

struct fmt_ima_Header {
    char        id[4];              // "fmt "
    uint32_t    size;               // size of chunk!

    uint16_t    wFormatTag;         // Format code
    uint16_t    nChannels;          // Number of interleaved channels
    uint32_t    nSamplesPerSec;     // Sampling rate (blocks per second)
    uint32_t    nAvgBytesPerSec;    // Data rate
    uint16_t    nBlockAlign;        // Data block size (bytes)
    uint16_t    wBitsPerSample;     // Bits per sample

    uint16_t    cbSize;             // size of additional data
    uint16_t    wSamplesPerBlock;   // sample count per each ADPCM block
};

// data format header
typedef chunk_Header data_Header;

struct ima_block_Header {
    int16_t     firstSample;
    uint8_t     stepIndex;
    uint8_t     dummy;
};

#pragma pack (pop)