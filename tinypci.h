#pragma once

#include <stdint.h>

union pciAddress {
    struct {
        uint8_t    function;
        uint8_t    device;
        uint8_t    bus;
        uint8_t    dummy;
    };
    uint32_t    addr;
};

union pciClass {
    struct {
        uint8_t revision;
        uint8_t progInterface;
        uint8_t subClass;
        uint8_t baseClass;
    };
    uint32_t val;
};

struct pciDeviceList {
    pciAddress address;
    pciClass deviceClass;

    // only the essential info stored here
    uint16_t vendorId, deviceId;
    uint32_t bar0;
    uint8_t  headerType;
    uint8_t  interruptLine;
};

// full info with first 64 bytes of configuration space cached 
struct pciDeviceInfo {
    pciAddress  address;
    union {

        struct {
            uint16_t vendorId;
            uint16_t deviceId;
            
            struct {
                uint16_t ioSpace : 1;
                uint16_t memSpace : 1;
                uint16_t busMaster : 1;
                uint16_t specialCycles : 1;
                uint16_t memWriteInvEnable : 1;
                uint16_t vgaSnoop : 1;
                uint16_t parityCheck : 1;
                uint16_t reserved1 : 1;
                uint16_t serrEnable : 1;
                uint16_t fastb2bEnable : 1;
                uint16_t intDisable : 1;
                uint16_t reserved2 : 5;
            } command;

            struct {
                uint16_t reserved1 : 3;
                uint16_t intStatus : 1;
                uint16_t capsList : 1;
                uint16_t cap66Mhz : 1;
                uint16_t reserved2 : 1;
                uint16_t capFastb2b : 1;
                uint16_t masterParityError : 1;
                uint16_t devselTiming : 2;
                uint16_t targetAbortSignaled : 1;
                uint16_t targetAbortRecieved : 1;
                uint16_t masterAbortRecieved : 1;
                uint16_t systemErrorSignaled : 1;
                uint16_t parityError : 1;
            } status;

            uint8_t  revision;
            uint8_t  progInterface;
            uint8_t  subClass;
            uint8_t  baseClass;

            uint8_t  cacheLineSize;
            uint8_t  latencyTimer;
            uint8_t  headerType;             // bit 7 - multifunctional device
            uint8_t  bist;

            uint32_t bar[6];
            uint32_t cardbusCIS;

            uint16_t subsysVendorId;
            uint16_t subsysDeviceId;

            uint32_t romBase;
            uint8_t  capsPointer;
            uint8_t  reserved[7];

            uint8_t  intLine;
            uint8_t  intPin;
            uint8_t  minGnt;
            uint8_t  maxLat;
        } header;

        // only first 64 bytes of config space are stored here
        uint8_t     cfg8[64];
        uint16_t    cfg16[32];
        uint32_t    cfg32[16];
    };
};

namespace tinypci {
    // ------------------------
    // enumeration methods, all return number of devices found and recorded to list

    // enumerate all devices
    uint32_t enumerateAll(pciDeviceList *list, uint32_t listSize);

    // enumerate by address
    uint32_t enumerateByAddress(pciDeviceList *list, uint32_t listSize, pciAddress addr);

    // enumerate by class
    uint32_t enumerateByClass(pciDeviceList *list, uint32_t listSize, pciClass devclass);

    // enumerate by vendor/device id 
    uint32_t enumerateByDeviceId(pciDeviceList *list, uint32_t listSize, uint32_t vendorId, uint32_t deviceId = -1);

    // enumerate by all, don't care fields set to -1
    uint32_t enumerate(pciDeviceList *list, uint32_t listSize, pciDeviceList& listInfo);

    // read config space byte/word/dword
    uint8_t  configReadByte (pciAddress addr, uint32_t index);
    uint16_t configReadWord (pciAddress addr, uint32_t index);
    uint32_t configReadDword(pciAddress addr, uint32_t index);

    // write config space byte/word/dword
    void  configWriteByte (pciAddress addr, uint32_t index, uint8_t data);
    void  configWriteWord (pciAddress addr, uint32_t index, uint16_t data);
    void  configWriteDword(pciAddress addr, uint32_t index, uint32_t data);

};

