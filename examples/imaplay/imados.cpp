#include <stdio.h>
#include "imados.h"
#include "decode.h"

#ifndef min
#define min(a, b)      ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b)      ((a) > (b) ? (a) : (b))
#endif
#ifndef sgn
#define sgn(a)         ((a) < (0) ? (-1) : ((a) > (0) ? (1) : (0)))
#endif
#ifndef clamp
#define clamp(a, l, h) ((a) > (h) ? (h) : ((a) < (l) ? (l) : (a)))
#endif

// ------------------------------------------
// IMA ADPCM tables
int16_t imaplay_nextstep_table[16][128];
int16_t imaplay_diff_table[16][128];

/* Intel ADPCM step variation table */
static const int8_t imaplay_indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};
static const int16_t imaplay_stepsizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

//#define DEBUG_LOG

// ------------------------------------------

#define arrayof(a) (sizeof(a) / sizeof(a[0]))

imaplay_dos::imaplay_dos() : isInitialized(false), isPlaying(false), isPaused(false), bufferStart(0) {
}

imaplay_dos::~imaplay_dos() {
    if (isPlaying) stop();
    if (isInitialized) done();
}

soundDeviceCallbackResult imaplay_dos::callbackBouncer(void* userPtr, void* buffer, uint32_t bufferSamples, soundFormatConverterInfo *fmt, uint64_t bufferPos) {
    return ((imaplay_dos*)userPtr)->callback(buffer, bufferSamples, fmt, bufferPos);
}

soundDeviceCallbackResult imaplay_dos::callback(void* buffer, uint32_t bufferSamples, soundFormatConverterInfo *fmt, uint64_t bufferPos) {
    // pump the circular buffer

#if DEBUG_RASTER == 1
    outp(0x3c8, 0); outp(0x3c9, 0x1f); outp(0x3c9, 0x1f); outp(0x3c9, 0x1f);
#endif

    uint32_t bytesToRender = bufferSamples * convinfo.bytesPerSample;
    uint8_t *dst = (uint8_t*)buffer;

    poolEntry* block = &poolEntries[state.first.index];
    uint32_t startPos = state.first.cursor;
    uint32_t blockRemainder = block->length - startPos; 
    uint32_t blockBytesToCopy = (bytesToRender < blockRemainder ? bytesToRender : blockRemainder);
    uint32_t blocksConsumed = 0;

    while (bytesToRender > 0) {
        // copy cur block
        memcpy((uint8_t*)dst, (uint8_t*)block->samples + startPos, blockBytesToCopy);
        dst           += blockBytesToCopy;
        startPos      += blockBytesToCopy;
        bytesToRender -= blockBytesToCopy;
        
        if (bytesToRender > 0) {
            block->consumed = true;
            blocksConsumed++;
            
            // jump to next block
            state.first.index++; if (state.first.index == state.poolSize) state.first.index = 0;
            block = &poolEntries[state.first.index];
            startPos = 0;
            blockBytesToCopy = (bytesToRender < block->length ? bytesToRender : block->length);
            
        }
    }
    // save current state
    state.first.cursor = startPos;
    state.poolAvail += blocksConsumed;

    // tell render to remove stuck blocks
    state.isrFinished = true;

#if DEBUG_RASTER == 1
    outp(0x3c8, 0); outp(0x3c9, 0x1f); outp(0x3c9, 0x0); outp(0x3c9, 0x1f);
#endif

    // render blocks if below buffer watermark
    if ((state.modifyLock == false) && ((state.poolSize - state.poolAvail) <= state.poolWatermark))
        decode(blocksConsumed);

#if DEBUG_RASTER == 1
    outp(0x3c8, 0); outp(0x3c9, 0); outp(0x3c9, 0); outp(0x3c9, 0);
#endif

    return callbackOk;
}

bool imaplay_dos::init(uint32_t decodedPool, uint32_t bufferSamples, uint32_t resampleMode, bool autosetup)
{
    if ((decodedPool == 0) || (bufferSamples == 0)) return false;

    // init sndlib
    uint32_t rtn = sndlibInit();
    if (rtn != SND_ERR_OK) {
        printf("unable to init sndlib\n");
        return false;
    }

    // query, detect, create sound device
    rtn = sndlibCreateDevice(&dev, autosetup ? SND_CREATE_DEVICE_AUTO_DETECT : SND_CREATE_DEVICE_MANUAL_SELECT);
    if (rtn == SND_ERR_USEREXIT) {
        printf("user exit\n");
        return false;
    }
    else if (rtn != SND_ERR_OK) {
        printf("imaplay_dos::init() error: no available sound devices found! (rtn = %d)\n", rtn);
        return false;
    }

    // init device
    rtn = dev->init();
    if (rtn != SND_ERR_OK) {
        printf("imaplay_dos::init() error: unable to init sound device (rtn = %d)\n", rtn);
        sndlibDestroyDevice(dev);
        return false;
    }

    printf("imaplay_dos::init(): %s init success\n", dev->getName());

    // save other props
    this->resampleMode = resampleMode;
    state.poolSize = decodedPool;
    dmaBufferSamples = bufferSamples;
    
    // init decoder tables;
    calculateTables();

    return true;
}

