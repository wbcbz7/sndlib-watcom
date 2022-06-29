#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#include "sndlib.h"
#include "snddev.h"
#include "sndfmt.h"
#include "convert.h"
#include "snderror.h"

// init sound library internal resources
uint32_t sndlibInit() {
    // init PM ISR stack
    snddev_pm_stack_top = &snddev_pm_stack_top;

    // init snddev_pm_old_stack so it doesn't dangle in the void
    snddev_pm_old_stack = NULL;

    // clear in use flag
    snddev_pm_stack_in_use = 0;

    // lock memory for stack/RMCB data
    dpmi_lockmemory(&snddev_bss_lock_start, (&snddev_bss_lock_end - &snddev_bss_lock_start));

    // clear active device storage
    memset(snd_activeDevice, 0, sizeof(snd_activeDevice));

    return SND_ERR_OK;   
}

// close sound library and cleanup
uint32_t sndlibDone() {
    // unlock memory for stack/RMCB data
    dpmi_lockmemory(&snddev_bss_lock_start, (&snddev_bss_lock_end - &snddev_bss_lock_start));

    return SND_ERR_OK;   
}


