# .: (the almighty) 32-bit MS-DOS digital sound library for Open Watcom C++ :.

totally work in progress



--wbcbz7 o9.o6.2o22







# supported sound devices

## a. non-DMA devices

implemented by reprogramming IRQ0 at sample rate, then installing custom bi-modal (separate real and protected mode) handler to push raw PCM samples to sound device at the sample rate.

- pros: no ISA DMA/bus mastering mess, available on almost every PC (even modern one)
- cons: questionable sound quality (IRQ0 jitter, low bit depth, generally mono only), practically doesn't work in multitasking environments (Windows 3.x/9x/NT+ DOS box limits IRQ0 frequency to 1024 Hz, others will probably refuse to work at all)

### 1. PC Speaker (PWM aka RealSound)

* **sample rates:**  up to 48 kHz, though anything above 20 kHz sounds grungy and dirty

* **sample bit depth**: 5..7 bits, less for higher sample rates due to PWM nature

* **sample channels:** mono only

* **HW resources:** i8254 timer 2 in mode 0 (interrupt on terminal count) + port 0x61 for speaker control

  extremely prone to IRQ0 jitter/latency, any CPU mode switches during DOS/BIOS calls obviously result in occasional crackling and whining, giving a scratched vinyl record-like appearance. someone can find that aesthetic, but most of you would quickly dump it for a better sound device :)

  although timer values written to the i8254 are 8-bit, they need to be rescaled by the conversion table depending on the stream sample rate; created upon `open()`, and passed to conversion routine as `parm2`

### 2. Covox Speech Thing aka LPT DAC

- **sample rates:**  up to 48 kHz and beyond

- **sample bit depth**: 8 bits unsigned, "real" bit depth depends on DAC linearity and other factors

- **sample channels:** mono only

