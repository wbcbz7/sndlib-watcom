#include "mp2play.h"
#include "mp2dos.h"

#include <stdio.h>
#include "fpstate.h"


unsigned long long rdtsc();
#pragma aux rdtsc = ".586" "rdtsc" value [edx eax]

void fpuSave(fpuState *state);
#pragma aux fpuSave = "fnsave [eax]" parm [eax]
void fpuRestore(fpuState *state);
#pragma aux fpuRestore = "frstor [eax]" parm [eax]

#define arrayof(a) (sizeof(a) / sizeof(a[0]))

soundDeviceCallbackResult mp2play_dos::callbackBouncer(void* userPtr, void* buffer, uint32_t bufferSamples, soundFormatConverterInfo *fmt, uint64_t bufferPos)
{
    // save FPU state
    fpuState fpustate;
    fpuSave(&fpustate);

    // call callback
    soundDeviceCallbackResult rtn = ((mp2play_dos*)userPtr)->callback(buffer, bufferSamples, fmt, bufferPos);
    
    // restore FPU state
    fpuRestore(&fpustate);

    return rtn;
}


soundDeviceCallbackResult mp2play_dos::callback(void* buffer, uint32_t bufferSamples, soundFormatConverterInfo *fmt, uint64_t bufferPos)
{
    /*
        this is pretty messy circular buffer handling, but at least it works somehow :)
    */

#if DEBUG_RASTER == 1
    outp(0x3c8, 0); outp(0x3c9, 0x1f); outp(0x3c9, 0x1f); outp(0x3c9, 0x1f);
#endif

    uint32_t bytesToRender = bufferSamples * convinfo.bytesPerSample;
    uint8_t *dst = (uint8_t*)buffer;
    
    poolEntry* block = &poolEntries[state.firstIdx];
    uint32_t startPos = state.firstRemainingPos;
    uint32_t blockRemainder = block->length - state.firstRemainingPos;  // b
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
            state.firstIdx++; if (state.firstIdx == state.poolSize) state.firstIdx = 0;
            block = &poolEntries[state.firstIdx];
            startPos = 0;
            blockBytesToCopy = (bytesToRender < block->length ? bytesToRender : block->length);
            
        }
    }
    // save current state
    state.firstRemainingPos = startPos;
    state.firstEntry = block;
    state.poolAvail += blocksConsumed;

    // tell render to remove stuck blocks
    isIsrFinished = true;

#if DEBUG_RASTER == 1
    outp(0x3c8, 0); outp(0x3c9, 0x1f); outp(0x3c9, 0x0); outp(0x3c9, 0x1f);
#endif

    // render blocks if below buffer watermark
    if ((isModifying == false) && ((state.poolSize - state.poolAvail) <= state.poolWatermark))
        decode(blocksConsumed);

#if DEBUG_RASTER == 1
    outp(0x3c8, 0); outp(0x3c9, 0); outp(0x3c9, 0); outp(0x3c9, 0);
#endif

    return callbackOk;
}


mp2play_dos::~mp2play_dos()
{
    if (isPlaying) stop();
    if (isInitialized) done();
}

bool mp2play_dos::init(uint32_t decodedPool, uint32_t bufferSamples, bool autosetup, bool mono, bool downsample2to1)
{
    if ((decodedPool == 0) || (bufferSamples == 0)) return false;

    // init sndlib
    if (sndlibInit() != SND_ERR_OK) return false;
    uint32_t rtn = SND_ERR_OK;

    // query, detect, create sound device
    rtn = sndlibCreateDevice(&dev, autosetup ? SND_CREATE_DEVICE_AUTO_DETECT : SND_CREATE_DEVICE_MANUAL_SELECT);
    if (rtn == SND_ERR_USEREXIT) {
        printf("user exit\n");
        return false;
    }
    if (rtn != SND_ERR_OK) {
        printf("mp2play_dos error: no available sound devices found!\n");
        return false;
    }

    // init device
    rtn = dev->init();
    if (rtn != SND_ERR_OK) {
        printf("mp2play_dos::init() error: unable to init sound device (rtn = %d)\n", rtn);
        sndlibDestroyDevice(dev);
        return false;
    }

    printf("mp2play_dos::init(): %s init success\n", dev->getName());

    // save other props
    isDownsampled = downsample2to1;
    isMono = mono;
    state.poolSize = decodedPool;
    state.bufferSamples = bufferSamples;
    
    // init decoder
    decoder.init();

    return true;
}

