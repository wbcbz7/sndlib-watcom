#pragma once 

enum {
    SND_ERR_OK = 0,
    SND_ERR_UNSUPPORTED,
    SND_ERR_INVALIDCONFIG,
    SND_ERR_MEMALLOC,
    SND_ERR_UNKNOWN_FORMAT,
    SND_ERR_NOTFOUND,
    SND_ERR_NULLPTR,
    SND_ERR_STUCK_IRQ,
    SND_ERR_DMA,
    SND_ERR_NO_DATA,
    SND_ERR_UNINITIALIZED,
    SND_ERR_RESUMED,            // for SoundDevide::start() if playback is paused, not an error!
    SND_ERR_USEREXIT,           // for sndlibCreateDevice()
    
    
    // ...more in future :)
};
