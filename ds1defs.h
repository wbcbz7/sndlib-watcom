#pragma once

#include <stdint.h>
#include "mmio.h"

// Yamaha DS-1 PCI (YMF724/744/754) driver 
// wbcbz7 o7.o8.2o22

#define YMF_REG_WRITE32(base, reg, value) (MMIO_WRITE32(((uint8_t*)(base) + (reg)), (value)))
#define YMF_REG_WRITE16(base, reg, value) (MMIO_WRITE16(((uint8_t*)(base) + (reg)), (value)))
#define YMF_REG_WRITE8(base, reg, value)  (MMIO_WRITE8(((uint8_t*)(base) + (reg)), (value)))

#define YMF_REG_READ32(base, reg) (MMIO_READ32(((uint8_t*)(base) + (reg))))
#define YMF_REG_READ16(base, reg) (MMIO_READ16(((uint8_t*)(base) + (reg))))
#define YMF_REG_READ8(base, reg)  (MMIO_READ8(((uint8_t*)(base) + (reg))))

#define YMF_REG_MODIFY32(base, reg, mask, value) (MMIO_MODIFY32(((uint8_t*)(base) + (reg)), (mask), (value)))
#define YMF_REG_MODIFY16(base, reg, mask, value) (MMIO_MODIFY16(((uint8_t*)(base) + (reg)), (mask), (value)))
#define YMF_REG_MODIFY8(base, reg, mask, value)  (MMIO_MODIFY8(((uint8_t*)(base) + (reg)), (mask), (value)))

#define YMF_REG_FORCE_WRITE32(base, reg, value) (MMIO_FORCE_WRITE32(((uint8_t*)(base) + (reg)), (value)))
#define YMF_REG_FORCE_WRITE16(base, reg, value) (MMIO_FORCE_WRITE16(((uint8_t*)(base) + (reg)), (value)))
#define YMF_REG_FORCE_WRITE8(base, reg, value)  (MMIO_FORCE_WRITE8(((uint8_t*)(base) + (reg)), (value)))

#define YMF_REG_FORCE_READ32(base, reg) (MMIO_FORCE_READ32(((uint8_t*)(base) + (reg))))
#define YMF_REG_FORCE_READ16(base, reg) (MMIO_FORCE_READ16(((uint8_t*)(base) + (reg))))
#define YMF_REG_FORCE_READ8(base, reg)  (MMIO_FORCE_READ8(((uint8_t*)(base) + (reg))))

#define YMF_REG_FORCE_MODIFY32(base, reg, mask, value) (MMIO_FORCE_MODIFY32(((uint8_t*)(base) + (reg)), (mask), (value)))
#define YMF_REG_FORCE_MODIFY16(base, reg, mask, value) (MMIO_FORCE_MODIFY16(((uint8_t*)(base) + (reg)), (mask), (value)))
#define YMF_REG_FORCE_MODIFY8(base, reg, mask, value)  (MMIO_FORCE_MODIFY8(((uint8_t*)(base) + (reg)), (mask), (value)))


