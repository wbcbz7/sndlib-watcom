#ifndef __IRQ_H__
#define __IRQ_H__

#ifdef __DOS__

#include <i86.h>
#include <dos.h>
#include "dpmi.h"

#define irqcall __interrupt __far

enum {
    IRQ_SECONDARYPIC    = 1 << 0,
    
    IRQ_SPURIOUS        = 1 << 2,   // irq 7 and 15, sanity check
};

enum {
    IRQENTRY_HOOK_REALMODE       = 1 << 0,   // set if realmode ISR
};

struct irqInfo {
    unsigned char intnum;
    unsigned char picbase;
    unsigned char mask;
    unsigned char flags;
};

struct irqEntry {
    unsigned long   hooked;
    unsigned long   num;
    unsigned long   flags;
    unsigned long   oldmask;
    irqInfo        *info;

    // protected-mode
    void (irqcall  *oldhandler)();
    void (irqcall  *handler)();

    // real-mode old mode handler
    _dpmi_rmpointer rmhandler;
    _dpmi_rmpointer oldrmhandler;
};

// protected mode hook
bool irqInit();
bool irqHook(unsigned long num, irqEntry *p, bool unmask);
bool irqUnhook(irqEntry *p, bool mask);

#else   

#error "irq stuff won't work on non-DOS environments"

#endif

#endif
