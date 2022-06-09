#ifndef DPMI_H
#define DPMI_H

#include <stdlib.h>
#include <i86.h>

// some useful DPMI functions
// by wbc\\bz7 zo.oz.zolb

// define _DPMI_DJGPP_COMPATIBILITY for adding DJGPP compatibility functions

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack (push, 1)

// dpmi descriptor struct
typedef struct {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char  base_mid;

    // access byte
    unsigned char  accessed : 1;   // set by CPU
    unsigned char  rw : 1;         // read enable bit for code, write enable bit for data
    unsigned char  dc : 1;         // direction (set if grows down) bit for data, "conforming" bit for code (read the docs :)
    unsigned char  code : 1;       // set if code, clear if data
    unsigned char  type : 1;       // set for code/data, clear for special segments
    unsigned char  dpl : 2;        // privilegie level
    unsigned char  present : 1;    // set if present

    unsigned char  limit_high : 4;

    unsigned char  avl : 1;        // available for software needs
    unsigned char  longseg : 1;    // set if 64bit segment
    unsigned char  size : 1;       // set if 32bit, clear if 16/64bit
    unsigned char  limitgran : 1;  // set if limit in pages (4k units), clear if in bytes

    unsigned char  base_high;
} _dpmi_descriptor;

// dpmi realmode regs structire
typedef struct {
    unsigned long EDI;
    unsigned long ESI;
    unsigned long EBP;
    unsigned long reserved;
    unsigned long EBX;
    unsigned long EDX;
    unsigned long ECX;
    unsigned long EAX;
    unsigned short flags;
    unsigned short ES,DS,FS,GS,IP,CS,SP,SS;
} _dpmi_rmregs;

// dpmi segment:selector pair
typedef struct {
    unsigned short int segment;
    unsigned short int selector;
} _dpmi_ptr;

// dpmi info 
typedef struct {
    unsigned long version;
    unsigned long flags;
    unsigned long cpu;
    
    unsigned long highpicbase;          // IRQ 8-15 PIC base
    unsigned long lowpicbase;           // IRQ 0-7  PIC base
} _dpmi_info;

typedef struct {
    unsigned long   signature;           // "PMDW"/"D32A"/etc...
    unsigned long   cpu;
    unsigned long   systemType;          // DPMI_SYSTEM_*
    unsigned long   version;
    char _far      *copyright;
} _dpmi_extender_info;

enum {
        DPMI_SYSTEM_RAW = 0,
        DPMI_SYSTEM_XMS,
        DPMI_SYSTEM_VCPI,
        DPMI_SYSTEM_DPMI,
};

typedef struct {
    unsigned long largestBlockFree;
    // items below are used by virtual memory DPMI hosts, -1 otherwise
    unsigned long maxUnlockedPagesAvail;
    unsigned long maxLockedPagesAvail;
    unsigned long linearAddressSpace;       // in pages
    unsigned long totalUnlockedPages;
    unsigned long freePages;
    unsigned long totalPhysicalPages;
    unsigned long freeLinearAddressSpace;   // in pages
    unsigned long pagefileSize;             // in pages
    unsigned char reserved[12];
} _dpmi_mem_info;

// dpmi realmode pointer struct
typedef union {
    struct {
        unsigned long offset:16;
        unsigned long segment:16;
    };
    unsigned long ptr;
    
} _dpmi_rmpointer;

typedef union {
    struct {
        unsigned long offset:16;
        unsigned long selector:16;
    };
    unsigned long ptr;
    
} _dpmi_ptr16_16;

#pragma pack (pop)


void* dpmi_ptr16_16_to_flat32(unsigned long ptr16_16);
void _far *dpmi_ptr16_16_to_16_32(unsigned long ptr16_16);

unsigned int dpmi_getdescriptors(unsigned long count);
void dpmi_freedescriptor(unsigned long desc) ;
unsigned long dpmi_segtodesc(unsigned long segment);
unsigned long dpmi_selectorincrement();
void * dpmi_getsegmentbase(unsigned long selector);
unsigned long dpmi_getsegmentlimit(unsigned long selector);
void dpmi_setsegmentbase(unsigned long selector, void *base);
void dpmi_setsegmentlimit(unsigned long selector, unsigned long limit);
void dpmi_setaccessrights(unsigned long selector, unsigned long rights);
unsigned long dpmi_getaccessrights(unsigned long selector);
unsigned long dpmi_getalias(unsigned long selector);
void dpmi_getdescriptor(unsigned long selector, _dpmi_descriptor __far *buffer);
void dpmi_setdescriptor(unsigned long selector, _dpmi_descriptor __far *buffer);

