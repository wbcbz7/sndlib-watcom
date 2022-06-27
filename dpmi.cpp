#include <i86.h>
#include <string.h>
#include "dpmi.h"

#if !defined(__WATCOMC__)
#error This file should have been compiled with Watcom C/C++ compiler only
#endif

/*
    some useful DPMI functions
    by wbcbz7 zo.oz.zolb - l5.ll.zozl

    this mostly covers frequently used DPMI 0.9 calls (+ some 1.0), and some DOS extender vendor extensions
    it was never meant to be a complete DPMI adapter, so if you have something missing here, feel free to
    add it by yourself :)
    
    compiling with -D_DPMI_DJGPP_COMPATIBILITY enables compatibility with DJGPP dpmi.h functions, some of them
    are defined as stubs, though (always returning -1)
*/

// internal regs/sregs/rmregs structures - do not modify!
static union  REGS  dpmi_regs;
static struct SREGS dpmi_sregs;
static _dpmi_rmregs dpmi_rmregs;

// dpmi status (carry flag)
unsigned int dpmi_status;

// dpmi fucntions return code (few servers support them, i.e. DOS/32A does)
unsigned int dpmi_returncode;


// -----------------
// helper stuff

// convert 16:16 far pointer to flat 32 bit - Watcom C only!
void* dpmi_ptr16_16_to_flat32(unsigned long ptr16_16) {
    void* segbase = dpmi_getsegmentbase(ptr16_16 >> 16); if (dpmi_status) return NULL;
    return (void*)((unsigned long)segbase + (ptr16_16 & 0xFFFF));
}

// expand 16:16 far pointer to 16:32, no selector validity checks
void _far *dpmi_ptr16_16_to_16_32(unsigned long ptr16_16) {
    return MK_FP(ptr16_16 >> 16, ptr16_16 & 0xFFFF);
}

// code size optimisation :)
static void dpmi_zero_reg_structs() {
    memset(&dpmi_sregs, 0, sizeof(dpmi_sregs));
    memset(&dpmi_regs, 0, sizeof(dpmi_regs));
}

//-----------------------
// low-level DPMI functions
// some texts in comments from PMODEW.TXT

/*
2.0 - Function 0000h - Allocate Descriptors:
--------------------------------------------

  Allocates one or more descriptors in the client's descriptor table. The
descriptor(s) allocated must be initialized by the application with other
function calls.

In:
  AX     = 0000h
  CX     = number of descriptors to allocate

Out:
  if successful:
    carry flag clear
    AX	   = base selector

  if failed:
    carry flag set
    AX = error code (DOS/32A only)
        8011h - descriptor unavailable 
    
Notes:
) If more that one descriptor was requested, the function returns a base
  selector referencing the first of a contiguous array of descriptors. The
  selector values for subsequent descriptors in the array can be calculated
  by adding the value returned by INT 31h function 0003h.

) The allocated descriptor(s) will be set to expand-up writeable data, with
  the present bit set and a base and limit of zero. The privilege level of the
  descriptor(s) will match the client's code segment privilege level.
*/

unsigned int dpmi_getdescriptors(unsigned long count) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x0000;
    dpmi_regs.w.cx = (unsigned short)count;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;   // valid if CF=1
    
    return dpmi_regs.w.ax;
}

/*
2.1 - Function 0001h - Free Descriptor:
---------------------------------------

  Frees a descriptor.

In:
  AX     = 0001h
  BX     = selector for the descriptor to free

Out:
  if successful:
    carry flag clear

  if failed:
    carry flag set
    AX = error code (DOS/32A only)
        8022h - invalid selector 

Notes:
) Each descriptor allocated with INT 31h function 0000h must be freed
  individually with the function. Even if it was previously allocated as part
  of a contiguous array of descriptors.

) Under DPMI 1.0/VCPI/XMS/raw, any segment registers which contain the
  selector being freed are zeroed by this function.
*/

void dpmi_freedescriptor(unsigned long desc) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x0001;
    dpmi_regs.w.bx = (unsigned short)desc;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;   // valid if CF=1
    
}

/*
2.2 - Function 0002h - Segment to Descriptor:
---------------------------------------------

  Converts a real mode segment into a protected mode descriptor.

In:
  AX     = 0002h
  BX     = real mode segment

Out:
  if successful:
    carry flag clear
    AX     = selector

  if failed:
    carry flag set
    AX = error code (DOS/32A only)
        8011h - descriptor unavailable 

Notes:
) Multiple calls for the same real mode segment return the same selector.

) The returned descriptor should never be modified or freed.
*/

unsigned long dpmi_segtodesc(unsigned long segment) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x0002;
    dpmi_regs.w.bx = (unsigned short)segment;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;   // valid if CF=1
    
    return dpmi_regs.w.ax;
}

/*
2.3 - Function 0003h - Get Selector Increment Value:
----------------------------------------------------

  The Allocate Descriptors function (0000h) can allocate an array of
contiguous descriptors, but only return a selector for the first descriptor.
The value returned by this function can be used to calculate the selectors for
subsequent descriptors in the array.

In:
  AX     = 0003h

Out:
  always successful:
    carry flag clear
    AX	   = selector increment value

Notes:
) The increment value is always a power of two.
*/

unsigned long dpmi_selectorincrement() {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x0003;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;   // valid if CF=1
    
    return dpmi_regs.w.ax;
}

/*
2.4 - Function 0006h - Get Segment Base Address:
------------------------------------------------

  Returns the 32bit linear base address from the descriptor table for the
specified segment.

In:
  AX     = 0006h
  BX     = selector

Out:
  if successful:
    carry flag clear
    CX:DX  = 32bit linear base address of segment

  if failed:
    carry flag set
    AX = error code (DOS/32A only)
        8022h - invalid selector 

Notes:
) Client programs must use the LSL instruction to query the limit for a
  descriptor.
*/

// wait, should it be named "get descriptor base address"?
// and yes, returns LINEAR address (usually fine under watcom)

void* dpmi_getsegmentbase(unsigned long selector) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x0006;
    dpmi_regs.w.bx = (unsigned short)selector;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;   // valid if CF=1
    
    return (void *)(((unsigned long)dpmi_regs.w.cx << 16) | dpmi_regs.w.dx);
}

// helper fucntion to get segment limit (unlike DOS/32A, doesn't return error code in case of fail :)
unsigned long dpmi_getsegmentlimit(unsigned long selector) {
    unsigned long limit = 0;
    _asm {
        lsl     eax, [selector]
        mov     [limit], eax                // *cough*
        
        // save flags
        pushfd
        pop     dword ptr [dpmi_status]
    }
    
    // extract ZF
    dpmi_status = (!dpmi_status & 0x40) >> 6;
    
    return limit;
}

// is segment readable?
unsigned long dpmi_isreadable(unsigned long selector) {
    unsigned long limit = 0;
    _asm {
        verr    [selector]
        pushfd
        pop     dword ptr [dpmi_status]
    }
    
    // extract ZF
    dpmi_status = (!dpmi_status & 0x40) >> 6;
    
    return !dpmi_status;
}

// is segment writeable?
unsigned long dpmi_iswriteable(unsigned long selector) {
    unsigned long limit = 0;
    _asm {
        verw    [selector]
        pushfd
        pop     dword ptr [dpmi_status]
    }
    
    // extract ZF
    dpmi_status = (!dpmi_status & 0x40) >> 6;
    
    return !dpmi_status;
}


/*
2.5 - Function 0007h - Set Segment Base Address:
------------------------------------------------

  Sets the 32bit linear base address field in the descriptor for the specified
segment.

In:
  AX     = 0007h
  BX     = selector
  CX:DX  = 32bit linear base address of segment

Out:
  if successful:
    carry flag clear

  if failed:
    carry flag set
    AX = error code (DOS/32A only)
        8022h - invalid selector
        8025h - invalid linear address (changing the limit would cause
                the descriptor to reference a linear address range
                outside that allowed for DPMI clients.) 

Notes:
) Under DPMI 1.0/VCPI/XMS/raw, any segment register which contains the
  selector specified in register BX will be reloaded. DPMI 0.9 may do this,
  but it is not guaranteed.

) We hope you have enough sense not to try to modify your current CS or SS
  descriptor.
*/

void dpmi_setsegmentbase(unsigned long selector, void *base) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x0007;
    dpmi_regs.w.bx = (unsigned short)selector;
    
    dpmi_regs.w.cx = (unsigned short)(((unsigned long)base) >> 16);
    dpmi_regs.w.dx = (unsigned short)(((unsigned long)base) & 0xFFFF);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;   // valid if CF=1
}

