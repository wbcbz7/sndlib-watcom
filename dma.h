#pragma once
#ifndef __DMA_H__
#define __DMA_H__

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
 
const struct dmaPorts {
    unsigned char address;
    unsigned char count;
    unsigned char page;
    unsigned char request;
    unsigned char mask;
    unsigned char mode;
    unsigned char clear;
    unsigned char dummy;
} dmaPorts[] = {
    { 0x00, 0x01, 0x87, 0x09, 0x0A, 0x0B, 0x0C, 0 }, // 0
    { 0x02, 0x03, 0x83, 0x09, 0x0A, 0x0B, 0x0C, 0 }, // 1
    { 0x04, 0x05, 0x81, 0x09, 0x0A, 0x0B, 0x0C, 0 }, // 2
    { 0x06, 0x07, 0x82, 0x09, 0x0A, 0x0B, 0x0C, 0 }, // 3
    
    { 0x00, 0x00, 0x00, 0xD2, 0xD4, 0xD6, 0xD8, 0 }, // 4 (unapplicable)
    { 0xC4, 0xC6, 0x8B, 0xD2, 0xD4, 0xD6, 0xD8, 0 }, // 5
    { 0xC8, 0xCA, 0x89, 0xD2, 0xD4, 0xD6, 0xD8, 0 }, // 5
    { 0xCC, 0xCE, 0x8A, 0xD2, 0xD4, 0xD6, 0xD8, 0 }, // 5
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

#endif