unsigned long dpmi_isreadable(unsigned long selector);
unsigned long dpmi_iswriteable(unsigned long selector);

void dpmi_getdosmem(int size, _dpmi_ptr *p, int *largestAvailBlock = NULL);    // SIZE IN PARAGRAPHS!!!!
int dpmi_dosmemlargestavail();

void dpmi_freedosmem(_dpmi_ptr *p);
void dpmi_rminterrupt(int int_num, _dpmi_rmregs *regs);
void * dpmi_mapphysical(unsigned long size, void *p);
void dpmi_unmapphysical(void *p);

void * dpmi_getrmvect(int num);
_dpmi_rmpointer dpmi_getrmvect_ex(int num);
void dpmi_setrmvect(int num, void *p);
void dpmi_setrmvect_ex(int num, _dpmi_rmpointer p);

void __far * dpmi_getpmvect(int num);
void dpmi_setpmvect(int num, void __far *proc);

void __far * dpmi_getpmexception(int num);
void dpmi_setpmexception(int num, void __far *proc);

void * dpmi_getcallback(void __far *proc, _dpmi_rmregs __far *rmregs);
_dpmi_rmpointer dpmi_getcallback_ex(void __far *proc, _dpmi_rmregs __far *rmregs);
void dpmi_freecallback(void *p);
void dpmi_freecallback_ex(_dpmi_rmpointer p);

void dpmi_getinfo(_dpmi_info *p);
void dpmi_getmeminfo(_dpmi_mem_info __far *p);
void dpmi_getextenderinfo(_dpmi_extender_info *p);
void _far *dpmi_getvendorapi(char _far *id);

void dpmi_lockmemory(void *p, unsigned long size);
void dpmi_unlockmemory(void *p, unsigned long size);

int rmint386(int intnum, union REGS *in, union REGS *out);
int rmint386x(int intnum, union REGS *in, union REGS *out, struct SREGS *seg);
void rmintr(int intnum, union REGPACK *r);

void dpmi_yield();

extern unsigned int dpmi_status;
extern unsigned int dpmi_returncode;

// dos32a stuff

typedef struct {
    void _far       *gdt;
    void _far       *idt;
    
    unsigned long   gdtLimit;
    unsigned long   idtLimit;
    
} _dos32a_gdt_idt_info;

typedef struct {
    unsigned short  code;
    unsigned short  data;
    unsigned short  zero;
    unsigned short  rmcode;
    unsigned short  codeClient;
} _dos32a_kernel_selectors;

typedef struct {
    unsigned long   int_rm2pm;
    unsigned long   int_pm2rm;
    unsigned long   irq_rm2pm;
    unsigned long   irq_pm2rm;
    unsigned long   irqCallback_rm2pm;
    unsigned long   irqCallback_pm2rm;
    unsigned long   callback_rm2pm;
    unsigned long   callback_pm2rm;
} _dos32a_performance_counters;

unsigned long dos4g_get_function(void _far (*apientry)(), char* package, char* function);
unsigned long cdecl dos4g_call_function(void _far (*apientry)(), unsigned long function, void* parms, unsigned long parmSize);

void dos32a_get_gdt_idt(void _far (*apientry)(), _dos32a_gdt_idt_info *p);
void dos32a_get_kernel_selectors(void _far (*apientry)(), _dos32a_kernel_selectors *p);
_dos32a_performance_counters _far *dos32a_get_performance_counters(void _far (*apientry)());



#ifdef _DPMI_DJGPP_COMPATIBILITY

// alias __dpmi_error to existing dpmi_returncode
#define __dpmi_error dpmi_returncode

typedef struct {
  unsigned short offset16;
  unsigned short segment;
} __dpmi_raddr;

typedef struct {
  unsigned long  offset32;
  unsigned short selector;
} __dpmi_paddr;

typedef struct {
  unsigned long handle;   /* 0, 2 */
  unsigned long size;  /* or count */ /* 4, 6 */
  unsigned long address;  /* 8, 10 */
} __dpmi_meminfo;

