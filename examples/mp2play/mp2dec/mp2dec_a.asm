; mp2dec assembler stuff
; --wbcbz7 lb.ob.zolq

; basically just clang asm output (-O3 -m32 -march=pentium -ffast-float) for certain transform.c functions
; // layer2 FDCT/windowing stuff  - uses bits of amp player sources - original header info below:

; /* this file is a part of amp software, (C) tomislav uzelac 1996,1997
; */

; /* transform.h  tables galore
; *
; * Created by: tomislav uzelac  May 1996
; * Last modified by: tomislav uzelac  Mar  1 97
; */

            segment .text
            align 16
            use32

extern _mp2dec_t_dewindow
extern _mp2dec_FDCTCoeffs

; define if you want to use clang output
%define CLANG_OUTPUT

global _mp2dec_poly_fdct_p5

%ifdef CLANG_OUTPUT
;mp2dec_poly_fdct_p5(mp2dec_poly_private_data *data, float *subbands, const int ch);
_mp2dec_poly_fdct_p5:
        push    edi
        push    esi
        sub     esp, 204
        mov     eax, dword [esp + 220]
        fld     dword [eax]
        fld     dword [eax + 124]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 36]            
        faddp   st1, st0
        fstp    dword [esp + 16]            
        fld     dword [eax + 4]
        fld     dword [eax + 120]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 64]            
        faddp   st1, st0
        fstp    dword [esp + 4]             
        fld     dword [eax + 8]
        fld     dword [eax + 116]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 32]            
        faddp   st1, st0
        fstp    dword [esp]                 
        fld     dword [eax + 12]
        fld     dword [eax + 112]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 28]            
        faddp   st1, st0
        fstp    dword [esp + 116]           
        fld     dword [eax + 16]
        fld     dword [eax + 108]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 60]            
        faddp   st1, st0
        fstp    dword [esp + 20]            
        fld     dword [eax + 20]
        fld     dword [eax + 104]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 12]            
        faddp   st1, st0
        fstp    dword [esp + 40]            
        fld     dword [eax + 24]
        fld     dword [eax + 100]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 56]            
        faddp   st1, st0
        fstp    dword [esp + 172]           
        fld     dword [eax + 28]
        fld     dword [eax + 96]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 72]            
        faddp   st1, st0
        fstp    dword [esp + 112]           
        fld     dword [eax + 32]
        fld     dword [eax + 92]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 52]            
        faddp   st1, st0
        fstp    dword [esp + 108]           
        fld     dword [eax + 36]
        fld     dword [eax + 88]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 24]            
        faddp   st1, st0
        fstp    dword [esp + 104]           
        fld     dword [eax + 40]
        fld     dword [eax + 84]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 48]            
        faddp   st1, st0
        fstp    dword [esp + 168]           
        fld     dword [eax + 44]
        fld     dword [eax + 80]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 8]             
        faddp   st1, st0
        fld     dword [eax + 48]
        fld     dword [eax + 76]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 44]            
        faddp   st1, st0
        fld     dword [eax + 52]
        fld     dword [eax + 72]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 68]            
        faddp   st1, st0
        fld     dword [eax + 56]
        fld     dword [eax + 68]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 176]           
        faddp   st1, st0
        fld     dword [eax + 60]
        fld     dword [eax + 64]
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 200]           
        faddp   st1, st0
        fld     dword [esp + 16]            
        fld     st1
        fadd    st0, st1
        fstp    dword [esp + 100]           
        fsubrp  st1, st0
        fstp    dword [esp + 16]            
        fld     dword [esp + 4]             
        fld     st1
        fadd    st0, st1
        fxch    st1
        fsubrp  st2, st0
        fxch    st1
        fstp    dword [esp + 4]             
        fld     dword [esp + 116]           
        fld     st3
        fadd    st0, st1
        fstp    dword [esp + 160]           
        fsubrp  st3, st0
        fxch    st2
        fstp    dword [esp + 164]           
        fld     dword [esp]                 
        fld     st1
        fadd    st0, st1
        fstp    dword [esp + 116]           
        fsubrp  st1, st0
        fstp    dword [esp]                 
        fld     dword [esp + 112]           
        fld     dword [esp + 108]           
        fld     st0
        fadd    st0, st2
        fxch    st2
        fsubrp  st1, st0
        fstp    dword [esp + 112]           
        fld     dword [esp + 172]           
        fld     dword [esp + 104]           
        fld     st0
        fadd    st0, st2
        fxch    st2
        fsubrp  st1, st0
        fstp    dword [esp + 104]           
        fld     dword [esp + 20]            
        fld     st4
        fadd    st0, st1
        fxch    st1
        fsubrp  st5, st0
        fxch    st4
        fstp    dword [esp + 156]           
        fld     dword [esp + 40]            
        fld     dword [esp + 168]           
        fld     st0
        fadd    st0, st2
        fxch    st2
        fsubrp  st1, st0
        fstp    dword [esp + 96]            
        fld     dword [esp + 100]           
        fld     st0
        fadd    st0, st4
        fstp    dword [esp + 20]            
        fsubrp  st3, st0
        fld     st3
        fadd    st0, st2
        fstp    dword [esp + 108]           
        fxch    st3
        fsubrp  st1, st0
        fstp    dword [esp + 40]            
        fld     dword [esp + 160]           
        fld     st0
        fadd    st0, st4
        fxch    st1
        fsubrp  st4, st0
        fxch    st3
        fstp    dword [esp + 92]            
        fld     dword [esp + 116]           
        fld     st0
        fadd    st0, st3
        fxch    st1
        fsubrp  st3, st0
        fxch    st2
        fstp    dword [esp + 88]            
        fld     dword [_mp2dec_FDCTCoeffs+8]
        fst     dword [esp + 160]           
        fld     dword [esp + 16]            
        fmulp   st1, st0
        fld     dword [_mp2dec_FDCTCoeffs+120]
        fst     dword [esp + 116]           
        fld     dword [esp + 112]           
        fmulp   st1, st0
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 16]            
        fsubp   st1, st0
        fstp    dword [esp + 100]           
        fld     dword [_mp2dec_FDCTCoeffs+24]
        fst     dword [esp + 196]           
        fld     dword [esp + 4]             
        fmulp   st1, st0
        fld     dword [_mp2dec_FDCTCoeffs+104]
        fst     dword [esp + 172]           
        fld     dword [esp + 104]           
        fmulp   st1, st0
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 152]           
        fsubp   st1, st0
        fstp    dword [esp + 4]             
        fld     dword [_mp2dec_FDCTCoeffs+56]
        fst     dword [esp + 192]           
        fld     dword [esp + 164]           
        fmulp   st1, st0
        fld     dword [_mp2dec_FDCTCoeffs+72]
        fst     dword [esp + 112]           
        fld     dword [esp + 156]           
        fmulp   st1, st0
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 80]            
        fsubp   st1, st0
        fstp    dword [esp + 84]            
        fld     dword [_mp2dec_FDCTCoeffs+40]
        fst     dword [esp + 156]           
        fld     dword [esp]                 
        fmulp   st1, st0
        fld     dword [_mp2dec_FDCTCoeffs+88]
        fst     dword [esp + 168]           
        fld     dword [esp + 96]            
        fmulp   st1, st0
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 76]            
        fsubp   st1, st0
        fstp    dword [esp + 96]            
        fld     dword [esp + 20]            
        fld     st0
        fadd    st0, st4
        fstp    dword [esp + 148]           
        fsubrp  st3, st0
        fxch    st2
        fstp    dword [esp]                 
        fld     dword [esp + 108]           
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 144]           
        fsubrp  st1, st0
        fstp    dword [esp + 132]           
        fld     dword [_mp2dec_FDCTCoeffs+16]
        fmul    st1, st0
        fld     st0
        fxch    st1
        fstp    dword [esp + 164]           
        fld     dword [_mp2dec_FDCTCoeffs+112]
        fld     dword [esp + 92]            
        fmul    st0, st1
        fxch    st1
        fst     dword [esp + 108]           
        fld     st1
        fadd    st0, st4
        fstp    dword [esp + 136]           
        fxch    st3
        fsubrp  st1, st0
        fstp    dword [esp + 140]           
        fld     dword [_mp2dec_FDCTCoeffs+48]
        fst     dword [esp + 20]            
        fld     dword [esp + 40]            
        fmulp   st1, st0
        fld     dword [esp + 20]            
        fstp    st0
        fld     dword [_mp2dec_FDCTCoeffs+80]
        fld     dword [esp + 88]            
        fmul    st0, st1
        fxch    st1
        fst     dword [esp + 104]           
        fld     st1
        fadd    st0, st3
        fstp    dword [esp + 124]           
        fxch    st2
        fsubrp  st1, st0
        fstp    dword [esp + 120]           
        fld     dword [esp + 16]            
        fld     dword [esp + 80]            
        fld     st0
        fadd    st0, st2
        fxch    st2
        fsubrp  st1, st0
        fstp    dword [esp + 16]            
        fld     dword [esp + 152]           
        fld     dword [esp + 76]            
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 128]           
        fsubp   st1, st0
        fstp    dword [esp + 76]            
        fld     dword [esp + 100]           
        fmulp   st3, st0
        fld     dword [esp + 84]            
        fmulp   st4, st0
        fld     st3
        fadd    st0, st3
        fstp    dword [esp + 80]            
        fxch    st2
        fsubrp  st3, st0
        fxch    st2
        fstp    dword [esp + 92]            
        fld     dword [esp + 4]             
        fld     dword [esp + 20]            
        fmulp   st1, st0
        fld     dword [esp + 96]            
        fmulp   st3, st0
        fld     st2
        fadd    st0, st1
        fstp    dword [esp + 84]            
        fsubrp  st2, st0
        fxch    st1
        fstp    dword [esp + 88]            
        fld     dword [esp + 148]           
        fld     dword [esp + 144]           
        fld     st1
        fadd    st0, st1
        fstp    dword [esp + 96]            
        fsubp   st1, st0
        fstp    dword [esp + 144]           
        fld     dword [_mp2dec_FDCTCoeffs+32]
        fld     dword [esp]                 
        fmul    st0, st1
        fld     dword [_mp2dec_FDCTCoeffs+96]
        fld     dword [esp + 132]           
        fmul    st0, st1
        fld     st0
        fadd    st0, st3
        fstp    dword [esp + 148]           
        fsubp   st2, st0
        fxch    st1
        fstp    dword [esp + 152]           
        fld     dword [esp + 136]           
        fld     dword [esp + 124]           
        fld     st0
        fadd    st0, st2
        fstp    dword [esp]                 
        fsubp   st1, st0
        fstp    dword [esp + 136]           
        fld     dword [esp + 140]           
        fmul    st0, st2
        fxch    st2
        fst     dword [esp + 40]            
        fld     dword [esp + 120]           
        fmul    st0, st2
        fld     st3
        fsub    st0, st1
        fstp    dword [esp + 132]           
        faddp   st3, st0
        fxch    st2
        fstp    dword [esp + 140]           
        fld     dword [esp + 128]           
        fld     st0
        fadd    st0, st4
        fstp    dword [esp + 4]             
        fsubp   st3, st0
        fld     dword [esp + 16]            
        fmulp   st2, st0
        fld     dword [esp + 40]            
        fstp    st0
        fld     dword [esp + 76]            
        fmul    st0, st1
        fld     st1
        fxch    st2
        fstp    dword [esp + 100]           
        fld     dword [_mp2dec_FDCTCoeffs+64]
        fst     dword [esp + 16]            
        fmul    st4, st0
        fxch    st4
        fst     dword [esp + 124]           
        fld     st3
        fsub    st0, st2
        fmulp   st5, st0
        fld     dword [esp + 4]             
        fadd    st1, st0
        fxch    st1
        faddp   st5, st0
        fxch    st4
        fstp    dword [esp + 76]            
        faddp   st2, st0
        fld     dword [esp + 80]            
        fld     dword [esp + 84]            
        fld     st0
        fadd    st0, st2
        fxch    st2
        fsubrp  st1, st0
        fstp    dword [esp + 184]           
        fld     dword [esp + 40]            
        fld     dword [esp + 92]            
        fmulp   st1, st0
        fld     dword [esp + 88]            
        fmulp   st3, st0
        fld     st2
        fadd    st0, st1
        fxch    st1
        fsubrp  st3, st0
        fxch    st2
        fstp    dword [esp + 188]           
        fxch    st3
        fchs
        fst     dword [esp + 128]           
        fsub    st0, st2
        fld     st2
        fxch    st3
        fstp    dword [esp + 180]           
        faddp   st1, st0
        fst     dword [esp + 120]           
        mov     ecx, dword [esp + 224]
        mov     edx, dword [esp + 216]
        mov     eax, dword [edx + 4*ecx + 4352]
        mov     esi, dword [edx + 4*ecx + 4360]
        fld     dword [esp + 36]            
        fmul    dword [_mp2dec_FDCTCoeffs+4]
        fstp    dword [esp + 80]            
        fld     dword [esp + 64]            
        fmul    dword [_mp2dec_FDCTCoeffs+12]
        fstp    dword [esp + 84]            
        fld     dword [esp + 32]            
        fmul    dword [_mp2dec_FDCTCoeffs+20]
        fstp    dword [esp + 92]            
        fld     dword [esp + 28]            
        fmul    dword [_mp2dec_FDCTCoeffs+28]
        fstp    dword [esp + 88]            
        fld     dword [esp + 60]            
        fmul    dword [_mp2dec_FDCTCoeffs+36]
        fstp    dword [esp + 64]            
        fld     dword [esp + 12]            
        fmul    dword [_mp2dec_FDCTCoeffs+44]
        fstp    dword [esp + 36]            
        fld     dword [esp + 56]            
        fmul    dword [_mp2dec_FDCTCoeffs+52]
        fstp    dword [esp + 28]            
        fld     dword [esp + 72]            
        fmul    dword [_mp2dec_FDCTCoeffs+60]
        fstp    dword [esp + 56]            
        fld     dword [esp + 52]            
        fmul    dword [_mp2dec_FDCTCoeffs+68]
        fstp    dword [esp + 72]            
        fld     dword [esp + 24]            
        fmul    dword [_mp2dec_FDCTCoeffs+76]
        fstp    dword [esp + 12]            
        fld     dword [esp + 48]            
        fmul    dword [_mp2dec_FDCTCoeffs+84]
        fstp    dword [esp + 32]            
        fld     dword [esp + 8]             
        fmul    dword [_mp2dec_FDCTCoeffs+92]
        fstp    dword [esp + 60]            
        fld     dword [esp + 44]            
        fmul    dword [_mp2dec_FDCTCoeffs+100]
        fstp    dword [esp + 24]            
        fld     dword [esp + 68]            
        fmul    dword [_mp2dec_FDCTCoeffs+108]
        fstp    dword [esp + 52]            
        fld     dword [esp + 176]           
        fmul    dword [_mp2dec_FDCTCoeffs+116]
        fstp    dword [esp + 48]            
        fld     dword [esp + 200]           
        fmul    dword [_mp2dec_FDCTCoeffs+124]
        fstp    dword [esp + 8]             
        fld     dword [esp + 16]            
        fld     dword [esp + 144]           
        fmul    st0, st1
        fst     dword [esp + 44]            
        imul    edi, ecx, 2176
        add     edi, edx
        imul    ecx, esi, 1088
        add     ecx, edi
        fstp    dword [ecx + 4*eax]
        fld     dword [esp + 124]           
        fsubr   st2, st0
        fxch    st2
        fstp    dword [ecx + 4*eax + 128]
        fld     dword [esp + 136]           
        fmul    st0, st1
        fld     dword [esp]                 
        fstp    st0
        fld     dword [esp]                 
        faddp   st1, st0
        fld     dword [esp + 132]           
        fmul    st0, st2
        fsub    st0, st1
        fstp    dword [esp + 68]            
        fld     dword [esp + 140]           
        fsub    st1, st0
        fxch    st1
        fstp    dword [ecx + 4*eax + 256]
        fld     dword [esp + 184]           
        fmul    st0, st2
        fadd    st0, st5
        fxch    st3
        faddp   st4, st0
        fld     dword [esp + 4]             
        fadd    st4, st0
        fxch    st4
        fsubr   st0, st3
        fstp    dword [ecx + 4*eax + 384]
        fld     dword [esp + 152]           
        fmul    st0, st2
        fld     dword [esp + 148]           
        fsub    st1, st0
        fxch    st1
        fstp    dword [ecx + 4*eax + 512]
        fld     dword [esp + 188]           
        fmulp   st3, st0
        fld     dword [esp + 76]            
        fsub    st3, st0
        fsubrp  st4, st0
        fxch    st3
        fstp    dword [ecx + 4*eax + 640]
        fld     dword [esp + 68]            
        fstp    dword [ecx + 4*eax + 768]
        fxch    st1
        fstp    dword [ecx + 4*eax + 896]
        fld     dword [esp + 96]            
        fmul    dword [.LCPI0_0]
        xor     edx, edx
        test    esi, esi
        sete    dl
        imul    edx, edx, 1088
        add     edx, edi
        fstp    dword [edx + 4*eax + 1024]
        fld     dword [esp + 128]           
        fstp    dword [edx + 4*eax + 896]
        fld     dword [esp]                 
        fld     st0
        fchs
        fstp    dword [edx + 4*eax + 768]
        fld     st3
        fsub    st0, st5
        fstp    dword [edx + 4*eax + 640]
        fxch    st2
        fchs
        fstp    dword [edx + 4*eax + 512]
        fld     dword [esp + 180]           
        faddp   st3, st0
        fxch    st3
        fsubrp  st2, st0
        fxch    st1
        fstp    dword [edx + 4*eax + 384]
        fsubrp  st1, st0
        fstp    dword [edx + 4*eax + 256]
        fld     dword [esp + 120]           
        fchs
        fstp    dword [edx + 4*eax + 128]
        fld     dword [esp + 44]            
        fchs
        fstp    dword [edx + 4*eax]
        fld     dword [esp + 80]            
        fld     dword [esp + 8]             
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 44]            
        fsubp   st1, st0
        fld     dword [esp + 160]           
        fmulp   st1, st0
        fstp    dword [esp]                 
        fld     dword [esp + 84]            
        fld     dword [esp + 48]            
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 8]             
        fsubp   st1, st0
        fld     dword [esp + 196]           
        fmulp   st1, st0
        fstp    dword [esp + 4]             
        fld     dword [esp + 88]            
        fld     dword [esp + 24]            
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 48]            
        fsubp   st1, st0
        fld     dword [esp + 192]           
        fmulp   st1, st0
        fstp    dword [esp + 24]            
        fld     dword [esp + 92]            
        fld     dword [esp + 52]            
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 68]            
        fsubp   st1, st0
        fld     dword [esp + 156]           
        fmulp   st1, st0
        fstp    dword [esp + 52]            
        fld     dword [esp + 56]            
        fld     dword [esp + 72]            
        fld     st0
        fadd    st0, st2
        fxch    st2
        fsubrp  st1, st0
        fld     dword [esp + 116]           
        fmulp   st1, st0
        fstp    dword [esp + 56]            
        fld     dword [esp + 28]            
        fld     dword [esp + 12]            
        fld     st0
        fadd    st0, st2
        fxch    st2
        fsubrp  st1, st0
        fld     dword [esp + 172]           
        fmulp   st1, st0
        fld     dword [esp + 64]            
        fld     dword [esp + 60]            
        fld     st0
        fadd    st0, st2
        fxch    st2
        fsubrp  st1, st0
        fld     dword [esp + 112]           
        fmulp   st1, st0
        fstp    dword [esp + 64]            
        fld     dword [esp + 36]            
        fld     dword [esp + 32]            
        fld     st0
        fadd    st0, st2
        fxch    st2
        fsubrp  st1, st0
        fld     dword [esp + 168]           
        fmulp   st1, st0
        fstp    dword [esp + 12]            
        fld     dword [esp + 44]            
        fld     st0
        fadd    st0, st6
        fstp    dword [esp + 36]            
        fsubrp  st5, st0
        fld     dword [esp + 8]             
        fld     st0
        fadd    st0, st5
        fstp    dword [esp + 32]            
        fsubrp  st4, st0
        fld     dword [esp + 48]            
        fld     st0
        fadd    st0, st3
        fstp    dword [esp + 8]             
        fsubrp  st2, st0
        fxch    st1
        fstp    dword [esp + 44]            
        fld     dword [esp + 68]            
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 28]            
        fsubrp  st1, st0
        fstp    dword [esp + 176]           
        fld     dword [esp]                 
        fld     dword [esp + 56]            
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 72]            
        fsubp   st1, st0
        fld     dword [esp + 164]           
        fmul    st4, st0
        fmulp   st1, st0
        fstp    dword [esp + 60]            
        fld     dword [esp + 4]             
        fld     st1
        fadd    st0, st1
        fstp    dword [esp + 48]            
        fsubrp  st1, st0
        fld     dword [esp + 20]            
        fmul    st2, st0
        fmulp   st1, st0
        fstp    dword [esp + 20]            
        fld     dword [esp + 24]            
        fld     dword [esp + 64]            
        fld     st0
        fadd    st0, st2
        fstp    dword [esp]                 
        fsubp   st1, st0
        fld     dword [esp + 108]           
        fld     dword [esp + 44]            
        fmul    st0, st1
        fxch    st1
        fmulp   st2, st0
        fxch    st1
        fstp    dword [esp + 68]            
        fld     dword [esp + 52]            
        fld     dword [esp + 12]            
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 24]            
        fsubp   st1, st0
        fld     dword [esp + 104]           
        fld     dword [esp + 176]           
        fmul    st0, st1
        fxch    st1
        fmulp   st2, st0
        fxch    st1
        fstp    dword [esp + 64]            
        fld     dword [esp + 36]            
        fld     dword [esp + 8]             
        fld     st1
        fadd    st0, st1
        fstp    dword [esp + 12]            
        fsubp   st1, st0
        fstp    dword [esp + 36]            
        fld     dword [esp + 32]            
        fld     dword [esp + 28]            
        fld     st1
        fadd    st0, st1
        fstp    dword [esp + 44]            
        fsubp   st1, st0
        fstp    dword [esp + 28]            
        fld     st1
        fadd    st0, st4
        fstp    dword [esp + 4]             
        fxch    st3
        fsubrp  st1, st0
        fld     st2
        fadd    st0, st2
        fstp    dword [esp + 52]            
        fxch    st1
        fsubrp  st2, st0
        fxch    st1
        fstp    dword [esp + 8]             
        fld     dword [esp + 72]            
        fld     dword [esp]                 
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 56]            
        fsubp   st1, st0
        fld     dword [esp + 48]            
        fld     dword [esp + 24]            
        fld     st0
        fadd    st0, st2
        fstp    dword [esp]                 
        fsubp   st1, st0
        fld     dword [esp + 60]            
        fld     dword [esp + 68]            
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 32]            
        fsubp   st1, st0
        fld     dword [esp + 40]            
        fld     dword [esp + 36]            
        fmul    st0, st1
        fxch    st5
        fmul    st0, st1
        fxch    st4
        fmul    st0, st1
        fstp    dword [esp + 24]            
        fmulp   st1, st0
        fstp    dword [esp + 36]            
        fld     dword [esp + 20]            
        fld     dword [esp + 64]            
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 72]            
        fsubp   st1, st0
        fld     dword [esp + 100]           
        fld     dword [esp + 28]            
        fmul    st0, st1
        fld     dword [esp + 8]             
        fmul    st0, st2
        fxch    st4
        fmul    st0, st2
        fstp    dword [esp + 8]             
        fxch    st2
        fmulp   st1, st0
        fstp    dword [esp + 60]            
        fld     dword [esp + 12]            
        fld     dword [esp + 44]            
        fld     st1
        fadd    st0, st1
        fxch    st2
        fsubrp  st1, st0
        fstp    dword [esp + 40]            
        fld     st1
        fadd    st0, st5
        fstp    dword [esp + 20]            
        fxch    st4
        fsubrp  st1, st0
        fstp    dword [esp + 28]            
        fld     dword [esp + 4]             
        fld     dword [esp + 52]            
        fld     st1
        fsub    st0, st1
        fstp    dword [esp + 12]            
        fxch    st1
        fadd    st0, st4
        faddp   st1, st0
        fstp    dword [esp + 64]            
        fld     st1
        fsub    st0, st1
        fxch    st2
        fadd    st0, st3
        fxch    st3
        fst     dword [esp + 48]            
        fxch    st3
        faddp   st1, st0
        fstp    dword [esp + 52]            
        fld     dword [esp + 16]            
        fld     dword [esp + 40]            
        fmul    st0, st1
        fst     dword [esp + 40]            
        fxch    st2
        fmul    st0, st1
        fxch    st2
        faddp   st3, st0
        fxch    st2
        faddp   st1, st0
        fstp    dword [esp + 4]             
        fld     dword [esp + 56]            
        fld     dword [esp]                 
        fld     st0
        fadd    st0, st2
        fxch    st2
        fsubrp  st1, st0
        fld     dword [esp + 24]            
        fld     dword [esp + 8]             
        fld     st1
        fsub    st0, st1
        fxch    st2
        fadd    st0, st4
        faddp   st1, st0
        fxch    st2
        fmul    st0, st4
        fst     dword [esp]                 
        fxch    st1
        fmul    st0, st4
        fxch    st1
        fadd    st0, st3
        faddp   st1, st0
        fstp    dword [esp + 56]            
        fld     dword [esp + 32]            
        fld     dword [esp + 72]            
        fld     st0
        fadd    st0, st2
        fstp    dword [esp + 8]             
        fsubp   st1, st0
        fstp    dword [esp + 32]            
        fld     dword [esp + 36]            
        fld     dword [esp + 60]            
        fld     st0
        fadd    st0, st2
        fxch    st2
        fsubrp  st1, st0
        fld     dword [esp + 28]            
        fmul    st0, st5
        fstp    dword [esp + 28]            
        fld     dword [esp + 12]            
        fmul    st0, st5
        fstp    dword [esp + 36]            
        fld     dword [esp + 32]            
        fmul    st0, st5
        fstp    dword [esp + 12]            
        fmulp   st4, st0
        fxch    st3
        fstp    dword [esp + 32]            
        fld     dword [esp + 20]            
        fld     dword [esp + 52]            
        fadd    st0, st1
        fsub    st4, st0
        fxch    st4
        fst     dword [esp + 16]            
        fchs
        fstp    dword [edx + 4*eax + 64]
        fld     st1
        fxch    st2
        fst     dword [esp + 24]            
        fsub    st0, st4
        fstp    dword [edx + 4*eax + 192]
        fld     dword [esp + 64]            
        fld     st1
        fadd    st0, st1
        fst     dword [esp + 60]            
        fsubrp  st3, st0
        fxch    st2
        fstp    dword [edx + 4*eax + 320]
        fld     dword [esp + 8]             
        fsub    st0, st2
        fsubr   st1, st0
        fxch    st1
        fstp    dword [edx + 4*eax + 448]
        fld     dword [esp + 12]            
        fadd    st0, st1
        fstp    dword [esp + 12]            
        fchs
        fstp    dword [edx + 4*eax + 576]
        fld     st1
        fsub    st0, st1
        fstp    dword [edx + 4*eax + 704]
        fld     dword [esp + 48]            
        fsubr   st2, st0
        fxch    st2
        fstp    dword [edx + 4*eax + 832]
        fxch    st1
        fchs
        fstp    dword [edx + 4*eax + 960]
        fld     dword [esp + 4]             
        fld     dword [esp + 28]            
        fadd    st1, st0
        fld     dword [esp + 32]            
        fsub    st0, st2
        fstp    dword [ecx + 4*eax + 960]
        fld     dword [esp + 56]            
        fsub    st2, st0
        fxch    st2
        fstp    dword [ecx + 4*eax + 832]
        fld     dword [esp + 40]            
        fld     dword [esp + 36]            
        fadd    st0, st1
        fxch    st4
        fadd    st0, st2
        fadd    st0, st4
        fsubp   st3, st0
        fxch    st2
        fstp    dword [ecx + 4*eax + 704]
        fld     dword [esp + 12]            
        fsub    st0, st3
        fsub    st1, st0
        fxch    st1
        fstp    dword [ecx + 4*eax + 576]
        fld     dword [esp + 20]            
        fsubp   st1, st0
        fstp    dword [ecx + 4*eax + 448]
        fld     dword [esp + 60]            
        faddp   st2, st0
        fld     dword [esp]                 
        fld     dword [esp + 24]            
        faddp   st1, st0
        fsub    st2, st0
        fxch    st2
        fstp    dword [ecx + 4*eax + 320]
        fadd    st2, st0
        fxch    st1
        fsubrp  st2, st0
        fxch    st1
        fstp    dword [ecx + 4*eax + 192]
        fld     dword [esp + 16]            
        fsubp   st1, st0
        fstp    dword [ecx + 4*eax + 64]
        add     esp, 204
        pop     esi
        pop     edi
        ret
        