/*
2.6 - Function 0008h - Set Segment Limit:
-----------------------------------------

  Sets the limit field in the descriptor for the specified segment.

In:
  AX     = 0008h
  BX     = selector
  CX:DX  = 32bit segment limit

Out:
  if successful:
    carry flag clear

  if failed:
    carry flag set
    AX = error code (DOS/32A only)
        8021h - invalid value (the limit is greater than 1 MB but the
                low 12 bits are not set)
        8022h - invalid selector
        8025h - invalid linear address (changing the limit would cause
                the descriptor to reference a linear address range
                outside that allowed for DPMI clients.) 
    
Notes:
) The value supplied to the function in CX:DX is the byte length of the
  segment-1.

) Segment limits greater than or equal to 1M must be page aligned. That is,
  they must have the low 12 bits set.

) This function has an implicit effect on the "G" bit in the segment's
  descriptor.

) Client programs must use the LSL instruction to query the limit for a
  descriptor.

) Under DPMI 1.0/VCPI/XMS/raw, any segment register which contains the
  selector specified in register BX will be reloaded. DPMI 0.9 may do this,
  but it is not guaranteed.

) We hope you have enough sense not to try to modify your current CS or SS
  descriptor.
*/

void dpmi_setsegmentlimit(unsigned long selector, unsigned long limit) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x0008;
    dpmi_regs.w.bx = (unsigned short)selector;
    
    // page-align limit if > 1MB
    if (limit > 0x100000) limit &= ~0xFFF;
    
    dpmi_regs.w.cx = (unsigned short)(((unsigned long)limit - 1) >> 16);
    dpmi_regs.w.dx = (unsigned short)(((unsigned long)limit - 1) & 0xFFFF);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;   // valid if CF=1
}

/*
2.7 - Function 0009h - Set Descriptor Access Rights:
----------------------------------------------------

  Modifies the access rights field in the descriptor for the specified
segment.

In:
  AX     = 0009h
  BX     = selector
  CX     = access rights/type word

Out:
  if successful:
    carry flag clear

  if failed:
    carry flag set
    AX = error code (DOS/32A only)
        8021h - invalid value (access rights/type bytes invalid)
        8022h - invalid selector
        8025h - invalid linear address (changing the access rights/type bytes
                would cause the descriptor to reference a linear address range
                outside that allowed for DPMI clients.) 

Notes:
) The access rights/type word passed to the function in CX has the following
  format:

    Bit: 15  14  13  12  11  10   9   8   7   6   5   4   3   2   1   0
       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
       | G |B/D| 0 | ? |       ?       | 1 |  DPL  | 1 |C/D|E/C|W/R| A |
       +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

    G   - 0=byte granular, 1=page granular
    B/D - 0=default 16bit, 1=default 32bit
    DPL - must be equal to caller's CPL
    C/D - 0=data, 1=code
    E/C - data: 0=expand-up, 1=expand-down
          code: must be 0 (non-conforming)
    W/R - data: 0=read, 1=read/write
          code: must be 1 (readable)
    A   - 0=not accessed, 1=accessed
    0   - must be 0
    1   - must be 1
    ?   - ignored

) Client programs should use the LAR instruction to examine the access rights
  of a descriptor.

) Under DPMI 1.0/VCPI/XMS/raw, any segment register which contains the
  selector specified in register BX will be reloaded. DPMI 0.9 may do this,
  but it is not guaranteed.

) We hope you have enough sense not to try to modify your current CS or SS
  descriptor.
*/

void dpmi_setaccessrights(unsigned long selector, unsigned long rights) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x0009;
    dpmi_regs.w.bx = (unsigned short)selector;
    dpmi_regs.w.cx = (unsigned short)rights;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;   // valid if CF=1
}

// as usual, helper function to get segment access rights :)
unsigned long dpmi_getaccessrights(unsigned long selector) {
    unsigned long rights = 0;
    _asm {
        lar     eax, [selector]
        shr     eax, 8                      // shift out base bits 16-23
        mov     [rights], eax               // *cough*
        
        // save flags
        pushfd
        pop     dword ptr [dpmi_status]
    }
    
    // extract ZF
    dpmi_status = (!dpmi_status & 0x40) >> 6;
    
    return rights;
}

/*
2.8 - Function 000Ah - Create Alias Descriptor:
-----------------------------------------------

  Creates a new data descriptor that has the same base and limit as the
specified descriptor.

In:
  AX     = 000ah
  BX     = selector

Out:
  if successful:
    carry flag clear
    AX	   = data selector (alias)

  if failed:
    carry flag set
    AX = error code (DOS/32A only)
        8011h - descriptor unavailable
        8022h - invalid selector 

Notes:
) The selector supplied to the function may be either a data descriptor or
  a code descriptor. The alias descriptor created is always an expand-up
  writeable data segment.

) The descriptor alias returned by this function will not track changes to the
  original descriptor.
*/

// yay, selfmodifying code! \o/
// (watcom allows it with near pointers 'cuz CS base/limit = DS base/limit :)
unsigned long dpmi_getalias(unsigned long selector) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x000A;
    dpmi_regs.w.bx = (unsigned short)selector;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;   // valid if CF=1
    
    return dpmi_regs.w.ax;
}

/*
2.9 - Function 000Bh - Get Descriptor:
--------------------------------------

  Copies the descriptor table entry for the specified selector into an 8 byte
buffer.

In:
  AX     = 000bh
  BX     = selector
  ES:EDI = selector:offset of 8 byte buffer

Out:
  if successful:
    carry flag clear
    buffer pointed to by ES:EDI contains descriptor

  if failed:
    carry flag set
    AX = error code (DOS/32A only)
        8022h - invalid selector 


*/

void dpmi_getdescriptor(unsigned long selector, _dpmi_descriptor __far *buffer) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax  = 0x000B;
    dpmi_regs.w.bx  = (unsigned short)selector;
    
    dpmi_sregs.es   = FP_SEG(buffer);
    dpmi_regs.x.edi = FP_OFF(buffer);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}

/*
2.10 - Function 000Ch - Set Descriptor:
---------------------------------------

  Copies the contents of an 8 byte buffer into the descriptor for the
specified selector.

In:
  AX     = 000ch
  BX     = selector
  ES:EDI = selector:offset of 8 byte buffer containing descriptor

Out:
  if successful:
    carry flag clear

  if failed:
    carry flag set
    AX = error code (DOS/32A only)
        8021h - invalid value (access rights/type byte invalid)
        8022h - invalid selector
        8025h - invalid linear address (descriptor references a linear address
                range outside that allowed for DPMI clients) 

) The descriptors access rights/type word at offset 5 within the descriptor
  follows the same format and restrictions as the access rights/type parameter
  CX to the Set Descriptor Access Rights function (0009h).

) Under DPMI 1.0/VCPI/XMS/raw, any segment register which contains the
  selector specified in register BX will be reloaded. DPMI 0.9 may do this,
  but it is not guaranteed.

) We hope you have enough sense not to try to modify your current CS or SS
  descriptor or the descriptor of the buffer.
*/

void dpmi_setdescriptor(unsigned long selector, _dpmi_descriptor __far *buffer) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax  = 0x000C;
    dpmi_regs.w.bx  = (unsigned short)selector;
    
    dpmi_sregs.es   = FP_SEG(buffer);
    dpmi_regs.x.edi = FP_OFF(buffer);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}


/*
2.11 - Function 0100h - Allocate DOS Memory Block:
--------------------------------------------------

  Allocates low memory through DOS function 48h and allocates it a descriptor.

In:
  AX     = 0100h
  BX     = paragraphs to allocate

Out:
  if successful:
    carry flag clear
    AX	   = real mode segment address
    DX	   = protected mode selector for memory block

  if failed:
    carry flag set
    AX	   = DOS error code
    BX	   = size of largest available block
*/
void dpmi_getdosmem(int size, _dpmi_ptr *p, int *largestAvailBlock) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x100;
    dpmi_regs.w.bx = (unsigned short)size;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    p->segment = dpmi_regs.w.ax;
    p->selector = dpmi_regs.w.dx;
    
    if (largestAvailBlock != NULL) 
        *largestAvailBlock = (dpmi_regs.x.cflag ? 0 : dpmi_regs.w.bx);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}

// helper routine to get largest available block
int dpmi_dosmemlargestavail() {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x100;
    dpmi_regs.w.bx = -1;            // unrealistic size
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
    
    return dpmi_regs.w.bx;
}

/*
2.12 - Function 0101h - Free DOS Memory Block:
----------------------------------------------

  Frees a low memory block previously allocated by function 0100h.

In:
  AX     = 0101h
  DX     = protected mode selector for memory block

Out:
  if successful:
    carry flag clear

  if failed:
    carry flag set
    AX	   = DOS error code
*/
void dpmi_freedosmem(_dpmi_ptr *p) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x101;
    dpmi_regs.w.dx = p->selector;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}