typedef union {
  struct {
    unsigned long edi;
    unsigned long esi;
    unsigned long ebp;
    unsigned long res;
    unsigned long ebx;
    unsigned long edx;
    unsigned long ecx;
    unsigned long eax;
  } d;
  struct {
    unsigned short di, di_hi;
    unsigned short si, si_hi;
    unsigned short bp, bp_hi;
    unsigned short res, res_hi;
    unsigned short bx, bx_hi;
    unsigned short dx, dx_hi;
    unsigned short cx, cx_hi;
    unsigned short ax, ax_hi;
    unsigned short flags;
    unsigned short es;
    unsigned short ds;
    unsigned short fs;
    unsigned short gs;
    unsigned short ip;
    unsigned short cs;
    unsigned short sp;
    unsigned short ss;
  } x;
  struct {
    unsigned char edi[4];
    unsigned char esi[4];
    unsigned char ebp[4];
    unsigned char res[4];
    unsigned char bl, bh, ebx_b2, ebx_b3;
    unsigned char dl, dh, edx_b2, edx_b3;
    unsigned char cl, ch, ecx_b2, ecx_b3;
    unsigned char al, ah, eax_b2, eax_b3;
  } h;
} __dpmi_regs;
  
typedef struct {
  unsigned char  major;
  unsigned char  minor;
  unsigned short flags;
  unsigned char  cpu;
  unsigned char  master_pic;
  unsigned char  slave_pic;
} __dpmi_version_ret;

typedef struct {
  unsigned long largest_available_free_block_in_bytes;
  unsigned long maximum_unlocked_page_allocation_in_pages;
  unsigned long maximum_locked_page_allocation_in_pages;
  unsigned long linear_address_space_size_in_pages;
  unsigned long total_number_of_unlocked_pages;
  unsigned long total_number_of_free_pages;
  unsigned long total_number_of_physical_pages;
  unsigned long free_linear_address_space_in_pages;
  unsigned long size_of_paging_file_partition_in_pages;
  unsigned long reserved[3];
} __dpmi_free_mem_info;

typedef struct {
  unsigned long total_allocated_bytes_of_physical_memory_host;
  unsigned long total_allocated_bytes_of_virtual_memory_host;
  unsigned long total_available_bytes_of_virtual_memory_host;
  unsigned long total_allocated_bytes_of_virtual_memory_vcpu;
  unsigned long total_available_bytes_of_virtual_memory_vcpu;
  unsigned long total_allocated_bytes_of_virtual_memory_client;
  unsigned long total_available_bytes_of_virtual_memory_client;
  unsigned long total_locked_bytes_of_memory_client;
  unsigned long max_locked_bytes_of_memory_client;
  unsigned long highest_linear_address_available_to_client;
  unsigned long size_in_bytes_of_largest_free_memory_block;
  unsigned long size_of_minimum_allocation_unit_in_bytes;
  unsigned long size_of_allocation_alignment_unit_in_bytes;
  unsigned long reserved[19];
} __dpmi_memory_info;

typedef struct {
  unsigned long data16[2];
  unsigned long code16[2];
  unsigned short ip;
  unsigned short reserved;
  unsigned long data32[2];
  unsigned long code32[2];
  unsigned long eip;
} __dpmi_callback_info;

typedef struct {
  unsigned long size_requested;
  unsigned long size;
  unsigned long handle;
  unsigned long address;
  unsigned long name_offset;
  unsigned short name_selector;
  unsigned short reserved1;
  unsigned long reserved2;
} __dpmi_shminfo;

// DJGPP compatibility headers
void __dpmi_yield(void);

int __dpmi_allocate_ldt_descriptors(int _count);
int __dpmi_free_ldt_descriptor(int _descriptor);
int __dpmi_segment_to_descriptor(int _seg);
int __dpmi_get_selector_increment_value(void);
int __dpmi_get_segment_base_address(int _selector, unsigned long *_addr);
int __dpmi_set_segment_base_address(int _selector, unsigned long _address);
unsigned long __dpmi_get_segment_limit(int _selector);
int __dpmi_set_segment_limit(int _selector, unsigned long _limit);
int __dpmi_get_descriptor_access_rights(int _selector);
int __dpmi_set_descriptor_access_rights(int _selector, int _rights);
int __dpmi_create_alias_descriptor(int _selector);
int __dpmi_get_descriptor(int _selector, void *_buffer);
int __dpmi_set_descriptor(int _selector, void *_buffer);
int __dpmi_allocate_specific_ldt_descriptor(int _selector);

int __dpmi_get_multiple_descriptors(int _count, void *_buffer); 
int __dpmi_set_multiple_descriptors(int _count, void *_buffer);