.LCPI0_0:
        dd   0xc0000000                      ; float -2

%else 

; alternative gcc output
_mp2dec_poly_fdct_p5:
        push    esi
        push    ebx
        sub     esp, 124
        mov     eax, DWORD [esp+140]
        mov     esi, DWORD [esp+144]
        mov     ebx, DWORD [esp+136]
        fld     DWORD [eax]
        fld     DWORD [eax+124]
        fld     st1
        fsub    st0, st1
        lea     edx, [esi+1088]
        mov     ecx, DWORD [ebx+8+edx*4]
        fmul    DWORD [_mp2dec_FDCTCoeffs+4]
        fstp    DWORD [esp+48]
        faddp   st1, st0
        fstp    DWORD [esp+8]
        fld     DWORD [eax+4]
        fld     DWORD [eax+120]
        fld     st1
        fsub    st0, st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+12]
        fstp    DWORD [esp+52]
        faddp   st1, st0
        fstp    DWORD [esp+12]
        fld     DWORD [eax+8]
        fld     DWORD [eax+116]
        fld     st1
        fsub    st0, st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+20]
        fstp    DWORD [esp+56]
        faddp   st1, st0
        fstp    DWORD [esp+16]
        fld     DWORD [eax+12]
        fld     DWORD [eax+112]
        fld     st1
        fsub    st0, st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+28]
        fstp    DWORD [esp+60]
        faddp   st1, st0
        fstp    DWORD [esp+20]
        fld     DWORD [eax+16]
        fld     DWORD [eax+108]
        fld     st1
        fsub    st0, st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+36]
        fstp    DWORD [esp+64]
        faddp   st1, st0
        fstp    DWORD [esp+24]
        fld     DWORD [eax+20]
        fld     DWORD [eax+104]
        fld     st1
        fsub    st0, st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+44]
        fstp    DWORD [esp+68]
        faddp   st1, st0
        fstp    DWORD [esp+28]
        fld     DWORD [eax+24]
        fld     DWORD [eax+100]
        fld     st1
        fsub    st0, st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+52]
        fstp    DWORD [esp+72]
        faddp   st1, st0
        fstp    DWORD [esp+32]
        fld     DWORD [eax+28]
        fld     DWORD [eax+96]
        fld     st1
        fsub    st0, st1
        fld     DWORD [_mp2dec_FDCTCoeffs+60]
        fmulp   st1, st0
        fstp    DWORD [esp+76]
        faddp   st1, st0
        fstp    DWORD [esp+36]
        fld     DWORD [eax+32]
        fld     DWORD [eax+92]
        fld     st1
        fsub    st0, st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+68]
        fstp    DWORD [esp+80]
        faddp   st1, st0
        fstp    DWORD [esp+40]
        fld     DWORD [eax+36]
        fld     DWORD [eax+88]
        fld     st1
        fsub    st0, st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+76]
        fstp    DWORD [esp+84]
        faddp   st1, st0
        fld     DWORD [eax+40]
        fld     DWORD [eax+84]
        fld     st1
        fsub    st0, st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+84]
        fstp    DWORD [esp+4]
        faddp   st1, st0
        fstp    DWORD [esp]
        fld     DWORD [eax+44]
        fld     DWORD [eax+80]
        fld     st1
        fsub    st0, st1
        fld     DWORD [_mp2dec_FDCTCoeffs+92]
        fmulp   st1, st0
        fstp    DWORD [esp+88]
        faddp   st1, st0
        fld     DWORD [eax+48]
        fld     DWORD [eax+76]
        fld     st1
        fsub    st0, st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+100]
        fstp    DWORD [esp+92]
        faddp   st1, st0
        fld     DWORD [eax+52]
        fld     DWORD [eax+72]
        fld     st1
        fsub    st0, st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+108]
        fstp    DWORD [esp+96]
        faddp   st1, st0
        fld     DWORD [eax+56]
        fld     DWORD [eax+68]
        fld     st1
        fsub    st0, st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+116]
        fstp    DWORD [esp+100]
        faddp   st1, st0
        fld     DWORD [eax+60]
        fld     DWORD [eax+64]
        fld     st1
        fsub    st0, st1
        mov     eax, esi
        sal     eax, 4
        add     eax, esi
        sal     eax, 5
        fmul    DWORD [_mp2dec_FDCTCoeffs+124]
        fstp    DWORD [esp+104]
        faddp   st1, st0
        fld     DWORD [esp+8]
        fld     st0
        fadd    st0, st2
        fstp    DWORD [esp+8]
        fsubrp  st1, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+8]
        fmulp   st1, st0
        fstp    DWORD [esp+44]
        fld     DWORD [esp+12]
        fld     st0
        fadd    st0, st2
        fstp    DWORD [esp+12]
        fsubrp  st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+24]
        fstp    DWORD [esp+108]
        fld     DWORD [esp+20]
        fld     st0
        fadd    st0, st3
        fstp    DWORD [esp+20]
        fsubrp  st2, st0
        fxch    st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+56]
        fstp    DWORD [esp+112]
        fld     DWORD [esp+16]
        fld     st0
        fadd    st0, st2
        fstp    DWORD [esp+16]
        fsubrp  st1, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+40]
        fmulp   st1, st0
        fstp    DWORD [esp+116]
        fld     DWORD [esp+36]
        fld     st0
        fld     DWORD [esp+40]
        fadd    st1, st0
        fsubp   st2, st0
        fxch    st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+120]
        fld     DWORD [esp+32]
        fld     st0
        fadd    st0, st5
        fxch    st5
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+104]
        fld     DWORD [esp+24]
        fld     st0
        fadd    st0, st5
        fxch    st5
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+72]
        fld     DWORD [esp+28]
        fld     DWORD [esp]
        fadd    st0, st1
        fxch    st1
        fsub    DWORD [esp]
        fmul    DWORD [_mp2dec_FDCTCoeffs+88]
        fstp    DWORD [esp+24]
        fld     DWORD [esp+8]
        fadd    st0, st5
        fstp    DWORD [esp+28]
        fxch    st4
        fsubr   DWORD [esp+8]
        fmul    DWORD [_mp2dec_FDCTCoeffs+16]
        fstp    DWORD [esp+32]
        fld     DWORD [esp+12]
        fld     st0
        fadd    st0, st7
        fstp    DWORD [esp+12]
        fsubrp  st6, st0
        fxch    st5
        fmul    DWORD [_mp2dec_FDCTCoeffs+48]
        fstp    DWORD [esp+36]
        fld     DWORD [esp+20]
        fld     st0
        fadd    st0, st5
        fxch    st5
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+112]
        fld     DWORD [esp+16]
        fld     st0
        fadd    st0, st5
        fxch    st5
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+80]
        fstp    DWORD [esp+20]
        fld     DWORD [esp+44]
        fld     st0
        fadd    st0, st4
        fstp    DWORD [esp+40]
        fsubrp  st3, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+16]
        fmulp   st3, st0
        fxch    st2
        fstp    DWORD [esp+44]
        fld     DWORD [esp+108]
        fld     st0
        fadd    st0, st2
        fstp    DWORD [esp+108]
        fsubrp  st1, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+48]
        fmulp   st1, st0
        fstp    DWORD [esp+120]
        fld     DWORD [esp+112]
        fld     st0
        fadd    st0, st5
        fxch    st1
        fsubrp  st5, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+112]
        fmulp   st5, st0
        fxch    st4
        fstp    DWORD [esp]
        fld     DWORD [esp+116]
        fld     st0
        fld     DWORD [esp+24]
        fadd    st1, st0
        fsubp   st2, st0
        fxch    st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+80]
        fld     DWORD [esp+28]
        fld     st0
        fadd    st0, st6
        fstp    DWORD [esp+8]
        fsubrp  st5, st0
        fxch    st4
        fmul    DWORD [_mp2dec_FDCTCoeffs+32]
        fld     DWORD [esp+12]
        fld     st0
        fadd    st0, st5
        fstp    DWORD [esp+12]
        fsubrp  st4, st0
        fxch    st3
        fmul    DWORD [_mp2dec_FDCTCoeffs+96]
        fstp    DWORD [esp+112]
        fld     DWORD [esp+32]
        fld     st0
        fadd    st0, st3
        fstp    DWORD [esp+116]
        fsubrp  st2, st0
        fxch    st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+32]
        fstp    DWORD [esp+16]
        fld     DWORD [esp+36]
        fld     st0
        fld     DWORD [esp+20]
        fadd    st1, st0
        fsubp   st2, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+96]
        fmulp   st2, st0
        fxch    st1
        fstp    DWORD [esp+20]
        fld     DWORD [esp+40]
        fld     st0
        fadd    st0, st6
        fxch    st6
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+32]
        fld     DWORD [esp+108]
        fld     st0
        fadd    st0, st4
        fstp    DWORD [esp+40]
        fsubrp  st3, st0
        fxch    st2
        fmul    DWORD [_mp2dec_FDCTCoeffs+96]
        fld     DWORD [esp+44]
        fld     DWORD [esp]
        fadd    st0, st1
        fstp    DWORD [esp+44]
        fld     DWORD [esp]
        fsubp   st1, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+32]
        fmulp   st1, st0
        fstp    DWORD [esp+108]
        fld     DWORD [esp+120]
        fld     st0
        fadd    st0, st6
        fxch    st6
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+96]
        fstp    DWORD [esp+24]
        fld     DWORD [esp+8]
        fsub    DWORD [esp+12]
        fmul    DWORD [_mp2dec_FDCTCoeffs+64]
        fstp    DWORD [esp+28]
        fld     st3
        fld     DWORD [esp+112]
        fadd    st1, st0
        fxch    st1
        fstp    DWORD [esp+32]
        fsubp   st4, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+64]
        fmulp   st4, st0
        fxch    st3
        fstp    DWORD [esp+112]
        fld     DWORD [esp+116]
        fld     st0
        fadd    st0, st2
        fxch    st1
        fsubrp  st2, st0
        fxch    st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+64]
        fxch    st1
        fst     DWORD [esp+116]
        faddp   st1, st0
        fstp    DWORD [esp+120]
        fld     DWORD [esp+16]
        fadd    DWORD [esp+20]
        fstp    DWORD [esp+36]
        fld     DWORD [esp+40]
        fld     st0
        fadd    st0, st5
        fxch    st5
        fsubrp  st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+64]
        fld     st4
        fadd    st0, st1
        fld     st2
        fsub    st0, st4
        fld     DWORD [_mp2dec_FDCTCoeffs+64]
        fmulp   st1, st0
        fadd    st0, st1
        fxch    st4
        faddp   st3, st0
        fxch    st2
        fstp    DWORD [esp]
        fld     DWORD [esp+44]
        fld     st0
        fadd    st0, st5
        fxch    st5
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+64]
        fld     st5
        fchs
        fst     DWORD [esp+44]
        fsubrp  st2, st0
        fld     st1
        fadd    st0, st5
        faddp   st1, st0
        fld     DWORD [esp+108]
        fld     DWORD [esp+24]
        fadd    st0, st1
        fst     DWORD [esp+40]
        fxch    st1
        fsub    DWORD [esp+24]
        fmul    DWORD [_mp2dec_FDCTCoeffs+64]
        fstp    DWORD [esp+24]
        mov     esi, DWORD [ebx+edx*4]
        mov     edx, ecx
        sal     edx, 4
        add     eax, esi
        add     edx, ecx
        sal     edx, 4
        add     edx, eax
        test    ecx, ecx
        fld     DWORD [esp+28]
        lea     edx, [ebx+edx*4]
        sete    cl
        fstp    DWORD [edx]
        fxch    st3
        fadd    DWORD [esp]
        and     ecx, 255
        mov     esi, ecx
        sal     ecx, 4
        fsubrp  st3, st0
        fxch    st2
        add     ecx, esi
        sal     ecx, 4
        add     eax, ecx
        fstp    DWORD [edx+128]
        fld     DWORD [esp+120]
        fld     DWORD [esp+36]
        fsubr   st0, st1
        lea     eax, [ebx+eax*4]
        fstp    DWORD [edx+256]
        fld     DWORD [esp]
        fsubr   st0, st3
        fstp    DWORD [edx+384]
        fld     DWORD [esp+112]
        fsub    DWORD [esp+32]
        fstp    DWORD [edx+512]
        fxch    st1
        fadd    st0, st3
        fsubrp  st2, st0
        fxch    st1
        fstp    DWORD [edx+640]
        fld     DWORD [esp+16]
        fsub    DWORD [esp+20]
        fmul    DWORD [_mp2dec_FDCTCoeffs+64]
        fsubrp  st1, st0
        fstp    DWORD [edx+768]
        fsubr   DWORD [esp+24]
        fstp    DWORD [edx+896]
        fld     DWORD [esp+8]
        fadd    DWORD [esp+12]
        fmul    DWORD [.LC0]
        fstp    DWORD [eax+1024]
        fld     DWORD [esp+44]
        fstp    DWORD [eax+896]
        fld     DWORD [esp+116]
        fld     st0
        fchs
        fstp    DWORD [eax+768]
        fld     st2
        fsub    st0, st2
        fstp    DWORD [eax+640]
        fld     DWORD [esp+32]
        fchs
        fstp    DWORD [eax+512]
        fld     DWORD [esp]
        faddp   st3, st0
        fxch    st1
        fsub    st0, st2
        fstp    DWORD [eax+384]
        fsub    DWORD [esp+36]
        fstp    DWORD [eax+256]
        fsub    DWORD [esp+40]
        fstp    DWORD [eax+128]
        fld     DWORD [esp+28]
        fchs
        fstp    DWORD [eax]
        fld     DWORD [esp+48]
        fld     st0
        fld     DWORD [esp+104]
        fadd    st1, st0
        fxch    st1
        fstp    DWORD [esp]
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+8]
        fstp    DWORD [esp+8]
        fld     DWORD [esp+52]
        fld     st0
        fld     DWORD [esp+100]
        fadd    st1, st0
        fxch    st1
        fstp    DWORD [esp+12]
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+24]
        fstp    DWORD [esp+16]
        fld     DWORD [esp+60]
        fld     st0
        fld     DWORD [esp+92]
        fadd    st1, st0
        fxch    st1
        fstp    DWORD [esp+20]
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+56]
        fstp    DWORD [esp+24]
        fld     DWORD [esp+56]
        fld     st0
        fld     DWORD [esp+96]
        fadd    st1, st0
        fxch    st1
        fstp    DWORD [esp+28]
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+40]
        fstp    DWORD [esp+32]
        fld     DWORD [esp+76]
        fld     st0
        fld     DWORD [esp+80]
        fadd    st1, st0
        fsubp   st2, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+120]
        fmulp   st2, st0
        fld     DWORD [esp+72]
        fld     st0
        fld     DWORD [esp+84]
        fadd    st1, st0
        fsubp   st2, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+104]
        fmulp   st2, st0
        fld     DWORD [esp+64]
        fld     st0
        fld     DWORD [esp+88]
        fadd    st1, st0
        fsubp   st2, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+72]
        fmulp   st2, st0
        fld     DWORD [esp+68]
        fld     DWORD [esp+4]
        fadd    st0, st1
        fxch    st1
        fsub    DWORD [esp+4]
        fmul    DWORD [_mp2dec_FDCTCoeffs+88]
        fstp    DWORD [esp+4]
        fld     DWORD [esp]
        fadd    st0, st6
        fstp    DWORD [esp+36]
        fxch    st5
        fsubr   DWORD [esp]
        fmul    DWORD [_mp2dec_FDCTCoeffs+16]
        fstp    DWORD [esp]
        fld     DWORD [esp+12]
        fld     st0
        fadd    st0, st4
        fstp    DWORD [esp+12]
        fsubrp  st3, st0
        fxch    st2
        fmul    DWORD [_mp2dec_FDCTCoeffs+48]
        fstp    DWORD [esp+40]
        fld     DWORD [esp+20]
        fld     st0
        fadd    st0, st3
        fxch    st3
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+112]
        fld     DWORD [esp+28]
        fld     st0
        fadd    st0, st6
        fxch    st6
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+80]
        fstp    DWORD [esp+20]
        fld     DWORD [esp+8]
        fld     st0
        fadd    st0, st7
        fstp    DWORD [esp+28]
        fsubrp  st6, st0
        fxch    st5
        fmul    DWORD [_mp2dec_FDCTCoeffs+16]
        fstp    DWORD [esp+44]
        fld     DWORD [esp+16]
        fld     st0
        fadd    st0, st4
        fstp    DWORD [esp+16]
        fsubrp  st3, st0
        fxch    st2
        fmul    DWORD [_mp2dec_FDCTCoeffs+48]
        fstp    DWORD [esp+48]
        fld     DWORD [esp+24]
        fld     st0
        fadd    st0, st3
        fxch    st3
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+112]
        fld     DWORD [esp+32]
        fld     st0
        fld     DWORD [esp+4]
        fadd    st1, st0
        fsubp   st2, st0
        fxch    st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+80]
        fstp    DWORD [esp+24]
        fld     DWORD [esp+36]
        fld     st0
        fadd    st0, st4
        fstp    DWORD [esp+32]
        fsubrp  st3, st0
        fxch    st2
        fmul    DWORD [_mp2dec_FDCTCoeffs+32]
        fld     DWORD [esp+12]
        fld     st0
        fadd    st0, st6
        fstp    DWORD [esp+12]
        fsubrp  st5, st0
        fxch    st4
        fmul    DWORD [_mp2dec_FDCTCoeffs+96]
        fld     DWORD [esp]
        fld     st0
        fadd    st0, st7
        fstp    DWORD [esp+36]
        fsubrp  st6, st0
        fxch    st5
        fmul    DWORD [_mp2dec_FDCTCoeffs+32]
        fstp    DWORD [esp+4]
        fld     DWORD [esp+40]
        fld     st0
        fld     DWORD [esp+20]
        fadd    st1, st0
        fsubp   st2, st0
        fxch    st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+96]
        fstp    DWORD [esp+8]
        fld     DWORD [esp+28]
        fld     st0
        fadd    st0, st5
        fstp    DWORD [esp+20]
        fsubrp  st4, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+32]
        fmulp   st4, st0
        fxch    st3
        fstp    DWORD [esp+28]
        fld     DWORD [esp+16]
        fld     st0
        fadd    st0, st3
        fstp    DWORD [esp+16]
        fsubrp  st2, st0
        fxch    st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+96]
        fld     DWORD [esp+44]
        fld     st0
        fadd    st0, st3
        fstp    DWORD [esp+40]
        fsubrp  st2, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+32]
        fmulp   st2, st0
        fxch    st1
        fstp    DWORD [esp+44]
        fld     DWORD [esp+48]
        fld     st0
        fld     DWORD [esp+24]
        fadd    st1, st0
        fxch    st1
        fstp    DWORD [esp+24]
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+96]
        fstp    DWORD [esp+48]
        fld     DWORD [esp+32]
        fld     st0
        fld     DWORD [esp+12]
        fadd    st1, st0
        fsubp   st2, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+64]
        fmulp   st2, st0
        fld     st4
        fadd    st0, st6
        fstp    DWORD [esp]
        fxch    st4
        fsubrp  st5, st0
        fxch    st4
        fmul    DWORD [_mp2dec_FDCTCoeffs+64]
        fld     DWORD [esp+36]
        fld     st0
        fadd    st0, st4
        fxch    st4
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+64]
        fxch    st3
        fadd    st0, st4
        fld     st5
        fxch    st4
        faddp   st6, st0
        fxch    st5
        fstp    DWORD [esp+52]
        fld     DWORD [esp+4]
        fsub    DWORD [esp+8]
        fmul    DWORD [_mp2dec_FDCTCoeffs+64]
        fld     DWORD [esp]
        fadd    st0, st5
        fadd    DWORD [esp+4]
        fadd    DWORD [esp+8]
        fstp    DWORD [esp+4]
        fxch    st4
        fst     DWORD [esp+12]
        fxch    st3
        fst     DWORD [esp+32]
        faddp   st3, st0
        fst     DWORD [esp+36]
        faddp   st2, st0
        fxch    st1
        faddp   st2, st0
        fxch    st1
        fstp    DWORD [esp+56]
        fld     DWORD [esp+20]
        fld     st0
        fld     DWORD [esp+16]
        fadd    st1, st0
        fsubp   st2, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+64]
        fmulp   st2, st0
        fld     DWORD [esp+28]
        fld     st0
        fadd    st0, st4
        fxch    st4
        fsubp   st1, st0
        fmul    DWORD [_mp2dec_FDCTCoeffs+64]
        fxch    st3
        fadd    st0, st1
        fstp    DWORD [esp+8]
        fxch    st1
        fst     DWORD [esp+16]
        fadd    st0, st1
        faddp   st2, st0
        fxch    st1
        fstp    DWORD [esp+20]
        fld     DWORD [esp+40]
        fld     st0
        fld     DWORD [esp+24]
        fadd    st1, st0
        fsubp   st2, st0
        fld     DWORD [_mp2dec_FDCTCoeffs+64]
        fmulp   st2, st0
        fld     st0
        fsub    st0, st4
        fadd    st2, st0
        fxch    st2
        fstp    DWORD [esp+24]
        fld     DWORD [esp+44]
        fld     st0
        fld     DWORD [esp+48]
        fsub    st1, st0
        fxch    st1
        fmul    DWORD [_mp2dec_FDCTCoeffs+64]
        fxch    st2
        faddp   st1, st0
        fld     DWORD [esp+4]
        fsubr   st1, st0
        fxch    st1
        fst     DWORD [eax+64]
        fld     DWORD [esp+8]
        fsubrp  st2, st0
        fxch    st1
        fstp    DWORD [eax+192]
        fld     DWORD [esp]
        fadd    st0, st6
        fld     DWORD [esp+8]
        fsubr   st0, st1
        fstp    DWORD [eax+320]
        fxch    st4
        fsub    DWORD [esp]
        fstp    DWORD [eax+448]
        fxch    st2
        fsubr   st0, st5
        fstp    DWORD [eax+576]
        fld     st3
        fsub    st0, st5
        fstp    DWORD [eax+704]
        fld     DWORD [esp+12]
        fsubr   st4, st0
        fxch    st4
        fstp    DWORD [eax+832]
        fxch    st3
        fchs
        fstp    DWORD [eax+960]
        fld     DWORD [esp+56]
        fsub    st3, st0
        fxch    st3
        fstp    DWORD [edx+960]
        fld     DWORD [esp+20]
        fsub    st3, st0
        fxch    st3
        fstp    DWORD [edx+832]
        fld     DWORD [esp+36]
        fadd    st4, st0
        fld     DWORD [esp+52]
        fadd    st5, st0
        fxch    st5
        fsubp   st4, st0
        fxch    st3
        fstp    DWORD [edx+704]
        fxch    st2
        fadd    st0, st3
        fld     DWORD [esp+24]
        fsub    st1, st0
        fxch    st1
        fstp    DWORD [edx+576]
        fld     DWORD [esp]
        fadd    st0, st4
        fsubp   st1, st0
        fstp    DWORD [edx+448]
        fld     DWORD [esp+16]
        fsub    st3, st0
        fxch    st1
        faddp   st3, st0
        fld     DWORD [esp+8]
        fsub    st3, st0
        fxch    st3
        fstp    DWORD [edx+320]
        fld     DWORD [esp+32]
        fsub    st1, st0
        fxch    st1
        faddp   st3, st0
        fxch    st2
        fsub    DWORD [esp+4]
        fstp    DWORD [edx+192]
        faddp   st1, st0
        fstp    DWORD [edx+64]
        add     esp, 124
        pop     ebx
        pop     esi
        ret
