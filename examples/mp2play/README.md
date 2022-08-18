# sndlib->mp2 player

**NOTE:** it's MPEG 1 Audio Layer II (aka **MP2**) player, it WON'T play any **MP3** files! 

TODO: more documentation, see source/makefile for now. basically you need to add both mp2play and mp2dec folders to include path and link with both ` mp2play.lib` and `mp2dec\mp2dec.lib`

needs FPU, mostly Pentium optimized, not optimized for AMD/Cyrix, think twice before trying it on 486 :)

3.6% mono / 6.7% stereo CPU load on Pentium 200, 1.8% mono / 3% stereo on Celeron 300A 



FDCT/windowing based on [amp](http://www.mp3-tech.org/programmer/sources/amp-0_7_6.tgz) sources ((C) tomislav uzelac 1996,1997), mp2 stream decoder and getbits stuff by me.



TODO: license. basically all files except `mp2dec\transfrm.c` and `mp2dec\mp2dec_a.asm` are licensed under following terms of MIT license:

```
Copyright (c) 2022 Artem Vasilev - wbcbz7

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```



`mp2dec\transfrm.c` and `mp2dec\mp2dec_a.asm` are derived from amp player and additionally licensed under following terms:

```
This software can be used freely for any purpose. It can be distributed
freely, as long as it is not sold commercially without permission from
Tomislav Uzelac <tuzelac@rasip.fer.hr>. However, including this software
on CD_ROMs containing other free software is explicitly permitted even 
when a modest distribution fee is charged for the CD, as long as this
software is not a primary selling argument for the CD.

Building derived versions of this software is permitted, as long as they
are not sold commercially without permission from Tomislav Uzelac 
<tuzelac@rasip.fer.hr>. Any derived versions must be clearly marked as
such, and must be called by a name other than amp. Any derived versions
must retain this copyright notice.

/* This license is itself copied from Tatu Ylonen's ssh package. It does 
 * not mention being copyrighted itself :)
 */

THERE IS NO WARRANTY FOR THIS PROGRAM - whatsoever. You use it entirely
at your risk, and neither Tomislav Uzelac, nor FER will be liable for
any damages that might occur to your computer, software, etc. in
consequence of you using this freeware program.
```



--wbcbz7 16.o8.2o22