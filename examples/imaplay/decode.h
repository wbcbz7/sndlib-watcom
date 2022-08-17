#pragma once

extern "C" {
    extern int16_t  imaplay_nextstep_table[16][128];
    extern int16_t  imaplay_diff_table[16][128];

    // decode one IMA ADPCM block
    uint32_t imaplay_decode_mono(int16_t * out, uint8_t * in, uint32_t samples);
    uint32_t imaplay_decode_stereo(int16_t * out, uint8_t * in, uint32_t samples);

    #pragma aux imaplay_decode_mono   "__*" parm caller [edi] [esi] [ecx] value [eax] modify [eax ebx ecx edx esi edi]
    #pragma aux imaplay_decode_stereo "__*" parm caller [edi] [esi] [ecx] value [eax] modify [eax ebx ecx edx esi edi]
}
