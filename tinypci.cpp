#include <i86.h>
#include <conio.h>
#include <string.h>
#include <stdio.h>
#include "tinypci.h"

//#define MORE_DEBUG

namespace tinypci {
    // bus enumeration helpers
    // list is incremented each time new device is enumerated
    struct enumContext {
        pciDeviceList  *list;
        uint32_t        listSize;
        pciDeviceList   listInfo;
    };

    // recursive bus enumeraion
    bool enumerateBus(enumContext& ctx, uint32_t bus = 0);

    // enumerate device
    bool enumerateDevice(enumContext& ctx, uint32_t bus, uint32_t device);

    // enumerate device function
    bool enumerateFunction(enumContext& ctx, pciAddress addr);

    // convert address to CF8 port value
    uint32_t pciAddrToCF8(pciAddress addr, uint32_t index);
}

uint32_t tinypci::enumerateAll(pciDeviceList *list, uint32_t listSize) {
    pciDeviceList listInfo; memset(&listInfo, -1, sizeof(listInfo));
    return enumerate(list, listSize, listInfo);
}

uint32_t tinypci::enumerateByAddress(pciDeviceList *list, uint32_t listSize, pciAddress addr) {
    pciDeviceList listInfo; memset(&listInfo, -1, sizeof(listInfo));
    listInfo.address = addr;
    return enumerate(list, listSize, listInfo);     // slooooowwww....
}

uint32_t tinypci::enumerateByClass(pciDeviceList *list, uint32_t listSize, pciClass devclass) {
    pciDeviceList listInfo; memset(&listInfo, -1, sizeof(listInfo));
    listInfo.deviceClass = devclass;
    return enumerate(list, listSize, listInfo);
}

uint32_t tinypci::enumerateByDeviceId(pciDeviceList *list, uint32_t listSize, uint32_t vendorId, uint32_t deviceId) {
    pciDeviceList listInfo; memset(&listInfo, -1, sizeof(listInfo));
    listInfo.vendorId = vendorId;
    listInfo.deviceId = deviceId;
    return enumerate(list, listSize, listInfo);
}

uint32_t tinypci::enumerate(pciDeviceList *list, uint32_t listSize, pciDeviceList& listInfo) {
    if ((list == NULL) || (listSize == 0)) return 0;
    enumContext ctx;
    ctx.list = list; ctx.listSize = listSize; ctx.listInfo = listInfo;
    
    // start from bus 0
    pciAddress rootAddr = {0};
    if (configReadWord(rootAddr, 0) == 0xFFFF) return 0;            // no bus?

    // check if multifunction
    if ((configReadByte(rootAddr, 0xE) & 0x80) == 0) {
        // single bus
        enumerateBus(ctx, 0);
    }
    else {
        // multiple buses
        for (rootAddr.function = 0; rootAddr.function < 8; rootAddr.function++) {
            // better to read configspace directly :)
            if (configReadWord(rootAddr, 0) != 0xFFFF) enumerateBus(ctx, rootAddr.function);
        }
    }

    // return total count of device found
    return (listSize - ctx.listSize);
}

bool tinypci::enumerateBus(enumContext& ctx, uint32_t bus) {
#ifdef MORE_DEBUG
    printf("enumerate bus %02X...\n", bus);
#endif
    for (uint32_t device = 0; device < 32; device++) enumerateDevice(ctx, bus, device);
    return true;
}

bool tinypci::enumerateDevice(enumContext& ctx, uint32_t bus, uint32_t device) {
    pciAddress addr; addr.bus = bus; addr.device = device; addr.function = 0;
#ifdef MORE_DEBUG
    printf("enumerate device %02X:%02X...\n", bus, device);
#endif
    // test for device presence
    if ((configReadWord(addr, 0) == 0xFFFF) || (ctx.list == NULL) || (ctx.listSize == 0)) return false;

    // check for multifunction
    if (configReadByte(addr, 0xE) & 0x80) {
        // enumerate subfunctions
        for (addr.function = 0; addr.function < 8; addr.function++) enumerateFunction(ctx, addr);
    } else 
        // enumerate single device
        enumerateFunction(ctx, addr);

    return true;
}