/*
2.20 - Function 0300h - Simulate Real Mode Interrupt:
-----------------------------------------------------

  Simulates an interrupt in real mode. The function transfers control to the
address specified by the real mode interrupt vector. The real mode handler
must return by executing an IRET.

In:
  AX     = 0300h
  BL     = interrupt number
  BH     = must be 0
  CX     = number of words to copy from the protected mode stack to the real
           mode stack
  ES:EDI = selector:offset of real mode register data structure in the
           following format:

           Offset  Length  Contents
           00h     4       EDI
           04h     4       ESI
           08h     4       EBP
           0ch     4       reserved, ignored
           10h     4       EBX
           14h     4       EDX
           18h     4       ECX
           1ch     4       EAX
           20h     2       CPU status flags
           22h     2       ES
           24h     2       DS
           26h     2       FS
           28h     2       GS
           2ah     2       IP (reserved, ignored)
           2ch     2       CS (reserved, ignored)
           2eh     2       SP
           30h     2       SS

Out:
  if successful:
    carry flag clear
    ES:EDI = selector offset of modified real mode register data structure

  if failed:
    carry flag set

Notes:
) The CS:IP in the real mode register data structure is ignored by this
  function. The appropriate interrupt handler will be called based on the
  value passed in BL.

) If the SS:SP fields in the real mode register data structure are zero, a
  real mode stack will be provided by the host. Otherwise the real mode SS:SP
  will be set to the specified values before the interrupt handler is called.

) The flags specified in the real mode register data structure will be put on
  the real mode interrupt handler's IRET frame. The interrupt handler will be
  called with the interrupt and trace flags clear.

) Values placed in the segment register positions of the data structure must
  be valid for real mode. That is, the values must be paragraph addresses, not
  protected mode selectors.

) The target real mode handler must return with the stack in the same state
  as when it was called. This means that the real mode code may switch stacks
  while it is running, but must return on the same stack that it was called
  on and must return with an IRET.

) When this function returns, the real mode register data structure will
  contain the values that were returned by the real mode interrupt handler.
  The CS:IP and SS:SP values will be unmodified in the data structure.

) It is the caller's responsibility to remove any parameters that were pushed
  on the protected mode stack.
*/
void dpmi_rminterrupt(int int_num, _dpmi_rmregs *regs) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax  = 0x300;
    dpmi_regs.w.bx  = (int_num & 0xFF);
    
    dpmi_sregs.es   = FP_SEG(regs);
    dpmi_regs.x.edi = FP_OFF(regs);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}

/*
2.21 - Function 0301h - Call Real Mode Procedure With Far Return Frame:
-----------------------------------------------------------------------

  Simulates a FAR CALL to a real mode procedure. The called procedure must
return by executing a RETF instruction.

In:
  AX     = 0301h
  BH     = must be 0
  CX     = number of words to copy from the protected mode stack to the real
           mode stack
  ES:EDI = selector:offset of real mode register data structure in the
           following format:

           Offset  Length  Contents
           00h     4       EDI
           04h     4       ESI
           08h     4       EBP
           0ch     4       reserved, ignored
           10h     4       EBX
           14h     4       EDX
           18h     4       ECX
           1ch     4       EAX
           20h     2       CPU status flags
           22h     2       ES
           24h     2       DS
           26h     2       FS
           28h     2       GS
           2ah     2       IP
           2ch     2       CS
           2eh     2       SP
           30h     2       SS

Out:
  if successful:
    carry flag clear
    ES:EDI = selector offset of modified real mode register data structure

  if failed:
    carry flag set

Notes:
) The CS:IP in the real mode register data structure specifies the address of
  the real mode procedure to call.

) If the SS:SP fields in the real mode register data structure are zero, a
  real mode stack will be provided by the host. Otherwise the real mode SS:SP
  will be set to the specified values before the procedure is called.

) Values placed in the segment register positions of the data structure must
  be valid for real mode. That is, the values must be paragraph addresses, not
  protected mode selectors.

) The target real mode procedure must return with the stack in the same state
  as when it was called. This means that the real mode code may switch stacks
  while it is running, but must return on the same stack that it was called
  on and must return with a RETF and should not clear the stack of any
  parameters that were passed to it on the stack.

) When this function returns, the real mode register data structure will
  contain the values that were returned by the real mode procedure. The CS:IP
  and SS:SP values will be unmodified in the data structure.

) It is the caller's responsibility to remove any parameters that were pushed
  on the protected mode stack.
*/

void dpmi_rmcall(_dpmi_rmregs *regs) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax  = 0x301;
    dpmi_regs.w.bx  = 0;
    
    dpmi_sregs.es   = FP_SEG(regs);
    dpmi_regs.x.edi = FP_OFF(regs);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}

void dpmi_rmcall_ex(_dpmi_rmregs *regs, _dpmi_rmpointer ptr) {
    regs->CS = ptr.segment; regs->IP = ptr.offset;
    regs->SS = regs->SP = 0;
    dpmi_rmcall(regs);
}

/*
2.32 - Function 0800h - Physical Address Mapping:
-------------------------------------------------

  Converts a physical address into a linear address. This functions allows the
client to access devices mapped at a specific physical memory address.
Examples of this are the frame buffers of certain video cards in extended
memory.

In:
  AX     = 0800h
  BX:CX  = physical address of memory
  SI:DI  = size of region to map in bytes

Out:
  if successful:
    carry flag clear
    BX:CX  = linear address that can be used to access the physical memory

  if failed:
    carry flag set

Notes:
) It is the caller's responsibility to allocate and initialize a descriptor
  for access to the memory.

) Clients should not use this function to access memory below the 1 MB
  boundary.
*/
void *dpmi_mapphysical(unsigned long size, void *p) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax  = 0x800;
    
    dpmi_regs.w.bx = (unsigned short)(((unsigned long)p) >> 16);
    dpmi_regs.w.cx = (unsigned short)(((unsigned long)p) & 0xFFFF);
    
    dpmi_regs.w.si = (unsigned short)(((unsigned long)size) >> 16);
    dpmi_regs.w.di = (unsigned short)(((unsigned long)size) & 0xFFFF);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
    
    return (void *)(((unsigned long)dpmi_regs.w.bx << 16) | dpmi_regs.w.cx);
}

/*
2.33 - Function 0801h - Free Physical Address Mapping:
------------------------------------------------------

  Releases a mapping of physical to linear addresses that was previously
obtained with function 0800h.

In:
  AX     = 0801h
  BX:CX  = linear address returned by physical address mapping call

Out:
  if successful:
    carry flag clear

  if failed:
    carry flag set

Notes:
) The client should call this function when it is finished using a device
  previously mapped to linear addresses with function 0801h.
*/  
void dpmi_unmapphysical(void *p) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax  = 0x801;
    
    dpmi_regs.w.bx = (unsigned short)(((unsigned long)p) >> 16);
    dpmi_regs.w.cx = (unsigned short)(((unsigned long)p) & 0xFFFF);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}

/*
2.14 - Function 0200h - Get Real Mode Interrupt Vector:
-------------------------------------------------------

  Returns the real mode segment:offset for the specified interrupt vector.

In:
  AX     = 0200h
  BL     = interrupt number

Out:
  always successful:
    carry flag clear
    CX:DX  = segment:offset of real mode interrupt handler

Notes:
) The value returned in CX is a real mode segment address, not a protected
  mode selector.
  
*/

/*
NOTE 1: this function converts realmode seg:ofs pointer to linear address in
        low aka conventional/DOS memory. watcom maps low memory 1:1 so it's
        safe to use result as-is; for other environments (DJGPP) address
        mapping is usually required
*/

void* dpmi_getrmvect(int num) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x200;
    dpmi_regs.h.bl = (unsigned char) num;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
    
    return (void *)(((unsigned long)dpmi_regs.w.cx << 4) + ((unsigned long)dpmi_regs.w.dx));
}

_dpmi_rmpointer dpmi_getrmvect_ex(int num) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x200;
    dpmi_regs.h.bl = (unsigned char) num;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
    
    _dpmi_rmpointer rtn;
    rtn.segment = (unsigned long)dpmi_regs.w.cx;
    rtn.offset = (unsigned long)dpmi_regs.w.dx;
    return rtn;
}

/*
2.15 - Function 0201h - Set Real Mode Interrupt Vector:
-------------------------------------------------------

  Sets the real mode segment:offset for the specified interrupt vector.

In:
  AX     = 0201h
  BL     = interrupt number
  CX:DX  = segment:offset of real mode interrupt handler

Out:
  always successful:
    carry flag clear

Notes:
) The value passed in CX must be a real mode segment address, not a protected
  mode selector. Consequently, the interrupt handler must either reside in
  DOS memory (below the 1M boundary) or the client must allocate a real mode
  callback address.
*/