// protect from leaking to root namespace
namespace sndlib {

#pragma pack(push, 1)

// playback slot (aka channel) control data
struct ymf_playSlotControlData {
    uint32_t        format;
    uint32_t        loopDefault;
    void*           pgBase;         // dword aligned, start/loop/loopEnd offsets are relative from the base
    uint32_t        pgLoop;
    uint32_t        pgLoopEnd;
    uint32_t        pgLoopFrac;
    uint32_t        pgDeltaEnd;
    uint32_t        lpfKEnd;
    uint32_t        egGainEnd;
    uint32_t        leftGainEnd;
    uint32_t        rightGainEnd;
    uint32_t        fx1GainEnd;
    uint32_t        fx2GainEnd;
    uint32_t        fx3GainEnd;
    uint32_t        lpfQ;
    uint32_t        status;
    uint32_t        numOfFrames;
    uint32_t        loopCount;
    uint32_t        pgStart;
    uint32_t        pgStartFrac;
    uint32_t        pgDelta;
    uint32_t        lpfK;
    uint32_t        egGain;
    uint32_t        leftGain;
    uint32_t        rightGain;
    uint32_t        fx1Gain;
    uint32_t        fx2Gain;
    uint32_t        fx3Gain;
    uint32_t        lpfD1;
    uint32_t        lpfD2;
};

// effect slot control data
struct ymf_fxSlotControlData {
    uint32_t        pgBase;         // dword aligned, start/loop/loopEnd offsets are relative from the base
    uint32_t        pgLoopEnd;
    uint32_t        pgStart;
    uint32_t        temp;
};

struct ymf_playControlDataTable {
    uint32_t                    numOfPlay;
    ymf_playSlotControlData    *slotBase[64];
};

// PCI configuration space registers
enum {
    YMF_PCI_REG_MMIO_BASE           = 0x0010,       // BAR0
    YMF_PCI_REG_LEGACY_SB_BASE      = 0x0014,       // BAR1
    YMF_PCI_REG_LEGACY_JOY_BASE     = 0x0018,       // BAR2
    YMF_PCI_REG_LEGACY_AUDIO_CTRL   = 0x0040,
    YMF_PCI_REG_LEGACY_AUDIO_EXT_CTRL= 0x0042,
    YMF_PCI_REG_SUBSYSTEM_ID_WRITE  = 0x0044,
    YMF_PCI_REG_DS1_CTRL            = 0x0048,
    YMF_PCI_REG_DS1_POWER_CTRL_1    = 0x004A,
    YMF_PCI_REG_DS1_POWER_CTRL_2    = 0x004C,
    YMF_PCI_REG_DDMA_CTRL           = 0x004E,
    YMF_PCI_REG_POWER_MANAGEMENT    = 0x0054,
    YMF_PCI_REG_ACPI_MODE           = 0x0058,
    YMF_PCI_REG_AC97_SEC_POWER_CTRL = 0x005A,
};

// MMIO registers
enum {
    YMF_REG_REVISION                = 0x0000,
    YMF_REG_INTERRUPT_FLAG          = 0x0004,
    YMF_REG_ACTIVITY                = 0x0006,
    YMF_REG_GLOBAL_CONTROL          = 0x0008,
    YMF_REG_ZOOMEDVIDEO_CONTROL     = 0x000A,
    YMF_REG_TIMER_CONTROL           = 0x0010,
    YMF_REG_TIMER_COUNT             = 0x0012,
    YMF_REG_SPDIF_OUT_CONTROL       = 0x0018,
    YMF_REG_SPDIF_OUT_CHAN_CONTROL  = 0x001C,
    YMF_REG_EEPROM_CONTROL          = 0x002C,
    YMF_REG_SPDIF_IN_CONTROL        = 0x0034,
    YMF_REG_SPDIF_OUT_CHAN_STATUS   = 0x0038,
    YMF_REG_GPIO_INT_FLAG           = 0x0050,
    YMF_REG_GPIO_INT_ENABLE         = 0x0052,
    YMF_REG_GPIO_IN_STATUS          = 0x0054,
    YMF_REG_GPIO_OUT_CONTROL        = 0x0056,
    YMF_REG_GPIO_FUNCTION           = 0x0058,
    YMF_REG_GPIO_IN_CONFIG          = 0x005A,

    YMF_REG_AC97_CMD_DATA           = 0x0060,
    YMF_REG_AC97_CMD_ADDRESS        = 0x0062,
    YMF_REG_AC97_STATUS0_DATA       = 0x0064,
    YMF_REG_AC97_STATUS0_ADDRESS    = 0x0066,
    YMF_REG_AC97_STATUS1_DATA       = 0x0068,
    YMF_REG_AC97_STATUS1_ADDRESS    = 0x006A,
    YMF_REG_AC97_SECONDARY_CONFIG   = 0x0070,

    // loword is left, hiword is right
    // [db] = 20 * log([volume]/16384), 14 bit, 0 - mute
    YMF_REG_VOL_LEGACY_OUT          = 0x0080,
    YMF_REG_VOL_AUDIO_DAC_OUT       = 0x0084,
    YMF_REG_VOL_ZV_OUT              = 0x0088,   // fuck putin!
    YMF_REG_VOL_AC97_SECONDARY_OUT  = 0x008C,
    YMF_REG_VOL_ADC_OUT             = 0x0090,
    YMF_REG_VOL_LEGACY_LOOPBACK     = 0x0094,
    YMF_REG_VOL_AUDIO_DAC_LOOPBACK  = 0x0098,
    YMF_REG_VOL_ZV_LOOPBACK         = 0x009C,
    YMF_REG_VOL_AC97_SECONDARY_LOOPBACK = 0x00A0,
    YMF_REG_VOL_ADC_LOOPBACK        = 0x00A4,
    YMF_REG_VOL_ADC_INPUT           = 0x00A8,
    YMF_REG_VOL_REC_INPUT           = 0x00AC,
    YMF_REG_VOL_P44_OUT             = 0x00B0,
    YMF_REG_VOL_P44_LOOPBACK        = 0x00B4,
    YMF_REG_VOL_SPDIF_IN_OUT        = 0x00B8,
    YMF_REG_VOL_SPDIF_IN_LOOPBACK   = 0x00BC,

