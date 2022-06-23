#pragma once

#include <stdint.h>
#include "mmio.h"

// Intel High Definition Audio structure definitions
// --wbcbz7 o5.o3.zozl

#define HDA_REG_WRITE32(base, reg, value) (MMIO_WRITE32(((uint8_t*)(base) + (reg)), (value)))
#define HDA_REG_WRITE16(base, reg, value) (MMIO_WRITE16(((uint8_t*)(base) + (reg)), (value)))
#define HDA_REG_WRITE8(base, reg, value)  (MMIO_WRITE8(((uint8_t*)(base) + (reg)), (value)))

#define HDA_REG_READ32(base, reg) (MMIO_READ32(((uint8_t*)(base) + (reg))))
#define HDA_REG_READ16(base, reg) (MMIO_READ16(((uint8_t*)(base) + (reg))))
#define HDA_REG_READ8(base, reg)  (MMIO_READ8(((uint8_t*)(base) + (reg))))

#define HDA_REG_MODIFY32(base, reg, mask, value) (MMIO_MODIFY32(((uint8_t*)(base) + (reg)), (mask), (value)))
#define HDA_REG_MODIFY16(base, reg, mask, value) (MMIO_MODIFY16(((uint8_t*)(base) + (reg)), (mask), (value)))
#define HDA_REG_MODIFY8(base, reg, mask, value)  (MMIO_MODIFY8(((uint8_t*)(base) + (reg)), (mask), (value)))

#define HDA_REG_FORCE_WRITE32(base, reg, value) (MMIO_FORCE_WRITE32(((uint8_t*)(base) + (reg)), (value)))
#define HDA_REG_FORCE_WRITE16(base, reg, value) (MMIO_FORCE_WRITE16(((uint8_t*)(base) + (reg)), (value)))
#define HDA_REG_FORCE_WRITE8(base, reg, value)  (MMIO_FORCE_WRITE8(((uint8_t*)(base) + (reg)), (value)))

#define HDA_REG_FORCE_READ32(base, reg) (MMIO_FORCE_READ32(((uint8_t*)(base) + (reg))))
#define HDA_REG_FORCE_READ16(base, reg) (MMIO_FORCE_READ16(((uint8_t*)(base) + (reg))))
#define HDA_REG_FORCE_READ8(base, reg)  (MMIO_FORCE_READ8(((uint8_t*)(base) + (reg))))

#define HDA_REG_FORCE_MODIFY32(base, reg, mask, value) (MMIO_FORCE_MODIFY32(((uint8_t*)(base) + (reg)), (mask), (value)))
#define HDA_REG_FORCE_MODIFY16(base, reg, mask, value) (MMIO_FORCE_MODIFY16(((uint8_t*)(base) + (reg)), (mask), (value)))
#define HDA_REG_FORCE_MODIFY8(base, reg, mask, value)  (MMIO_FORCE_MODIFY8(((uint8_t*)(base) + (reg)), (mask), (value)))

#define HDA_STREAM_WRITE32(base, stream, reg, value) HDA_REG_WRITE32((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg), (value))
#define HDA_STREAM_WRITE16(base, stream, reg, value) HDA_REG_WRITE16((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg), (value))
#define HDA_STREAM_WRITE8(base, stream, reg, value)  HDA_REG_WRITE8((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg), (value))

#define HDA_STREAM_READ32(base, stream, reg) HDA_REG_READ32((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg))
#define HDA_STREAM_READ16(base, stream, reg) HDA_REG_READ16((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg))
#define HDA_STREAM_READ8(base, stream, reg)  HDA_REG_READ8((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg))

#define HDA_STREAM_MODIFY32(base, stream, reg, mask, value) HDA_REG_FORCE_MODIFY32((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg), (mask), (value))
#define HDA_STREAM_MODIFY16(base, stream, reg, mask, value) HDA_REG_FORCE_MODIFY16((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg), (mask), (value))
#define HDA_STREAM_MODIFY8(base, stream, reg, mask, value)  HDA_REG_FORCE_MODIFY8((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg), (mask), (value))