void dpmi_setrmvect(int num, void  *p) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.x.eax = 0x201;
    dpmi_regs.x.ebx = num;
    
    dpmi_regs.x.ecx = (((unsigned long)p & 0xF0000) >> 4);
    dpmi_regs.x.edx =  ((unsigned long)p & 0xFFFF);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}

void dpmi_setrmvect_ex(int num, _dpmi_rmpointer p) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.x.eax = 0x201;
    dpmi_regs.x.ebx = num;
    
    dpmi_regs.x.ecx = ((unsigned long)p.segment);
    dpmi_regs.x.edx = ((unsigned long)p.offset);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}

/*
2.16 - Function 0202h - Get Processor Exception Handler Vector:
---------------------------------------------------------------

  Returns the address of the current protected mode exception handler for the
specified exception number.

In:
  AX     = 0202h
  BL     = exception number (00h-1fh)

Out:
  if successful:
    carry flag clear
    CX:EDX = selector:offset of exception handler

  if failed:
    carry flag set

Notes:
) PMODE/W handles exceptions under clean/XMS/VCPI environments. Under a DPMI
  environment, exception handling is provided by the DPMI host.

) PMODE/W only traps exceptions 0 through 14. The default behavior is to
  terminate execution and do a debug dump. PMODE/W will terminate on
  exceptions 0, 1, 2, 3, 4, 5, and 7, instead of passing them down to the
  real mode handlers as DPMI specifications state.
*/
void __far * dpmi_getpmexception(int num) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x202;
    dpmi_regs.h.bl = (unsigned char) num;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
    
    return MK_FP(dpmi_regs.w.cx, dpmi_regs.x.edx);
}

/*
2.17 - Function 0203h - Set Processor Exception Handler Vector:
---------------------------------------------------------------

  Sets the address of a handler for a CPU exception or fault, allowing a
protected mode application to intercept processor exceptions.

In:
  AX     = 0203h
  BL     = exception number (00h-1fh)
  CX:EDX = selector:offset of exception handler

Out:
  if successful:
    carry flag clear

  if failed:
    carry flag set

Notes:
) PMODE/W handles exceptions under clean/XMS/VCPI environments. Under a DPMI
  environment, exception handling is provided by the DPMI host.

) PMODE/W only traps exceptions 0 through 14. The default behavior is to
  terminate execution and do a debug dump. PMODE/W will terminate on
  exceptions 0, 1, 2, 3, 4, 5, and 7, instead of passing them down to the
  real mode handlers as DPMI specifications state.

) If you wish to hook one of the low 8 interrupts, you must hook it as an
  exception. It will not be called if you hook it with function 0205h.
*/
void dpmi_setpmexception(int num, void __far *proc) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x205;
    dpmi_regs.h.bl = (unsigned char)num;
    
    dpmi_regs.w.cx  = FP_SEG(proc);
    dpmi_regs.x.edx = FP_OFF(proc);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}


/*
2.18 - Function 0204h - Get Protected Mode Interrupt Vector:
------------------------------------------------------------

  Returns the address of the current protected mode interrupt handler for the
specified interrupt.

In:
  AX     = 0204h
  BL     = interrupt number

Out:
  always successful:
    carry flag clear
    CX:EDX = selector:offset of protected mode interrupt handler

Notes:
) The value returned in CX is a valid protected mode selector, not a real mode
  segment address.
*/
void __far * dpmi_getpmvect(int num) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x204;
    dpmi_regs.h.bl = (unsigned char) num;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
    
    return MK_FP(dpmi_regs.w.cx, dpmi_regs.x.edx);
}

/*
2.19 - Function 0205h - Set Protected Mode Interrupt Vector:
------------------------------------------------------------

  Sets the address of the protected mode interrupt handler for the specified
interrupt.

In:
  AX     = 0205h
  BL     = interrupt number
  CX:EDX = selector offset of protected mode interrupt handler

Out:
  if successful:
    carry flag clear

  if failed:
    carry flag set

Notes:
) The value passed in CX must be a valid protected mode selector, not a real
  mode segment address.

) If you wish to hook one of the low 8 interrupts, you must hook it as an
  exception. It will not be called if you hook it with function 0205h.
*/
void dpmi_setpmvect(int num, void __far *proc) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x205;
    dpmi_regs.h.bl = (unsigned char) num;
    
    dpmi_regs.w.cx  = FP_SEG(proc);
    dpmi_regs.x.edx = FP_OFF(proc);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}


/*
2.23 - Function 0303h - Allocate Real Mode Callback Address:
------------------------------------------------------------

  Returns a unique real mode segment:offset, known as a "real mode callback",
that will transfer control from real mode to a protected mode procedure.
Callback addresses obtained with this function can be passed by a protected
mode program to a real mode application, interrupt handler, device driver,
TSR, etc... so that the real mode program can call procedures within the
protected mode program.

In:
  AX     = 0303h
  DS:ESI = selector:offset of protected mode procedure to call
  ES:EDI = selector:offset of 32h byte buffer for real mode register data
           structure to be used when calling the callback routine.

Out:
  if successful:
    carry flag clear
    CX:DX  = segment:offset of real mode callback

  if failed:
    carry flag set

Notes:
) A descriptor may be allocated for each callback to hold the real mode SS
  descriptor. Real mode callbacks are a limited system resource. A client
  should release a callback that it is no longer using.
*/
void* dpmi_getcallback(void __far *proc, _dpmi_rmregs __far *rmregs) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x303;
    
    dpmi_sregs.ds   = FP_SEG(proc);
    dpmi_regs.x.esi = FP_OFF(proc);
    
    dpmi_sregs.es   = FP_SEG(rmregs);
    dpmi_regs.x.edi = FP_OFF(rmregs);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
    
    // assuming DOS4GW maps low memory 1:1
    return (void *)(((unsigned long)dpmi_regs.w.cx << 4) + ((unsigned long)dpmi_regs.w.dx));
}

_dpmi_rmpointer dpmi_getcallback_ex(void __far *proc, _dpmi_rmregs __far *rmregs) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x303;
    
    dpmi_sregs.ds   = FP_SEG(proc);
    dpmi_regs.x.esi = FP_OFF(proc);
    
    dpmi_sregs.es   = FP_SEG(rmregs);
    dpmi_regs.x.edi = FP_OFF(rmregs);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
    
    _dpmi_rmpointer rtn;
    rtn.segment = (unsigned long)dpmi_regs.w.cx;
    rtn.offset = (unsigned long)dpmi_regs.w.dx;
    return rtn;
}

/*
2.24 - Function 0304h - Free Real Mode Callback Address:
--------------------------------------------------------

  Releases a real mode callback address that was previously allocated with the
Allocate Real Mode Callback Address function (0303h).

In:
  AX     = 0304h
  CX:DX  = segment:offset of real mode callback to be freed

Out:
  if successful:
    carry flag clear

  if failed:
    carry flag set

Notes:
) Real mode callbacks are a limited system resource. A client should release
  any callback that it is no longer using.
*/
void dpmi_freecallback(void *p) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x304;
    
    dpmi_regs.w.cx = (unsigned short)(((unsigned long)p & 0xF0000) >> 4);
    dpmi_regs.w.dx = (unsigned short) ((unsigned long)p & 0xFFFF);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
    
}

void dpmi_freecallback_ex(_dpmi_rmpointer p) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x304;
    
    dpmi_regs.w.cx = (unsigned short)(((unsigned long)p.segment) >> 4);
    dpmi_regs.w.dx = (unsigned short) ((unsigned long)p.offset);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
    
}

/*
2.27 - Function 0400h - Get Version:
------------------------------------

  Returns the version of the DPMI Specification implemented by the DPMI host.
The client can use this information to determine what functions are available.

In:
  AX     = 0400h

Out:
  always successful:
    carry flag clear
    AH      = DPMI major version as a binary number (VCPI/XMS/raw returns 00h)
    AL      = DPMI minor version as a binary number (VCPI/XMS/raw returns 5ah)
    BX	    = flags:
	    Bits    Significance
        0       0 = host is 16bit (PMODE/W never runs under one of these)
                1 = host is 32bit
	    1	    0 = CPU returned to V86 mode for reflected interrupts
                1 = CPU returned to real mode for reflected interrupts
	    2	    0 = virtual memory not supported
                1 = virtual memory supported
	    3-15    reserved
    CL	    = processor type:
	    03h = 80386
        04h = 80486
        05h = 80586 (Pentium)
        06h = 80686 (Pentium Pro or Pentium II)
        07h-FFh = reserved for future Intel processors
    DH	   = current value of master PIC base interrupt (low 8 IRQs)
    DL	   = current value of slave PIC base interrupt (high 8 IRQs)

Notes:
) The major and minor version numbers are binary, not BCD. So a DPMI 0.9
  implementation would return AH as 0 and AL as 5ah (90).
*/