.LC0    dd   -1073741824        

%endif
 
; dewindow mono/interleaved stereo channels - ported from gcc inline asm with a bit of trickery (better t put it into watcom pragma aux i guess? well no)
;mp2dec_poly_window_mono_p5(poly_private_data*, short*, int, int): # @mp2dec_poly_window_mono_p5(poly_private_data*, short*, int, int)
global _mp2dec_poly_window_mono_p5
_mp2dec_poly_window_mono_p5:
        push    ebp
        push    ebx
        push    edi
        push    esi
        sub     esp, 8
        mov     esi, dword [esp + 32]
        mov     ebp, dword [esp + 40]
        mov     eax, dword [esp + 36]
        mov     ecx, dword [esp + 28]
        mov     edx, dword [ecx + 4*eax + 4352]
        mov     dword [esp + 4], edx
        mov     edi, dword [ecx + 4*eax + 4360]
        mov     dword [esp], edi
        add     ebp, ebp
        imul    eax, eax, 2176
        add     eax, ecx
        imul    ebx, edi, 1088
        add     ebx, eax
        shl     edx, 2
        neg     edx
        lea     edx, [edx + _mp2dec_t_dewindow+64]
        mov     eax, 15
.Ltmp531:
        fld     dword [ebx]
        fmul    dword [edx]
        fld     dword [ebx + 4]
        fmul    dword [edx + 4]
        fld     dword [ebx + 8]
        fmul    dword [edx + 8]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 12]
        fmul    dword [edx + 12]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 16]
        fmul    dword [edx + 16]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 20]
        fmul    dword [edx + 20]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 24]
        fmul    dword [edx + 24]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 28]
        fmul    dword [edx + 28]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 32]
        fmul    dword [edx + 32]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 36]
        fmul    dword [edx + 36]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 40]
        fmul    dword [edx + 40]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 44]
        fmul    dword [edx + 44]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 48]
        fmul    dword [edx + 48]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 52]
        fmul    dword [edx + 52]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 56]
        fmul    dword [edx + 56]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 60]
        fmul    dword [edx + 60]
        fxch    st2
        faddp   st1, st0
        add     ebx, 64
        add     edx, 128
        sub     esp, 4
        faddp   st1, st0
        fistp   dword [esp]
        pop     ecx
        cmp     ecx, 32767
        jle     .Ltmp532
        mov     cx, 32767
        jmp     .Ltmp533