    YMF_REG_ADC_SAMPLE_RATE         = 0x00C0,
    YMF_REG_REC_SAMPLE_RATE         = 0x00C4,
    YMF_REG_ADC_FORMAT              = 0x00C8,
    YMF_REG_REC_FORMAT              = 0x00CC,

    YMF_REG_AUDIO_STATUS            = 0x0100,
    YMF_REG_AUDIO_CONTROLSELECT     = 0x0104,
    YMF_REG_AUDIO_MODE              = 0x0108,
    YMF_REG_AUDIO_SAMPLE_COUNT      = 0x010C,
    YMF_REG_AUDIO_NUM_OF_SAMPLE     = 0x0110,
    YMF_REG_AUDIO_CONFIG            = 0x0114,

    // size of each bank in bytes
    YMF_REG_AUDIO_PLAY_CONTROL_SIZE = 0x0140,
    YMF_REG_AUDIO_REC_CONTROL_SIZE  = 0x0144,
    YMF_REG_AUDIO_FX_CONTROL_SIZE   = 0x0148,
    YMF_REG_AUDIO_WORK_SIZE         = 0x014C,   // used for p44 slots
    YMF_REG_AUDIO_MAP_OF_REC        = 0x0150,
    YMF_REG_AUDIO_MAP_OF_FX         = 0x0154,

    // physical pointer to struct, dword aligned
    YMF_REG_AUDIO_PLAY_CONTROL_BASE = 0x0158,
    YMF_REG_AUDIO_REC_CONTROL_BASE  = 0x015C,
    YMF_REG_AUDIO_FX_CONTROL_BASE   = 0x0160,
    YMF_REG_AUDIO_WORK_CONTROL_BASE = 0x0164,   // used for p44 slots

    // DSP instruction RAM
    YMF_REG_DSP_INSTRUCTION_RAM     = 0x1000,
    YMF_REG_CTRL_INSTRUCTION_RAM    = 0x4000,
};

void ds1_dspDisable(void *ds1regs);
void ds1_dspEnable(void *ds1regs);
void ds1_dspReset(void *ds1regs, bool enable = true);
void ds1_dspUpload(void *ds1regs, uint8_t* dspCode, uint32_t dspCodeLength, uint8_t* ctrlCode, uint32_t ctrlCodeLength);

// memory allocation structure:
// for sound playback, 3 memory ppols are allocated:
// - audio DMA block (classic double-buffer for sound data)
// - play slot control data, 2 banks
// - rest of DS1 buffers - play control data table, fx work buffer, p44slot work buffer

struct ymf_memoryAllocInfo {
    // buffer size info
    uint32_t                    playCtrlSize;
    uint32_t                    recCtrlSize;
    uint32_t                    fxCtrlSize;
    uint32_t                    p44WorkSize;

    // DMA block for control arrays
    dmaBlock                    workBlk;

    // work buffer pointers
    uint8_t*                    p44WorkBuf;
    ymf_fxSlotControlData**     fxCtrl;
    ymf_playControlDataTable*   playCtrl;

    // DMA block for play slot data arrays
    dmaBlock                    playSlotBlk;

    // play slot control data for every active channel
    uint32_t                    playChannels;
    ymf_playSlotControlData**   playPtrBuf;
    ymf_playSlotControlData**   playSlot[2];    // for each bank

    // DMA block for effects control slots
    dmaBlock                    fxSlotBlk;
    ymf_playSlotControlData**   fxPtrBuf;
    ymf_fxSlotControlData*      fxSlot[2][5];
};

bool ds1_allocBuffers(void *ds1regs, ymf_memoryAllocInfo *alloc, uint32_t playChannels);
bool ds1_freeBuffers (void *ds1regs, ymf_memoryAllocInfo *alloc);
void ds1_setBuffers(void *ds1regs, ymf_memoryAllocInfo *alloc);
void ds1_dspDisable(void *ds1regs);
void ds1_dspEnable(void *ds1regs);
void ds1_dspReset(void *ds1regs, bool enable);
void ds1_dspUpload(void *ds1regs, uint8_t* dspCode, uint32_t dspCodeLength, uint8_t* ctrlCode, uint32_t ctrlCodeLength);

#pragma pack(pop)

}