void dpmi_getinfo(_dpmi_info *p) {
    if (p == NULL) return;
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x0400;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = 0;                // CF = 0 always
    
    p->version      = (unsigned long)dpmi_regs.w.ax;
    p->flags        = (unsigned long)dpmi_regs.w.bx;
    p->cpu          = (unsigned long)dpmi_regs.h.cl;
    p->highpicbase  = (unsigned long)dpmi_regs.h.dl;
    p->lowpicbase   = (unsigned long)dpmi_regs.h.dh;
}

/*
2.28 - Function 0500h - Get Free Memory Information:
----------------------------------------------------

  Returns information about the amount of available memory. Since DPMI clients
could be running in a multitasking environment, the information returned by
this function should be considered advisory.

In:
  AX     = 0500h
  ES:EDI = selector:offset of 48 byte buffer

Out:
  if successful:
    carry flag clear
    buffer is filled with the following information:

      ofs       description
      00h       Largest available free block in
                bytes
      04h       Maximum unlocked page allocation
      08h       Maximum locked page allocation
      0Ch       Linear addr space size in pages
      10h       Total number of unlocked pages
      14h       Number of free pages
      18h       Total number of physical pages
      1Ch       Free linear address space in pages
      20h       Size of paging file/partition in
                pages
      24h-2Fh   Reserved

  if failed:
    carry flag set

Notes:
) Only the first field of the structure is guaranteed to contain a valid
  value. Any fields that are not supported by the host will be set to -1
  (0ffffffffh) to indicate that the information is not available.
*/

void dpmi_getmeminfo(_dpmi_mem_info __far *p) {
    if (p == NULL) return;
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x500;
    
    dpmi_sregs.es   = FP_SEG(p);
    dpmi_regs.x.edi = FP_OFF(p);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}

/*
2.29 - Function 0501h - Allocate Memory Block:
----------------------------------------------

  Allocates a block of extended memory.

In:
  AX     = 0501h
  BX:CX  = size of block in bytes (must be non-zero)

Out:
  if successful:
    carry flag clear
    BX:CX  = linear address of allocated memory block
    SI:DI  = memory block handle (used to resize and free block)

  if failed:
    carry flag set

Notes:
) The allocated block is guaranteed to have at least dword alignment.

) This function does not allocate any descriptors for the memory block. It is
  the responsibility of the client to allocate and initialize any descriptors
  needed to access the memory with additional function calls.

*/

void dpmi_getmem(unsigned long size, _dpmi_memory_block *p) {
    if (p == NULL) return;
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x501;
    
    dpmi_regs.w.bx = (unsigned short)(size >> 16);
    dpmi_regs.w.cx = (unsigned short)(size & 0xFFFF);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    p->handle    = (unsigned long)(((unsigned long)dpmi_regs.w.si << 16) | dpmi_regs.w.di);
    p->linearPtr = (void *)       (((unsigned long)dpmi_regs.w.bx << 16) | dpmi_regs.w.cx);

    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}

/*
2.30 - Function 0502h - Free Memory Block:
------------------------------------------

  Frees a memory block previously allocated with the Allocate Memory Block
function (0501h).

In:
  AX     = 0502h
  SI:DI  = memory block handle

Out:
  if successful:
    carry flag clear

  if failed:
    carry flag set

Notes:
) No descriptors are freed by this call. It is the client's responsibility to
  free any descriptors that it previously allocated to 
*/

void dpmi_freemem(_dpmi_memory_block *p) {
    if (p == NULL) return;
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0x501;
    
    dpmi_regs.w.si = (unsigned short)(p->handle >> 16);
    dpmi_regs.w.di = (unsigned short)(p->handle & 0xFFFF);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);

    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}

/*

2.37 - Function EEFFh - Get DOS Extender Information:
-----------------------------------------------------

  Returns information about the DOS extender.

In:
  AX     = EEFFh

Out:
  if successful:
    carry flag clear
    EAX    = 'PMDW' (504D4457h)
    ES:EBX = selector:offset of ASCIIZ copyright string
    CH     = protected mode system type (0=raw, 1=XMS, 2=VCPI, 3=DPMI)
    CL     = processor type (3=386, 4=486, 5=586)
    DH     = extender MAJOR version (binary)
    DL     = extender MINOR version (binary)

  if failed:
    carry flag set

Notes:
) In PMODE/W's implementation of this function, the value returned in ES is
  equivalent to the 4G data selector returned in DS at startup.

) This function is always successful under PMODE/W.

---- alternative description from DOS32/A docs :

2.51 - DPMI function 0EEFFh - Get DOS Extender Info

Returns information about the DOS Extender.
 

In: 	AX = 0EEFFh
Out: 	

    if successful:
        CF clear
        EAX = "D32A" (44333241h)
        CL = CPU type:

            03h = 80386
            04h = 80486
            05h = 80586 (Pentium)
            06h = 80686 (Pentium Pro)
            07h-0FFh = reserved

        CH = System software type:

            00h = Clean
            01h = XMS
            02h = VCPI
            03h = DPMI

        DL = DOS Extender minor version (binary)
        DH = DOS Extender major version (binary)
        ES:EBX = selector:offset of ASCIIZ copyright string

    if failed:
        CF set
        AX = error code
            8001h - unsupported function 

Notes:
    a) This function is not a part of DPMI 0.9 specification and is invented
       for compatibility with PMODE/W DOS Extender only.
    b) Under DOS/32 Advanced the pointer ES:EBX will point to an empty string (to a byte 00h).

*/


void dpmi_getextenderinfo(_dpmi_extender_info *p) {
    if (p == NULL) return;
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax = 0xEEFF;
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    if (!dpmi_regs.x.cflag ) {
        p->signature    = (unsigned long)dpmi_regs.x.eax;
        p->cpu          = (unsigned long)dpmi_regs.h.cl;
        p->systemType   = (unsigned long)dpmi_regs.h.ch;
        p->version      = (unsigned long)dpmi_regs.w.dx;
        p->copyright    = (char _far *)MK_FP(dpmi_sregs.es, dpmi_regs.x.ebx);
    }
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}



/*

(sparsely documented):
DPMI(not) function 0A00h - Get Vendor-Specific API Entrypoint

In:
    AX = 0A00h
    DS:ESI = pointer to vendor-specific identification ASCIIZ string:
        "RATIONAL DOS/4G" for DOS4G(W),
        "SUNSYS DOS/32A"  for DOS32/A,
        ....
        
Out: 
    if successful:
        CF clear
        ES:EDI = vendor API entrypoint
        DS, FS, GS, EAX, EBX, ECX, EDX, ESI, and EBP may be modified
        
    if function unsuccessful
        CF set
        
oh wait we have RBIL:

    INT 2F - DPMI 0.9+ - GET VENDOR-SPECIFIC API ENTRY POINT
            AX = 168Ah
            DS:(E)SI = selector:offset of ASCIZ vendor name (see #02719)
    Return: AL = status
            00h successful
            ES:(E)DI -> extended API entry point
            8Ah unsuccessful
    Notes:	the vendor name is used to determine which entry point to return; it is
            case-sensitive
            available in protected mode only
            32-bit applications use ESI and EDI, 16-bit applications use SI and DI
            this call is present but not documented for DPMI 0.9
            the Borland C++ 3.1 DPMILOAD does not handle requests for entry points
            other than the MS-DOS one gracefully, producing an unhandled
            exception report; this has been fixed in the Borland Pascal 7 version
    SeeAlso: AX=1687h,INT 31/AX=0A00h,INT 31/AH=57h
    
    (Table 02719)
    Values for DPMI vendor-specific API names:
        "MS-DOS"            MS Windows and 386MAX v6.00+ (see #02720)
        "386MAX"            386MAX v6.00+
        "HELIX_DPMI"        Helix Netroom's DPMI server
        "Phar Lap"          Phar Lap 286|DOS-Extender RUN286 (see #02721)
        "RATIONAL DOS/4G"   DOS/4G, DOS/4GW
        "VIRTUAL SUPPORT"   Borland 32RTM

*/