bool imaplay_dos::load(const char *filename, uint32_t filebufsize) {
    FILE *f = fopen(filename, "rb");
    size_t fsize = 0;

    if (f == NULL) {
        printf("imaplay_dos::load() error: unable to open wave file!");
        return false;
    }

    // get filesize
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    // allocate memory
    wavBuffer = new uint8_t[fsize];   // reserve extra word for end of stream
    if (wavBuffer == NULL) {
        printf("imaplay_dos::load() error: insufficient memory to load wav stream!\n");
        return false;
    }

    // read file
    size_t rtn = fread(wavBuffer, sizeof(uint8_t), fsize, f);
    if (rtn != fsize) {
        printf("imaplay_dos::load() error: expected %d bytes, read %d\n", fsize, rtn);
        return false;
    }

    fclose(f);

    // yeah :) 
    wavBase = wavBuffer;
    wavPos  = wavBuffer;
    wavSize = fsize;
    return allocateBuffers();
}

bool imaplay_dos::loadmem(void * ptr, uint32_t size)
{
    // doesn't check for validity
    wavBase = (uint8_t*)ptr;
    wavSize = size;
    return allocateBuffers();
}

// load 2nd stage - open sound stream, get converter info
bool imaplay_dos::allocateBuffers()
{
    RIFF_Header * riffhead = (RIFF_Header*)wavPos;
    if (memcmp("RIFF", riffhead->id, sizeof(unsigned long)) || 
        memcmp("WAVE", riffhead->fourcc, sizeof(unsigned long)))
    {
        printf("imaplay_dos::load() error: not a wave file!\n");
        return false;
    }
    wavPos += sizeof(RIFF_Header);
    
    fmtHeader = (fmt_ima_Header*)wavPos;
    if (memcmp("fmt ", fmtHeader->id, sizeof(unsigned long))) {
        printf("imaplay_dos::load() error: format chunk not found\n");
        return false;
    }
    if (fmtHeader->wFormatTag != 0x11) {
        printf("imaplay_dos::load() error: not a IMA ADPCM file!\n");
        return false;
    }
    
    if ((fmtHeader->nChannels < 1) || (fmtHeader->nChannels > 2)) {
        printf("imaplay_dos::load() error: fule must be mono or stereo!\n");
        return false;
    }
    
    if (fmtHeader->wBitsPerSample != 4) {
        printf("imaplay_dos::load() error: file must be 4 bit!\n");
        return false;
    }
    
    // skip other crap
    wavPos += fmtHeader->size + sizeof(chunk_Header);
    
    // find data chunk
    chunk_Header *chunkhead = (chunk_Header *)wavPos;
    while (memcmp("data", chunkhead->id, sizeof(unsigned long))) {
        wavPos += chunkhead->size + sizeof(chunk_Header); 
        chunkhead = (chunk_Header *)wavPos;
        if ((wavPos - wavBase) > wavSize) {
            printf("imaplay_dos::load() error: data chunk not found!\n");
            return false;
        }
    }
    
    // skip header
    wavPos += sizeof(chunk_Header); wavBase = wavPos;
    wavSize = chunkhead->size;

    // init stream
    // request format
    uint32_t format = SND_FMT_INT16 | SND_FMT_SIGNED | (fmtHeader->nChannels == 2 ? SND_FMT_STEREO : SND_FMT_MONO);

    uint32_t rtn = dev->open(fmtHeader->nSamplesPerSec, format, dmaBufferSamples, 0, callbackBouncer, (void*)this, &convinfo);
    if (rtn != SND_ERR_OK) {
        printf("imaplay_dos::load() error: unable to open sound device (rtn = %d)\n", rtn);
        return false;
    } 

    printf("imaplay_dos::load(): stream open success: %d->%d hz %sbit %s %s\n",
        convinfo.sourceSampleRate,  convinfo.sampleRate,
        convinfo.format & SND_FMT_INT8  ? "8" : (convinfo.format & SND_FMT_INT16 ? "16" : "unk"),
        convinfo.format & SND_FMT_MONO ? "mono" : "stereo",
        convinfo.format & SND_FMT_SIGNED ? "signed" : "unsigned"
        );
    
    // init decoded stream pool
    wavSamplesPerBlock = fmtHeader->wSamplesPerBlock;
    state.poolBytesPerEntry = wavSamplesPerBlock * convinfo.bytesPerSample;
    wavTotalBlocks = (wavSize / fmtHeader->nBlockAlign);

#ifdef DEBUG_LOG
    printf("bytes per frame - %d\n", state.poolBytesPerEntry);
#endif  
  
    // init more memory
    pool = new uint8_t[state.poolSize * state.poolBytesPerEntry];
    poolEntries = new poolEntry[state.poolSize];
    imaDecodedTempBuffer = new int16_t[(wavSamplesPerBlock * fmtHeader->nChannels) + 0];
    if (!pool || !poolEntries || !imaDecodedTempBuffer) {
        printf("mp2play_dos::load() error: not enough memory to initialize pool\n", rtn);
        return false;
    }

#ifdef DEBUG_LOG
    printf("alloc %x %x %x\n", pool, poolEntries, imaDecodedTempBuffer);
#endif  

    memset(pool,        0, state.poolSize * state.poolBytesPerEntry);
    memset(poolEntries, 0, state.poolSize * (sizeof(poolEntry)));
    //memset(imaDecodedTempBuffer, 0, (((fmtHeader->wSamplesPerBlock * 2) + 32) * (sizeof(int16_t))));

    // set pool head/tail
    state.first.index = 0;
    state.first.cursor = 0;
    state.last.index = 0;
    state.last.cursor = 0;
    state.poolAvail = state.poolSize;

    state.modifyLock = true;
    state.isrFinished = false;

    // update watermark
    state.poolWatermark = ((4 * dmaBufferSamples * convinfo.bytesPerSample) / state.poolBytesPerEntry);

    isInitialized = true;

    // request to fill pool
#ifdef DEBUG_LOG
    printf("start decoding to pool...\n");
#endif  
    decode(0);  // fill until pool end
#ifdef DEBUG_LOG
    printf("done\n");
#endif  

    return true;
}

