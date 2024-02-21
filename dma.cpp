#include <conio.h>
#include <stdint.h>

#include "cli.h"
#include "dma.h"
#include "dpmi.h"
#include "logerror.h"

dmaPorts dmaPorts[] = {
    { 0x00, 0x01, 0x87, 0x09, 0x0A, 0x0B, 0x0C, 0 }, // 0
    { 0x02, 0x03, 0x83, 0x09, 0x0A, 0x0B, 0x0C, 0 }, // 1
    { 0x04, 0x05, 0x81, 0x09, 0x0A, 0x0B, 0x0C, 0 }, // 2
    { 0x06, 0x07, 0x82, 0x09, 0x0A, 0x0B, 0x0C, 0 }, // 3
    
    { 0x00, 0x00, 0x00, 0xD2, 0xD4, 0xD6, 0xD8, 0 }, // 4 (unapplicable)
    { 0xC4, 0xC6, 0x8B, 0xD2, 0xD4, 0xD6, 0xD8, 0 }, // 5
    { 0xC8, 0xCA, 0x89, 0xD2, 0xD4, 0xD6, 0xD8, 0 }, // 5
    { 0xCC, 0xCE, 0x8A, 0xD2, 0xD4, 0xD6, 0xD8, 0 }, // 5
};

bool dmaAllocUnaligned(size_t len, dmaBlock *blk) {
    // check block size
    if ((len == 0) || (len > 65536)) {
        logerr("length must be nonzero and less than 64k\n");
        return false;
    }
    
    // allocate block "as-is"
    _dpmi_ptr dpmi_block;
    dpmi_getdosmem((len + 15) >> 4, &dpmi_block);
    if (dpmi_status) {
        logerr("unable to allocate memory for DMA buffer\n");
        return false;
    }

    size_t linaddr = (dpmi_block.segment << 4);
    blk->ptr = (void*)linaddr;
    blk->dpmi = dpmi_block;
    return true;
}

bool dmaAlloc(size_t len, dmaBlock *blk) {
    // check block size
    if ((len == 0) || (len > 65536)) {
        logerr("length must be nonzero and less than 64k\n");
        return false;
    }
    
    // allocate block "as-is"
    _dpmi_ptr dpmi_block;
    dpmi_getdosmem((len + 15) >> 4, &dpmi_block);
    if (dpmi_status) {
        logerr("unable to allocate memory for DMA buffer\n");
        return false;
    }
    
    size_t linaddr = (dpmi_block.segment << 4);
    
    // test for 64k boundary crossing
    if (((linaddr + len - 1) & ~0xFFFF) != (linaddr & ~0xFFFF)) {
        // calculate block size needed for alignment
        size_t adjust  = ((linaddr + len) & ~0xFFFF) - linaddr;
        size_t newsize = len + adjust;
        
        // realloc block
        dpmi_freedosmem(&dpmi_block);
        if (dpmi_status) {
            logerr("DPMI error\n");
            return false;
        }
        
        dpmi_getdosmem((newsize + 15) >> 4, &dpmi_block);
        if (dpmi_status) {
            logerr("unable to allocate memory for DMA buffer\n");
            return false;
        }
        
        // test again
        blk->ptr = (void*)(((dpmi_block.segment << 4) + adjust) & ~0xFFFF);
        
        // test again
        linaddr = (size_t)blk->ptr;
        if (((linaddr + len - 1) & ~0xFFFF) != (linaddr & ~0xFFFF)) {
            logerr("unable to allocate non-64K crossing DMA buffer\n");
            return false;
        }
    } else {
        blk->ptr = (void*)linaddr;
    }
    
    blk->dpmi = dpmi_block;
    return true;
}

bool dmaFree(dmaBlock *blk) {
    dpmi_freedosmem(&blk->dpmi);
    if (dpmi_status) {
        logerr("unable to free DMA buffer\n");
        return false;
    }
    
    return true;
}