#define HDA_STREAM_FORCE_WRITE32(base, stream, reg, value) HDA_REG_FORCE_WRITE32((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg), (value))
#define HDA_STREAM_FORCE_WRITE16(base, stream, reg, value) HDA_REG_FORCE_WRITE16((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg), (value))
#define HDA_STREAM_FORCE_WRITE8(base, stream, reg, value)  HDA_REG_FORCE_WRITE8((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg), (value))

#define HDA_STREAM_FORCE_READ32(base, stream, reg) HDA_REG_FORCE_READ32((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg))
#define HDA_STREAM_FORCE_READ16(base, stream, reg) HDA_REG_FORCE_READ16((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg))
#define HDA_STREAM_FORCE_READ8(base, stream, reg)  HDA_REG_FORCE_READ8((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg))

#define HDA_STREAM_FORCE_MODIFY32(base, stream, reg, mask, value) HDA_REG_FORCE_MODIFY32((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg), (mask), (value))
#define HDA_STREAM_FORCE_MODIFY16(base, stream, reg, mask, value) HDA_REG_FORCE_MODIFY16((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg), (mask), (value))
#define HDA_STREAM_FORCE_MODIFY8(base, stream, reg, mask, value)  HDA_REG_FORCE_MODIFY8((base), HDA_REG_STREAM_BASE + HDA_REG_STREAM_SIZE * (stream) + (reg), (mask), (value))

// protect from leaking to root namespace
namespace sndlib {

#pragma pack(push, 1)

// buffer descriptor list entry (128b aligned)
struct hda_bufferDescriptor {
    void*                   ptr;
    void*                   ptrHigh;

    uint32_t                length;
    uint32_t                status;
};

enum {
    HDA_BDL_STATUS_IOC = (1 << 0),              // interrupt on completion flag;
};

// stream postion (128b aligned)
typedef uint64_t            hda_streamPosition;

// HDA codec verb
union hda_codecVerb {
    struct {
        uint32_t        payload         : 20;
        uint32_t        nodeId          : 8;
        uint32_t        codecAddress    : 4;
    };
    uint32_t            val;

    hda_codecVerb(uint32_t raw = 0) : val(raw) {}
    uint32_t& operator()() { return val; }
    uint32_t& value()      { return val; }
};

// HDA codec response
union hda_codecResponse {
    // unsolicied
    struct {
        uint32_t            vendorSpecific : 21;
        uint32_t            subTag : 5;
        uint32_t            tag : 6;
    } unsol;

    // solicited
    uint32_t                sol;
    uint32_t                raw;

    hda_codecResponse(uint32_t raw = 0) : sol(raw) {}
    uint32_t& operator()() { return sol; }
    uint32_t& value()      { return sol; }
};

// HDA codec response
struct hda_codecResponseExt {
    // raw response
    hda_codecResponse           resp;

    // extended info
    uint32_t                    codecAddress : 4;
    uint32_t                    isSolicited  : 1;
    uint32_t                    reserved1    : 27;
};

// HDA stream format
union hda_streamFormat {
    struct {
        uint16_t            channels       : 4;         // minus one
        uint16_t            bitDepth       : 3;         // 0 - 8, 1 - 16, 2 - 20, 3 - 24, 4 - 32
        uint16_t            reserved1      : 1;
        uint16_t            rateDivider    : 3;         // minus one
        uint16_t            rateMultiplier : 3;         // minus one
        uint16_t            baseRate       : 1;         // 0 - 48 khz, 1 - 44.1 khz
        uint16_t            type           : 1;
    };
    uint16_t                raw;
};

// HD Audio Controller registers
enum {
    HDA_REG_GCAP =          0x00,
    HDA_REG_VMIN =          0x02,
    HDA_REG_VMAJ =          0x03,
    HDA_REG_OUTPAY =        0x04,
    HDA_REG_INPAY =         0x06,
    HDA_REG_GCTL =          0x08,
    HDA_REG_WAKEEN  =       0x0c,
    HDA_REG_STATESTS =      0x0e,
    HDA_REG_GSTS  =         0x10,
    HDA_REG_INTCTL =        0x20,
    HDA_REG_INTSTS  =       0x24,
    HDA_REG_WALLCLK =       0x30,
    HDA_REG_SSYNC =         0x34,
    HDA_REG_SSYNC_ALIAS =   0x38,   // HDA_REG_SSYNC as documented in HDA specification, seems to be incorrect