bool tinypci::enumerateFunction(enumContext& ctx, pciAddress addr) {
#ifdef MORE_DEBUG
    printf("enumerate function %02X:%02X.%01X...\n", addr.bus, addr.device, addr.function);
#endif
    // test for device presence
    if ((configReadWord(addr, 0) == 0xFFFF) || (ctx.list == NULL) || (ctx.listSize == 0)) return false;

    // prefill list info
    pciDeviceList list;
    list.address                    = addr;
    list.vendorId                   = configReadWord(addr, 0x0);
    list.deviceId                   = configReadWord(addr, 0x2);
    list.deviceClass.baseClass      = configReadByte(addr, 0xB);
    list.deviceClass.subClass       = configReadByte(addr, 0xA);
    list.deviceClass.progInterface  = configReadByte(addr, 0x9);
    list.headerType                 = configReadByte(addr, 0xE);

    // check if PCI-PCI bridge
    if ((list.deviceClass.baseClass == 0x06) && (list.deviceClass.subClass == 0x04) && ((list.headerType & 0x7F) == 0x1)) {
        // enum secondary bus
        enumerateBus(ctx, configReadByte(addr, 0x19));
    }

#ifdef MORE_DEBUG
    printf("%x %x %x %x\n", ctx.listInfo.address.addr, ctx.listInfo.deviceClass.val, ctx.listInfo.deviceId, ctx.listInfo.vendorId);
    printf("%x %x %x %x\n", list.address.addr, list.deviceClass.val, list.deviceId, list.vendorId);
    printf("%x %x %x\n", ctx.listInfo.address.bus, ctx.listInfo.address.device, ctx.listInfo.address.function);
#endif

    // filter by ctx criterias
    if ((ctx.listInfo.address.bus != 0xFF) && (ctx.listInfo.address.bus != list.address.bus)) return false;
    if ((ctx.listInfo.address.device != 0xFF) && (ctx.listInfo.address.device != list.address.device)) return false;
    if ((ctx.listInfo.address.function != 0xFF) && (ctx.listInfo.address.function != list.address.function)) return false;

    if ((ctx.listInfo.vendorId != 0xFFFF) && (ctx.listInfo.vendorId != list.vendorId)) return false;
    if ((ctx.listInfo.deviceId != 0xFFFF) && (ctx.listInfo.deviceId != list.deviceId)) return false;

    if ((ctx.listInfo.deviceClass.baseClass != 0xFF) && (ctx.listInfo.deviceClass.baseClass != list.deviceClass.baseClass)) return false;
    if ((ctx.listInfo.deviceClass.subClass != 0xFF) && (ctx.listInfo.deviceClass.subClass != list.deviceClass.subClass)) return false;
    if ((ctx.listInfo.deviceClass.progInterface != 0xFF) && (ctx.listInfo.deviceClass.progInterface != list.deviceClass.progInterface)) return false;

    // if passed the checks - put info to the list
    list.interruptLine  = configReadByte (addr, 0x3C);
    list.bar0           = configReadDword(addr, 0x10);
    memcpy(ctx.list, &list, sizeof(list)); ctx.list++; ctx.listSize--;

#ifdef MORE_DEBUG
    printf("ok\n");
#endif

    return true;
}

uint32_t tinypci::pciAddrToCF8(pciAddress addr, uint32_t index) {
    return 0x80000000 | ((addr.function & 0x7) << 8) | ((addr.device & 0x1F) << 11) | ((addr.bus & 0xFF) << 16) | (index & 0xFC) | ((index & 0x300) << 16);
}

// don't inline to prevent timing conflicts
uint8_t tinypci::configReadByte(pciAddress addr, uint32_t index)
{
    outpd(0xCF8, pciAddrToCF8(addr, index));
    return (inpd(0xCFC) >> ((index & 3) << 3)) & 0xFF;
}

uint16_t tinypci::configReadWord(pciAddress addr, uint32_t index)
{
    outpd(0xCF8, pciAddrToCF8(addr, index));
    return (inpd(0xCFC) >> ((index & 2) << 3)) & 0xFFFF;
}

uint32_t tinypci::configReadDword(pciAddress addr, uint32_t index)
{
    outpd(0xCF8, pciAddrToCF8(addr, index));
    return inpd(0xCFC);
}

void tinypci::configWriteByte(pciAddress addr, uint32_t index, uint8_t data)
{
    outpd(0xCF8, pciAddrToCF8(addr, index));

    // read/modify/write
    uint32_t shift = (index & 3) << 3;
    uint32_t mask = 0xFF << shift;
    outpd(0xCFC, (inpd(0xCFC) & ~mask) | ((data << shift) & mask));
}

void tinypci::configWriteWord(pciAddress addr, uint32_t index, uint16_t data)
{
    outpd(0xCF8, pciAddrToCF8(addr, index));

    // read/modify/write
    uint32_t shift = (index & 2) << 3;
    uint32_t mask = 0xFFFF << shift;
    outpd(0xCFC, (inpd(0xCFC) & ~mask) | ((data << shift) & mask));
}

void tinypci::configWriteDword(pciAddress addr, uint32_t index, uint32_t data)
{
    outpd(0xCF8, pciAddrToCF8(addr, index));
    outpd(0xCFC, data);
}