bool dmaSetup(size_t chan, dmaBlock *blk, size_t len, unsigned char mode, uint32_t offset) {
    
    if ((chan == 4) || (chan > 7)) {
        logerr("channel number must be for 0 to 7 (excluding 4)\n");
        return false;
    }
    
    if ((len == 0) || (len > 65536)) {
        logerr("length must be nonzero and less than 64k\n");
        return false;
    }
    
    unsigned char rawchan = chan & 3;
    
    // disable interrupts!
    unsigned long flags = pushf();
    _disable();
    
    // mask channel
    outp(dmaPorts[chan].mask,    0x04 | rawchan);
    
    // clear flip-flop
    inp(dmaPorts[chan].clear);
    
    // set mode
    outp(dmaPorts[chan].mode,    mode | rawchan);
    
    // apparently high DMAs require some shifting to make it work
    size_t rawptr = (size_t)blk->ptr + offset;
           rawptr = rawptr           >> (chan >= 4 ? 1 : 0);
    size_t rawlen = (len             >> (chan >= 4 ? 1 : 0)) - 1;
    size_t rawpage =(size_t)blk->ptr >> 16;
    
    // set offset
    outp(dmaPorts[chan].address,  rawptr        & 0xFF);
    outp(dmaPorts[chan].address, (rawptr >>  8) & 0xFF);
    outp(dmaPorts[chan].page,     rawpage       & 0xFF);
    
    // clear flip-flop again
    inp(dmaPorts[chan].clear);
    
    // set length
    outp(dmaPorts[chan].count,    rawlen       & 0xFF);
    outp(dmaPorts[chan].count,   (rawlen >> 8) & 0xFF);
    
    // unmask channel
    outp(dmaPorts[chan].mask,    0x00 | rawchan);
    
    // enable interrupts
    //_enable_if_enabled(flags);
    _enable();

    return true;
}

bool dmaPause(size_t chan) {
#ifdef DEBUG
    if ((chan == 4) || (chan > 7)) {
        logerr("channel number must be for 0 to 7 (excluding 4)\n");
        return true;
    }
#endif
    
    // mask channel
    outp(dmaPorts[chan].mask,    0x04 | (chan & 3));
    
    return true;
};

bool dmaResume(size_t chan) {
#ifdef DEBUG
    if ((chan == 4) || (chan > 7)) {
        logerr("channel number must be for 0 to 7 (excluding 4)\n");
        return true;
    }
#endif
    
    // unmask channel
    outp(dmaPorts[chan].mask,    0x00 | (chan & 3));
    
    return true;
};

bool dmaStop(size_t chan) {
#ifdef DEBUG
    if ((chan == 4) || (chan > 7)) {
        logerr("channel number must be for 0 to 7 (excluding 4)\n");
        return true;
    }
#endif
    
    // mask channel
    outp(dmaPorts[chan].mask,    0x04 | (chan & 3));
    
    // clear channel
    outp(dmaPorts[chan].clear,   0x00);
    
    return true;
};

uint16_t dmaRead(unsigned short port); 
#pragma aux dmaRead = "in  al, dx" "mov  bl, al" "in  al, dx" "mov  bh, al" parm [dx] value [bx] modify [ax]

// read DMA controller 16-bit register
uint32_t dmaRead16bit(uint32_t reg, bool lockInts) {   
    unsigned long flags;
    
    if (lockInts == true) {
        // disable interrupts!
        flags = pushf();
        _disable();
    }
    
    // i8237 does not feature latching current address/count, so high byte can change while reading low byte!
    // so read it until retrieving correct value
    size_t timeout = 20;        // try at least 20 times
    volatile unsigned short oldpos, pos = dmaRead(reg);
    
    do {
        oldpos = pos;
        pos = dmaRead(reg);
        
        if ((oldpos & 0xFF00) == (pos & 0xFF00)) break;
    } while (--timeout);
    
    if (lockInts == true) {
        // enable interrupts
        _enable_if_enabled(flags);
    }

    return pos;
}

// return current address (A[15..0] for 8bit channels, A[16..1] for 16bit)
uint32_t dmaGetCurrentAddress(uint32_t chan, bool lockInts) {
    // clear flip-flop
    inp(dmaPorts[chan].clear);
    return dmaRead16bit(dmaPorts[chan].address, lockInts);
}

// return current count register value
uint32_t dmaGetCurrentCount(uint32_t chan, bool lockInts) {
    // clear flip-flop
    inp(dmaPorts[chan].clear);
    return dmaRead16bit(dmaPorts[chan].count, lockInts);
}