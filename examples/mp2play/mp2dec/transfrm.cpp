#pragma once

// layer2 FDCT/windowing stuff  - uses bits of amp player sources - original header info below:

/* this file is a part of amp software, (C) tomislav uzelac 1996,1997
*/

/* transform.h  tables galore
*
* Created by: tomislav uzelac  May 1996
* Last modified by: tomislav uzelac  Mar  1 97
*/

#include <stdint.h>
#include "transfrm.h"

// FDCT coefficients

float mp2dec_FDCTCoeffs[32] = {
2.0,
1.997590912,  1.990369453, 1.978353019,
1.961570560,  1.940062506, 1.913880671,
1.883088130,  1.847759065, 1.807978586,
1.763842529,  1.715457220, 1.662939225,
1.606415063,  1.546020907, 1.481902251,
1.414213562,  1.343117910, 1.268786568,
1.191398609,  1.111140466, 1.028205488,
0.942793474,  0.855110187, 0.765366865,
0.673779707,  0.580569355, 0.485960360,
0.390180644,  0.293460949, 0.196034281,
0.098135349,
};

// window coefficients
float mp2dec_t_dewindow[17][32] = { {
        0.000000000 ,-0.000442505 , 0.003250122 ,-0.007003784 , 0.031082153 ,-0.078628540 , 0.100311279 ,-0.572036743 ,
        1.144989014 , 0.572036743 , 0.100311279 , 0.078628540 , 0.031082153 , 0.007003784 , 0.003250122 , 0.000442505 ,
        0.000000000 ,-0.000442505 , 0.003250122 ,-0.007003784 , 0.031082153 ,-0.078628540 , 0.100311279 ,-0.572036743 ,
        1.144989014 , 0.572036743 , 0.100311279 , 0.078628540 , 0.031082153 , 0.007003784 , 0.003250122 , 0.000442505 ,
    },{
        -0.000015259 ,-0.000473022 , 0.003326416 ,-0.007919312 , 0.030517578 ,-0.084182739 , 0.090927124 ,-0.600219727 ,
        1.144287109 , 0.543823242 , 0.108856201 , 0.073059082 , 0.031478882 , 0.006118774 , 0.003173828 , 0.000396729 ,
        -0.000015259 ,-0.000473022 , 0.003326416 ,-0.007919312 , 0.030517578 ,-0.084182739 , 0.090927124 ,-0.600219727 ,
        1.144287109 , 0.543823242 , 0.108856201 , 0.073059082 , 0.031478882 , 0.006118774 , 0.003173828 , 0.000396729 ,
    },{
        -0.000015259 ,-0.000534058 , 0.003387451 ,-0.008865356 , 0.029785156 ,-0.089706421 , 0.080688477 ,-0.628295898 ,
        1.142211914 , 0.515609741 , 0.116577148 , 0.067520142 , 0.031738281 , 0.005294800 , 0.003082275 , 0.000366211 ,
        -0.000015259 ,-0.000534058 , 0.003387451 ,-0.008865356 , 0.029785156 ,-0.089706421 , 0.080688477 ,-0.628295898 ,
        1.142211914 , 0.515609741 , 0.116577148 , 0.067520142 , 0.031738281 , 0.005294800 , 0.003082275 , 0.000366211 ,
    },{
        -0.000015259 ,-0.000579834 , 0.003433228 ,-0.009841919 , 0.028884888 ,-0.095169067 , 0.069595337 ,-0.656219482 ,
        1.138763428 , 0.487472534 , 0.123474121 , 0.061996460 , 0.031845093 , 0.004486084 , 0.002990723 , 0.000320435 ,
        -0.000015259 ,-0.000579834 , 0.003433228 ,-0.009841919 , 0.028884888 ,-0.095169067 , 0.069595337 ,-0.656219482 ,
        1.138763428 , 0.487472534 , 0.123474121 , 0.061996460 , 0.031845093 , 0.004486084 , 0.002990723 , 0.000320435 ,
    },{
        -0.000015259 ,-0.000625610 , 0.003463745 ,-0.010848999 , 0.027801514 ,-0.100540161 , 0.057617187 ,-0.683914185 ,
        1.133926392 , 0.459472656 , 0.129577637 , 0.056533813 , 0.031814575 , 0.003723145 , 0.002899170 , 0.000289917 ,
        -0.000015259 ,-0.000625610 , 0.003463745 ,-0.010848999 , 0.027801514 ,-0.100540161 , 0.057617187 ,-0.683914185 ,
        1.133926392 , 0.459472656 , 0.129577637 , 0.056533813 , 0.031814575 , 0.003723145 , 0.002899170 , 0.000289917 ,
    },{
        -0.000015259 ,-0.000686646 , 0.003479004 ,-0.011886597 , 0.026535034 ,-0.105819702 , 0.044784546 ,-0.711318970 ,
        1.127746582 , 0.431655884 , 0.134887695 , 0.051132202 , 0.031661987 , 0.003005981 , 0.002792358 , 0.000259399 ,
        -0.000015259 ,-0.000686646 , 0.003479004 ,-0.011886597 , 0.026535034 ,-0.105819702 , 0.044784546 ,-0.711318970 ,
        1.127746582 , 0.431655884 , 0.134887695 , 0.051132202 , 0.031661987 , 0.003005981 , 0.002792358 , 0.000259399 ,
    },{
        -0.000015259 ,-0.000747681 , 0.003479004 ,-0.012939453 , 0.025085449 ,-0.110946655 , 0.031082153 ,-0.738372803 ,
        1.120223999 , 0.404083252 , 0.139450073 , 0.045837402 , 0.031387329 , 0.002334595 , 0.002685547 , 0.000244141 ,
        -0.000015259 ,-0.000747681 , 0.003479004 ,-0.012939453 , 0.025085449 ,-0.110946655 , 0.031082153 ,-0.738372803 ,
        1.120223999 , 0.404083252 , 0.139450073 , 0.045837402 , 0.031387329 , 0.002334595 , 0.002685547 , 0.000244141 ,
    },{
        -0.000030518 ,-0.000808716 , 0.003463745 ,-0.014022827 , 0.023422241 ,-0.115921021 , 0.016510010 ,-0.765029907 ,
        1.111373901 , 0.376800537 , 0.143264771 , 0.040634155 , 0.031005859 , 0.001693726 , 0.002578735 , 0.000213623 ,
        -0.000030518 ,-0.000808716 , 0.003463745 ,-0.014022827 , 0.023422241 ,-0.115921021 , 0.016510010 ,-0.765029907 ,
        1.111373901 , 0.376800537 , 0.143264771 , 0.040634155 , 0.031005859 , 0.001693726 , 0.002578735 , 0.000213623 ,
    },{
        -0.000030518 ,-0.000885010 , 0.003417969 ,-0.015121460 , 0.021575928 ,-0.120697021 , 0.001068115 ,-0.791213989 ,
        1.101211548 , 0.349868774 , 0.146362305 , 0.035552979 , 0.030532837 , 0.001098633 , 0.002456665 , 0.000198364 ,
        -0.000030518 ,-0.000885010 , 0.003417969 ,-0.015121460 , 0.021575928 ,-0.120697021 , 0.001068115 ,-0.791213989 ,
        1.101211548 , 0.349868774 , 0.146362305 , 0.035552979 , 0.030532837 , 0.001098633 , 0.002456665 , 0.000198364 ,
    },{
        -0.000030518 ,-0.000961304 , 0.003372192 ,-0.016235352 , 0.019531250 ,-0.125259399 ,-0.015228271 ,-0.816864014 ,
        1.089782715 , 0.323318481 , 0.148773193 , 0.030609131 , 0.029937744 , 0.000549316 , 0.002349854 , 0.000167847 ,
        -0.000030518 ,-0.000961304 , 0.003372192 ,-0.016235352 , 0.019531250 ,-0.125259399 ,-0.015228271 ,-0.816864014 ,
        1.089782715 , 0.323318481 , 0.148773193 , 0.030609131 , 0.029937744 , 0.000549316 , 0.002349854 , 0.000167847 ,
    },{
        -0.000030518 ,-0.001037598 , 0.003280640 ,-0.017349243 , 0.017257690 ,-0.129562378 ,-0.032379150 ,-0.841949463 ,
        1.077117920 , 0.297210693 , 0.150497437 , 0.025817871 , 0.029281616 , 0.000030518 , 0.002243042 , 0.000152588 ,
        -0.000030518 ,-0.001037598 , 0.003280640 ,-0.017349243 , 0.017257690 ,-0.129562378 ,-0.032379150 ,-0.841949463 ,
        1.077117920 , 0.297210693 , 0.150497437 , 0.025817871 , 0.029281616 , 0.000030518 , 0.002243042 , 0.000152588 ,
    },{
        -0.000045776 ,-0.001113892 , 0.003173828 ,-0.018463135 , 0.014801025 ,-0.133590698 ,-0.050354004 ,-0.866363525 ,
        1.063217163 , 0.271591187 , 0.151596069 , 0.021179199 , 0.028533936 ,-0.000442505 , 0.002120972 , 0.000137329 ,
        -0.000045776 ,-0.001113892 , 0.003173828 ,-0.018463135 , 0.014801025 ,-0.133590698 ,-0.050354004 ,-0.866363525 ,
        1.063217163 , 0.271591187 , 0.151596069 , 0.021179199 , 0.028533936 ,-0.000442505 , 0.002120972 , 0.000137329 ,
    },{
        -0.000045776 ,-0.001205444 , 0.003051758 ,-0.019577026 , 0.012115479 ,-0.137298584 ,-0.069168091 ,-0.890090942 ,
        1.048156738 , 0.246505737 , 0.152069092 , 0.016708374 , 0.027725220 ,-0.000869751 , 0.002014160 , 0.000122070 ,
        -0.000045776 ,-0.001205444 , 0.003051758 ,-0.019577026 , 0.012115479 ,-0.137298584 ,-0.069168091 ,-0.890090942 ,
        1.048156738 , 0.246505737 , 0.152069092 , 0.016708374 , 0.027725220 ,-0.000869751 , 0.002014160 , 0.000122070 ,
    },{
        -0.000061035 ,-0.001296997 , 0.002883911 ,-0.020690918 , 0.009231567 ,-0.140670776 ,-0.088775635 ,-0.913055420 ,
        1.031936646 , 0.221984863 , 0.151962280 , 0.012420654 , 0.026840210 ,-0.001266479 , 0.001907349 , 0.000106812 ,
        -0.000061035 ,-0.001296997 , 0.002883911 ,-0.020690918 , 0.009231567 ,-0.140670776 ,-0.088775635 ,-0.913055420 ,
        1.031936646 , 0.221984863 , 0.151962280 , 0.012420654 , 0.026840210 ,-0.001266479 , 0.001907349 , 0.000106812 ,
    },{
        -0.000061035 ,-0.001388550 , 0.002700806 ,-0.021789551 , 0.006134033 ,-0.143676758 ,-0.109161377 ,-0.935195923 ,
        1.014617920 , 0.198059082 , 0.151306152 , 0.008316040 , 0.025909424 ,-0.001617432 , 0.001785278 , 0.000106812 ,
        -0.000061035 ,-0.001388550 , 0.002700806 ,-0.021789551 , 0.006134033 ,-0.143676758 ,-0.109161377 ,-0.935195923 ,
        1.014617920 , 0.198059082 , 0.151306152 , 0.008316040 , 0.025909424 ,-0.001617432 , 0.001785278 , 0.000106812 ,
    },{
        -0.000076294 ,-0.001480103 , 0.002487183 ,-0.022857666 , 0.002822876 ,-0.146255493 ,-0.130310059 ,-0.956481934 ,
        0.996246338 , 0.174789429 , 0.150115967 , 0.004394531 , 0.024932861 ,-0.001937866 , 0.001693726 , 0.000091553 ,
        -0.000076294 ,-0.001480103 , 0.002487183 ,-0.022857666 , 0.002822876 ,-0.146255493 ,-0.130310059 ,-0.956481934 ,
        0.996246338 , 0.174789429 , 0.150115967 , 0.004394531 , 0.024932861 ,-0.001937866 , 0.001693726 , 0.000091553 ,
    },{
        -0.000076294 ,-0.001586914 , 0.002227783 ,-0.023910522 ,-0.000686646 ,-0.148422241 ,-0.152206421 ,-0.976852417 ,
        0.976852417 , 0.152206421 , 0.148422241 , 0.000686646 , 0.023910522 ,-0.002227783 , 0.001586914 , 0.000076294 ,
        -0.000076294 ,-0.001586914 , 0.002227783 ,-0.023910522 ,-0.000686646 ,-0.148422241 ,-0.152206421 ,-0.976852417 ,
        0.976852417 , 0.152206421 , 0.148422241 , 0.000686646 , 0.023910522 ,-0.002227783 , 0.001586914 , 0.000076294 ,
    } };


