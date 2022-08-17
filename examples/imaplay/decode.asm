; imaplay - asm decoding routines
; wbcbz7 18.o8.2o22

            segment .text
            align 16
            use32

; generated in player, 4kb each (could be optimized)
; low 8 bits - stepIndex << 1, high 4 bits - sample code
extern _imaplay_nextstep_table
extern _imaplay_diff_table

%if IMAPLAY_USE_C_DECODERS != 1
global __imaplay_decode_mono
global __imaplay_decode_stereo
%endif

; edi - out, esi - in, ecx - blockSize in samples, first sample aka predictor is extracted from header

;uint32_t imaplay_decode_mono(int16_t * out, uint8_t * in, uint32_t blockSize)
__imaplay_decode_mono:
            pusha

            ; get first sample
            mov         ax, [esi]
            mov         [edi], ax

            ; get step index
            xor         edx, edx
            mov         dl,  [esi + 2]
            add         esi, 4

            ; shift step index
            add         edx, edx
            add         edi, 2
            
    .loop:  
            ; get first 8 samples
            mov         ebx, [esi]

%assign smp 0
%rep 8
            ; sample loop
            mov         dh, bl
            shr         ebx, 4
            and         dh, 0xF
            add         ax, word [_imaplay_diff_table + edx]
            jno         .skip_overflow_%+ smp
            ; greeting to baze :)
            sbb         ax, ax
            xor         ax, 0x7FFF
    .skip_overflow_%+ smp:
            mov         [edi + smp*2], ax
            mov         dl, byte [_imaplay_nextstep_table + edx]
%assign smp smp + 1
%endrep

            add         esi, 4
            add         edi, 8*2

            sub         ecx, 8
            jnz         .loop

            popa
            ret

__imaplay_decode_stereo:
            pusha
            push        ecx

            ; eax     - sample storage (4 samples per channel)
            ; ebx     - lookup index   (note we have to save index somewhere)
            ; ecx     - (sample counter << 16) | index storage
            ; edx/ebp - predictors
            ; esi/edi - src/dst

            xor         ebx, ebx
            xor         edx, edx
            xor         ebp, ebp

            ; get first sample
            mov         dx, [esi]
            mov         cl, [esi + 2]
            mov         bp, [esi + 4]
            mov         ch, [esi + 6]
            add         cx, cx        ; step_index in 0..88 range

            ; store first sample
            mov         [edi], dx
            mov         [edi + 2], bp

            add         esi, 8
            add         edi, 4

    .loop:
            mov         eax, [esi]
            mov         bl, cl

            ; do left channel (BL = CL - left index)
%assign smp 0
%rep 8
            mov         bh, al
            and         bh, 0xF
            add         dx, word [_imaplay_diff_table + ebx]
            jno         .skip_overflow_left_%+ smp
            ; greetings to baze :)
            sbb         dx, dx
            xor         dx, 0x7FFF
    .skip_overflow_left_%+ smp:
            shr         eax, 4
            mov         [edi + smp*4], dx
            mov         bl, byte [_imaplay_nextstep_table + ebx]
%assign smp smp + 1
%endrep
            ; switch to right channel (BL = CH - right index)
            mov         cl, bl
            mov         bl, ch
            mov         eax, [esi + 4]
%assign smp 0
%rep 8
            mov         bh, al
            and         bh, 0xF
            add         bp, word [_imaplay_diff_table + ebx]
            jno         .skip_overflow_right_%+ smp
            ; greetings to baze :)
            sbb         bp, bp
            xor         bp, 0x7FFF
    .skip_overflow_right_%+ smp:
            shr         eax, 4
            mov         [edi + smp*4 + 2], bp
            mov         bl, byte [_imaplay_nextstep_table + ebx]
%assign smp smp + 1
%endrep
            ; save channels
            mov         ch, bl
            add         esi, 8

            add         edi, 32

            sub         dword [esp], 8
            jnz         .loop
            pop         eax

            popa
            ret