bool imaplay_dos::decode(uint32_t frames)
{
    // lock buffer tail
    state.modifyLock = true; bool prefill = (frames == 0);
    uint32_t availAtStart = state.poolAvail;
    
    // don't fill
    if ((frames == 0) || (frames >= availAtStart)) {
        frames = availAtStart - 1; // fill until end, leave 1 free "end" frame
    };

    uint32_t currentAvail = availAtStart;
    uint32_t currentIdx = state.last.index;

    for (size_t block = 0; block < frames; block++) {
#ifdef DEBUG_LOG
        if (prefill) printf("block %d decode\n", block);
#endif  
        if (fmtHeader->nChannels == 2) 
            imaplay_decode_stereo(imaDecodedTempBuffer, wavPos, wavSamplesPerBlock - 1);
        else
            imaplay_decode_mono(imaDecodedTempBuffer, wavPos, wavSamplesPerBlock - 1);
        wavPos += fmtHeader->nBlockAlign; if ((wavPos - wavBase) >= wavSize) wavPos = wavBase;

#ifdef DEBUG_LOG
        if (prefill) printf("block %d convert\n", block);
#endif  

        // convert to target format
        uint8_t* samplePtr = (uint8_t*)pool + (state.poolBytesPerEntry * currentIdx);
        convinfo.proc(samplePtr, imaDecodedTempBuffer, wavSamplesPerBlock, convinfo.parm, convinfo.parm2);

#ifdef DEBUG_LOG
        if (prefill) printf("block %d link\n", block);
#endif  

        // fill pool stuff
        poolEntries[currentIdx].consumed = false;
        poolEntries[currentIdx].length = state.poolBytesPerEntry;
        poolEntries[currentIdx].samples = samplePtr;
        poolEntries[currentIdx].next = NULL;

        currentIdx++; if (currentIdx == state.poolSize) currentIdx = 0;
        currentAvail--;
    }

    // link tail
    state.last.index = currentIdx;
    
    // purge already played blocks
    if (state.isrFinished) {
        state.isrFinished = false;
        currentAvail += (state.poolAvail - availAtStart);
    }
    state.poolAvail = currentAvail;
    
    // release lock
    state.modifyLock = false;

    return true;
}