#ifdef MP2DEC_TRANSFORM_C

/* fast DCT according to Lee[84]
* reordering according to Konstantinides[94]
*/
void _cdecl mp2dec_poly_fdct(mp2dec_poly_private_data *data, float *subbands, const int ch)
{

    int start = data->u_start[ch];
    int div = data->u_div[ch];
    float(*u_p)[16];

    {
        float d16, d17, d18, d19, d20, d21, d22, d23, d24, d25, d26, d27, d28, d29, d30, d31;
        float d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15;

        /* step 1: initial reordering and 1st (16 wide) butterflies
        */

        d0 = subbands[0]; d16 = (d0 - subbands[31]) * mp2dec_FDCTCoeffs[1]; d0 += subbands[31];
        d1 = subbands[1]; d17 = (d1 - subbands[30]) * mp2dec_FDCTCoeffs[3]; d1 += subbands[30];
        d3 = subbands[2]; d19 = (d3 - subbands[29]) * mp2dec_FDCTCoeffs[5]; d3 += subbands[29];
        d2 = subbands[3]; d18 = (d2 - subbands[28]) * mp2dec_FDCTCoeffs[7]; d2 += subbands[28];
        d6 = subbands[4]; d22 = (d6 - subbands[27]) * mp2dec_FDCTCoeffs[9]; d6 += subbands[27];
        d7 = subbands[5]; d23 = (d7 - subbands[26]) * mp2dec_FDCTCoeffs[11]; d7 += subbands[26];
        d5 = subbands[6]; d21 = (d5 - subbands[25]) * mp2dec_FDCTCoeffs[13]; d5 += subbands[25];
        d4 = subbands[7]; d20 = (d4 - subbands[24]) * mp2dec_FDCTCoeffs[15]; d4 += subbands[24];
        d12 = subbands[8]; d28 = (d12 - subbands[23]) * mp2dec_FDCTCoeffs[17]; d12 += subbands[23];
        d13 = subbands[9]; d29 = (d13 - subbands[22]) * mp2dec_FDCTCoeffs[19]; d13 += subbands[22];
        d15 = subbands[10]; d31 = (d15 - subbands[21]) * mp2dec_FDCTCoeffs[21]; d15 += subbands[21];
        d14 = subbands[11]; d30 = (d14 - subbands[20]) * mp2dec_FDCTCoeffs[23]; d14 += subbands[20];
        d10 = subbands[12]; d26 = (d10 - subbands[19]) * mp2dec_FDCTCoeffs[25]; d10 += subbands[19];
        d11 = subbands[13]; d27 = (d11 - subbands[18]) * mp2dec_FDCTCoeffs[27]; d11 += subbands[18];
        d9 = subbands[14]; d25 = (d9 - subbands[17]) * mp2dec_FDCTCoeffs[29]; d9 += subbands[17];
        d8 = subbands[15]; d24 = (d8 - subbands[16]) * mp2dec_FDCTCoeffs[31]; d8 += subbands[16];

        {
            float c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15;

            /* a test to see what can be done with memory separation
            * first we process indexes 0-15
            */
            c0 = d0 + d8; c8 = (d0 - d8) *  mp2dec_FDCTCoeffs[2];
            c1 = d1 + d9; c9 = (d1 - d9) *  mp2dec_FDCTCoeffs[6];
            c2 = d2 + d10; c10 = (d2 - d10) * mp2dec_FDCTCoeffs[14];
            c3 = d3 + d11; c11 = (d3 - d11) * mp2dec_FDCTCoeffs[10];
            c4 = d4 + d12; c12 = (d4 - d12) * mp2dec_FDCTCoeffs[30];
            c5 = d5 + d13; c13 = (d5 - d13) * mp2dec_FDCTCoeffs[26];
            c6 = d6 + d14; c14 = (d6 - d14) * mp2dec_FDCTCoeffs[18];
            c7 = d7 + d15; c15 = (d7 - d15) * mp2dec_FDCTCoeffs[22];

            /* step 3: 4-wide butterflies
            */
            d0 = c0 + c4; d4 = (c0 - c4) * mp2dec_FDCTCoeffs[4];
            d1 = c1 + c5; d5 = (c1 - c5) * mp2dec_FDCTCoeffs[12];
            d2 = c2 + c6; d6 = (c2 - c6) * mp2dec_FDCTCoeffs[28];
            d3 = c3 + c7; d7 = (c3 - c7) * mp2dec_FDCTCoeffs[20];

            d8 = c8 + c12; d12 = (c8 - c12) * mp2dec_FDCTCoeffs[4];
            d9 = c9 + c13; d13 = (c9 - c13) * mp2dec_FDCTCoeffs[12];
            d10 = c10 + c14; d14 = (c10 - c14) * mp2dec_FDCTCoeffs[28];
            d11 = c11 + c15; d15 = (c11 - c15) * mp2dec_FDCTCoeffs[20];


            /* step 4: 2-wide butterflies
            */
            {
                float rb8 = mp2dec_FDCTCoeffs[8];
                float rb24 = mp2dec_FDCTCoeffs[24];

                /**/	c0 = d0 + d2; c2 = (d0 - d2) *  rb8;
                c1 = d1 + d3; c3 = (d1 - d3) * rb24;
                /**/	c4 = d4 + d6; c6 = (d4 - d6) *  rb8;
                c5 = d5 + d7; c7 = (d5 - d7) * rb24;
                /**/	c8 = d8 + d10; c10 = (d8 - d10) *  rb8;
                c9 = d9 + d11; c11 = (d9 - d11) * rb24;
                /**/	c12 = d12 + d14; c14 = (d12 - d14) *  rb8;
                c13 = d13 + d15; c15 = (d13 - d15) * rb24;
            }

            /* step 5: 1-wide butterflies
            */
            {
                float rb16 = mp2dec_FDCTCoeffs[16];

                /* this is a little 'hacked up'
                */
                d0 = (-c0 - c1) * 2; d1 = (c0 - c1) * rb16;
                d2 = c2 + c3; d3 = (c2 - c3) * rb16;
                d3 -= d2;

                d4 = c4 + c5; d5 = (c4 - c5) * rb16;
                d5 += d4;
                d7 = -d5;
                d7 += (c6 - c7) * rb16; d6 = +c6 + c7;

                d8 = c8 + c9; d9 = (c8 - c9) * rb16;
                d11 = +d8 + d9;
                d11 += (c10 - c11) * rb16; d10 = c10 + c11;

                d12 = c12 + c13; d13 = (c12 - c13) * rb16;
                d13 += -d8 - d9 + d12;
                d14 = c14 + c15; d15 = (c14 - c15) * rb16;
                d15 -= d11;
                d14 += -d8 - d10;
            }

            /* step 6: final resolving & reordering
            * the other 32 are stored for use with the next granule
            */

            u_p = (float(*)[16]) &data->u[ch][div][0][start];

            /*16*/  u_p[0][0] = +d1;
            u_p[2][0] = +d9 - d14;
            /*20*/  u_p[4][0] = +d5 - d6;
            u_p[6][0] = -d10 + d13;
            /*24*/  u_p[8][0] = d3;
            u_p[10][0] = -d8 - d9 + d11 - d13;
            /*28*/  u_p[12][0] = +d7;
            u_p[14][0] = +d15;

            /* the other 32 are stored for use with the next granule
            */

            u_p = (float(*)[16]) &data->u[ch][!div][0][start];

            /*0*/   u_p[16][0] = d0;
            u_p[14][0] = -(+d8);
            /*4*/   u_p[12][0] = -(+d4);
            u_p[10][0] = -(-d8 + d12);
            /*8*/   u_p[8][0] = -(+d2);
            u_p[6][0] = -(+d8 + d10 - d12);
            /*12*/  u_p[4][0] = -(-d4 + d6);
            u_p[2][0] = -d14;
            u_p[0][0] = -d1;
        }

        {
            float c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11, c12, c13, c14, c15;

            /* memory separation, second part
            */
            /* 2
            */
            c0 = d16 + d24; c8 = (d16 - d24) *  mp2dec_FDCTCoeffs[2];
            c1 = d17 + d25; c9 = (d17 - d25) *  mp2dec_FDCTCoeffs[6];
            c2 = d18 + d26; c10 = (d18 - d26) * mp2dec_FDCTCoeffs[14];
            c3 = d19 + d27; c11 = (d19 - d27) * mp2dec_FDCTCoeffs[10];
            c4 = d20 + d28; c12 = (d20 - d28) * mp2dec_FDCTCoeffs[30];
            c5 = d21 + d29; c13 = (d21 - d29) * mp2dec_FDCTCoeffs[26];
            c6 = d22 + d30; c14 = (d22 - d30) * mp2dec_FDCTCoeffs[18];
            c7 = d23 + d31; c15 = (d23 - d31) * mp2dec_FDCTCoeffs[22];

            /* 3
            */
            d16 = c0 + c4; d20 = (c0 - c4) * mp2dec_FDCTCoeffs[4];
            d17 = c1 + c5; d21 = (c1 - c5) * mp2dec_FDCTCoeffs[12];
            d18 = c2 + c6; d22 = (c2 - c6) * mp2dec_FDCTCoeffs[28];
            d19 = c3 + c7; d23 = (c3 - c7) * mp2dec_FDCTCoeffs[20];

            d24 = c8 + c12; d28 = (c8 - c12) * mp2dec_FDCTCoeffs[4];
            d25 = c9 + c13; d29 = (c9 - c13) * mp2dec_FDCTCoeffs[12];
            d26 = c10 + c14; d30 = (c10 - c14) * mp2dec_FDCTCoeffs[28];
            d27 = c11 + c15; d31 = (c11 - c15) * mp2dec_FDCTCoeffs[20];

            /* 4
            */
            {
                float rb8 = mp2dec_FDCTCoeffs[8];
                float rb24 = mp2dec_FDCTCoeffs[24];

                /**/    c0 = d16 + d18; c2 = (d16 - d18) *  rb8;
                c1 = d17 + d19; c3 = (d17 - d19) * rb24;
                /**/    c4 = d20 + d22; c6 = (d20 - d22) *  rb8;
                c5 = d21 + d23; c7 = (d21 - d23) * rb24;
                /**/    c8 = d24 + d26; c10 = (d24 - d26) *  rb8;
                c9 = d25 + d27; c11 = (d25 - d27) * rb24;
                /**/    c12 = d28 + d30; c14 = (d28 - d30) *  rb8;
                c13 = d29 + d31; c15 = (d29 - d31) * rb24;
            }

            /* 5
            */
            {
                float rb16 = mp2dec_FDCTCoeffs[16];
                d16 = c0 + c1; d17 = (c0 - c1) * rb16;
                d18 = c2 + c3; d19 = (c2 - c3) * rb16;

                d20 = c4 + c5; d21 = (c4 - c5) * rb16;
                d20 += d16; d21 += d17;
                d22 = c6 + c7; d23 = (c6 - c7) * rb16;
                d22 += d16; d22 += d18;
                d23 += d16; d23 += d17; d23 += d19;


                d24 = c8 + c9; d25 = (c8 - c9) * rb16;
                d26 = c10 + c11; d27 = (c10 - c11) * rb16;
                d26 += d24;
                d27 += d24; d27 += d25;

                d28 = c12 + c13; d29 = (c12 - c13) * rb16;
                d28 -= d20; d29 += d28; d29 -= d21;
                d30 = c14 + c15; d31 = (c14 - c15) * rb16;
                d30 -= d22;
                d31 -= d23;
            }

            /* step 6: final resolving & reordering
            * the other 32 are stored for use with the next granule
            */

            u_p = (float(*)[16]) &data->u[ch][!div][0][start];

            u_p[1][0] = -(+d30);
            u_p[3][0] = -(+d22 - d26);
            u_p[5][0] = -(-d18 - d20 + d26);
            u_p[7][0] = -(+d18 - d28);
            u_p[9][0] = -(+d28);
            u_p[11][0] = -(+d20 - d24);
            u_p[13][0] = -(-d16 + d24);
            u_p[15][0] = -(+d16);

            /* the other 32 are stored for use with the next granule
            */

            u_p = (float(*)[16]) &data->u[ch][div][0][start];

            u_p[15][0] = +d31;
            u_p[13][0] = +d23 - d27;
            u_p[11][0] = -d19 - d20 - d21 + d27;
            u_p[9][0] = +d19 - d29;
            u_p[7][0] = -d18 + d29;
            u_p[5][0] = +d18 + d20 + d21 - d25 - d26;
            u_p[3][0] = -d17 - d22 + d25 + d26;
            u_p[1][0] = +d17 - d30;
        }
    }

    // separate me?
    // indeed separate! :)
}

