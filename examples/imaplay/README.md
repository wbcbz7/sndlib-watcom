# sndlib->ima adpcm wave player

plays IMA ADPCM-compressed .wav files. designed for 486 and early Pentiums.

currently in alpha, plays stereo files fine, occasionally clicks on mono files, otherwise seems to be stable enough. use at own risk :) Note that .wav file is loaded in memory at once, trackloading is not added yet.

~0.4% mono / 0.8% stereo CPU load on Pentium 100. my 486 motherboard is broken yet so I can't test on those machines. 

TODO: more documentation, see source/makefile for now. basically you need to add mp2play folder to include path and link with `imaplay.lib` . 



--wbcbz7 17.o8.2o22