; SOUND QUALITY NEVER WAS AND OPTION
; jk

; non-DMA IRQ0 routines -- wbcbz7 o7.lz.zozo-zl.o5.zozz

; actually, as you can see, pc speaker and covox use same irq0 proc actually, only snddev_irq0_struct.dataport is changed

; since fasm doesn't like mixing 16- and 32bit code in COFF (cuz' officially COFF doesn't support 16bit code!), i'll use nasm and assemble it to OMF

; ------------------------------
; private IRQ handler sturcture

struc snddev_irq0_struct
    .chain_rm       resd  1           ; real      mode IRQ0 old routine
    .chain_pm_ofs   resd  1           ; protected mode ----//--- offset
    .chain_pm_seg   resw  1           ; protected mode ----//--- selector
    .data_port      resw  1           ; data port (used for covox)
    
    .rm_callback    resd  1           ; rm->pm callback routine
    .pm_callback    resd  1           ; pm     callback routine (NEAR pointer -> same code segment!)
    
    .rmcount        resd  1           ; rm handler invocations
    .pmcount        resd  1           ; pm handler invocations
    
    .data_port2     resw  1           ; data port #2
    .bufferseg      resw  1           ; sound buffer segment      (for rm hander) - rounded down to 4k paras (0x0000, 0x1000 up to 0xF000)   ]
    .bufferofs      resd  1           ; sound buffer linear start (pm handler uses it as-is, rm handler uses only least 16bits + .bufferseg) ] seg:ofs pair for rm handler
    .bufferpos      resd  1           ; current buffer position, relative to buffer start
    .bufferlencur   resd  1           ; current buffer length
    .bufferlen      resd  1           ; buffer length
    .bufferlentotal resd  1           ; total buffer length
    
    .chain_acc      resd  1           ; IRQ0 old routine invokation accumulator
    
endstruc
; bufferofs is reused by both RM and PM handlers using a simple trick - buffer segment is set at multiple of 4096 paragraphs = 64k

; ------------------------------
; DPMI realmode registers struct
struc dpmi_realmode_registers
    .edi            resd 1
    .esi            resd 1
    .ebp            resd 1
    .reserved       resd 1            ; reserved, always 0
    .ebx            resd 1
    .edx            resd 1
    .ecx            resd 1
    .eax            resd 1
    .flags          resw 1
    .es             resw 1
    .ds             resw 1
    .fs             resw 1
    .gs             resw 1
    .ip             resw 1
    .cs             resw 1
    .sp             resw 1
    .ss             resw 1
endstruc

; -----------------------------------
; driver patch table

struc snddev_patch_table
    ; real-mode section
    .rm_start           resd 1              ; code/data start       ] copy and lock range
    .rm_end             resd 1              ; code/data end         ] 
    .rm_entry           resd 1              ; ISR entrypoint

    ; protected-mode section
    .pm_start           resd 1              ; code/data start       ] copy and lock range
    .pm_end             resd 1              ; code/data end         ] 
    .pm_entry           resd 1              ; ISR entrypoint

    ; patch table
    .rm_patch_dataseg   resd 1              ; pointer to  WORD snddev_irq0_struct segment
    .rm_patch_dt        resd 1              ; pointer to DWORD chain_acc increment

    .pm_patch_dataseg   resd 1              ; pointer to DWORD flat 4G data selector
    .pm_patch_dataofs   resd 1              ; pointer to DWORD snddev_irq0_struct linear offset
    .pm_patch_dt        resd 1              ; pointer to DWORD chain_acc increment
endstruc


; ------------------------------
; REAL MODE SECTION
            segment .text
            align 16
            use16

; IRQ0 ISR prologue
; out: DS - snddev_irq0_struct segment, ES - buffer segment, BX - buffer offset, SI - buffer position, DX - data port
%macro  ISR_RM_PROLOGUE 0
            push        ds
            push        es
            push        ax
            push        si
            push        bx
            push        dx
            
            ; load data segment
            mov         ax, word 0xCCCC
