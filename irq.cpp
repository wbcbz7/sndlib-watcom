#include <conio.h>
#include <i86.h>
#include <dos.h>

#include "irq.h"
#include "dpmi.h"
#include "cli.h"
#include "logerror.h"

static irqInfo irqInfoTable[] = {
    { 0x08, 0x20, 0x01, 0                               },  // 0
    { 0x09, 0x20, 0x02, 0                               },  // 1
    { 0x0A, 0x20, 0x04, 0                               },  // 2
    { 0x0B, 0x20, 0x08, 0                               },  // 3
    { 0x0C, 0x20, 0x10, 0                               },  // 4
    { 0x0D, 0x20, 0x20, 0                               },  // 5
    { 0x0E, 0x20, 0x40, 0                               },  // 6
    { 0x0F, 0x20, 0x80, IRQ_SPURIOUS                    },  // 7
    
    { 0x70, 0xA0, 0x01, IRQ_SECONDARYPIC                },  // 8
    { 0x71, 0xA0, 0x02, IRQ_SECONDARYPIC                },  // 9
    { 0x72, 0xA0, 0x04, IRQ_SECONDARYPIC                },  // 10
    { 0x73, 0xA0, 0x08, IRQ_SECONDARYPIC                },  // 11
    { 0x74, 0xA0, 0x10, IRQ_SECONDARYPIC                },  // 12
    { 0x75, 0xA0, 0x20, IRQ_SECONDARYPIC                },  // 13
    { 0x76, 0xA0, 0x40, IRQ_SECONDARYPIC                },  // 14
    { 0x77, 0xA0, 0x80, IRQ_SECONDARYPIC | IRQ_SPURIOUS }   // 15
};

// install ISR for given interrupt
// unmask == true - unmask IRQ channel after setup
bool irqHook(unsigned long num, irqEntry *p, bool unmask) {
    if (num > 15) {
        logerr  ("unable to hook IRQ %d (IRQ must be from 0 to 15)\n", num); return true;}
    if (p->handler == NULL) {
        logfatal("null irqEntry->handler pointer!\n"); return true;}
    if (p->hooked != 0) {
        logerr  ("IRQ is already hooked\n"); return true;
    }
        
    // disable interrupts and setup handler
    //unsigned long flags = pushf();
    _disable();
    
    // hook real-mode handler if requested
    if ((p->flags & IRQENTRY_HOOK_REALMODE) == IRQENTRY_HOOK_REALMODE) {
        p->oldrmhandler = dpmi_getrmvect_ex(irqInfoTable[num].intnum);
        dpmi_setrmvect_ex(irqInfoTable[num].intnum, p->rmhandler);
        p->hooked = 1;
    }

    // hook protected mode handler
    if (p->handler != NULL) {
        p->oldhandler = _dos_getvect(irqInfoTable[num].intnum);
        _dos_setvect(irqInfoTable[num].intnum, p->handler);
        p->hooked = 1;
    }
    
    p->num  = num;
    p->info = &irqInfoTable[num];
    p->oldmask = inp(irqInfoTable[num].picbase + 1) & irqInfoTable[num].mask;

    // unmask if requested
    if (unmask) {
        outp(irqInfoTable[num].picbase + 1, (inp(irqInfoTable[num].picbase + 1) & ~irqInfoTable[num].mask));
    } 
    _enable();
    //_enable_if_enabled(flags);
    
    p->hooked = 1;
    
    return false;
}

bool irqUnhook(irqEntry *p, bool mask) {
    if (p == NULL) {
        logfatal("null irqEntry pointer!\n"); return true;}
    if (p->num > 15) {
        logfatal("unable to unhook IRQ %d (IRQ must be from 0 to 15)\n", p->num); return true;}
    if (p->hooked == 0) {
        logerr  ("IRQ is not hooked\n"); return true;
    }
    
    // disable interrupts and setup handler
    //unsigned long flags = pushf();
    _disable(); 
    
    if ((p->flags & IRQENTRY_HOOK_REALMODE) == IRQENTRY_HOOK_REALMODE) 
        dpmi_setrmvect_ex(irqInfoTable[p->num].intnum, p->oldrmhandler);
    if (p->oldhandler != NULL) _dos_setvect(irqInfoTable[p->num].intnum, p->oldhandler);
    
    if (mask) {
        outp(irqInfoTable[p->num].picbase, 0x40); // nop
        outp(irqInfoTable[p->num].picbase + 1, ((inp(irqInfoTable[p->num].picbase + 1) & ~irqInfoTable[p->num].mask) | p->oldmask));
    }
    _enable();
    //_enable_if_enabled(flags);
    
    p->hooked = 0;
    
    return false;
}