#define PUT_SAMPLE_MONO(out) *samples++ = (out > 32767 ? 32767 : (out < -32768 ? 32768 : out));

#if 1
#define PUT_SAMPLE_STEREO(left, right) \
    *samples++ = (left  > 32767 ? 32767 : (left  < -32768 ? 32768 : left )); \
    *samples++ = (right > 32767 ? 32767 : (right < -32768 ? 32768 : right));
#else
// typecast samples to uint32_t, write both samples at once
// actually 16bit writes are merged/reordered in write buffers anyway, so i doubt that trick would speed thigs up
#define PUT_SAMPLE_STEREO(left, right) \
    *((uint32_t*)samples) = ((short)(left  > 32767 ? 32767 : (left  < -32768 ? 32768 : left))) | ((int)((short)(right > 32767 ? 32767 : (right < -32768 ? 32768 : right))) << 16); samples += 2;
#endif

// dewindow both (L/R) samples , requires data->u_start/data->u_div to be in sync between channels!
void _cdecl mp2dec_poly_window(mp2dec_poly_private_data *data, int16_t *samples) {

    int start = data->u_start[0];
    int div = data->u_div[0];
    float(*u_p)[16];
    /* we're doing dewindowing and calculating final samples now
    */

    const float *dewindow = mp2dec_t_dewindow[0] + 16 - start;

    /* These optimisations are tuned specifically for architectures
    without autoincrement and -decrement. */

    int out, j;

#define WINDOW_REG(res) float res##_l, res##_r;

#define WINDOW_MUL(res, pos, windowpos) \
        res##_l = u_ptr_l[0][pos] * dewindow[windowpos]; \
        res##_r = u_ptr_r[0][pos] * dewindow[windowpos]; \

#define WINDOW_MAD(res, pos, windowpos) \
        res##_l += u_ptr_l[0][pos] * dewindow[windowpos]; \
        res##_r += u_ptr_r[0][pos] * dewindow[windowpos]; \

    {

        float(*u_ptr_l)[16] = data->u[0][div];
        float(*u_ptr_r)[16] = data->u[1][div];

        for (j = 0; j < 16; ++j) {
            // maximize fpu regs usage (hope clang can into p5 fxch magic!)
            WINDOW_REG(outf1);
            WINDOW_REG(outf2);
            WINDOW_REG(outf3);
            WINDOW_REG(outf4);

            WINDOW_MUL(outf1, 0, 0);
            WINDOW_MUL(outf2, 1, 1);
            WINDOW_MUL(outf3, 2, 2);
            WINDOW_MUL(outf4, 3, 3);
            WINDOW_MAD(outf1, 4, 4);
            WINDOW_MAD(outf2, 5, 5);
            WINDOW_MAD(outf3, 6, 6);
            WINDOW_MAD(outf4, 7, 7);
            WINDOW_MAD(outf1, 8, 8);
            WINDOW_MAD(outf2, 9, 9);
            WINDOW_MAD(outf3, 10, 10);
            WINDOW_MAD(outf4, 11, 11);
            WINDOW_MAD(outf1, 12, 12);
            WINDOW_MAD(outf2, 13, 13);
            WINDOW_MAD(outf3, 14, 14);
            WINDOW_MAD(outf4, 15, 15);

            float left  = outf1_l + outf2_l + outf3_l + outf4_l;
            float right = outf1_r + outf2_r + outf3_r + outf4_r;

            dewindow += 32;
            u_ptr_l++; u_ptr_r++;

            PUT_SAMPLE_STEREO(left, right)
        }

        if (div & 0x1) {
            {

                WINDOW_REG(outf1);
                WINDOW_REG(outf2);
                WINDOW_REG(outf3);
                WINDOW_REG(outf4);

                WINDOW_MUL(outf1, 0, 0);
                WINDOW_MUL(outf2, 2, 2);
                WINDOW_MUL(outf3, 4, 4);
                WINDOW_MUL(outf4, 6, 6);
                WINDOW_MAD(outf1, 8, 8);
                WINDOW_MAD(outf2, 10, 10);
                WINDOW_MAD(outf3, 12, 12);
                WINDOW_MAD(outf4, 14, 14);

                float left = outf1_l + outf2_l + outf3_l + outf4_l;
                float right = outf1_r + outf2_r + outf3_r + outf4_r;

                PUT_SAMPLE_STEREO(left, right)
            }

            dewindow -= 48;
            dewindow += start;
            dewindow += start;          // wtf??

            for (; j < 31; ++j) {
                WINDOW_REG(outf1);
                WINDOW_REG(outf2);
                WINDOW_REG(outf3);
                WINDOW_REG(outf4);

                --u_ptr_l; --u_ptr_r;

                /// uuuuggggghhhh...
                WINDOW_MUL(outf1, 0, 15);
                WINDOW_MUL(outf2, 1, 14);
                WINDOW_MUL(outf3, 2, 13);
                WINDOW_MUL(outf4, 3, 12);
                WINDOW_MAD(outf1, 4, 11);
                WINDOW_MAD(outf2, 5, 10);
                WINDOW_MAD(outf3, 6, 9);
                WINDOW_MAD(outf4, 7, 8);
                WINDOW_MAD(outf1, 8, 7);
                WINDOW_MAD(outf2, 9, 6);
                WINDOW_MAD(outf3, 10, 5);
                WINDOW_MAD(outf4, 11, 4);
                WINDOW_MAD(outf1, 12, 3);
                WINDOW_MAD(outf2, 13, 2);
                WINDOW_MAD(outf3, 14, 1);
                WINDOW_MAD(outf4, 15, 0);

                // note the signs!
                float left = -outf1_l + outf2_l - outf3_l + outf4_l;
                float right = -outf1_r + outf2_r - outf3_r + outf4_r;

                dewindow -= 32;

                PUT_SAMPLE_STEREO(left, right)
            }
        }
        else {
            {
                WINDOW_REG(outf1);
                WINDOW_REG(outf2);
                WINDOW_REG(outf3);
                WINDOW_REG(outf4);

                WINDOW_MUL(outf1, 1, 1);
                WINDOW_MUL(outf2, 3, 3);
                WINDOW_MUL(outf3, 5, 5);
                WINDOW_MUL(outf4, 7, 7);
                WINDOW_MAD(outf1, 9, 9);
                WINDOW_MAD(outf2, 11, 11);
                WINDOW_MAD(outf3, 13, 13);
                WINDOW_MAD(outf4, 15, 15);

                float left = outf1_l + outf2_l + outf3_l + outf4_l;
                float right = outf1_r + outf2_r + outf3_r + outf4_r;

                PUT_SAMPLE_STEREO(left, right)
                
            }

            dewindow -= 48;
            dewindow += start;
            dewindow += start;

            for (; j < 31; ++j) {
                WINDOW_REG(outf1);
                WINDOW_REG(outf2);
                WINDOW_REG(outf3);
                WINDOW_REG(outf4);

                --u_ptr_l; --u_ptr_r;

                /// uuuuggggghhhh...
                WINDOW_MUL(outf1, 0, 15);
                WINDOW_MUL(outf2, 1, 14);
                WINDOW_MUL(outf3, 2, 13);
                WINDOW_MUL(outf4, 3, 12);
                WINDOW_MAD(outf1, 4, 11);
                WINDOW_MAD(outf2, 5, 10);
                WINDOW_MAD(outf3, 6, 9);
                WINDOW_MAD(outf4, 7, 8);
                WINDOW_MAD(outf1, 8, 7);
                WINDOW_MAD(outf2, 9, 6);
                WINDOW_MAD(outf3, 10, 5);
                WINDOW_MAD(outf4, 11, 4);
                WINDOW_MAD(outf1, 12, 3);
                WINDOW_MAD(outf2, 13, 2);
                WINDOW_MAD(outf3, 14, 1);
                WINDOW_MAD(outf4, 15, 0);

                // note the signs!
                float left = outf1_l - outf2_l + outf3_l - outf4_l;
                float right = outf1_r - outf2_r + outf3_r - outf4_r;

                dewindow -= 32;

                PUT_SAMPLE_STEREO(left, right)
            }
        }
    }
}

