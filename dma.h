#pragma once

#include <stdlib.h>
#include "dpmi.h"

enum {
    ISA_DMA_MAX_BUFFER_SIZE = 65536
};

// DMA block struct
struct dmaBlock {
    void        *ptr;       // linear address
    _dpmi_ptr   dpmi;       // internal DPMI structs
};
 
struct dmaPorts {
    unsigned char address;
    unsigned char count;
    unsigned char page;
    unsigned char request;
    unsigned char mask;
    unsigned char mode;
    unsigned char clear;
    unsigned char dummy;
};

// mode consts
enum {
    dmaModeDemand       = 0x0,
    dmaModeSingle       = 0x40,
    dmaModeBlock        = 0x80,
    dmaModeCascade      = 0xC0,

    dmaModeInc          = 0x0,
    dmaModeDec          = 0x20,

    dmaModeNoAutoInit   = 0x0,
    dmaModeAutoInit     = 0x10,

    dmaModeVerify       = 0x0,
    dmaModeWrite        = 0x4,
    dmaModeRead         = 0x8,
};

// allocate memory for DMA transfers
bool dmaAlloc(size_t len, dmaBlock *blk);
bool dmaAllocUnaligned(size_t len, dmaBlock *blk);
bool dmaFree(dmaBlock *blk);

// setup DMA for transfer and start it
bool dmaSetup(size_t chan, dmaBlock *blk, size_t len, unsigned char mode);

// pause/resume transfer
bool dmaPause(size_t chan);
bool dmaResume(size_t chan);

// stop transfer
bool dmaStop(size_t chan);

// get current DMA transfer position
unsigned long dmaGetPos(size_t chan, bool lockInts = true);