    HDA_REG_CORBLBASE =     0x40,
    HDA_REG_CORBUBASE =     0x44,
    HDA_REG_CORBWP =        0x48,
    HDA_REG_CORBRP =        0x4A,
    HDA_REG_CORBCTL =       0x4c,
    HDA_REG_CORBSTS =       0x4d,
    HDA_REG_CORBSIZE =      0x4e,

    HDA_REG_RIRBLBASE =     0x50,
    HDA_REG_RIRBUBASE =     0x54,
    HDA_REG_RIRBWP =        0x58,
    HDA_REG_RINTCNT =       0x5a,
    HDA_REG_RIRBCTL =       0x5c,
    HDA_REG_RIRBSTS =       0x5d,
    HDA_REG_RIRBSIZE =      0x5e,

    // immediate command interface
    HDA_REG_ICW =           0x60,
    HDA_REG_IRR =           0x64,
    HDA_REG_ICS =           0x68,

    HDA_REG_DPLBASE =       0x70,
    HDA_REG_DPUBASE =       0x74,

    HDA_REG_STREAM_BASE =   0x80,
    HDA_REG_STREAM_SIZE =   0x20,

    // stream registers, 0-based
    HDA_REG_STREAM_CTRL =     0x00,
    HDA_REG_STREAM_STATUS =   0x03,
    HDA_REG_STREAM_LINKPOS =  0x04,
    HDA_REG_STREAM_CBL      = 0x08,
    HDA_REG_STREAM_LVI      = 0x0c,
    HDA_REG_STREAM_FIFOSIZE = 0x10,
    HDA_REG_STREAM_FORMAT   = 0x12,
    HDA_REG_STREAM_BDLPL    = 0x18,
    HDA_REG_STREAM_BDLPU    = 0x1c,
};

enum {
    // register bitfields
    HDA_DPLBASE_ENABLE =            (1 << 0),            // enable position buffer
    
    HDA_CORBCTL_MEI =               (1 << 0),
    HDA_CORBCTL_RUN =               (1 << 1),
    HDA_RIRBCTL_INT =               (1 << 0),
    HDA_RIRBCTL_RUN =               (1 << 1),
    HDA_RIRBCTL_ORI =               (1 << 2),

    HDA_ICS_BUSY =                  (1 << 0),
    HDA_ICS_VALID =                 (1 << 1),
    HDA_ICS_VERSION =               (1 << 2),

    HDA_GCTL_CRST =                 (1 << 0),
    HDA_GCTL_FNCTRL =               (1 << 1),
    HDA_GCTL_UNSOL_ENABLE =         (1 << 8),

    HDA_INT_GLOBAL_MASK     =       (1 << 31),
    HDA_INT_CONTROLLER_MASK =       (1 << 30),
    HDA_INT_STREAM_MASK       =     (1 << 30) - 1,

    HDA_STREAM_CTRL_SRST =          (1 << 0),
    HDA_STREAM_CTRL_RUN =           (1 << 1),
    HDA_STREAM_CTRL_STRIPE =        (1 << 16),
    HDA_STREAM_CTRL_STRIPE_MASK =   (1 << 16) | (1 << 17),
    HDA_STREAM_CTRL_TP =            (1 << 18),
    HDA_STREAM_CTRL_DIR =           (1 << 19),
    HDA_STREAM_CTRL_STREAM_TAG_MASK  = 15 << 20,
    HDA_STREAM_CTRL_STREAM_TAG_SHIFT = 20,

