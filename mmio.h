#pragma once

// force MMIO read
uint32_t MMIO_READ32_IMPL(void *addr);
#pragma aux MMIO_READ32_IMPL = "mov eax, [edi]" parm [edi] value [eax]
uint16_t MMIO_READ16_IMPL(void *addr);
#pragma aux MMIO_READ16_IMPL = "mov ax,  [edi]" parm [edi] value [ax]
uint8_t  MMIO_READ8_IMPL (void *addr);
#pragma aux MMIO_READ8_IMPL  = "mov al,  [edi]" parm [edi] value [al]

#define MMIO_FORCE_READ32(addr) (MMIO_READ32_IMPL((void*)addr))
#define MMIO_FORCE_READ16(addr) (MMIO_READ16_IMPL((void*)addr))
#define MMIO_FORCE_READ8(addr)  (MMIO_READ8_IMPL ((void*)addr))

// force MMIO write
uint32_t MMIO_WRITE32_IMPL(void *addr, uint32_t val);
#pragma aux MMIO_WRITE32_IMPL = "mov [edi], eax" parm [edi] [eax] value [eax]
uint16_t MMIO_WRITE16_IMPL(void *addr, uint16_t val);
#pragma aux MMIO_WRITE16_IMPL = "mov [edi], ax" parm [edi] [ax] value [ax]
uint8_t  MMIO_WRITE8_IMPL (void *addr, uint8_t val);
#pragma aux MMIO_WRITE8_IMPL  = "mov [edi], al" parm [edi] [al] value [al]
#define MMIO_FORCE_WRITE32(addr, val) (MMIO_WRITE32_IMPL((void*)addr, val))
#define MMIO_FORCE_WRITE16(addr, val) (MMIO_WRITE16_IMPL((void*)addr, val))
#define MMIO_FORCE_WRITE8(addr, val)  (MMIO_WRITE8_IMPL ((void*)addr, val))

#define MMIO_FORCE_MODIFY32(addr, mask, val) MMIO_FORCE_WRITE32(addr, (MMIO_FORCE_READ32(addr) & (~mask)) | ((val) & (mask)))
#define MMIO_FORCE_MODIFY16(addr, mask, val) MMIO_FORCE_WRITE16(addr, (MMIO_FORCE_READ16(addr) & (~mask)) | ((val) & (mask)))
#define MMIO_FORCE_MODIFY8(addr, mask, val)  MMIO_FORCE_WRITE8 (addr, (MMIO_FORCE_READ8 (addr) & (~mask)) | ((val) & (mask)))

#define MMIO_ALWAYS_FORCE

// instantinate MMIO read
#ifdef MMIO_ALWAYS_FORCE

    #define MMIO_READ32(addr) MMIO_FORCE_READ32(addr)
    #define MMIO_READ16(addr) MMIO_FORCE_READ16(addr) 
    #define MMIO_READ8(addr)  MMIO_FORCE_READ8(addr)

    #define MMIO_WRITE32(addr, val) MMIO_FORCE_WRITE32(addr, val)
    #define MMIO_WRITE16(addr, val) MMIO_FORCE_WRITE16(addr, val)
    #define MMIO_WRITE8(addr, val)  MMIO_FORCE_WRITE8(addr, val)

    #define MMIO_MODIFY32(addr, mask, val) MMIO_FORCE_MODIFY32(addr, mask, val)
    #define MMIO_MODIFY16(addr, mask, val) MMIO_FORCE_MODIFY16(addr, mask, val)
    #define MMIO_MODIFY8(addr, mask, val)  MMIO_FORCE_MODIFY8(addr, mask, val)

#else 

    #define MMIO_READ32(addr) (*((uint32_t*)(addr)))
    #define MMIO_READ16(addr) (*((uint16_t*)(addr)))
    #define MMIO_READ8(addr)  (*((uint8_t *) (addr)))

    #define MMIO_WRITE32(addr, val) (*((uint32_t*)(addr)) = (uint32_t)(val))
    #define MMIO_WRITE16(addr, val) (*((uint16_t*)(addr)) = (uint16_t)(val))
    #define MMIO_WRITE8(addr, val)  (*((uint8_t *)(addr)) = (uint8_t )(val))

    #define MMIO_MODIFY32(addr, mask, val) (*((uint32_t*)(addr)) = (uint32_t)(*((uint32_t*)(addr)) & (uint32_t)(~mask)) | (uint32_t)((val) & (mask)))
    #define MMIO_MODIFY16(addr, mask, val) (*((uint16_t*)(addr)) = (uint16_t)(*((uint16_t*)(addr)) & (uint16_t)(~mask)) | (uint16_t)((val) & (mask)))
    #define MMIO_MODIFY8(addr, mask, val)  (*((uint8_t *)(addr)) = (uint8_t) (*((uint8_t *)(addr)) & (uint8_t )(~mask)) | (uint8_t )((val) & (mask)))

#endif