bool mp2play_dos::load(const char * filename)
{
    FILE *f = fopen(filename, "rb");
    size_t fsize = 0;
    
    if (f) {
        // get filesize
        fseek(f, 0, SEEK_END);
        fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        // allocate memory
        mp2 = new uint8_t[fsize + 4];   // reserve extra word for end of stream
        if (mp2 == NULL) {
            printf("mp2play_dos::load() error: insufficient memory to load mp2 stream!\n");
            return false;
        }

        // read file
        size_t rtn = fread(mp2, sizeof(uint8_t), fsize, f);
        if (rtn != fsize) {
            printf("mp2play_dos::load() error: expected %d bytes, read %d\n", fsize, rtn);
            return false;
        }
        // mark end of stream
        *(uint32_t*)(mp2 + fsize) = 0;

        fclose(f);
    }
    else {
        perror("mp2play_dos::load() error: unable to open mp2 file: ");
        return false;
    }

    // yeah :) 
    mp2pos = mp2;
    mp2size = fsize;
    return allocateBuffers();
}

bool mp2play_dos::loadmem(void * ptr, uint32_t size)
{
    // doesn't check for validity
    mp2 = (uint8_t*)ptr;
    mp2size = size;
    return allocateBuffers();
}

// load 2nd stage - open sound stream, get converter info
bool mp2play_dos::allocateBuffers()
{
    // parse header
    uint32_t rtn = decoder.decodeHeader(mp2pos, mp2header);
    
    if (rtn == 0) {
        printf("mp2play_dos::load() error: no MPEG frame found or unknown frame format\n");
        return false;
    }

    // request format
    uint32_t samplerate = mp2header.sampleRate >> (isDownsampled ? 1 : 0);
    if (dev->isFormatSupported(samplerate, SND_FMT_DEPTH_MASK | SND_FMT_SIGN_MASK | (isMono ? SND_FMT_MONO : SND_FMT_STEREO), NULL) != SND_ERR_OK) {
        // try mono
        isMono = true;
        if (dev->isFormatSupported(samplerate, SND_FMT_DEPTH_MASK | SND_FMT_SIGN_MASK | SND_FMT_MONO, NULL) != SND_ERR_OK) {
            printf("mp2play_dos::load() error: unsupported sample rate or format!\n", rtn);
        }
    }
    uint32_t format = SND_FMT_INT16 | SND_FMT_SIGNED | (isMono ? SND_FMT_MONO : SND_FMT_STEREO);

    rtn = dev->open(samplerate, format, state.bufferSamples, 0, callbackBouncer, (void*)this, &convinfo);
    if (rtn != SND_ERR_OK) {
        printf("mp2play_dos::load() error: unable to open sound device (rtn = %d)\n", rtn);
        return false;
    }
    
    printf("mp2play_dos::load(): stream open success: %d->%d hz %sbit %s %s\n",
        convinfo.sourceSampleRate,  convinfo.sampleRate,
        convinfo.format & SND_FMT_INT8  ? "8" : (convinfo.format & SND_FMT_INT16 ? "16" : "unk"),
        convinfo.format & SND_FMT_MONO ? "mono" : "stereo",
        convinfo.format & SND_FMT_SIGNED ? "signed" : "unsigned"
        );

    // init decoded stream pool
    state.poolBytesPerEntry = 576 * (isDownsampled ? 1 : 2) * convinfo.bytesPerSample;

#ifdef DEBUG
    printf("bytes per frame - %d\n", state.poolBytesPerEntry);
#endif  
  
    pool = new uint8_t[state.poolSize * state.poolBytesPerEntry];
    poolEntries = new poolEntry[state.poolSize];
    if (!pool || !poolEntries) {
        printf("mp2play_dos::load() error: not enough memory to initialize pool\n", rtn);
        return false;
    }

    memset(pool,        0, sizeof(state.poolSize * (state.poolBytesPerEntry)));
    memset(poolEntries, 0, sizeof(state.poolSize * (sizeof(poolEntry))));

    // set pool head/tail
    state.firstCurrentPos = 0;
    state.firstIdx = 0;
    state.firstEntry = poolEntries;
    state.firstPos = 0;

    state.lastEntry = poolEntries;
    state.lastIdx = 0;
    state.lastPos = 0;

    state.poolAvail = state.poolSize;
    state.firstRemainingPos = 0;
    
    // update watermark
    state.poolWatermark = ((4 * state.bufferSamples * convinfo.bytesPerSample) / state.poolBytesPerEntry);

    isModifying = false;
    isInitialized = true;
    isIsrFinished = false;

    // request to fill pool
    decode(0);  // fill until pool end
    
    return true;
}

