#include <stdio.h>
#include <stdint.h>
#include "convert.h"
#include "snddefs.h"

// 16bit stereo -> 16bit stereo
int __sndconvcall __declspec(naked) sndconv_memcpy(void *dst, void *src, uint32_t length, uint32_t div, uint32_t) {
    _asm {
        mov     eax, ecx
        mov     ecx, edx
        shr     eax, cl
        mov     ecx, eax
        rep     movsd
        ret
    }
}

//
int __sndconvcall __declspec(naked) sndconv_memcpy_shl(void *dst, void *src, uint32_t length, uint32_t div, uint32_t) {
    _asm {
        mov     eax, ecx
        mov     ecx, edx
        shl     eax, cl
        mov     ecx, eax
        rep     movsd
        ret
    }
}

#ifdef SNDLIB_CONVERT_ENABLE_ARBITRARY

// 8 bit any -> 16 bit any (for HD audio and other PCI devices)
int __sndconvcall sndconv_8_16(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t postmul) {
    int8_t *p = (int8_t*)src;
    int16_t *v = (int16_t*)dst;
    
    length *= postmul;
    if (length == 0) return 0;
    do {
        *v++ = ((int16_t)(*p++ ^ xormask) << 8);
    } while (--length != 0);
    
    return 0;
}

// 16bit stereo -> 16bit mono SIGNED
int __sndconvcall sndconv_16s_16m(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t) {
    int16_t *p = (int16_t*)src;
    int16_t *v = (int16_t*)dst;
    
    if (length == 0) return 0;
    do {
        *v++ = ((*p >> 1) + (*(p+1) >> 1)) ^ xormask; p+=2;
    } while (--length != 0);
    
    return 0;
}

// 8bit stereo -> 8bit mono
int __sndconvcall sndconv_8s_8m(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t) {
    uint8_t *p = (uint8_t*)src;
    uint8_t *v = (uint8_t*)dst;

    if (length == 0) return 0;
    do {
        *v++ = (((int)*p + *(p + 1)) >> 1) ^ xormask; p += 2;
    } while (--length != 0);

    return 0;
}

// 8bit stereo -> 8bit mono UNSIGNED
int __sndconvcall sndconv_8sus_8m(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t) {
    uint8_t *p = (uint8_t*)src;
    uint8_t *v = (uint8_t*)dst;

    if (length == 0) return 0;
    do {
        *v++ = ((((int)(*p ^ 0x80)) + ((int)(*(p+1) ^ 0x80))) >> 1) ^ xormask; p += 2;
    } while (--length != 0);

    return 0;
}

// 16bit stereo -> 8 bit (un)signed stereo, no dither
int __sndconvcall sndconv_16s_8s(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t) {
    uint32_t *p = (uint32_t*)src;
    uint32_t *v = (uint32_t*)dst;
    
    // wtf
    if (length < 2) return 0;
    do {
        *v++ = (((*p >> 8) & 0x000000FF) | ((*p >> 16) & 0x0000FF00) | ((*(p+1) << 8) & 0x00FF0000) | ((*(p+1) & 0xFF000000))) ^ xormask; p += 2; length -= 2;
    } while (length != 0);
    
    return 0;
}

// 16bit stereo -> 8 bit (un)signed stereo, no dither
int __sndconvcall sndconv_16m_8m(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t) {
    uint32_t *p = (uint32_t*)src;
    uint32_t *v = (uint32_t*)dst;
    
    // wtf
    if (length < 4) return 0;
    do {
        *v++ = (((*p >> 8) & 0x000000FF) | ((*p >> 16) & 0x0000FF00) | ((*(p+1) << 8) & 0x00FF0000) | ((*(p+1) & 0xFF000000))) ^ xormask; p += 2; length -= 4;
    } while (length != 0);
    
    return 0;
}