void _far *dpmi_getvendorapi(char _far *id) {
    dpmi_zero_reg_structs();
     
    void _far *rtn = NULL;
    
    // first try INT31/AX=0A00
    dpmi_regs.w.ax  = 0xA00;
    dpmi_sregs.ds   = FP_SEG(id);
    dpmi_regs.x.esi = FP_OFF(id);
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    if (!dpmi_regs.x.cflag) {
        rtn = MK_FP(dpmi_sregs.es, dpmi_regs.x.edi);
        dpmi_status = dpmi_regs.x.cflag;
        dpmi_returncode = dpmi_regs.w.ax;
        return rtn;
        
    } else {
        // if failed, try INT2F/AX=168A
        dpmi_zero_reg_structs();
        
        dpmi_regs.w.ax  = 0x168A;
        dpmi_sregs.ds   = FP_SEG(id);
        dpmi_regs.x.esi = FP_OFF(id);
        int386x(0x2F, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
        
        if (dpmi_regs.h.al != 0) {
            // failure
            dpmi_status = 1;
            dpmi_returncode = dpmi_regs.w.ax;
            return NULL;
        } else {
            rtn = MK_FP(dpmi_sregs.es, dpmi_regs.x.edi);
            dpmi_status = 0;
            dpmi_returncode = dpmi_regs.w.ax;
            return rtn;
        }
    }
    
    return rtn;
}

// these are not directly supported by PMODE/W (its DPMI server doesn't use paging)
// but other servers (DOS4GW, QDPMI, mustdie9x DOSbox) support 'em
// (and actually require using 'em when you're doing fancy ISR stuff and willing
// to save you mental health from sleepless hours of debugging to find out your 
// ISR code swapped out by host VMM, taking your CPU to abyss trip; or, rather,
// firing a page fault making your debug ritual a *little* easier :)

/*
DPMI function 0600h - Lock Linear Region

Locks the specified linear address range.

In:
    AX = 0600h
    BX:CX = starting linear address of memory to lock
    SI:DI = size of region to lock (bytes)
Out: 	

    if successful:
        CF clear

    if function unsuccessful
        CF set
        AX = error code (DOS/32A only)

            8013h - physical memory unavailable
            8017h - lock count exceeded
            8025h - invalid linear address (unallocated pages) 


*/

// as you see, it accepts LINEAR address, not a far pointer! (again fine under watcom)

void dpmi_lockmemory(void *p, unsigned long size) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax  = 0x600;
    
    dpmi_regs.w.bx = (unsigned short)(((unsigned long)p) >> 16);
    dpmi_regs.w.cx = (unsigned short)(((unsigned long)p) & 0xFFFF);
    
    dpmi_regs.w.si = (unsigned short)(((unsigned long)size) >> 16);
    dpmi_regs.w.di = (unsigned short)(((unsigned long)size) & 0xFFFF);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}

/*
DPMI function 0601h - Unlock Linear Region

Unlocks a linear address range that was previously locked using the Lock Linear Region DPMI function 0600h.

In:
    AX = 0601h
    BX:CX = starting linear address of memory to unlock
    SI:DI = size of region to unlock (bytes)
Out: 	

    if successful:
    CF clear

    if failed:
    CF set
    AX = error code

        8002h - invalid state (page not locked)
        8025h - invalid linear address (unallocated pages) 
*/

void dpmi_unlockmemory(void *p, unsigned long size) {
    dpmi_zero_reg_structs();
    
    dpmi_regs.w.ax  = 0x601;
    
    dpmi_regs.w.bx = (unsigned short)(((unsigned long)p) >> 16);
    dpmi_regs.w.cx = (unsigned short)(((unsigned long)p) & 0xFFFF);
    
    dpmi_regs.w.si = (unsigned short)(((unsigned long)size) >> 16);
    dpmi_regs.w.di = (unsigned short)(((unsigned long)size) & 0xFFFF);
    
    int386x(0x31, &dpmi_regs, &dpmi_regs, &dpmi_sregs);
    
    dpmi_status = dpmi_regs.x.cflag;
    dpmi_returncode = dpmi_regs.w.ax;
}

// dpmi realmode interrupt caller using REGS\SREGS structures
// WARNING: be careful with pointing to memory structures because they MUST
// be located in DOS memory area, not in extendend memory!
// (use dpmi_getdosmem() and dpmi_freedosmem())

int rmint386(int intnum, union REGS *in, union REGS *out) {
    _dpmi_rmregs rmregs;
    union REGS   inregs;
    
    memset(&rmregs, 0, sizeof(rmregs));
    memcpy(&inregs, in, sizeof(union REGS));
    
    // translate regs from REGS to rmregs
    rmregs.EAX = inregs.x.eax;
    rmregs.EBX = inregs.x.ebx;
    rmregs.ECX = inregs.x.ecx;
    rmregs.EDX = inregs.x.edx;
    rmregs.ESI = inregs.x.esi;
    rmregs.EDI = inregs.x.edi;
    // translate regs from SREGS to rmregs
    rmregs.ES  = rmregs.DS = rmregs.FS = rmregs.GS = 0;
    // call dmpi_rminterrupt()
    dpmi_rminterrupt(intnum, &rmregs);
    // translate regs back
    inregs.x.eax = rmregs.EAX;
    inregs.x.ebx = rmregs.EBX;
    inregs.x.ecx = rmregs.ECX;
    inregs.x.edx = rmregs.EDX;
    inregs.x.esi = rmregs.ESI;
    inregs.x.edi = rmregs.EDI;
    
    memcpy(out, &inregs, sizeof(union REGS));
    
    return rmregs.EAX;
}

int rmint386x(int intnum, union REGS *in, union REGS *out, struct SREGS *seg) {
    _dpmi_rmregs rmregs;
    union REGS   inregs;
    struct SREGS sregs;
    
    memset(&rmregs, 0, sizeof(rmregs));
    memcpy(&inregs, in, sizeof(union REGS));
    memcpy(&sregs, seg, sizeof(struct SREGS));
    
    // translate regs from REGS to rmregs
    rmregs.EAX = inregs.x.eax;
    rmregs.EBX = inregs.x.ebx;
    rmregs.ECX = inregs.x.ecx;
    rmregs.EDX = inregs.x.edx;
    rmregs.ESI = inregs.x.esi;
    rmregs.EDI = inregs.x.edi;
    // translate regs from SREGS to rmregs
    rmregs.ES  = sregs.es;
    rmregs.DS  = sregs.ds;
    rmregs.FS  = sregs.fs;
    rmregs.GS  = sregs.gs;
    // call dmpi_rminterrupt()
    dpmi_rminterrupt(intnum, &rmregs);
    // translate regs back
    sregs.es  = rmregs.ES;
    sregs.ds  = rmregs.DS;
    sregs.fs  = rmregs.FS;
    sregs.gs  = rmregs.GS;
    inregs.x.eax = rmregs.EAX;
    inregs.x.ebx = rmregs.EBX;
    inregs.x.ecx = rmregs.ECX;
    inregs.x.edx = rmregs.EDX;
    inregs.x.esi = rmregs.ESI;
    inregs.x.edi = rmregs.EDI;
    
    memcpy(out, &inregs, sizeof(union REGS));
    memcpy(seg, &sregs,  sizeof(struct SREGS));
    
    return rmregs.EAX;
}

void rmintr(int intnum, union REGPACK *r) {
    _dpmi_rmregs  rmregs;
    union REGPACK regs;

    memset(&rmregs, 0, sizeof(rmregs));
    memcpy(&regs, r, sizeof(union REGPACK));
    
    // translate regs from REGS to rmregs
    rmregs.EAX = regs.x.eax;
    rmregs.EBX = regs.x.ebx;
    rmregs.ECX = regs.x.ecx;
    rmregs.EDX = regs.x.edx;
    rmregs.ESI = regs.x.esi;
    rmregs.EDI = regs.x.edi;
    rmregs.EBP = regs.x.ebp;
    // translate regs from SREGS to rmregs
    rmregs.ES  = regs.x.es;
    rmregs.DS  = regs.x.ds;
    rmregs.FS  = regs.x.fs;
    rmregs.GS  = regs.x.gs;
    // call dmpi_rminterrupt()
    dpmi_rminterrupt(intnum, &rmregs);
    // translate regs back
    regs.x.es  = rmregs.ES;
    regs.x.ds  = rmregs.DS;
    regs.x.fs  = rmregs.FS;
    regs.x.gs  = rmregs.GS;
    regs.x.eax = rmregs.EAX;
    regs.x.ebx = rmregs.EBX;
    regs.x.ecx = rmregs.ECX;
    regs.x.edx = rmregs.EDX;
    regs.x.esi = rmregs.ESI;
    regs.x.edi = rmregs.EDI;
    regs.x.ebp = rmregs.EBP;
    
    memcpy(r, &regs, sizeof(union REGPACK));
}

#ifdef _DPMI_VENDOR_API

/*
    danger zone
*/

/*
    DOS4G(W) vendor API functions
*/

/*
    Get package function entry.

IN:
    EAX = 000000000h
    EDX = address of function name
    EBX = address of package name

OUT:
    EAX = 16:16 far ptr to entry point
*/

