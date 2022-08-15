#include <stdlib.h>
#include "getbits.h"

// init
BitReader& BitReader::init(uint8_t *buffer) {

    // set buffer
    bufferPos = buffer;
    
    // fill window with new bits
    window = (*bufferPos << 24) | (*(bufferPos + 1) << 16) | (*(bufferPos + 2) << 8) | (*(bufferPos + 3) << 0);
    bufferPos += 4;
    bitsInWindow = 32;

    return *this;
}

#if 0
// get count bits
uint32_t BitReader::getbits(uint32_t count) {
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

// get count bits, do not refill
uint32_t BitReader::popbits(uint32_t count) {
    if (count == 0) return 0;

    // extract bits from window
    uint32_t result = (window >> (bitsInWindow - count)) & ((1 << count) - 1);
    bitsInWindow -= count;

    return result;
}

// refill window (gurantees at least 24 bits available)
void BitReader::refill() {
    // fill window with new bits
    while (bitsInWindow <= 24) {
        window = (window << 8) | *bufferPos++;
        bitsInWindow += 8;
    }
}

// deadcode...
/*
void initbits(getbits_data* data, uint8_t* buffer) {
    // set buffer
    data->bufferPos = buffer;
    
    // fill window with new bits
    data->window = (*data->bufferPos << 24) | (*(data->bufferPos + 1) << 16) | (*(data->bufferPos + 2) << 8) | (*(data->bufferPos + 3) << 0);
    data->bufferPos += 4;
    data->bitsInWindow = 32;
};
*/
/*
uint32_t getbits(getbits_data* data, uint32_t count) {
    static uint32_t countmask[] = {
        0x00000000, 0x00000001, 0x00000003, 0x00000007,
        0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F,
        0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
        0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF,
        0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF,
        0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
        0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
        0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF
    };
    
    uint32_t rtn;
    _asm {
        mov     eax, [count]
        mov     ebx, [data]
        
        and     eax, eax
        jz      quit
        
        mov     ecx, [ebx].bitsInWindow     // ecx = bitsInWindow
        mov     esi, [ebx].bufferPos        // esi = bufferPos
        
        sub     ecx, eax                    // ecx = bitsInWindow - count
        mov     edx, edi                    // eax - saved window
        
        shr     edi, cl                     // edi = window >> (bitsInWindow - count)
        
        and     edi, [countmask + 4*eax]    // edi = window >> (bitsInWindow - count) & ((1 << count) - 1);
        mov     eax, [ebx].window           // edi - window
        
        cmp     ecx, 24
        ja      quit
        
    refill:
        shl     edx, 8  
        inc     edi
        
        mov     dl, [edi - 1]
        add     ecx, 8
    
        cmp     ecx, 24
        jle     refill
        
        mov     [ebx].window, edx
        
    quit:
        mov     [rtn], edi
    };
    
    return rtn;
}
*/

#endif