// dewindow mono samples
void _cdecl mp2dec_poly_window_mono(mp2dec_poly_private_data *data, int16_t *samples, const int ch) {

    int start = data->u_start[ch];
    int div = data->u_div[ch];
    float(*u_p)[16];
    /* we're doing dewindowing and calculating final samples now
    */

    const float *dewindow = mp2dec_t_dewindow[0] + 16 - start;

    /* These optimisations are tuned specifically for architectures
    without autoincrement and -decrement. */

    int out, j;

#define WINDOW_REG(res) float res;

#define WINDOW_MUL(res, pos, windowpos) \
        res = u_ptr[0][pos] * dewindow[windowpos]; \

#define WINDOW_MAD(res, pos, windowpos) \
        res += u_ptr[0][pos] * dewindow[windowpos]; \

    {

        float(*u_ptr)[16] = data->u[ch][div];

        for (j = 0; j < 16; ++j) {
            // maximize fpu regs usage (hope clang can into p5 fxch magic!)
            WINDOW_REG(outf1);
            WINDOW_REG(outf2);
            WINDOW_REG(outf3);
            WINDOW_REG(outf4);

            WINDOW_MUL(outf1, 0, 0);
            WINDOW_MUL(outf2, 1, 1);
            WINDOW_MUL(outf3, 2, 2);
            WINDOW_MUL(outf4, 3, 3);
            WINDOW_MAD(outf1, 4, 4);
            WINDOW_MAD(outf2, 5, 5);
            WINDOW_MAD(outf3, 6, 6);
            WINDOW_MAD(outf4, 7, 7);
            WINDOW_MAD(outf1, 8, 8);
            WINDOW_MAD(outf2, 9, 9);
            WINDOW_MAD(outf3, 10, 10);
            WINDOW_MAD(outf4, 11, 11);
            WINDOW_MAD(outf1, 12, 12);
            WINDOW_MAD(outf2, 13, 13);
            WINDOW_MAD(outf3, 14, 14);
            WINDOW_MAD(outf4, 15, 15);

            float left = outf1 + outf2 + outf3 + outf4;

            dewindow += 32;
            u_ptr++;

            PUT_SAMPLE_MONO(left)
        }

        if (div & 0x1) {
            {

                WINDOW_REG(outf1);
                WINDOW_REG(outf2);
                WINDOW_REG(outf3);
                WINDOW_REG(outf4);

                WINDOW_MUL(outf1, 0, 0);
                WINDOW_MUL(outf2, 2, 2);
                WINDOW_MUL(outf3, 4, 4);
                WINDOW_MUL(outf4, 6, 6);
                WINDOW_MAD(outf1, 8, 8);
                WINDOW_MAD(outf2, 10, 10);
                WINDOW_MAD(outf3, 12, 12);
                WINDOW_MAD(outf4, 14, 14);

                float left = outf1 + outf2 + outf3 + outf4;

                PUT_SAMPLE_MONO(left)
            }

            dewindow -= 48;
            dewindow += start;
            dewindow += start;          // wtf??

            for (; j < 31; ++j) {
                WINDOW_REG(outf1);
                WINDOW_REG(outf2);
                WINDOW_REG(outf3);
                WINDOW_REG(outf4);

                --u_ptr;

                /// uuuuggggghhhh...
                WINDOW_MUL(outf1, 0, 15);
                WINDOW_MUL(outf2, 1, 14);
                WINDOW_MUL(outf3, 2, 13);
                WINDOW_MUL(outf4, 3, 12);
                WINDOW_MAD(outf1, 4, 11);
                WINDOW_MAD(outf2, 5, 10);
                WINDOW_MAD(outf3, 6, 9);
                WINDOW_MAD(outf4, 7, 8);
                WINDOW_MAD(outf1, 8, 7);
                WINDOW_MAD(outf2, 9, 6);
                WINDOW_MAD(outf3, 10, 5);
                WINDOW_MAD(outf4, 11, 4);
                WINDOW_MAD(outf1, 12, 3);
                WINDOW_MAD(outf2, 13, 2);
                WINDOW_MAD(outf3, 14, 1);
                WINDOW_MAD(outf4, 15, 0);

                // note the signs!
                float left = -outf1 + outf2 - outf3 + outf4;

                dewindow -= 32;

                PUT_SAMPLE_MONO(left)
            }
        }
        else {
            {
                WINDOW_REG(outf1);
                WINDOW_REG(outf2);
                WINDOW_REG(outf3);
                WINDOW_REG(outf4);

                WINDOW_MUL(outf1, 1, 1);
                WINDOW_MUL(outf2, 3, 3);
                WINDOW_MUL(outf3, 5, 5);
                WINDOW_MUL(outf4, 7, 7);
                WINDOW_MAD(outf1, 9, 9);
                WINDOW_MAD(outf2, 11, 11);
                WINDOW_MAD(outf3, 13, 13);
                WINDOW_MAD(outf4, 15, 15);

                float left = outf1 + outf2 + outf3 + outf4;

                PUT_SAMPLE_MONO(left, right)

            }

            dewindow -= 48;
            dewindow += start;
            dewindow += start;

            for (; j < 31; ++j) {
                WINDOW_REG(outf1);
                WINDOW_REG(outf2);
                WINDOW_REG(outf3);
                WINDOW_REG(outf4);

                --u_ptr;

                /// uuuuggggghhhh...
                WINDOW_MUL(outf1, 0, 15);
                WINDOW_MUL(outf2, 1, 14);
                WINDOW_MUL(outf3, 2, 13);
                WINDOW_MUL(outf4, 3, 12);
                WINDOW_MAD(outf1, 4, 11);
                WINDOW_MAD(outf2, 5, 10);
                WINDOW_MAD(outf3, 6, 9);
                WINDOW_MAD(outf4, 7, 8);
                WINDOW_MAD(outf1, 8, 7);
                WINDOW_MAD(outf2, 9, 6);
                WINDOW_MAD(outf3, 10, 5);
                WINDOW_MAD(outf4, 11, 4);
                WINDOW_MAD(outf1, 12, 3);
                WINDOW_MAD(outf2, 13, 2);
                WINDOW_MAD(outf3, 14, 1);
                WINDOW_MAD(outf4, 15, 0);

                // note the signs!
                float left = outf1 - outf2 + outf3 - outf4;

                dewindow -= 32;

                PUT_SAMPLE_MONO(left)
            }
        }
    }
}