.Ltmp532:
        cmp     ecx, -32768
        jge     .Ltmp533
        mov     cx, -32768
.Ltmp533:
        mov     word [esi], cx
        add     esi, ebp
        dec     eax
        jns     .Ltmp531
        test    byte [esp], 1
        je      .Ltmp534
        fld     dword [ebx]
        fmul    dword [edx]
        fld     dword [ebx + 8]
        fmul    dword [edx + 8]
        fld     dword [ebx + 16]
        fmul    dword [edx + 16]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 24]
        fmul    dword [edx + 24]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 32]
        fmul    dword [edx + 32]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 40]
        fmul    dword [edx + 40]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 48]
        fmul    dword [edx + 48]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 56]
        fmul    dword [edx + 56]
        fxch    st2
        faddp   st1, st0
        sub     esp, 4
        sub     ebx, 64
        sub     edx, 192
        faddp   st1, st0
        fistp   dword [esp]
        pop     ecx
        cmp     ecx, 32767
        jle     .Ltmp535
        mov     cx, 32767
        jmp     .Ltmp536
.Ltmp535:
        cmp     ecx, -32768
        jge     .Ltmp536
        mov     cx, -32768
.Ltmp536:
        mov     word [esi], cx
        mov     ecx, dword [esp + 4]
        shl     ecx, 3
        add     edx, ecx
        add     esi, ebp
        mov     eax, 14
