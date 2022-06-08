; sound foramt converter procedures
; by wbcbz7 ob.ll.zozo
; compile with FASM ONLY!

; info about __convcall calling convention (used here):
; edi - destination ptr
; esi - source ptr
; ecx - number of SAMPLES(!)
; edx - optional parameter (see below)

; eax, ecx, edx, esi and edi can be changed, all other registers must be preserved!

format MS COFF
use32

section ".data" data align 32



section ".text" code align 32

; ------------------------------------------
; memcpy 32bpp thunk
; ecx - number of pixels (aligned by 4), edx is not used

align 32
public sndconv_memcpy_32bpp as "ptc_memcpy_32bpp_\conv"
ptc_memcpy_32bpp:
        and         ecx, not 3      ; yep-yep, fuck misaligners! =)
        shl         ecx, 2
        jmp         ptc_memcpy
        

; ------------------------------------------
; 16bit signed stereo -> 16bit mono signed converter       
align 32
public sndconv_16s_16m as "sndconv_16s_16m_\conv"
sndconv_16s_16m:
        push        ebp                     ; 1
        mov         ebp, ecx                ; .
        
        ; address counter trick
        lea         esi, [esi + 4*ebp]      ; 2
        lea         edi, [edi + 2*ebp]      ; .
        xor         ebp, -1                 ; 3
        inc         ebp                     ; .
        
        push        ebx                     ; .
      
        ; unpaired, 2 samples per iteration
.loop:
        mov         eax, [esi + 4*ebp]
        mov         ecx, eax  
        shr         ecx, 16
        add         eax, ecx
        shr         eax, 1
        and         eax, 0x0000FFFF
        
        mov         ebx, [esi + 4*ebp + 4]
        mov         edx, ebx
        shl         ebx, 16
        add         edx, ebx 
        ror         ebx, 1
        and         ebx, 0xFFFF0000
        
        or          eax, ebx
        mov         [edi + 2*ebp] , eax
        
        add         ebp, 2
        jnc         .loop
        
        jz          .end
        
        ; process last sample (check for off-by-one!)
        mov         eax, [esi + 4*ebp]
        mov         ecx, eax  
        shr         ecx, 16
        add         eax, ecx
        shr         eax, 1
        mov         [edi + 2*ebp] , eax
        
.end
        pop         ebx
        ret

; ------------------------------------------
; 16bit signed stereo -> 8bit signed stereo converter, no dither
align 32
public sndconv_16s_8s as "sndconv_16s_8s_\conv"
sndconv_16s_8s:
        push        ebp                     ; 1
        mov         ebp, ecx                ; .
        
        lea         esi, [esi + 4*ebp
        lea         edi, [edi + 2*ebp]
        xor         ebp, -1
        inc         ebp
        
.loop:
        mov         esi