// 16bit stereo -> 8 bit (un)signed mono, no dither
int __sndconvcall sndconv_16s_8m(void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t) {
    /*
    int32_t *p = (int32_t*)src;
    int32_t *v = (int32_t*)dst;
    
    // wtf
    if (length < 4) return 0;
    do {
        *v++ =  (((((*(p + 0) + (*(p + 0) >> 16))) >> 9)  & 0x000000FF)  |
                 ((((*(p + 1) + (*(p + 1) >> 16))) >> 1)  & 0x0000FF00)  |
                 ((((*(p + 2) + (*(p + 2) >> 16))) << 7)  & 0x00FF0000)  | 
                 ((((*(p + 3) + (*(p + 3) >> 16))) << 15) & 0xFF000000)) ^ 0x80808080;
                 
        length -= 4; p += 4;
    } while (length != 0);
    */
    
    // BROKEN TO THE FUCK CODE AAAAAAAAARHGGGGGGGGG
    
    int16_t *p = (int16_t*)src;
    int8_t *v  = (int8_t*)dst;
    if (length == 0) return 0;
    
    #if 0
    _asm {
        mov     esi, [p]
        mov     edi, [v]
        mov     ecx, [length]
        mov     ebx, [xormask]
        
    _loop:
        mov     ax, [esi]
        //add     ax, [esi+2]
        //rcr     ax, 1
        xor     ah, bh
        mov     [edi], ah
        add     esi, 4
        add     edi, 1
        dec     ecx
        jnz     _loop
    }
    #endif
    
    do {
        *v++ = (((*p >> 1) + (*(p+1) >> 1)) >> 8) ^ 0x80; p += 2;
    } while(--length != 0);
    
    
    return 0;
}

#endif

#ifdef SNDLIB_CONVERT_ENABLE_PCSPEAKER

// 8bit mono to 8bit xlated (for PC speaker)
int __sndconvcall sndconv_8m_xlat (void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t xlatPtr) {
    uint8_t *p = (uint8_t*)src; uint8_t *v = (uint8_t*)dst; uint8_t *x = (uint8_t*)xlatPtr; xormask &= 0xFF;

    if (length == 0) return 0;
    do {
        *v++ = x[*p++ ^ xormask];
    } while (--length != 0);

    return 0;
}
// 8bit stereo to 8bit xlated (for PC speaker)
int __sndconvcall sndconv_8s_xlat (void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t xlatPtr) {
    uint8_t *p = (uint8_t*)src; uint8_t *v = (uint8_t*)dst; uint8_t *x = (uint8_t*)xlatPtr; xormask &= 0xFF;
    xormask = (int8_t)xormask; // sign-extend it

    if (length == 0) return 0;
    
    _asm {
            mov     edi, [v]
            mov     esi, [p]
            mov     ebx, [x]
            mov     ecx, [length]

        _loop:
            movsx   eax, byte ptr [esi + 0]      // fuck P5
            movsx   edx, byte ptr [esi + 1]
            xor     eax, [xormask]
            xor     edx, [xormask]
            add     eax, edx
            shr     eax, 1
            and     eax, 0xFF
            mov     edx, [ebx + eax]
            mov     [edi], edx

            add     esi, 2
            add     edi, 1
            dec     ecx
            jnz     _loop
    }

    /*
    do {
        uint8_t left   = *(p+0) ^ xormask;
        uint8_t right  = *(p+1) ^ xormask;

        *v++ = x[((left + right) >> 1) & 0xFF]; p += 2;
    } while (--length != 0);
    */

    return 0;
}
// 16bit mono to 8bit xlated (for PC speaker)
int __sndconvcall sndconv_16m_xlat (void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t xlatPtr) {
    uint16_t *p = (uint16_t*)src; uint8_t *v = (uint8_t*)dst; uint8_t *x = (uint8_t*)xlatPtr; xormask &= 0xFFFF;

    if (length == 0) return 0;
    do {
        *v++ = x[((*p++ ^ xormask) >> 8) & 0xFF];
    } while (--length != 0);

    return 0;
}
// 16bit stereo to 8bit xlated (for PC speaker)
int __sndconvcall sndconv_16s_xlat (void *dst, void *src, uint32_t length, uint32_t xormask, uint32_t xlatPtr) {
    uint16_t *p = (uint16_t*)src; uint8_t *v = (uint8_t*)dst; uint8_t *x = (uint8_t*)xlatPtr;
    xormask = (int16_t)xormask; // sign-extend it

    if (length == 0) return 0;
    
    _asm {
            mov     edi, [v]
            mov     esi, [p]
            mov     ebx, [x]
            mov     ecx, [length]

        _loop:
            movsx   eax, word ptr [esi + 0]      // fuck P5
            movsx   edx, word ptr [esi + 2]
            xor     eax, [xormask]
            xor     edx, [xormask]
            add     eax, edx
            shr     eax, 9
            and     eax, 0xFF
            mov     edx, [ebx + eax]
            mov     [edi], edx

            add     esi, 4
            add     edi, 1
            dec     ecx
            jnz     _loop
    }

    /*
    do {
        uint16_t left   = *(p+0) ^ xormask;
        uint16_t right  = *(p+1) ^ xormask;

        *v++ = x[((left + right) >> 9) & 0xFF]; p += 2;
    } while (--length != 0);
    */

    return 0;
}

#endif