.Ltmp537:
        fld     dword [ebx + 4]
        fmul    dword [edx + 56]
        fld     dword [ebx]
        fmul    dword [edx + 60]
        fld     dword [ebx + 12]
        fmul    dword [edx + 48]
        fxch    st2
        fsubrp  st1, st0
        fld     dword [ebx + 8]
        fmul    dword [edx + 52]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 20]
        fmul    dword [edx + 40]
        fxch    st2
        fsubp   st1, st0
        fld     dword [ebx + 16]
        fmul    dword [edx + 44]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 28]
        fmul    dword [edx + 32]
        fxch    st2
        fsubp   st1, st0
        fld     dword [ebx + 24]
        fmul    dword [edx + 36]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 36]
        fmul    dword [edx + 24]
        fxch    st2
        fsubp   st1, st0
        fld     dword [ebx + 32]
        fmul    dword [edx + 28]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 44]
        fmul    dword [edx + 16]
        fxch    st2
        fsubp   st1, st0
        fld     dword [ebx + 40]
        fmul    dword [edx + 20]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 52]
        fmul    dword [edx + 8]
        fxch    st2
        fsubp   st1, st0
        fld     dword [ebx + 48]
        fmul    dword [edx + 12]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 60]
        fmul    dword [edx]
        fxch    st2
        fsubp   st1, st0
        fld     dword [ebx + 56]
        fmul    dword [edx + 4]
        fxch    st2
        faddp   st1, st0
        sub     ebx, 64
        sub     edx, 128
        sub     esp, 4
        fsubrp  st1, st0
        fistp   dword [esp]
        pop     ecx
        cmp     ecx, 32767
        jle     .Ltmp538
        mov     cx, 32767
        jmp     .Ltmp539
