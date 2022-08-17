quick glossary:

* DMA block refers to contiguous memory region (i.e 4 kB) used for ISA DMA transfers, divided by two or more DMA buffers. The ISA DMA controller is programmed for transfer size of entire DMA block, and the soundcard is programmed for transfer size of one DMA buffer, thus we can track which current buffer is playing now and update other buffers in the background



bugs:

 - SB 2.0/Pro driver:
    - hispeed mode: fix pause/resume (cmd 0xD4 doesn't work for hispeed transfers and 0x90 resets current transfer count, thus SB goes out of sync with i8237)
      possible solution: rewind i8237 to start of current DMA buffer, but it's complicated because if playing DMA buffer is not first in the DMA block (see glossary if your mind exploding right now :). we need to re-setup i8237 when the SB finishes to play last DMA buffer

fixed:

 - high IRQs (8-15) doesn't work?
   - it was chipset blocking IRQs assigned to PCI devices from being triggered by ISA
 - EMM386:
   - pagefaults on exit *fixed - unlock every locked memory region on exit*
   - SB2/Pro driver sometimes can't detect my CT4520 (while SB16 driver does) *DSP reset fixed, I forgot to poll "data available" flag while waiting for 0xAA in the read buffer; this should also fix stuck 0xAA bug in the first sbDspRead() call*  

* REGRESSION: IRQ routine crashes under Win9x (stack fault in sound callback, since Windows calls ISR on separate stack segment with non-zero base) - *fixed by changing stack switch code so no local variables are used*
* ymf715 - autodetect hangs if ULTRA16 var is empty - *fixed - messed up with caps and IRQ detection*

won't fix

- non-DMA ISR jitter issues (PC Speaker sounds like scratched vinyl record, while Covox sounds way better)
- Windows XP NTVDM:
  - Dual Covox/Stereo-on-1 detection always succeeds (finding two LPT ports at 0x378 and 0x278, and selecting the latter)
  - SB 2.0 driver horribly stutters under Windows XP NTVDM, broken DMA position tracking - *use VDMSound or DOSBox. no, really*



OH NOES that page fault is back!