unsigned long dos4g_get_function(void _far (*apientry)(), char* package, char* function) {
    if ((apientry == NULL) || (package == NULL) || (function == NULL) ) return 0;
    static void _far (*apiLocal)() = apientry;
    
    unsigned long  rtn;
    
    _asm {
        pusha
        
        xor     eax, eax
        mov     ebx, [package]
        mov     edx, [function]
        call    fword ptr [apiLocal]
        
        mov     [rtn], eax
        
        popa
    }
    
    return rtn;
}


/*
    Call 16-bit function.
    
IN:
    EAX = 000000002h
    ECX = number of dwords in the stack (only for DOS/4G 2.00 and later,
          older versions of DOS/4G(W) ignore this value)
    EDX = 16:16 far ptr to function
    EBX = address of stack parameters

OUT:
    EAX = function return value in DX:AX
*/
unsigned long cdecl dos4g_call_function(void _far (*apientry)(), unsigned long function, void* parms, unsigned long parmSize) {
    if ((apientry == NULL) || (function == NULL)) return 0;
    static void _far (*apiLocal)() = apientry;
    
    unsigned long  rtn;
    
    _asm {
        pusha
        
        mov     eax, 2
        mov     ebx, [parms]
        mov     ecx, [parmSize]
        mov     edx, [function]
        call    fword ptr [apiLocal]
        
        mov     [rtn], eax
        
        popa
    }
    
    return rtn;
}


/*
    DOS32/A vendor API functions
*/

/*
2.53 - API function 00h - Get Access to GDT and IDT

In: 	AL = 00h
Out: 	BX = writeable data selector
        ECX = GDT limit
        EDX = IDT limit
        ESI = pointer to GDT
        EDI = pointer to IDT

Notes:
    a) Selector returned in BX is a writeable data selector that should be
       used to access needed variables/addresses.

*/

void dos32a_get_gdt_idt(void _far (*apientry)(), _dos32a_gdt_idt_info *p) {
    unsigned long dataSel, gdt, idt, gdtLimit, idtLimit;
    
    if ((apientry == NULL) || (p == NULL)) return;
    static void _far (*apiLocal)() = apientry;
    
    _asm {
        pusha
        
        xor     eax, eax
        call    fword ptr [apiLocal]
        
        mov     word ptr [dataSel],   bx
        mov     word ptr [dataSel+2], 0
        
        mov     [gdt],      esi
        mov     [idt],      edi
        
        mov     [gdtLimit], ecx
        mov     [idtLimit], edx
        
        popa
    }
    
    p->gdt      = MK_FP(dataSel, gdt);
    p->idt      = MK_FP(dataSel, idt);
    p->gdtLimit = gdtLimit;
    p->idtLimit = idtLimit;
    
    return;
}

/*
2.59 - API function 06h - Get DOS/32 Advanced DPMI Kernel Selectors

In:     AL = 06h
Out:    BX = DOS/32 Advanced DPMI Kernel CODE selector
        CX = DOS/32 Advanced DPMI Kernel DATA selector
        DX = DOS/32 Advanced DPMI Kernel ZERO selector
        SI = DOS/32 Advanced DPMI Kernel CODE segment (real-mode)
        DI = DOS/32 Advanced DOS Extender Client CODE selector

Notes:
    a) Selectors returned by this function should NOT be modified or freed by the user.

*/

void dos32a_get_kernel_selectors(void _far (*apientry)(), _dos32a_kernel_selectors *p) {
    
    if ((apientry == NULL) || (p == NULL)) return;
    static void _far (*apiLocal)() = apientry;
    
    _asm {
        pusha
        
        mov     eax, 0x6
        call    fword ptr [apiLocal]
        
        mov     eax, [p]
        mov     [eax + 0], bx
        mov     [eax + 2], cx
        mov     [eax + 4], dx
        mov     [eax + 6], si
        mov     [eax + 8], di
        
        popa
    }
    
    return;
}

/*
2.62 - API function 09h - Get Access to Performance Counters

In: 	AL = 09h
Out: 	CX:EDX = selector:offset pointer to the base address of Performance Counters

Notes:

    a) Selector returned by this function should NOT be modified or freed by the user.
    b) Performance Counters are 32-bit wide counters (located inside DOS/32A DPMI Kernel)
       that count the number of mode switches performed by the DOS Extender. These
       counters can be read from or written to at any time by the application.
    c) The structure of the Performance Counters is the following (relative to EDX as
       a base address):

        00h - INT RM -> PM (up)
        04h - INT PM -> RM (down)
        08h - IRQ RM -> PM (up)
        0Ch - IRQ PM -> RM (down)
        10h - IRQ Callback RM -> PM (up)
        14h - IRQ Callback PM -> RM (down)
        18h - Callback RM -> PM (up)
        1Ch - Callback PM -> RM (down)

        Where "INT" is a Software Interrupt, "IRQ" is a Hardware Interrupt, "IRQ Callback"
        is a Hardware Interrupt Callback (valid for IRQ 0-15) and "PM" and "RM" stand for
        Protected and Real Modes respective.

    d) Note that the counters will increment each time a (respective) mode switch occurs,
       and cannot be setup to count specific Interrupts/IRQs/Callbacks (ie INT 21h, etc).

*/

_dos32a_performance_counters _far *dos32a_get_performance_counters(void _far (*apientry)()) {
    if (apientry == NULL) return NULL;
    static void _far (*apiLocal)() = apientry;
    unsigned short sel; unsigned long ofs;
    
    _asm {
        pusha
        
        mov     eax, 0x9
        call    fword ptr [apiLocal]
        
        mov     [sel], cx
        mov     [ofs], edx
        
        popa
    }
    
    return (_dos32a_performance_counters _far *)MK_FP(sel, ofs);
}

#endif

// DJGPP compatibility stuff
#ifdef _DPMI_DJGPP_COMPATIBILITY

void __dpmi_yield(void) {
    // TODO: call INT2F/AX=1680
}

int	__dpmi_allocate_ldt_descriptors(int _count) {
    int rtn = dpmi_getdescriptors(_count);
    return (dpmi_status ? -1 : rtn);
}

int	__dpmi_free_ldt_descriptor(int _descriptor) {
    dpmi_freedescriptor(_descriptor);
    return (dpmi_status ? -1 : 0);
}

int	__dpmi_segment_to_descriptor(int _seg) {
    int rtn = dpmi_segtodesc(_seg);
    return (dpmi_status ? -1 : rtn);
}
int	__dpmi_get_selector_increment_value(void) {
    int rtn = dpmi_selectorincrement();
    return (dpmi_status ? -1 : rtn);
}

int	__dpmi_get_segment_base_address(int _selector, unsigned long *_addr)
{
    void* rtn = dpmi_getsegmentbase(_selector);     // NULL IS VAILD under Watcom!
    
    // test for error
    if ((_addr == NULL) || (dpmi_status)) return 1;
    
    // if everything is fine - fill data
    _addr = (unsigned long*)rtn;
    return 0;
}
int	__dpmi_set_segment_base_address(int _selector, unsigned long _address) {
    dpmi_setsegmentbase(_selector, (void*)_address);
    return (dpmi_status ? -1 : 0);
}
unsigned long __dpmi_get_segment_limit(int _selector) {
    return dpmi_getsegmentlimit(_selector);
}

int	__dpmi_set_segment_limit(int _selector, unsigned long _limit) {
    dpmi_setsegmentlimit(_selector, _limit);
    return (dpmi_status ? -1 : 0);
}

int	__dpmi_get_descriptor_access_rights(int _selector) {
    return dpmi_getaccessrights(_selector);
}

int	__dpmi_set_descriptor_access_rights(int _selector, int _rights) {
    dpmi_setsegmentlimit(_selector, _rights);
    return (dpmi_status ? -1 : 0);
}

int	__dpmi_create_alias_descriptor(int _selector) {
    dpmi_getalias(_selector);
    return (dpmi_status ? -1 : 0);
}

int	__dpmi_get_descriptor(int _selector, void *_buffer) {
    // TODO: convert to far pointer on the fly
    _dpmi_descriptor temp;
    dpmi_getdescriptor(_selector, &temp); if (dpmi_status) return -1;
    memcpy(_buffer, &temp, sizeof(temp));
    return 0;
}

int	__dpmi_set_descriptor(int _selector, void *_buffer) {
    // TODO: convert to far pointer on the fly
    _dpmi_descriptor temp;
    memcpy(&temp, _buffer, sizeof(temp));
    dpmi_setdescriptor(_selector, &temp);
    return (dpmi_status ? -1 : 0);
}

int	__dpmi_allocate_specific_ldt_descriptor(int _selector) {
    return -1;      // stub
}
int	__dpmi_get_multiple_descriptors(int _count, void *_buffer) {
    return -1;      // stub
}
int	__dpmi_set_multiple_descriptors(int _count, void *_buffer) {
    return -1;      // stub
}