.Ltmp538:
        cmp     ecx, -32768
        jge     .Ltmp539
        mov     cx, -32768
.Ltmp539:
        mov     word [esi], cx
        add     esi, ebp
        dec     eax
        jns     .Ltmp537
        jmp     .Ltmp540
.Ltmp534:
        fld     dword [ebx + 4]
        fmul    dword [edx + 4]
        fld     dword [ebx + 12]
        fmul    dword [edx + 12]
        fld     dword [ebx + 20]
        fmul    dword [edx + 20]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 28]
        fmul    dword [edx + 28]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 36]
        fmul    dword [edx + 36]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 44]
        fmul    dword [edx + 44]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 52]
        fmul    dword [edx + 52]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 60]
        fmul    dword [edx + 60]
        fxch    st2
        faddp   st1, st0
        sub     esp, 4
        sub     ebx, 64
        sub     edx, 192
        faddp   st1, st0
        fistp   dword [esp]
        pop     ecx
        cmp     ecx, 32767
        jle     .Ltmp541
        mov     cx, 32767
        jmp     .Ltmp542
.Ltmp541:
        cmp     ecx, -32768
        jge     .Ltmp542
        mov     cx, -32768
.Ltmp542:
        mov     word [esi], cx
        mov     ecx, dword [esp + 4]
        shl     ecx, 3
        add     edx, ecx
        add     esi, ebp
        mov     eax, 14