- **HW resources:** one 8-bit I/O port, scanned from BIOS Data Area LPT1/2/3

  the definite sound device choice back in the late-80s/early-90s for those who weren't able to afford a soundcard but have a dozen of resistors, soldering equipment and a bit of free time :) advanced variants include discrete DACs in place of R-2R ladder, stereo capability, audio sampling, etc...

  sounds considerably better that PC Speaker (one might note it even performs better than early SB cards!), less prone to IRQ0 jitter; decent choice for laptops

  by default, `detect()` scans BIOS Data Area for available LPT ports, and if there are any, picks the last one in the list (i.e. if you have LPT1 at 0x378 and LPT2 at 0x278, it'll pick 0x278); aside for that, no I/O read/write checks are applied (write-only ports would work fine). as a sidenote, supporting "custom" Covox at non-standard port detection is no more than writing it's base port to the BDA LPT list

### 3. Dual Covox DAC

- **sample rates:**  up to 48 kHz and beyond

- **sample bit depth**: 8 bits unsigned, "real" bit depth depends on DAC linearity and other factors

- **sample channels:** stereo only (!)

- **HW resources:** two 8-bit I/O ports, scanned from BIOS Data Area LPT1/2/3

  same as single Covox, except that the first port is used for left channel DAC, and the second one for the right. `detect()` picks two last ports in the BDA LPT list (i.e in case of LPT1=0x378, LPT2=0x278, LPT3=0x3BC ports 0x278 (left) and 0x3BC (right) will be picked).

### 4. Stereo-On-1 LPT DAC

- **sample rates:**  up to 48 kHz and beyond

- **sample bit depth**: 8 bits unsigned, "real" bit depth depends on DAC linearity and other factors

- **sample channels:** stereo only (for mono sources, select regular Covox DAC)

- **HW resources:** one "true" LPT port, scanned from BIOS Data Area LPT1/2/3

  basically two DACs attached to single LPT port, with DAC data strobed by LPT control signals: STROBE (pin 1, bit 0 of control port *inverted*) for left channel and LINE FEED (pin 14, bit 1 of control port *inverted*) for right channel.

  generally speaking, there are three distinctive Stereo-On-1 DAC types:

  * latch-based ('373 + parallel DAC) - data latched while strobe is low
  * D-flop-based ('374 + parallel DAC) - data latched on strobe' positive edge
  * TLC7528-like - single strobe (pin 1), 0 - latch left channel data, 1 - latch right channel data

  sndlib supports two different Stereo-On-1 protocols: slower but compatible MODPLAY (6 OUTs per stereo sample, default), and faster FT2 protocol (4 OUTs); switched by IOCTL before `init()` call.

  FT2 is generally compatible with all types of devices, using pin14 as inverted pin1, however for latch-based and '7528 devices it can result to inter-channel leaks for a brief amount of time. R-2R or multiplying DACs handle this fine, with slight stereo narrowing, but PWM/sigma-delta based devices could experience bad aliasing artifacts. MODPLAY protocol activates strobe only when valid data is put on the data port, hence eliminating artifacts at expense of two more OUTs.

  `detect()` relies on the fact that pins 9 (D7) and 11 (bit 7 of status port *inverted*) on Stereo-On-1 are bridged together. During detection, LPT ports are scanned from the BDA LPT list backwards; for each port, if bit 7 of status port equals *inverted* bit 7 of data port, then Stereo-On-1 is assumed to be detected. 

### ...future plans (likely to not being implemented)

* Adlib (too quiet, non-linear, slow IO response, won't implement it)
* Disney Sound Source (low ~9kHz sample rate + tricky FIFO)
* Sound Blaster non-DMA (is it really worth it?)



## b. ISA DMA devices

pioneered by the original Sound Blaster back in 1989, becoming the de-facto standard for PCM sound under DOS, challenged by numerous clones and rivals.

most ISA sound cards use one ISA DMA channel for sample transfers (in auto-init mode) and one IRQ channel to notify CPU on buffer playback completion. notable exception is SB16 which employ two DMA channels (one 8-bit and other 16-bit, although it also supports 16-bit playback over 8-bit DMA channel)

### 1. Sound Blaster 1.x/2.x/Pro

- **sample rates:** 

  * 4..22 kHz - SB 1.x and 2.00 (non-highspeed), and SB Pro stereo
  * 4..44 kHz - SB 2.01+ and Pro mono

- **sample bit depth**: 8 bits unsigned

- **sample channels:** mono only for 1.x/2.x, mono/stereo for Pro

- **HW resources:** one I/O range, one IRQ, one DMA channel

  nuff said :) supported or emulated by almost every ISA sound card, as well as by certain PCI cards.

  SB 2.x/Pro support auto-init playback via normal (up to 22 kHz mono/11 kHz stereo), or highspeed (anything above) modes. Note that in highspeed mode, `pause()/resume()` will not work reliably, because DSP reset is required to stop playback, and "resume DMA" DSP command seems to be ignored for highspeed modes.

  SB 1.x are supported via single-cycle mode, which requires restarting playback every IRQ call, with the audible click between buffers.

  NOTE: SB cards prior to SB16 use very coarse 1 MHz timing reference (originated from i8051 internal timer), divided by "time constant", so sample rates above ~32 kHz mono/16 kHz stereo will sound a bit out of tune (i.e, 22050 Hz is rounded to 22222 Hz, and 44100 Hz would play at 43478 Hz). Currently there is no way to obtain actual sample rate, although I might add it in the future :)

  `detect()` first reads settings from BLASTER variable, then:

  * if IO base address is unknown, common IO ranges (0x220...0x280) are probed for DSP presence. if DSP reset success, current IO base address is saved
  * if IRQ is unknown, "trigger IRQ" DSP command is issued and each possible IRQ is tested. Note that it can result in false positives if other cards (network/SCSI/whatever) are installed in the system.
  * if DMA is unknown, for each DMA channel, short silent buffer in single-cycle mode is played back. if DMA counter for given channel is changed, then this DMA channel is assumed to be used by the SB.

### 2. Sound Blaster 16

- **sample rates:**  5..44 kHz 

- **sample bit depth**: 8/16 bits unsigned/signed

- **sample channels:** mono/stereo 

- **HW resources:** one I/O range, one IRQ, one or two DMA channels

  supported by original SB16/AWE32/64, certain ISA cards (like ALS100) and SBLive! emulation.

  16-bit playback over 8-bit DMA channel is suported.

  `detect()` first reads settings from BLASTER variable, then:

  - if IO base address is unknown, common IO ranges (0x220...0x280) are probed for DSP presence. if DSP reset success, current IO base address is saved
  - if IRQ/DMA is unknown, current IRQ/DMA settings are read from mixer registers 0x80/81.
  - the rest is done as for SB 2.x/Pro driver

### 3. Windows Sound System, GUS MAX/PnP

- **sample rates:**  fixed set of rates, generally in 8..48 kHz range

- **sample bit depth**: 8 bits unsigned, 16 bits signed

- **sample channels:** mono/stereo 

- **HW resources:** one I/O range, one IRQ, one DMA channel

  an alternative PCM audio standard based on AD1848/CS4231 codec, supported by ISA chips from Crystal, Yamaha, OPTi and several other vendors; Gravis Ultrasound MAX and AMD Interwave (GUS PnP, etc) cards also include WSS-compatible codec at non-standard IO base, albeit support is **UNTESTED!**.

  `detect()` works as following:

  - first, ULTRA16 variable is read (format is `ULTRA16=iobase,dma,irq,1,0`). example: 
    `ULTRA16=530,3,10,1,0` select WSS at IO base 0x534 (first 4 IO ports are for WSS semi-PnP capability), IRQ 10, DMA 3. `ULTRA16=534,3,10,1,0` is equivalent.
    *for GUS MAX/PnP:* `ULTRA16=32C,3,10,1,0` select WSS at IO base 0x32C, IRQ 10, DMA 3, additionally testing for GUS presence at 0x220 (0x32C - 0x10C).
  - then, if ULTRASND variable is available, unknown resources are extracted from it.
  - if IO base is still unknown, common WSS and GUS IO ranges are probed.
  - if DMA is unknown, for each DMA channel, short silent buffer is played back. if DMA counter for given channel is changed, then this DMA channel is assumed to be used by the sound card.
  - if IRQ is unknown, short buffer is played back again, then each possible IRQ is tested. Note that it can result in false positives if other cards (network/SCSI/whatever) are installed in the system.

### 4. ESS AudioDrive

- **sample rates:**  4..48 kHz 

- **sample bit depth**: 8/16 bits unsigned/signed

- **sample channels:** mono/stereo 

- **HW resources:** one I/O range, one IRQ, one DMA channel

  supported by ESS AudioDrive family. 48 kHz is a bit out of tune on pre-ES1869 cards. PCI cards (Solo-1) are probably not supported.

  ISA DMA Demand transfer mode can be enabled by IOCTL before `init()` call, **untested!**

  `detect()` first reads settings from BLASTER variable, then:

  - if IO base address is unknown, common IO ranges (0x220...0x280) are probed for DSP presence. if DSP reset success, current IO base address is saved
  - if IRQ/DMA is unknown, current IRQ/DMA settings are read from ESS enhanced registers.
  - the rest is done as for SB 2.x/Pro driver

### 5.Pro Audio Spectrum / PAS+ / PAS16

- **sample rates:**  4..48 kHz 

- **sample bit depth**: 8 bits unsigned, 16 bits signed

- **sample channels:** mono/stereo 

- **HW resources:** one I/O range, one IRQ, one DMA channel

  **absolutely untested**, implemented by careful(-less) documentation/source reading :)

  `detect()` first probes common PAS IO ranges, then, if IRQ/DMA are unknown, calls MVSOUND.SYS driver to get IRQ/DMA settings. if MVSOUND.SYS is not loaded, you MUST pass valid IRQ/DMA settings in `deviceInfo` structure, or else device initialization will fail.

### ...future plans

- original Gravis Ultrasound via GF1 onboard RAM
- anyway idk, current ISA driver support covers perhaps 95% of all ISA sound cards :)

