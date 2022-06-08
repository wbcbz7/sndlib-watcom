#pragma once

enum {
    // range definitions
    SND_IOCTL_GENERAL = 0,

    // mixer controls
    SND_IOCTL_MIXER = 0x4000,

    SND_IOCTL_VOL_MAIN_GET,         // 16.16 fx 2's complement, 0x10000 - 1 db, 0x80000000 - silence
    SND_IOCTL_VOL_MAIN_SET,

    SND_IOCTL_VOL_WAVE_GET,
    SND_IOCTL_VOL_WAVE_SET,

    // device-specific controls
    SND_IOCTL_DEVICE_SPECIFIC = 0x8000,
};

// ioctl types
enum {
    SND_IOCTRL_TYPE_BOOL,
    SND_IOCTRL_TYPE_INT32,
    SND_IOCTRL_TYPE_UINT32,
    SND_IOCTRL_TYPE_VOLUME,
};

// ioctl import table
struct SoundDeviceIoctlImport {
    uint32_t    handle;
    uint32_t    type;
    uint32_t    minValue;
    uint32_t    maxValue;
    const char* name;
};