int	__dpmi_allocate_dos_memory(int _paragraphs, int *_ret_selector_or_max) {
    _dpmi_ptr temp;
    dpmi_getdosmem(_paragraphs, &temp, _ret_selector_or_max);
    if (dpmi_status) return -1; else {
        *_ret_selector_or_max = temp.selector;
        return temp.segment;
    }
}

int	__dpmi_free_dos_memory(int _selector) {
    _dpmi_ptr temp; temp.selector = _selector;
    dpmi_freedosmem(&temp);
    return (dpmi_status ? -1 : 0);
}

int	__dpmi_resize_dos_memory(int _selector, int _newpara, int *_ret_max) {
    return -1;      // stub
}

int	__dpmi_get_real_mode_interrupt_vector(int _vector, __dpmi_raddr *_address) {
    _dpmi_rmpointer rm = dpmi_getrmvect_ex(_vector);
    if ((dpmi_status) || (_address == NULL)) return -1;
    _address->offset16 = rm.offset;
    _address->segment  = rm.segment;
    return 0;
}

int	__dpmi_set_real_mode_interrupt_vector(int _vector, __dpmi_raddr *_address) {
    _dpmi_rmpointer rm;
    if (_address == NULL) return -1;
    rm.offset  = _address->offset16;
    rm.segment = _address->segment;
    
    dpmi_setrmvect_ex(_vector, rm);
    return (dpmi_status ? -1 : 0);
}

int	__dpmi_get_processor_exception_handler_vector(int _vector, __dpmi_paddr *_address) {
    void __far *addr = dpmi_getpmexception(_vector);
    
    if ((dpmi_status) || (_address == NULL)) return -1;
    _address->offset32 = FP_OFF(addr);
    _address->selector = FP_SEG(addr);
    return 0;
}

int	__dpmi_set_processor_exception_handler_vector(int _vector, __dpmi_paddr *_address) {
    if (_address == NULL) return -1;
    
    dpmi_setpmexception(_vector, MK_FP(_address->selector, _address->offset32));
    return (dpmi_status ? -1 : 0);
}

int	__dpmi_get_protected_mode_interrupt_vector(int _vector, __dpmi_paddr *_address) {
    void __far *addr = dpmi_getpmvect(_vector);
    
    if ((dpmi_status) || (_address == NULL)) return -1;
    _address->offset32 = FP_OFF(addr);
    _address->selector = FP_SEG(addr);
    return 0;
}

int	__dpmi_set_protected_mode_interrupt_vector(int _vector, __dpmi_paddr *_address) {
    if (_address == NULL) return -1;
    
    dpmi_setpmvect(_vector, MK_FP(_address->selector, _address->offset32));
    return (dpmi_status ? -1 : 0);
}

int	__dpmi_get_extended_exception_handler_vector_pm(int _vector, __dpmi_paddr *_address) {
    return -1;      // stub
}
int	__dpmi_get_extended_exception_handler_vector_rm(int _vector, __dpmi_paddr *_address) {
    return -1;      // stub
}
int	__dpmi_set_extended_exception_handler_vector_pm(int _vector, __dpmi_paddr *_address) {
    return -1;      // stub
}
int	__dpmi_set_extended_exception_handler_vector_rm(int _vector, __dpmi_paddr *_address) {
    return -1;      // stub
}

int	__dpmi_simulate_real_mode_interrupt(int _vector, __dpmi_regs *_regs) {
    dpmi_rminterrupt(_vector, (_dpmi_rmregs*)_regs);
    return (dpmi_status ? -1 : 0);
}

int	__dpmi_int(int _vector, __dpmi_regs *_regs) {
    _dpmi_rmregs regs;
    memcpy(&regs, _regs, sizeof(regs));
    
    regs.SS    = __dpmi_int_ss;
    regs.SS    = __dpmi_int_sp;
    regs.flags = __dpmi_int_flags;
    
    dpmi_rminterrupt(_vector, &regs);
    return (dpmi_status ? -1 : 0);
}

// if you are REALLY want to use this....
short __dpmi_int_ss, __dpmi_int_sp, __dpmi_int_flags; /* default to zero */

int	__dpmi_simulate_real_mode_procedure_retf(__dpmi_regs *_regs) {
    return -1;      // stub
}
int	__dpmi_simulate_real_mode_procedure_retf_stack(__dpmi_regs *_regs, int stack_words_to_copy, const void *stack_data) {
    return -1;      // stub
}

int	__dpmi_simulate_real_mode_procedure_iret(__dpmi_regs *_regs) {
    return -1;      // stub
}

int	__dpmi_allocate_real_mode_callback(void (*_handler)(void), __dpmi_regs *_regs, __dpmi_raddr *_ret) {
    return -1;      // stub
}
int	__dpmi_free_real_mode_callback(__dpmi_raddr *_addr) {
    return -1;      // stub
}
int	__dpmi_get_state_save_restore_addr(__dpmi_raddr *_rm, __dpmi_paddr *_pm) {
    return -1;      // stub
}
int	__dpmi_get_raw_mode_switch_addr(__dpmi_raddr *_rm, __dpmi_paddr *_pm) {
    return -1;      // stub
}

int	__dpmi_get_version(__dpmi_version_ret *_ret) {
    _dpmi_info info;
    
    if (_ret == NULL) return -1;
    
    _ret->major = (info.version >> 8);
    _ret->minor = (info.version >> 0); 
    _ret->flags = info.flags;
    _ret->cpu   = info.cpu; 
    _ret->master_pic = info.lowpicbase; 
    _ret->slave_pic  = info.highpicbase;  
    return 0;
}

int	__dpmi_get_capabilities(int *_flags, char *vendor_info) {
    return -1;      // stub
}

int	__dpmi_get_free_memory_information(__dpmi_free_mem_info *_info) {
    return -1;      // stub
}

int	__dpmi_allocate_memory(__dpmi_meminfo *_info);						/* DPMI 0.9 AX=0501 */
int	__dpmi_free_memory(unsigned long _handle);						/* DPMI 0.9 AX=0502 */
int	__dpmi_resize_memory(__dpmi_meminfo *_info);						/* DPMI 0.9 AX=0503 */

int	__dpmi_allocate_linear_memory(__dpmi_meminfo *_info, int _commit) {
    return -1;      // stub
}
int	__dpmi_resize_linear_memory(__dpmi_meminfo *_info, int _commit) {
    return -1;      // stub
}
int	__dpmi_get_page_attributes(__dpmi_meminfo *_info, short *_buffer) {
    return -1;      // stub
}
int	__dpmi_set_page_attributes(__dpmi_meminfo *_info, short *_buffer) {
    return -1;      // stub
}
int	__dpmi_map_device_in_memory_block(__dpmi_meminfo *_info, unsigned long _physaddr) {
    return -1;      // stub
}
int	__dpmi_map_conventional_memory_in_memory_block(__dpmi_meminfo *_info, unsigned long _physaddr) {
    return -1;      // stub
}
int	__dpmi_get_memory_block_size_and_base(__dpmi_meminfo *_info) {
    return -1;      // stub
}
int	__dpmi_get_memory_information(__dpmi_memory_info *_buffer) {
    return -1;      // stub
}

int	__dpmi_lock_linear_region(__dpmi_meminfo *_info) {
    if (_info == NULL) return -1;
    dpmi_lockmemory((void*)_info->address, _info->size);
    return (dpmi_status ? -1 : 0);
}
int	__dpmi_unlock_linear_region(__dpmi_meminfo *_info) {
    if (_info == NULL) return -1;
    dpmi_unlockmemory((void*)_info->address, _info->size);
    return (dpmi_status ? -1 : 0);
}
int	__dpmi_mark_real_mode_region_as_pageable(__dpmi_meminfo *_info) {
    return -1;      // stub
}
int	__dpmi_relock_real_mode_region(__dpmi_meminfo *_info) {
    return -1;      // stub
}
int	__dpmi_get_page_size(unsigned long *_size) {
    if (_size != NULL) *_size = 4096;           // HACK!!!
    return 0;
}

int	__dpmi_mark_page_as_demand_paging_candidate(__dpmi_meminfo *_info) {
    return -1;      // stub
}
int	__dpmi_discard_page_contents(__dpmi_meminfo *_info) {
    return -1;      // stub
}

int	__dpmi_physical_address_mapping(__dpmi_meminfo *_info) {
    if (_info == NULL) return -1;
    
    _info->address = (unsigned int)dpmi_mapphysical(_info->size, (void*)_info->address);
    return (dpmi_status ? -1 : 0);
}
int	__dpmi_free_physical_address_mapping(__dpmi_meminfo *_info) {
    if (_info == NULL) return -1;
    
    dpmi_unmapphysical((void*)_info->address);
    return (dpmi_status ? -1 : 0);
}


#endif