## c. PCI devices

most PCI devices uses PCI Bus Master for playing back/recording audio from the system memory, which, in case of DOS, complicates things a bit. 

First, if paging is enabled (VCPI/DPMI hosts are known for this), then physical addresses are no longer correspond to linear ones. ISA drivers rely on transparent handling of ISA DMA controller registers (which trigger virtual memory manager' automatic memory remapping), but as we are talking directly with the PCI device, this hook will never trigger. Moreover, DPMI doesn't support reverse memory mapping functions (mapping linear memory to one or several physical regions), we have to revert to other APIs like Virtual DMA Services, whose are, alas, doesn't work reliably for extended (>1MB) memory. Another known universal workaround is to allocate XMS memory, lock it and map physical XMS block address to linear via DPMI function 0x800. Alternatively, you can run in raw/XMS environment, with paging disabled and 1:1 address mapping :) 

sndlib workarounds this by allocating sound buffer and necessary descriptors in low memory; in single-tasking systems this seems to work fine.

Second, memory coherency issues are becoming important. PCI systems handle this fine, but in some environments (like PCIe systems), we have to handle unusual stuff like PCIe traffic classes, snooping, etc. 



### 1. High Definition Audio

- **sample rates:**  depends on HDA codec capability, at least 44 and 48 kHz 

- **sample bit depth**: 16 bits signed (24/32 bits are not supported by sndlib)

- **sample channels:** mono/stereo (sndlib supports max. 2 channels)

- **HW resources:** one PCI device, one MMIO range

  supported by almost every PC since late-00s, HDA controller interface is standardized well, HDA codec are entirely different story with it's flexible architecture, making universal driver a PITA to implement.

  `detect()` scans PCI configuration space for HDA devices (class 04.03.00), then filters by following rules:

  - if device is not available, BAR is empty or points to I/O space - skip it entirely.
  - if HDA device is PCI function 1..7 and first function is VGA-compatible controller, then this HDA controller is filtered out.
  - additionally, several HDA controllers (Intel Haswell/Broadwell HDMI) are blacklisted by PCI vendor/device ID.
  - then, check if WALLCLK register (BAR0 offset 0x30, dword) is constantly incrementing, as on HDA controllers
  - then, try to reset HDA controller and check if codecs are present.

  by default, audio is routed to every pin with connectivity field set to other than None and designated as Line Out, SPDIF Out and Headphone Out (see HDA spec for further info). Internal amplifiers, EAPD pin and SPDIF transmitters are activated as well. If appropriate IOCTL is sent, then Speaker pins are also configured to output, if applicable.

  **NOTE:** unfortunately, while HDA controller spec is well defined and most controllers are conform with it, several quirks can result in faults such as frozen audio position, static and even occasional hangups. sndlib tries to work around these quirks, although success rate on non-Intel HDA controllers is dependent on many other factors.

  In case of troubles, run any DOS application that supports HD Audio (such as MPXPLAY), exit and restart sndlib application again. If it doesn't help, restart without CONFIG.SYS/AUTOEXEC.BAT, and try again.

  Currently, this driver has been successfully tested on:

  * AMD Ryzen 5 3600 + Realtek ALC892 (ASRock B450M Pro4) - line out and front panel, also note HDA controller integrated on the CPU
  * Intel Z77 + Realtek ALC892 (ASUS P8Z77-V Pro) - both line out and front panel output, SPDIF works
  * Intel Core i5-4200U + Realtek ALC3225 (Acer E1-572G) - line out
  * Intel H61 + Realtek ALC662 (Pegatron IPSMB-VH1) - line out and front panel, SPDIF untested
  * Intel HM10 + Realtek ALC662 (Intel D525MW) - line out