int __dpmi_allocate_dos_memory(int _paragraphs, int *_ret_selector_or_max);
int __dpmi_free_dos_memory(int _selector);
int __dpmi_resize_dos_memory(int _selector, int _newpara, int *_ret_max);

int __dpmi_get_real_mode_interrupt_vector(int _vector, __dpmi_raddr *_address);
int __dpmi_set_real_mode_interrupt_vector(int _vector, __dpmi_raddr *_address);
int __dpmi_get_processor_exception_handler_vector(int _vector, __dpmi_paddr *_address);
int __dpmi_set_processor_exception_handler_vector(int _vector, __dpmi_paddr *_address);
int __dpmi_get_protected_mode_interrupt_vector(int _vector, __dpmi_paddr *_address);
int __dpmi_set_protected_mode_interrupt_vector(int _vector, __dpmi_paddr *_address);

int __dpmi_get_extended_exception_handler_vector_pm(int _vector, __dpmi_paddr *_address);
int __dpmi_get_extended_exception_handler_vector_rm(int _vector, __dpmi_paddr *_address);
int __dpmi_set_extended_exception_handler_vector_pm(int _vector, __dpmi_paddr *_address);
int __dpmi_set_extended_exception_handler_vector_rm(int _vector, __dpmi_paddr *_address);

int __dpmi_simulate_real_mode_interrupt(int _vector, __dpmi_regs *_regs);
int __dpmi_int(int _vector, __dpmi_regs *_regs); /* like above, but sets ss sp fl */
extern short __dpmi_int_ss, __dpmi_int_sp, __dpmi_int_flags; /* default to zero */
int __dpmi_simulate_real_mode_procedure_retf(__dpmi_regs *_regs);
int __dpmi_simulate_real_mode_procedure_retf_stack(__dpmi_regs *_regs, int stack_words_to_copy, const void *stack_data); /* DPMI 0.9 AX=0301 */
int __dpmi_simulate_real_mode_procedure_iret(__dpmi_regs *_regs);
int __dpmi_allocate_real_mode_callback(void (*_handler)(void), __dpmi_regs *_regs, __dpmi_raddr *_ret); /* DPMI 0.9 AX=0303 */
int __dpmi_free_real_mode_callback(__dpmi_raddr *_addr);
int __dpmi_get_state_save_restore_addr(__dpmi_raddr *_rm, __dpmi_paddr *_pm);
int __dpmi_get_raw_mode_switch_addr(__dpmi_raddr *_rm, __dpmi_paddr *_pm);

int __dpmi_get_version(__dpmi_version_ret *_ret);

int __dpmi_get_capabilities(int *_flags, char *vendor_info);

int __dpmi_get_free_memory_information(__dpmi_free_mem_info *_info);
int __dpmi_allocate_memory(__dpmi_meminfo *_info);
int __dpmi_free_memory(unsigned long _handle);
int __dpmi_resize_memory(__dpmi_meminfo *_info);

int __dpmi_allocate_linear_memory(__dpmi_meminfo *_info, int _commit);
int __dpmi_resize_linear_memory(__dpmi_meminfo *_info, int _commit);
int __dpmi_get_page_attributes(__dpmi_meminfo *_info, short *_buffer);
int __dpmi_set_page_attributes(__dpmi_meminfo *_info, short *_buffer);
int __dpmi_map_device_in_memory_block(__dpmi_meminfo *_info, unsigned long _physaddr);
int __dpmi_map_conventional_memory_in_memory_block(__dpmi_meminfo *_info, unsigned long _physaddr);
int __dpmi_get_memory_block_size_and_base(__dpmi_meminfo *_info);
int __dpmi_get_memory_information(__dpmi_memory_info *_buffer);

int __dpmi_lock_linear_region(__dpmi_meminfo *_info);
int __dpmi_unlock_linear_region(__dpmi_meminfo *_info);
int __dpmi_mark_real_mode_region_as_pageable(__dpmi_meminfo *_info);
int __dpmi_relock_real_mode_region(__dpmi_meminfo *_info);
int __dpmi_get_page_size(unsigned long *_size);

int __dpmi_mark_page_as_demand_paging_candidate(__dpmi_meminfo *_info);
int __dpmi_discard_page_contents(__dpmi_meminfo *_info);

int __dpmi_physical_address_mapping(__dpmi_meminfo *_info);
int __dpmi_free_physical_address_mapping(__dpmi_meminfo *_info);

#endif

#ifdef __cplusplus
};
#endif

#endif