bool mp2play_dos::decode(uint32_t frames)
{
    int16_t decodeBuffer[1152 * 2];

    // lock buffer tail
    isModifying = true;
    uint32_t availAtStart = state.poolAvail;
    
    // don't fill
    if ((frames == 0) || (frames >= availAtStart)) {
        frames = availAtStart - 1; // fill until end, leave 1 free "end" frame
    };
    
    uint32_t currentAvail = availAtStart;
    uint32_t currentIdx = state.lastIdx;
    for (size_t block = 0; block < frames; block++) {
        uint32_t rtn = decoder.decode(mp2pos, &decodeBuffer[0], isMono); if (rtn == 0) return false;
        mp2pos += rtn; if ((mp2pos - mp2) >= mp2size) mp2pos = mp2;
        
        // totally crappy downsampler (TODO: implement half-rate mode in mp2 decoder library)
        if (isDownsampled == true) {
            if (isMono == true) {
                for (size_t i = 0; i < 576; i++) {
                    decodeBuffer[i] = (decodeBuffer[2*i] + decodeBuffer[2*i]) >> 1;
                }
            } else {
                for (size_t i = 0; i < 576*2; i += 2) {
                    decodeBuffer[i + 0] = (decodeBuffer[2*i + 0] + decodeBuffer[2*i + 2]) >> 1;
                    decodeBuffer[i + 1] = (decodeBuffer[2*i + 1] + decodeBuffer[2*i + 3]) >> 1;
                }
            }
        }

        // convert    
        uint8_t* samplePtr = (uint8_t*)pool + (state.poolBytesPerEntry * currentIdx);
        convinfo.proc(samplePtr, decodeBuffer, 576 * (isDownsampled ? 1 : 2), convinfo.parm, convinfo.parm2);          // store preconverted samples (hi pc honker)

        // fill pool stuff
        poolEntries[currentIdx].consumed = false;
        poolEntries[currentIdx].length = state.poolBytesPerEntry;
        poolEntries[currentIdx].samples = samplePtr;
        poolEntries[currentIdx].next = NULL;
        
        currentIdx++; if (currentIdx == state.poolSize) currentIdx = 0; currentAvail--; state.lastPos += 1152;
    }
    
    // link tail
    state.lastIdx = currentIdx;
    state.lastEntry = &poolEntries[currentIdx];
    
    if (isIsrFinished) {
        isIsrFinished = false;
        currentAvail += (state.poolAvail - availAtStart);
    }
    state.poolAvail = currentAvail;
    
    // release lock
    isModifying = false;
    
    return true;
}

bool mp2play_dos::play(uint64_t pos)
{
    if (!isInitialized) return false;

    // set start position
    if (pos != -1) {
        mp2pos = mp2;
        for (uint64_t i = 0; i < pos; i++) {
            uint32_t rtn = decoder.decodeHeader(mp2pos, mp2header);
            if (rtn == 0) {
                printf("mp2play_dos::play() error: end of file reached\n");
            } else mp2pos += rtn;
        }

        // flush entire ringbuffer
        state.firstCurrentPos = 0;
        state.firstIdx = 0;
        state.firstEntry = poolEntries;
        state.firstPos = 0;

        state.lastEntry = poolEntries;
        state.lastIdx = 0;
        state.lastPos = 0;

        state.poolAvail = state.poolSize;
        state.firstRemainingPos = 0;

        // refill
        decode(0);
    }

    // start for first time
    uint32_t rtn = dev->start();
    if (rtn != SND_ERR_OK) {
        printf("mp2play_dos::play() error: unable to start stream (rtn = %d)\n", rtn);
        return false;
    }

    isPlaying = true;
    bufferPlay = 0;
    
    return true;
}

bool mp2play_dos::pause()
{
    return (dev->pause() != SND_ERR_OK);
}

bool mp2play_dos::resume()
{
    return (dev->resume() != SND_ERR_OK);
}

int64_t mp2play_dos::getPos()
{
    uint64_t pos = dev->getPos() << (isDownsampled ? 1 : 0);
    bufferPlay = pos;
    return pos;
}

bool mp2play_dos::stop()
{
    // start for first time
    uint32_t rtn = dev->stop();
    if (rtn != SND_ERR_OK) {
        printf("mp2play_dos::stop() error: unable to stop stream (rtn = %d)\n", rtn);
        return false;
    }

    isPlaying = false;

    return true;
}

bool mp2play_dos::done()
{
    if (isPlaying) stop();
    
    dev->done();
    
    // destroy device
    sndlibDestroyDevice(dev);

    // destroy pool
    delete[] pool;
    delete[] poolEntries;
    
    isInitialized = false;
    
    // cleanup sndlib
    if (sndlibDone() != SND_ERR_OK) return false;

    return true;
}