.data_seg   equ         $-2         ; patched during installation
            mov         ds, ax
        
            ; load and output sample
            mov         es, [snddev_irq0_struct.bufferseg]
            mov         bx, [snddev_irq0_struct.bufferofs]
            mov         si, [snddev_irq0_struct.bufferpos]
            mov         dx, [snddev_irq0_struct.data_port]
%endmacro

; insert device-specific output code between both macros

; IRQ0 ISR epilogue
%macro ISR_RM_EPILOGUE 0
            ; check for buffer wraparound
            cmp         si, [snddev_irq0_struct.bufferlentotal]
            jb          .nowrap

            ; wrap buffer to 0
            sub         si, [snddev_irq0_struct.bufferlentotal]
.nowrap:
            mov         [snddev_irq0_struct.bufferpos], si

            ; check for IRQ0 pending
            add         [snddev_irq0_struct.chain_acc], dword 0xCCCCCCCC
.dt         equ         $-4
            jc          .call_oldirq0                                     ; call old IRQ0 if timer overflowed (this would also send EOI)

            ; signal EOI
            mov         al, 0x20
            out         0x20, al

.skip_eoi:
            sti
            inc         dword [snddev_irq0_struct.rmcount]

            ; check for buffer end
            dec         word [snddev_irq0_struct.bufferlencur]
            jz          .nextbuffer

            ; return
            pop         dx
            pop         bx
            pop         si
            pop         ax
            pop         es
            pop         ds    
            iret

.call_oldirq0:
            pushf
            call        far [snddev_irq0_struct.chain_rm]
            jmp         .skip_eoi
            
.nextbuffer:
            ; restart
            mov         ax, [snddev_irq0_struct.bufferlen]
            mov         [ds:snddev_irq0_struct.bufferlencur], ax
        
            ; check if RMCB/PM stack in use (TODO: make it work in 16bit code)
            ;cmp         byte [cs:snddev_pm_stack_in_use], 0
            ;jnz         .rmcb_busy

            ; call callback
            call        far [snddev_irq0_struct.rm_callback]

.rmcb_busy:
            ; return
            pop         dx
            pop         bx
            pop         si
            pop         ax
            pop         es
            pop         ds    
            iret
%endmacro


global _snddev_rm_lock_start
_snddev_rm_lock_start:
        
; ------------------------------
; PC Speaker/Covox mono real-mode ISR
_snddev_pcspeaker_irq0_proc_rm:
            ISR_RM_PROLOGUE
            
            ; here goes device-specific stuff...
            mov         al, [es:si+bx]
            out         dx, al
            inc         si
            
            ISR_RM_EPILOGUE

_snddev_pcspeaker_irq0_proc_rm_end:

; ------------------------------
; Dual Covox real-mode ISR
_snddev_dualcovox_irq0_proc_rm:
            ISR_RM_PROLOGUE
            
            ; here goes device-specific stuff...

            ; left channel
            mov         al, [es:si+bx]
            out         dx, al
            
            ; right channel
            mov         dx, [snddev_irq0_struct.data_port2]
            mov         al, [es:si+bx+1]
            out         dx, al

            add         si, 2
            
            ISR_RM_EPILOGUE
_snddev_dualcovox_irq0_proc_rm_end:

; ------------------------------
; Stereo-On-1 real-mode ISR
_snddev_stereo1_irq0_proc_rm:
            ISR_RM_PROLOGUE
            
            ; here goes device-specific stuff...
            ; left channel
            mov         al, [es:si+bx]
            out         dx, al
            mov         al, 1
            add         dx, 2
            out         dx, al
            xor         ax, ax
            out         dx, al
            sub         dx, 2

            ; right channel
            mov         al, [es:si+bx+1]
            out         dx, al
            mov         al, 2
            add         dx, 2
            out         dx, al
            xor         ax, ax
            out         dx, al

            add         si, 2
            
            ISR_RM_EPILOGUE

