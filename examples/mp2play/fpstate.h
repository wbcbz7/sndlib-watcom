#include <i86.h>

#include <float.h>
#include <fenv.h>

struct fpuState {
    uint32_t    controlWord;
    uint32_t    statusWord;
    uint32_t    tagWord;
    uint32_t    ip;
    uint32_t    cs;
    uint32_t    ptrOfs;
    uint32_t    ptrSel;
    uint32_t    fpuStack[20];   // 80 bits per extended float, 8 registers -> 80 bytes
};