    // assumes status register read as byte
    HDA_STREAM_STATUS_FIFO_READY =  (1 << 5),

    // interrupt flags
    HDA_STREAM_INT_BUFFER_COMPLETE =   (1 << 2),
    HDA_STREAM_INT_FIFO_ERROR =        (1 << 3),
    HDA_STREAM_INT_DESCRIPTOR_ERROR =  (1 << 4),
};

// global capabilities field
union hda_gcap {
    struct {
        uint8_t             is64bit     : 1;
        uint8_t             sdoCount    : 2;        // 00 - 1, 01 - 2, 10 - 4, 11 - reserved
        uint8_t             bidirCount  : 5;
        uint8_t             inputCount  : 4;
        uint8_t             outputCount : 4;
    };
    uint8_t     raw;
};

#pragma pack(pop)

// verb descrption

#define HDA_RAW_VERB_TO_4BIT(verb)  ((verb & 0xF0000) >> 16)
#define HDA_RAW_VERB_TO_12BIT(verb) ((verb & 0xFFF00) >> 8)

enum {
    HDA_VERB_GET_PARAMETER = 0xF0000,
    HDA_VERB_GET_CONNECTION_SELECT_CONTROL = 0xF0100,
    HDA_VERB_SET_CONNECTION_SELECT = 0x70100,
    HDA_VERB_GET_CONNECTION_LIST_ENTRY = 0xF0200,
    HDA_VERB_GET_PROCESSING_STATE = 0xF0300,
    HDA_VERB_SET_PROCESSING_STATE = 0x70300,
    HDA_VERB_GET_SDI_SELECT = 0xF0400,
    HDA_VERB_SET_SDI_SELECT = 0x70400,
    HDA_VERB_GET_POWER_STATE = 0xF0500,
    HDA_VERB_SET_POWER_STATE = 0x70500,
    HDA_VERB_GET_CONVERTER_STREAM_CHANNEL = 0xF0600,
    HDA_VERB_SET_CONVERTER_STREAM_CHANNEL = 0x70600,
    HDA_VERB_GET_PIN_WIDGET_CONTROL = 0xF0700,
    HDA_VERB_SET_PIN_WIDGET_CONTROL = 0x70700,
    HDA_VERB_GET_UNSOLICITED_RESPONSE_CONTROL = 0xF0800,
    HDA_VERB_SET_UNSOLICITED_RESPONSE_CONTROL = 0x70800,
    HDA_VERB_GET_PIN_SENSE = 0xF0900,
    HDA_VERB_EXEC_PIN_SENSE = 0x70900,
    HDA_VERB_GET_EAPD_BTL = 0xF0C00,
    HDA_VERB_SET_EAPD_BTL = 0x70C00,
    HDA_VERB_GET_CONFIGURATION_DEFAULT = 0xF1C00,
    HDA_VERB_SET_CONFIGURATION_DEFAULT = 0x71C00,         // 0x71C..0x71F for bytes 0..3

    HDA_VERB_GET_DIGITAL_CONVERTER_CONTROL = 0xF0D00,
    HDA_VERB_SET_DIGITAL_CONVERTER_CONTROL = 0x70D00,         // 0x71C..0x71F for bytes 0..3

    HDA_VERB_GET_COEFFICIENT_INDEX = 0xD0000,
    HDA_VERB_SET_COEFFICIENT_INDEX = 0x50000,
    HDA_VERB_GET_COEFFICIENT = 0xC0000,
    HDA_VERB_SET_COEFFICIENT = 0x40000,
    HDA_VERB_GET_AMPLIFIER_GAIN = 0xB0000,
    HDA_VERB_SET_AMPLIFIER_GAIN = 0x30000,
    HDA_VERB_GET_CONVERTER_FORMAT = 0xA0000,
    HDA_VERB_SET_CONVERTER_FORMAT = 0x20000,
};

enum {
    HDA_PIN_CONNECTIVITY_JACK =                 (0 << 30),
    HDA_PIN_CONNECTIVITY_NONE =                 (1 << 30),
    HDA_PIN_CONNECTIVITY_INTERNAL =             (2 << 30),
    HDA_PIN_CONNECTIVITY_JACK_INTERNAL =        (3 << 30),
    HDA_PIN_CONNECTIVITY_MASK =                 (3 << 30),