bool imaplay_dos::play(uint64_t pos)
{
    if (!isInitialized) return false;

    // start for first time
    uint32_t rtn = dev->start();
    if (rtn != SND_ERR_OK) {
        printf("imaplay_dos::play() error: unable to start stream (rtn = %d)\n", rtn);
        return false;
    }

    isPlaying = true;
    bufferStart = (pos == -1 ? 0 : pos);
    
    return true;
}

bool imaplay_dos::pause()
{
    return (dev->pause() != SND_ERR_OK);
}

bool imaplay_dos::resume()
{
    return (dev->resume() != SND_ERR_OK);
}

uint64_t imaplay_dos::getPos()
{
    return dev->getPos() + bufferStart;
}

bool imaplay_dos::stop()
{
    // start for first time
    uint32_t rtn = dev->stop();
    if (rtn != SND_ERR_OK) {
        printf("imaplay_dos::stop() error: unable to stop stream (rtn = %d)\n", rtn);
        return false;
    }

    isPlaying = false;
    bufferStart = 0;

    return true;
}

bool imaplay_dos::done()
{
    // stop playback
    if (isPlaying) stop();

    // cleanup memory
    delete[] pool;
    delete[] poolEntries;
    delete[] imaDecodedTempBuffer;
    delete[] wavBuffer;
    
    // cleanup device
    dev->done();
    // destroy device
    sndlibDestroyDevice(dev);
    // cleanup sndlib
    if (sndlibDone() != SND_ERR_OK) return false;

    isInitialized = false;
    return true;
}

bool imaplay_dos::calculateTables()
{
    for (int32_t sample = 0; sample < 16; sample++) {
        for (int32_t step = 0; step < 89; step++) {
            int32_t diff = 0;
            int32_t stepsize = imaplay_stepsizeTable[step];
            if (sample & 4) diff += stepsize;
            if (sample & 2) diff += stepsize >> 1;
            if (sample & 1) diff += stepsize >> 2;
            diff += stepsize >> 3;
            if (sample & 8) diff = -diff;
            imaplay_diff_table    [sample][step] = diff;
            imaplay_nextstep_table[sample][step] = clamp(step + imaplay_indexTable[sample], 0, 88) << 1;
        }
    }
    return true;
}

#if USE_C_DECODERS == 1
// sample C decoders. might do assembly version later :)
uint32_t imaplay_decode_mono(int16_t * out, uint8_t * in, uint32_t samples) {
    int16_t* oldOut = out;
    ima_block_Header* header = (ima_block_Header*)in; in += sizeof(ima_block_Header);
    int32_t predictor = header->firstSample;
    header->stepIndex <<= 1;

    // decompress first sample
    *out++ = predictor;

    // decompress the rest
    for (size_t i = 0; i < samples / 8; i++) {
        uint32_t data = *(uint32_t*)in; in += sizeof(uint32_t);
        
        for (uint32_t s = 0; s < 8; s++) {
            predictor = clamp(predictor + imaplay_diff_table[data & 15][header->stepIndex >> 1], -32768, 32767);
            *(out + s) = predictor;
            header->stepIndex = imaplay_nextstep_table[data & 15][header->stepIndex >> 1];
            data >>= 4;
        }

        out += 8;
    }

    return (out - oldOut);
}

uint32_t imaplay_decode_stereo(int16_t * out, uint8_t * in, uint32_t samples) {
    int16_t* oldOut = out;
    ima_block_Header* header[2];
    header[0] = (ima_block_Header*)in; in += sizeof(ima_block_Header);
    header[1] = (ima_block_Header*)in; in += sizeof(ima_block_Header);
    int32_t predictor[2] = { header[0]->firstSample, header[1]->firstSample };
    header[0]->stepIndex <<= 1; header[1]->stepIndex <<= 1;

    // decompress first sample
    *out++ = predictor[0];
    *out++ = predictor[1];

    // decompress the rest
    for (size_t i = 0; i < samples / 8; i++) {
        for (size_t ch = 0; ch < 2; ch++) {
            uint32_t data = *(uint32_t*)in; in += sizeof(uint32_t);

            for (uint32_t s = 0; s < 16; s += 2) {
                predictor[ch] = clamp(predictor[ch] + imaplay_diff_table[data & 15][header[ch]->stepIndex >> 1], -32768, 32767);
                *(out + ch + s) = predictor[ch];
                header[ch]->stepIndex = imaplay_nextstep_table[data & 15][header[ch]->stepIndex >> 1];
                data >>= 4;
            }
        }
        out += 16;
    }

    return (out - oldOut);
}
#endif