_snddev_stereo1_irq0_proc_rm_end:

; ------------------------------
; Stereo-On-1 real-mode ISR, FastTracker 2.x protocol (faster, '373/'374/'7528 compatible but aliasy on PWM)
_snddev_stereo1_fast_irq0_proc_rm:
            ISR_RM_PROLOGUE

            ; left channel
            mov         al, 1
            add         dx, 2
            out         dx, al
            sub         dx, 2
            mov         al, [es:si+bx]
            out         dx, al

            ; right channel
            mov         al, 2
            add         dx, 2
            out         dx, al
            sub         dx, 2
            mov         al, [es:si+bx+1]
            out         dx, al

            add         esi, 2

            ISR_RM_EPILOGUE
_snddev_stereo1_fast_irq0_proc_rm_end:

; -----------------------------------
; END OF REAL MODE CODE
global _snddev_rm_lock_end
_snddev_rm_lock_end:

; ------------------------------
; PROTECTED MODE SECTION
; only relocatable code here!

            align 16
            use32

global _snddev_pm_lock_start
_snddev_pm_lock_start:
        
; IRQ0 ISR prologue
; out: DS - flat 4GB segment, EBX - snddev_irq0_struct offset, ECX - buffer offset, ESI - buffer position, EDX - data port
%macro  ISR_PM_PROLOGUE 0
            push        ds
            push        eax
            push        esi
            push        ebx
            push        edx
            push        ecx
            
            ; load data segment
            mov         eax, dword 0xCCCCCCCC
.data_seg   equ         $-4         ; patched during installation
            mov         ds, eax
            
            ; load data offset
            mov         ebx, dword 0xCCCCCCCC
.data_ofs   equ         $-4         ; patched during installation   
        
            ; load and output sample
            mov         ecx, [ebx + snddev_irq0_struct.bufferofs]
            mov         esi, [ebx + snddev_irq0_struct.bufferpos]
            mov         dx,  [ebx + snddev_irq0_struct.data_port]
%endmacro

; insert device-specific output code between both macros

; IRQ0 ISR epilogue
%macro ISR_PM_EPILOGUE 0
            cmp         esi, [ebx + snddev_irq0_struct.bufferlentotal]
            jb          .nowrap

            ; wrap buffer to start
            sub         esi, [ebx + snddev_irq0_struct.bufferlentotal]
.nowrap:
            mov         [ebx + snddev_irq0_struct.bufferpos], esi

            ; check for IRQ0 pending
            add         dword [ebx + snddev_irq0_struct.chain_acc], dword 0xCCCCCCCC
.dt         equ         $-4
            jc          .call_oldirq0                                       ; call old IRQ0 if accumulator overflowed (this would also send EOI)

            ; signal EOI
            mov         al, 0x20
            out         0x20, al

.skip_eoi:
            sti
            inc         dword [ebx + snddev_irq0_struct.pmcount]
            
            ; check for buffer end
            dec         dword [ebx + snddev_irq0_struct.bufferlencur]
            jz          .nextbuffer
            
            ; return
            pop         ecx
            pop         edx
            pop         ebx
            pop         esi
            pop         eax
            pop         ds
            
            cli                         ; dos4g faq suggests this
            iret

.call_oldirq0:
            pushf
            call        far [ebx + snddev_irq0_struct.chain_pm_ofs]
            jmp         .skip_eoi
            
.nextbuffer:
            ; restart
            mov         eax, [ebx + snddev_irq0_struct.bufferlen]
            mov         [ebx + snddev_irq0_struct.bufferlencur], eax
        
            ; call callback
            cmp         byte [_snddev_pm_stack_in_use], 0
            jnz         .exit
            inc         byte [_snddev_pm_stack_in_use]

            ; switch stacks
            mov         word  [_snddev_pm_old_stack + 4], ss
            mov         dword [_snddev_pm_old_stack],     esp
            lss         esp, [_snddev_pm_stack_top]

            push        es
            mov         eax, ds
            mov         es, eax
            
            pusha
            call        near [ebx + snddev_irq0_struct.pm_callback]
            popa
            pop         es

            ; switch back
            lss         esp, [_snddev_pm_old_stack]

            dec         byte [_snddev_pm_stack_in_use]
.exit:
            ; return
            pop         ecx
            pop         edx
            pop         ebx
            pop         esi
            pop         eax
            pop         ds
            
            cli                         ; dos4g faq suggests this
            iret
%endmacro

; ------------------------------
; PC Speaker/Covox mono protected-mode ISR
_snddev_pcspeaker_irq0_proc_pm:
            ISR_PM_PROLOGUE

            ; device-specific code
            mov         al, [ecx + esi]
            out         dx, al
            inc         esi
            
            ISR_PM_EPILOGUE
_snddev_pcspeaker_irq0_proc_pm_end:

; ------------------------------
; Dual Covox protected-mode ISR
_snddev_dualcovox_irq0_proc_pm:
            ISR_PM_PROLOGUE

            ; device-specific code
            
            ; left channel
            mov         al, [ecx + esi]
            out         dx, al

            ; right channel
            mov         dx, [ebx + snddev_irq0_struct.data_port2]
            mov         al, [ecx + esi + 1]
            out         dx, al

            add         esi, 2
            
            ISR_PM_EPILOGUE
_snddev_dualcovox_irq0_proc_pm_end:

; ------------------------------
; Stereo-On-1 protected-mode ISR
_snddev_stereo1_irq0_proc_pm:
            ISR_PM_PROLOGUE
            
            ; here goes device-specific stuff...
            ; left channel
            mov         al, [ecx + esi]
            out         dx, al
            mov         al, 1
            add         edx, 2
            out         dx, al
            xor         eax, eax
            out         dx, al
            sub         edx, 2

            ; right channel
            mov         al, [ecx + esi + 1]
            out         dx, al
            mov         al, 2
            add         edx, 2
            out         dx, al
            xor         eax, eax
            out         dx, al

            add         esi, 2
            
            ISR_PM_EPILOGUE
_snddev_stereo1_irq0_proc_pm_end:

; ------------------------------
; Stereo-On-1 protected-mode ISR, FastTracker 2.x protocol (faster, '373/'374/'7528 compatible but aliasy on PWM)
_snddev_stereo1_fast_irq0_proc_pm:
            ISR_PM_PROLOGUE

            ; left channel
            mov         al, 1
            add         edx, 2
            out         dx, al
            sub         edx, 2
            mov         al, [ecx + esi]
            out         dx, al

            ; right channel
            mov         al, 2
            add         edx, 2
            out         dx, al
            sub         edx, 2
            mov         al, [ecx + esi + 1]
            out         dx, al

            add         esi, 2

            ISR_PM_EPILOGUE
_snddev_stereo1_fast_irq0_proc_pm_end:

            ; -----------------------------------
            ; callback trampoline code (NON-REENTRANT!)
global _snddev_irq0_callback
global _snddev_irq0_callback_dataofs
global _snddev_irq0_callback_end
_snddev_irq0_callback:
            ; extract return address from the stack
            cld
            lodsw
            mov         [es:edi + dpmi_realmode_registers.ip], ax
            lodsw
            mov         [es:edi + dpmi_realmode_registers.cs], ax
            add         word [es:edi + dpmi_realmode_registers.sp], 4

            cmp         byte [cs:_snddev_pm_stack_in_use], 0
            jnz         .exit

            ; copy current structure
            push        es
            pop         ds

            inc         byte [_snddev_pm_stack_in_use]
            
            ; switch stacks
            mov         word  [_snddev_pm_old_stack + 4], ss
            mov         dword [_snddev_pm_old_stack],     esp
            lss         esp, [_snddev_pm_stack_top]

            ; enable interrupts (we don't want squacking sounds)
            sti

            ; call callback
            push        edi
            mov         eax, 0xCCCCCCCC
_snddev_irq0_callback_dataofs   equ $-4
            call        near [eax + snddev_irq0_struct.pm_callback]
            pop         edi

            ; switch back
            lss         esp, [_snddev_pm_old_stack]

            dec         byte [_snddev_pm_stack_in_use]

            cli
.exit:
            ; phew :)
            iret

; -----------------------------------
; END OF PROTECTED MODE CODE
global _snddev_pm_lock_end
_snddev_pm_lock_end:

; ------------------------------------------------------------------------------------------------------
; BSS DATA SECTION
; ------------------------------------------------------------------------------------------------------
            section .bss
            align   16
            use32
global _snddev_bss_lock_start
global _snddev_bss_lock_end

_snddev_bss_lock_start:
; -----------------------------------

; protected mode stack
global _snddev_pm_stack
global _snddev_pm_stack_top
global _snddev_pm_old_stack
            align 16
_snddev_pm_stack:
            resb            32768
_snddev_pm_stack_top:
            resb            8

; old stack pointer
_snddev_pm_old_stack:
            resb            8

; RMCB registers
            align 16
global _snddev_irq0_callback_registers
_snddev_irq0_callback_registers:
            resb            dpmi_realmode_registers_size

; RMCB "in use" flag
global _snddev_pm_stack_in_use
_snddev_pm_stack_in_use:
            resb            0

_snddev_bss_lock_end:
; -----------------------------------
; end of BSS

; ------------------------------------------------------------------------------------------------------
; CONST DATA SECTION
; ------------------------------------------------------------------------------------------------------
            section .const
            align   16
            use32

; ---------------------------------------------------
; driver patch table for PC-Speaker/Covox mono driver
global _snddev_irq0_patch_pcspeaker
_snddev_irq0_patch_pcspeaker:
    istruc snddev_patch_table
        at snddev_patch_table.rm_start,           dd  _snddev_pcspeaker_irq0_proc_rm 
        at snddev_patch_table.rm_end,             dd  _snddev_pcspeaker_irq0_proc_rm_end
        at snddev_patch_table.rm_entry,           dd  _snddev_pcspeaker_irq0_proc_rm 

        at snddev_patch_table.pm_start,           dd  _snddev_pcspeaker_irq0_proc_pm 
        at snddev_patch_table.pm_end,             dd  _snddev_pcspeaker_irq0_proc_pm_end
        at snddev_patch_table.pm_entry,           dd  _snddev_pcspeaker_irq0_proc_pm

        at snddev_patch_table.rm_patch_dataseg,   dd  _snddev_pcspeaker_irq0_proc_rm.data_seg
        at snddev_patch_table.rm_patch_dt,        dd  _snddev_pcspeaker_irq0_proc_rm.dt

        at snddev_patch_table.pm_patch_dataseg,   dd  _snddev_pcspeaker_irq0_proc_pm.data_seg
        at snddev_patch_table.pm_patch_dataofs,   dd  _snddev_pcspeaker_irq0_proc_pm.data_ofs
        at snddev_patch_table.pm_patch_dt,        dd  _snddev_pcspeaker_irq0_proc_pm.dt
    iend

; ---------------------------------------------------
; driver patch table for Dual Covox
global _snddev_irq0_patch_dualcovox
_snddev_irq0_patch_dualcovox:
    istruc snddev_patch_table
        at snddev_patch_table.rm_start,           dd  _snddev_dualcovox_irq0_proc_rm 
        at snddev_patch_table.rm_end,             dd  _snddev_dualcovox_irq0_proc_rm_end
        at snddev_patch_table.rm_entry,           dd  _snddev_dualcovox_irq0_proc_rm 

        at snddev_patch_table.pm_start,           dd  _snddev_dualcovox_irq0_proc_pm
        at snddev_patch_table.pm_end,             dd  _snddev_dualcovox_irq0_proc_pm_end
        at snddev_patch_table.pm_entry,           dd  _snddev_dualcovox_irq0_proc_pm

        at snddev_patch_table.rm_patch_dataseg,   dd  _snddev_dualcovox_irq0_proc_rm.data_seg
        at snddev_patch_table.rm_patch_dt,        dd  _snddev_dualcovox_irq0_proc_rm.dt

        at snddev_patch_table.pm_patch_dataseg,   dd  _snddev_dualcovox_irq0_proc_pm.data_seg
        at snddev_patch_table.pm_patch_dataofs,   dd  _snddev_dualcovox_irq0_proc_pm.data_ofs
        at snddev_patch_table.pm_patch_dt,        dd  _snddev_dualcovox_irq0_proc_pm.dt
    iend


; ---------------------------------------------------
; driver patch table for Stereo-on-1
global _snddev_irq0_patch_stereo1
_snddev_irq0_patch_stereo1:
    istruc snddev_patch_table
        at snddev_patch_table.rm_start,           dd  _snddev_stereo1_irq0_proc_rm 
        at snddev_patch_table.rm_end,             dd  _snddev_stereo1_irq0_proc_rm_end
        at snddev_patch_table.rm_entry,           dd  _snddev_stereo1_irq0_proc_rm 

        at snddev_patch_table.pm_start,           dd  _snddev_stereo1_irq0_proc_pm
        at snddev_patch_table.pm_end,             dd  _snddev_stereo1_irq0_proc_pm_end
        at snddev_patch_table.pm_entry,           dd  _snddev_stereo1_irq0_proc_pm

        at snddev_patch_table.rm_patch_dataseg,   dd  _snddev_stereo1_irq0_proc_rm.data_seg
        at snddev_patch_table.rm_patch_dt,        dd  _snddev_stereo1_irq0_proc_rm.dt

        at snddev_patch_table.pm_patch_dataseg,   dd  _snddev_stereo1_irq0_proc_pm.data_seg
        at snddev_patch_table.pm_patch_dataofs,   dd  _snddev_stereo1_irq0_proc_pm.data_ofs
        at snddev_patch_table.pm_patch_dt,        dd  _snddev_stereo1_irq0_proc_pm.dt
    iend

global _snddev_irq0_patch_stereo1_fast
_snddev_irq0_patch_stereo1_fast:
    istruc snddev_patch_table
        at snddev_patch_table.rm_start,           dd  _snddev_stereo1_fast_irq0_proc_rm 
        at snddev_patch_table.rm_end,             dd  _snddev_stereo1_fast_irq0_proc_rm_end
        at snddev_patch_table.rm_entry,           dd  _snddev_stereo1_fast_irq0_proc_rm 

        at snddev_patch_table.pm_start,           dd  _snddev_stereo1_fast_irq0_proc_pm
        at snddev_patch_table.pm_end,             dd  _snddev_stereo1_fast_irq0_proc_pm_end
        at snddev_patch_table.pm_entry,           dd  _snddev_stereo1_fast_irq0_proc_pm

        at snddev_patch_table.rm_patch_dataseg,   dd  _snddev_stereo1_fast_irq0_proc_rm.data_seg
        at snddev_patch_table.rm_patch_dt,        dd  _snddev_stereo1_fast_irq0_proc_rm.dt

        at snddev_patch_table.pm_patch_dataseg,   dd  _snddev_stereo1_fast_irq0_proc_pm.data_seg
        at snddev_patch_table.pm_patch_dataofs,   dd  _snddev_stereo1_fast_irq0_proc_pm.data_ofs
        at snddev_patch_table.pm_patch_dt,        dd  _snddev_stereo1_fast_irq0_proc_pm.dt
    iend