    HDA_PIN_LOCATION_CLASS_EXTERNAL =           (0 << 28),
    HDA_PIN_LOCATION_CLASS_INTERNAL =           (1 << 28),
    HDA_PIN_LOCATION_CLASS_SEPARATE_CHASSIS =   (2 << 28),
    HDA_PIN_LOCATION_CLASS_OTHER =              (3 << 28),
    HDA_PIN_LOCATION_CLASS_MASK =               (3 << 28),

    HDA_PIN_DEFAULTDEVICE_LINE_OUT =            (0 << 20),
    HDA_PIN_DEFAULTDEVICE_SPEAKER =             (1 << 20),
    HDA_PIN_DEFAULTDEVICE_HP_OUT   =            (2 << 20),
    HDA_PIN_DEFAULTDEVICE_CD =                  (3 << 20),
    HDA_PIN_DEFAULTDEVICE_SPDIF_OUT =           (4 << 20),
    HDA_PIN_DEFAULTDEVICE_DIGITAL_OUT =         (5 << 20),
    HDA_PIN_DEFAULTDEVICE_MODEM_LINE =          (6 << 20),
    HDA_PIN_DEFAULTDEVICE_MODEM_HANDSET =       (7 << 20),
    HDA_PIN_DEFAULTDEVICE_LINE_IN =             (8 << 20),
    HDA_PIN_DEFAULTDEVICE_AUX =                 (9 << 20),
    HDA_PIN_DEFAULTDEVICE_MIC_IN =              (10 << 20),
    HDA_PIN_DEFAULTDEVICE_TELEPHONY =           (11 << 20),
    HDA_PIN_DEFAULTDEVICE_SPDIF_IN =            (12 << 20),
    HDA_PIN_DEFAULTDEVICE_DIGITAL_IN =          (13 << 20),
    HDA_PIN_DEFAULTDEVICE_RESERVED =            (14 << 20),
    HDA_PIN_DEFAULTDEVICE_OTHER =               (15 << 20),
    HDA_PIN_DEFAULTDEVICE_MASK =                (15 << 20),

    // only common here
    HDA_PIN_LOCATION_REAR =                     (1 << 24),
    HDA_PIN_LOCATION_FRONT =                    (2 << 24),
    HDA_PIN_LOCATION_LEFT =                     (3 << 24),
    HDA_PIN_LOCATION_RIGHT =                    (4 << 24),
    HDA_PIN_LOCATION_TOP =                      (5 << 24),
    HDA_PIN_LOCATION_BOTTOM =                   (6 << 24),
    HDA_PIN_LOCATION_MASK =                     (15 << 24),

    HDA_PIN_CONNECTION_TYPE_3_5MM_JACK =        (1 << 16),
    HDA_PIN_CONNECTION_TYPE_6_35MM_JACK =       (2 << 16),
    HDA_PIN_CONNECTION_ATAPI =                  (3 << 16),
    HDA_PIN_CONNECTION_RCA =                    (4 << 16),
    HDA_PIN_CONNECTION_OPTICAL =                (5 << 16),
    HDA_PIN_CONNECTION_OTHER_DIGITAL =          (6 << 16),
    HDA_PIN_CONNECTION_OTHER_ANALOG =           (7 << 16),
    HDA_PIN_CONNECTION_DIN =                    (8 << 16),
    HDA_PIN_CONNECTION_XLR =                    (9 << 16),
    HDA_PIN_CONNECTION_RJ11 =                   (10 << 16),
    HDA_PIN_CONNECTION_OTHER =                  (11 << 16),
    HDA_PIN_CONNECTION_MASK =                   (15 << 16),