#endif

void mp2dec_poly_next_granule(mp2dec_poly_private_data *data, const int ch) {
    --data->u_start[ch];
    data->u_start[ch] &= 0xf;
    data->u_div[ch] = data->u_div[ch] ? 0 : 1;
}

void mp2dec_poly_premultiply()
{
    int i, t;

    // test for already premultiplied
    if (mp2dec_t_dewindow[0][8] > 2.0) return;

    for (i = 0; i < 17; ++i)
        for (t = 0; t < 32; ++t)
            mp2dec_t_dewindow[i][t] *= 16383.5f;
}

void mp2dec_samples_monofy(float *left, float *right) {
    for (int smp = 0; smp < 12 * 3 * 32; smp += 12) {
        *(left +  0) = 0.5 * (*(left +  0) + *(right +  0));
        *(left +  1) = 0.5 * (*(left +  1) + *(right +  1));
        *(left +  2) = 0.5 * (*(left +  2) + *(right +  2));
        *(left +  3) = 0.5 * (*(left +  3) + *(right +  3));
        *(left +  4) = 0.5 * (*(left +  4) + *(right +  4));
        *(left +  5) = 0.5 * (*(left +  5) + *(right +  5));
        *(left +  6) = 0.5 * (*(left +  6) + *(right +  6));
        *(left +  7) = 0.5 * (*(left +  7) + *(right +  7));
        *(left +  8) = 0.5 * (*(left +  8) + *(right +  8));
        *(left +  9) = 0.5 * (*(left +  9) + *(right +  9));
        *(left + 10) = 0.5 * (*(left + 10) + *(right + 10));
        *(left + 11) = 0.5 * (*(left + 11) + *(right + 11));
        left += 12; right += 12;
        //*left = 0.5 * (*left + *right++); left++; right++;
    }
}

void mp2dec_samples_mono2stereo(int16_t* out) {
    for (int smp = 0; smp < 12 * 3 * 32; smp += 12) {
        *(out +  1) = *(out +  0); *(out +  3) = *(out +  2);
        *(out +  5) = *(out +  4); *(out +  7) = *(out +  6);
        *(out +  9) = *(out +  8); *(out + 11) = *(out + 10);
        *(out + 13) = *(out + 12); *(out + 15) = *(out + 14);
        *(out + 17) = *(out + 16); *(out + 19) = *(out + 18);
        *(out + 21) = *(out + 20); *(out + 23) = *(out + 22);
        out += 24;
    }
}