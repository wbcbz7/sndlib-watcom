#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern float mp2dec_FDCTCoeffs[32];
extern float mp2dec_t_dewindow[17][32];

#pragma pack(push, 1)
struct mp2dec_poly_private_data {
    float u[2][2][17][16]; /* no v[][], it's redundant */
    int u_start[2]; /* first element of u[][] */
    int u_div[2]; /* which part of u[][] is currently used */
};
#pragma pack(pop)

// monofy samples
void mp2dec_samples_monofy(float *left, float *right);
// convert mono to stereo
void mp2dec_samples_mono2stereo(int16_t* out);
// feed FDCT pipeline
#ifdef MP2DEC_TRANSFORM_C
void _cdecl mp2dec_poly_fdct(mp2dec_poly_private_data *data, float *subbands, const int ch);
#endif
void _cdecl mp2dec_poly_fdct_p5(mp2dec_poly_private_data *data, float *subbands, const int ch);
// dewindow and output samples 
#ifdef MP2DEC_TRANSFORM_C
void _cdecl mp2dec_poly_window(mp2dec_poly_private_data *data, int16_t *samples);
void _cdecl mp2dec_poly_window_mono(mp2dec_poly_private_data *data, int16_t *samples, const int ch = 0);
#endif
void _cdecl mp2dec_poly_window_mono_p5(mp2dec_poly_private_data *data, int16_t *samples, const int ch = 0, const int channels = 1);
// advance pipeline
void mp2dec_poly_next_granule(mp2dec_poly_private_data *data, const int ch);
// prepare tables
void mp2dec_poly_premultiply();


#ifdef __cplusplus
}
#endif