    HDA_PIN_COLOR_UNKNOWN =                     (0 << 12),
    HDA_PIN_COLOR_BLACK =                       (1 << 12),
    HDA_PIN_COLOR_GREY =                        (2 << 12),
    HDA_PIN_COLOR_BLUE =                        (3 << 12),
    HDA_PIN_COLOR_GREEN =                       (4 << 12),
    HDA_PIN_COLOR_RED =                         (5 << 12),
    HDA_PIN_COLOR_ORANGE =                      (6 << 12),
    HDA_PIN_COLOR_YELLOW =                      (7 << 12),
    HDA_PIN_COLOR_PURPLE =                      (8 << 12),
    HDA_PIN_COLOR_PINK =                        (9 << 12),
    HDA_PIN_COLOR_WHITE =                       (14 << 12),
    HDA_PIN_COLOR_OTHER =                       (15 << 12),
    HDA_PIN_COLOR_MASK =                        (15 << 12),


    HDA_WIDGET_AUDIO_OUTPUT  =                  0,
    HDA_WIDGET_AUDIO_INPUT  =                   1,
    HDA_WIDGET_AUDIO_MIXER  =                   2,
    HDA_WIDGET_AUDIO_SELECTOR  =                3,
    HDA_WIDGET_AUDIO_PIN  =                     4,
    HDA_WIDGET_AUDIO_POWER  =                   5,
    HDA_WIDGET_AUDIO_VOLUME_KNOB  =             6,
    HDA_WIDGET_AUDIO_BEEP_GENERATOR=            7,
    HDA_WIDGET_AUDIO_VENDOR_DEFINED  =         15,
};

// scan sctructure for widget
struct hda_widgetInfo {
    uint32_t        nodeId;
    union {
        struct {
            uint32_t    stereo : 1;
            uint32_t    inAmpPresent : 1;
            uint32_t    outAmpPresent : 1;
            uint32_t    ampParamOverride : 1;
            uint32_t    formatOverride : 1;
            uint32_t    striping : 1;
            uint32_t    procControls : 1;
            uint32_t    unsol : 1;
            uint32_t    connectionList : 1;
            uint32_t    digital : 1;
            uint32_t    powerControl : 1;
            uint32_t    leftRightSwap : 1;
            uint32_t    contentProtection : 1;
            uint32_t    channelCountExt : 3;        // LSB is caps.stereo
            uint32_t    delay : 4;
            uint32_t    widgetType : 4;
            uint32_t    reserved : 4;
        };
        uint32_t    raw;
    } caps;

    hda_widgetInfo  **connList;           // null-terminated list, NULL if non-present
    uint32_t        connListLength;

    union {
        // dac/adc widgets
        struct {
            uint32_t supportedFormats;
            uint32_t currentFormat;
            uint32_t currentStream;
        };
        // pin widgets
        struct {
            uint32_t pinCaps;
            uint32_t pinControl;
            uint32_t pinConfigDefault;
        };
    };

    uint32_t ampInputCaps;
    uint32_t ampOutputCaps;
};

// HDA codec graph
struct hda_codecInfo {
    uint32_t            codecId;

    // widget pool
    hda_widgetInfo     *widgetPool;
    uint32_t            widgetCount;
    uint32_t            widgetStartId;

    // conn list pool
    hda_widgetInfo    **connListPool;
    uint32_t            connListPoolLength;
    uint32_t            connListPoolPointer;

    uint32_t            dacCount;
    uint32_t            adcCount;
    uint32_t            pinCount;
    uint32_t            mixerCount;
    uint32_t            otherCount;

    hda_widgetInfo     *dac[16];
    hda_widgetInfo     *adc[16];
    hda_widgetInfo     *pin[32];
    hda_widgetInfo     *mixer[32];
    hda_widgetInfo     *other[32];

    hda_codecInfo();
    ~hda_codecInfo();

    void clear();
};

// init search structure
struct hda_widgetSearchStruct {
    hda_widgetInfo *parent;
    uint32_t        distance;
    bool            visited;
};

}
