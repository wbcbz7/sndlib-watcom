#pragma once

#include "convert.h"

typedef uint32_t soundFormat;
enum {
    SND_FMT_NULL            = 0,
    SND_FMT_INT8            = (1 << 0),
    SND_FMT_INT16           = (1 << 1),
    SND_FMT_INT20           = (1 << 2),
    SND_FMT_INT24           = (1 << 3),
    SND_FMT_FLOAT           = (1 << 4),
    SND_FMT_DOUBLE          = (1 << 5),
    
    SND_FMT_XLAT8           = (1 << 7),
    SND_FMT_DEPTH_MASK      = (1 << 8) - 1,
    
    SND_FMT_MONO            = (1 << 8),
    SND_FMT_STEREO          = (1 << 9),
    
    SND_FMT_CHANNELS_MASK   = ((1 << (12 - 8  + 1)) - 1) << 8,
    
    SND_FMT_SIGNED          = (1 << 15),
    SND_FMT_UNSIGNED        = (1 << 14),
    
    SND_FMT_SIGN_MASK       = ((1 << (15 - 14 + 1)) - 1) << 14,
};

struct soundFormatCapability {
    soundFormat     format;             // combination of soundFormat flags
    int32_t         ratesLength;        // positive for fixed rates, -2 for rates range [rates[0]; rates[1]]
    const uint32_t *rates;              // ratesLength length, NOT sorted!
};