.Ltmp543:
        fld     dword [ebx]
        fmul    dword [edx + 60]
        fld     dword [ebx + 4]
        fmul    dword [edx + 56]
        fld     dword [ebx + 8]
        fmul    dword [edx + 52]
        fxch    st2
        fsubrp  st1, st0
        fld     dword [ebx + 12]
        fmul    dword [edx + 48]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 16]
        fmul    dword [edx + 44]
        fxch    st2
        fsubp   st1, st0
        fld     dword [ebx + 20]
        fmul    dword [edx + 40]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 24]
        fmul    dword [edx + 36]
        fxch    st2
        fsubp   st1, st0
        fld     dword [ebx + 28]
        fmul    dword [edx + 32]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 32]
        fmul    dword [edx + 28]
        fxch    st2
        fsubp   st1, st0
        fld     dword [ebx + 36]
        fmul    dword [edx + 24]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 40]
        fmul    dword [edx + 20]
        fxch    st2
        fsubp   st1, st0
        fld     dword [ebx + 44]
        fmul    dword [edx + 16]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 48]
        fmul    dword [edx + 12]
        fxch    st2
        fsubp   st1, st0
        fld     dword [ebx + 52]
        fmul    dword [edx + 8]
        fxch    st2
        faddp   st1, st0
        fld     dword [ebx + 56]
        fmul    dword [edx + 4]
        fxch    st2
        fsubp   st1, st0
        fld     dword [ebx + 60]
        fmul    dword [edx]
        fxch    st2
        faddp   st1, st0
        sub     ebx, 64
        sub     edx, 128
        sub     esp, 4
        fsubrp  st1, st0
        fistp   dword [esp]
        pop     ecx
        cmp     ecx, 32767
        jle     .Ltmp544
        mov     cx, 32767
        jmp     .Ltmp545
.Ltmp544:
        cmp     ecx, -32768
        jge     .Ltmp545
        mov     cx, -32768
.Ltmp545:
        mov     word [esi], cx
        add     esi, ebp
        dec     eax
        jns     .Ltmp543
.Ltmp540:
        add     esp, 8
        pop     esi
        pop     edi
        pop     ebx
        pop     ebp
        ret