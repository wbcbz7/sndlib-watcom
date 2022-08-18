#include "snddefs.h"
#ifdef SNDLIB_DEVICE_ENABLE_HDA

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "convert.h"
#include "sndmisc.h"
#include "snderror.h"
#include "devhda.h"
#include "logerror.h"
#include "tinypci.h"

// inpirt HDA definitions
#include "hdadefs.h"
using namespace sndlib;

//define to enable logging
//#define DEBUG_LOG

#define arrayof(x) (sizeof(x) / sizeof(x[0]))

// due to HDA codec "variable" nature, all relevant capability info is read from the codec and stored
// in deviceInfo::privateBuf

// TODO: handle 96/192k
const uint32_t hdaRates[] = {44100, 48000};
const uint32_t hdaRates48kOnly[] = {48000};       // todo: are there any codecs suporting 48k only?

// HDA controller blacklist (to filter out HDMI output codecs and other broken crap)
static const uint32_t hda_DeivceBlacklist[] = {
    0x0A0C8086,             // Intel Haswell   HDMI Audio
    0x0C0C8086,             // Intel Haswell   HDMI Audio
    0x0D0C8086,             // Intel Haswell   HDMI Audio
    0x160C8086,             // Intel Broadwell HDMI Audio
    0x160C8086,             // Intel Broadwell HDMI Audio
};

// internal HDA functions
namespace sndlib {

// ----------------------------------------------
// hda_codecInfo class constructors/destructors
hda_codecInfo::hda_codecInfo()  :
    widgetPool(NULL), connListPool(NULL) {
    clear();
}

void hda_codecInfo::clear() {
    if (widgetPool != NULL) delete[] widgetPool;
    if (connListPool != NULL) delete[] connListPool;

    codecId = dacCount = adcCount = pinCount = mixerCount = otherCount = 0;
    widgetCount = widgetStartId = 0;
    connListPoolLength = connListPoolPointer = 0;

    memset(dac, NULL, sizeof(dac));
    memset(adc, NULL, sizeof(adc));
    memset(pin, NULL, sizeof(pin));
    memset(mixer, NULL, sizeof(mixer));
    memset(other, NULL, sizeof(other));
}

hda_codecInfo::~hda_codecInfo() {
    clear();
}

// delay via wall clock
void __stdcall hdaDelay(void * base, uint32_t delay) {
    uint32_t newWall = HDA_REG_FORCE_READ32(base, HDA_REG_WALLCLK) + delay; if (newWall < 0x100) newWall = 0x100;
    // test if rolls over
    if (HDA_REG_FORCE_READ32(base, HDA_REG_WALLCLK)  > newWall)
        while (HDA_REG_FORCE_READ32(base, HDA_REG_WALLCLK)  > newWall);
    // do the delay
    while (HDA_REG_FORCE_READ32(base, HDA_REG_WALLCLK)  < newWall);
}
void __stdcall hdaDelay_us(void * base, uint32_t delay) {hdaDelay(base, (delay * 49) >> 1);}  // in microseconds
void __stdcall hdaDelay_ms(void * base, uint32_t delay) {hdaDelay(base,  delay * 24576);}     // in milliseconds

// send command verb over HDA link, returns response from the codec
hda_codecResponse __stdcall hdaSendVerb(void * hdaregs, hda_codecVerb verb) {
    // use immediate command interface

    // wait until ICS bit 0 is 0 (with timeout)
    uint32_t timeout = 1000;
    while ((--timeout) && ((HDA_REG_READ32(hdaregs, HDA_REG_ICS) & HDA_ICS_BUSY) != 0)) hdaDelay_us(hdaregs, 10);
    if (timeout == 0) {
        // clear ICS bit 0 manually, try again
        HDA_REG_MODIFY32(hdaregs, HDA_REG_ICS, HDA_ICS_BUSY, 0);
        timeout = 1000;
        while ((--timeout) && ((HDA_REG_READ32(hdaregs, HDA_REG_ICS) & HDA_ICS_BUSY) != 0)) hdaDelay_us(hdaregs, 10);
        // if timed out - oops..
        if (timeout == 0) {
#ifdef DEBUG_LOG
            printf("busy timeout!\n");
#endif
            return 0;        // unable to send blabla
        }
    }
    // send verb
    HDA_REG_WRITE32 (hdaregs, HDA_REG_ICW, verb.val);
    HDA_REG_MODIFY32(hdaregs, HDA_REG_ICS, HDA_ICS_BUSY, 1);

    // wait for ICS bit 1
    timeout = 1000;
    while ((--timeout) && ((HDA_REG_READ32(hdaregs, HDA_REG_ICS) & (HDA_ICS_BUSY | HDA_ICS_VALID)) != HDA_ICS_VALID)) hdaDelay_us(hdaregs, 10);
    if (timeout == 0) {
#ifdef DEBUG_LOG
        printf("response valid timeout!\n");
#endif
        return 0;        // unable to send blabla
    }

    // small delay for slow HDA controllers/codecs
    hdaDelay_us(hdaregs, 30);

    // read response, clear ICS bit 1 by writing 1
    hda_codecResponse response = HDA_REG_READ32(hdaregs, HDA_REG_IRR); HDA_REG_WRITE32(hdaregs, HDA_REG_ICS, HDA_ICS_VALID);

#ifdef DEBUG_LOG
    logdebug("verb %08X, response %08X\n", verb.value(), response.value());
#endif

    return response;
}

// size-optimized shortcuts
hda_codecResponse __stdcall hdaSendVerb(void * hdaregs, uint32_t codecId, uint32_t nodeId, uint32_t verb, uint32_t param) {
    hda_codecVerb v;
    v.codecAddress = codecId;
    v.nodeId       = nodeId;
    v.payload      = verb | param;
    return hdaSendVerb(hdaregs, v);
}
hda_codecResponse __stdcall hdaSendVerb(void * hdaregs, uint32_t codecNodeId, uint32_t verbParam) {
    hda_codecVerb v; v.val = (codecNodeId << 20) | (verbParam);
    return hdaSendVerb(hdaregs, v);
}

uint32_t hda_getConnectionListLength(void * hdaregs, uint32_t codecId, uint32_t nodeId) {
    return hdaSendVerb(hdaregs, codecId, nodeId, HDA_VERB_GET_PARAMETER, 0xE).value() & 0x7F;
}

uint32_t hda_getConnectionList(void * hdaregs, uint32_t codecId, uint32_t nodeId, uint32_t *connList, uint32_t listLength) {
    hda_codecResponse resp = hdaSendVerb(hdaregs, codecId, nodeId, HDA_VERB_GET_PARAMETER, 0xE);
    // fill conn list
    if (resp.value() & (1 << 7)) {
        // long form
        for (uint32_t i = 0; i < listLength; i += 2) {
            resp = hdaSendVerb(hdaregs, codecId, nodeId, HDA_VERB_GET_CONNECTION_LIST_ENTRY, i);
            connList[i] = resp.value() & 0xFFFF; connList[i + 1] = resp.value() >> 16;
        }
    } else {
        // short form
        for (uint32_t i = 0; i < listLength; i += 4) {
            resp = hdaSendVerb(hdaregs, codecId, nodeId, HDA_VERB_GET_CONNECTION_LIST_ENTRY, i);
            connList[i] = resp.value() & 0xFF; connList[i + 1] = (resp.value() >> 8) & 0xFF;
            connList[i + 2] = (resp.value() >> 16) & 0xFF; connList[i + 3] = (resp.value() >> 24) & 0xFF;
        }
    }
    return listLength;
}

// build codec graph
bool hda_buildCodecGraph(void * hdaregs, uint32_t codecId, hda_codecInfo * graph) {
    // find AFG
    hda_codecResponse resp = hdaSendVerb(hdaregs, codecId, HDA_VERB_GET_PARAMETER + 4);

    // get function group type
    //hda_codecResponse functionGroup    = hdaSendVerb(hdaregs, codecId | ((resp.value() >> 16) & 0xFF), HDA_VERB_GET_PARAMETER | 5);
    hda_codecResponse subordinateNodes = hdaSendVerb(hdaregs, codecId + ((resp.value() >> 16) & 0xFF), HDA_VERB_GET_PARAMETER | 4);
    uint32_t firstChild = (subordinateNodes.value() >> 16) & 0xFF, numChilds = subordinateNodes.value() & 0xFF;
    // set D0 power state
    hdaSendVerb(hdaregs, codecId + ((resp.value() >> 16) & 0xFF), HDA_VERB_SET_POWER_STATE + 0);

    // alloc memory for audioInfo
    graph->codecId    = codecId;
    graph->widgetPool = new hda_widgetInfo[numChilds];
    memset(graph->widgetPool, 0, sizeof(hda_widgetInfo) * numChilds);
    graph->widgetCount   = numChilds;
    graph->widgetStartId = firstChild;
    // alloc connection list pool
    graph->connListPool = new hda_widgetInfo*[numChilds * 16];   // should be enough for most codecs
    memset(graph->connListPool, 0, sizeof(hda_widgetInfo*) * numChilds * 16);
    graph->connListPoolLength = numChilds * 16;

    // 1st pass - iterate over all widgets, fill preliminary info
    hda_widgetInfo* currentWidget = graph->widgetPool;
    uint32_t        codecIdNodeId = (codecId << 8) + firstChild;
    for (uint32_t n = firstChild; n < firstChild + numChilds; n++, currentWidget++, codecIdNodeId++) {
        currentWidget->nodeId = n;

        // get child info
        hda_codecResponse caps = hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_GET_PARAMETER + 9);
        uint32_t widgetType = (caps.value() >> 20) & 15;

        currentWidget->caps.raw = caps.value();
        currentWidget->connList = NULL;
        currentWidget->connListLength = 0;

        switch (currentWidget->caps.widgetType) {
            case HDA_WIDGET_AUDIO_OUTPUT:
            case HDA_WIDGET_AUDIO_INPUT:
                // link widget
                if (currentWidget->caps.widgetType == HDA_WIDGET_AUDIO_INPUT) {
                    if (graph->adcCount < arrayof(graph->adc)) {
                        graph->adc[graph->adcCount] = currentWidget;
                        graph->adcCount++;
                    }
                } else {
                    if (graph->dacCount < arrayof(graph->dac)) {
                        graph->dac[graph->dacCount] = currentWidget;
                        graph->dacCount++;
                    }
                }

                // get supported formats and other info
                currentWidget->supportedFormats = hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_GET_PARAMETER | 0xA).value();
                currentWidget->currentFormat    = hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_GET_CONVERTER_FORMAT | 0).value();
                currentWidget->currentStream    = hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_GET_CONVERTER_STREAM_CHANNEL | 0).value();

                break;

            case HDA_WIDGET_AUDIO_MIXER:
            case HDA_WIDGET_AUDIO_SELECTOR:
                // link widget
                if (graph->mixerCount < arrayof(graph->mixer)) {
                    graph->mixer[graph->mixerCount] = currentWidget;
                    graph->mixerCount++;
                }
                break;

            case HDA_WIDGET_AUDIO_PIN:
                // link widget
                if (graph->pinCount < arrayof(graph->pin)) {
                    graph->pin[graph->pinCount] = currentWidget;
                    graph->pinCount++;
                }

                // get pin info
                currentWidget->pinCaps          = hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_GET_PARAMETER | 0xC).value();
                currentWidget->pinControl       = hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_GET_PIN_WIDGET_CONTROL | 0).value();
                currentWidget->pinConfigDefault = hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_GET_CONFIGURATION_DEFAULT | 0).value();

                break;

            default:
                // link widget
                if (graph->otherCount < arrayof(graph->other)) {
                    graph->other[graph->otherCount] = currentWidget;
                    graph->otherCount++;
                }
                break;
        }

        // get amplifier caps
        if (currentWidget->caps.inAmpPresent)  currentWidget->ampInputCaps  = hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_GET_PARAMETER | 0xD).value();
        if (currentWidget->caps.outAmpPresent) currentWidget->ampOutputCaps = hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_GET_PARAMETER | 0x12).value();
    }

    // 2nd pass - build connection list
    currentWidget = graph->widgetPool;
    for (uint32_t n = firstChild; n < firstChild + numChilds; n++, currentWidget++) {
        uint32_t connListLength = hda_getConnectionListLength(hdaregs, codecId, n);
        if (connListLength == 0) continue;

        // fill connection list from the pool
        currentWidget->connList = graph->connListPool + graph->connListPoolPointer;
        currentWidget->connListLength = connListLength;
        hda_getConnectionList(hdaregs, codecId, n, (uint32_t*)currentWidget->connList, connListLength);

        for (uint32_t i = 0; i < connListLength; i++) {
            graph->connListPool[graph->connListPoolPointer] = graph->widgetPool + (*((uint32_t*)graph->connListPool + graph->connListPoolPointer) - firstChild);
            graph->connListPoolPointer++;
        }
        // null-terminate
        graph->connListPool[graph->connListPoolPointer] = NULL; graph->connListPoolPointer++;
    }

#ifdef DEBUG_LOG
    logdebug("widget graph done\n");
#endif
    return true;
}

// build BFS tree for given widget, treeLength = widgetStartId + widgetCount
// filters out vendor-defined widgets and other absent crap
bool hdaBuildBFSTree(hda_widgetSearchStruct * tree, uint32_t treeLength, hda_widgetInfo * widget) {
    if ((widget == NULL) || (tree == NULL) || (treeLength == 0)) return false;   // invalid arguments
    if (widget->connListLength == 0) return true;

    memset(tree, 0, sizeof(hda_widgetSearchStruct) * treeLength);

    // init queue
    hda_widgetInfo *queueRing[128]; uint32_t queueHead = 0, queueTail = 0;

    // start from widget
    tree[widget->nodeId].distance = 0; tree[widget->nodeId].visited = true;
    queueRing[queueHead++] = widget; queueHead &= 127;

    // iterate!
    while (queueHead != queueTail) {
        hda_widgetInfo **connPin = queueRing[queueTail]->connList;
        while ((connPin != NULL) && (*connPin != NULL)) {
            // reject vendor-defined widgets and other crap
            if ((*connPin)->caps.widgetType == HDA_WIDGET_AUDIO_VENDOR_DEFINED) continue;
            if (tree[(*connPin)->nodeId].visited == false) {
                tree[(*connPin)->nodeId].visited = true;
                tree[(*connPin)->nodeId].distance = tree[queueRing[queueTail]->nodeId].distance + 1;
                tree[(*connPin)->nodeId].parent = queueRing[queueTail];
                queueRing[queueHead++] = *connPin; queueHead &= 127;
            }
            connPin++;
        }
        // dequeue
        queueTail++; queueTail &= 127;
    };

    // tree is built
    return true;
}

// get closest path by BFS tree, fills reversed path, returns path length
uint32_t hdaGetShortestPath(hda_widgetSearchStruct * tree, hda_widgetInfo * source, hda_widgetInfo * target, hda_widgetInfo** path, uint32_t maxPathLength) {
    hda_widgetInfo* currentNode = target; uint32_t pathLength = 0;
    if (tree[currentNode->nodeId].parent == NULL) return NULL;

    while (maxPathLength != 0) {
        *path++ = currentNode; maxPathLength--; pathLength++;
        if ((currentNode->caps.widgetType == HDA_WIDGET_AUDIO_PIN) && (currentNode->nodeId != source->nodeId)) return 0; // pin loop, ABORT!
        if (tree[currentNode->nodeId].parent == NULL) break; else currentNode = tree[currentNode->nodeId].parent;
    };

    return pathLength;
}

// returns position in connlist or -1 or not found, O(N) :D
uint32_t hda_findWidgetInConnectionList(hda_widgetInfo * src, uint32_t dst) {
    for (size_t i = 0; i < src->connListLength; i++) {
        if (src->connList[i]->nodeId == dst) return i;
    }
    return -1;
}

// enable playback path
void hda_initPlaybackPath(void * hdaregs, uint32_t codecId, hda_widgetInfo **path, uint32_t pathSize, uint32_t streamFormat, uint32_t streamTag) {
    for (size_t i = 0; i < pathSize; i++) {
        uint32_t codecIdNodeId = (codecId << 8 | path[i]->nodeId);

        if (path[i]->caps.powerControl) {
            // set D0 power state
            hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_SET_POWER_STATE | 0);
        }

        switch (path[i]->caps.widgetType) {
            case HDA_WIDGET_AUDIO_PIN:
                if (path[i]->pinCaps & (1 << 4)) {
                    // output only, hp amplifier enabled, vref disabled
                    hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_SET_PIN_WIDGET_CONTROL | 0xC0); 
                }
                if (path[i]->pinCaps & (1 << 3)) {
                    // enable EAPD
                    hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_SET_EAPD_BTL | 1);
                }
                break;
            
            case HDA_WIDGET_AUDIO_OUTPUT:
                // set format
                hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_SET_CONVERTER_FORMAT + streamFormat);
                // set stream tag, 0th lowest channel
                hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_SET_CONVERTER_STREAM_CHANNEL + (streamTag << 4));
                if (path[i]->caps.digital) {
                    // enable SPDIF, consumer format, PCM, original
                    hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_SET_DIGITAL_CONVERTER_CONTROL + 1);
                }
                break;

            default:
                // idk, skip it
                break;
        }

        // check if mux/mixer available
        if (path[i]->caps.inAmpPresent) {
            if (i != 0) {
                // find next widget index
                uint32_t nextNodeId = hda_findWidgetInConnectionList(path[i], path[i - 1]->nodeId);
                if (nextNodeId != -1) {
                    // set 0db and unmute both left and right channels
                    hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_SET_AMPLIFIER_GAIN |
                        0x4000 | 0x3000 | (nextNodeId << 8) | (path[i]->ampInputCaps & 0x7F)
                    );        
                }
            }
        }
        if (path[i]->caps.outAmpPresent) {
            // set 0db and unmute both left and right channels
            hdaSendVerb(hdaregs, codecIdNodeId, HDA_VERB_SET_AMPLIFIER_GAIN |
                0x8000 | 0x3000 | (path[i]->ampOutputCaps & 0x7F)
            );   
        }
    }
}

// reset stream,
void hda_resetStream(void *hdaregs, uint32_t stream) {
    uint32_t timeout;

    // stop stream
    HDA_STREAM_MODIFY8(hdaregs, stream, HDA_REG_STREAM_CTRL, HDA_STREAM_CTRL_RUN, 0);
    // reset stream
    HDA_STREAM_MODIFY8(hdaregs, stream, HDA_REG_STREAM_CTRL, HDA_STREAM_CTRL_SRST, HDA_STREAM_CTRL_SRST);
    hdaDelay_us(hdaregs, 80);

    // wait until stream acknowledged reset
    timeout = 100;
    while ((--timeout) && ((HDA_STREAM_READ8(hdaregs, stream, HDA_REG_STREAM_CTRL) & HDA_STREAM_CTRL_SRST) == 0)) hdaDelay_us(hdaregs, 20);

    // pull stream from reset
    HDA_STREAM_MODIFY8(hdaregs, stream, HDA_REG_STREAM_CTRL, HDA_STREAM_CTRL_SRST, 0);

    // wait until stream restarted
    timeout = 100;
    while ((--timeout) && ((HDA_STREAM_READ8(hdaregs, stream, HDA_REG_STREAM_CTRL) & HDA_STREAM_CTRL_SRST) != 0)) hdaDelay_us(hdaregs, 20);
}

// clear HDA controller engines
void hda_clearEngines(void * hdaregs) {
    // stop RIRB/CORB DMA engines
    HDA_REG_WRITE8(hdaregs, HDA_REG_CORBCTL, 0);
    HDA_REG_WRITE8(hdaregs, HDA_REG_RIRBCTL, 0);
    HDA_REG_WRITE8(hdaregs, HDA_REG_CORBSTS, HDA_REG_FORCE_READ8(hdaregs, HDA_REG_CORBSTS));
    HDA_REG_WRITE8(hdaregs, HDA_REG_RIRBSTS, HDA_REG_FORCE_READ8(hdaregs, HDA_REG_RIRBSTS));

    // stop DMA position engine
    HDA_REG_WRITE32(hdaregs, HDA_REG_DPLBASE, 0);
    HDA_REG_WRITE32(hdaregs, HDA_REG_DPUBASE, 0);

    // stop stream DMA engines, clear pending interrupts
    for (uint32_t i = 0 ; i < 30; i++) HDA_STREAM_MODIFY32(hdaregs, i, HDA_REG_STREAM_CTRL, 0xFF000000, 0);

    // disable all interrupts, clear pending interrupts
    HDA_REG_WRITE32(hdaregs, HDA_REG_INTCTL, 0);
    HDA_REG_WRITE32(hdaregs, HDA_REG_INTSTS, HDA_REG_FORCE_READ32(hdaregs, HDA_REG_INTSTS));
}

void hda_setupStreamController(void *hdaregs, uint32_t stream, uint32_t streamTag, hda_streamFormat streamFormat, uint32_t bufferTotalBytes, hda_bufferDescriptor* bufferDesc, uint32_t bufferCount) {
    // set stream tag, enable buffer completion interrupt
    HDA_STREAM_MODIFY32(hdaregs, stream, HDA_REG_STREAM_CTRL, 0x00FFFFFF, (streamTag << HDA_STREAM_CTRL_STREAM_TAG_SHIFT) | HDA_STREAM_INT_BUFFER_COMPLETE);

    // set other properties
    HDA_STREAM_FORCE_WRITE32(hdaregs, stream, HDA_REG_STREAM_CBL, bufferTotalBytes);
    HDA_STREAM_FORCE_WRITE16(hdaregs, stream, HDA_REG_STREAM_LVI, bufferCount - 1);
    HDA_STREAM_FORCE_WRITE16(hdaregs, stream, HDA_REG_STREAM_FORMAT, streamFormat.raw);
    HDA_STREAM_FORCE_WRITE32(hdaregs, stream, HDA_REG_STREAM_BDLPU, 0);
    HDA_STREAM_FORCE_WRITE32(hdaregs, stream, HDA_REG_STREAM_BDLPL, (uint32_t)bufferDesc);

    // wait a bit
    hdaDelay_us(hdaregs, 40);
}

// reset HDA controller, returns codec wake up mask or 0 if failed/no codecs
uint32_t hda_reset(void* hdaregs) {
    uint32_t timeout;

    // clear HDA controller engines
    hda_clearEngines(hdaregs);

    // put controller on reset
    HDA_REG_WRITE16(hdaregs, HDA_REG_STATESTS, HDA_REG_FORCE_READ16(hdaregs, HDA_REG_STATESTS));
    HDA_REG_WRITE32(hdaregs, HDA_REG_GCTL, 0);
    timeout = (1 << 21);
    while ((--timeout) && ((HDA_REG_READ32(hdaregs, HDA_REG_GCTL) & HDA_GCTL_CRST) != 0));
    if (timeout == 0) {
#ifdef DEBUG_LOG
        printf("controller reset timeout!\n");
#endif
        return 0;
    }

    // additional delay
    timeout = (1 << 16);
    while (--timeout) (HDA_REG_FORCE_READ32(hdaregs, HDA_REG_GCTL));    // read and discard result

    // put controller from reset
    HDA_REG_WRITE32(hdaregs, HDA_REG_GCTL, 1);
    timeout = (1 << 21);
    while ((--timeout) && ((HDA_REG_READ32(hdaregs, HDA_REG_GCTL) & HDA_GCTL_CRST) == 0));
    if (timeout == 0) {
#ifdef DEBUG_LOG
        printf("controller wakeup timeout!\n");
#endif
        return 0;
    }

    // controller is running, wait for codecs wakeup
    timeout = 1000;
    while ((--timeout) && (HDA_REG_READ16(hdaregs, HDA_REG_STATESTS) == 0)) hdaDelay_us(hdaregs, 10);
    if (timeout == 0) {
#ifdef DEBUG_LOG
        printf("controller wakeup timeout!\n");
#endif
        return 0;
    }

    // another safety delay
    hdaDelay_ms(hdaregs, 1);

    uint32_t codecMask = HDA_REG_READ16(hdaregs, HDA_REG_STATESTS);
#ifdef DEBUG_LOG
    printf("codec mask: 0x%02X\n", codecMask);
#endif
    
    // acknowledge codec wakeup
    HDA_REG_WRITE16(hdaregs, HDA_REG_STATESTS, codecMask);

    // clear HDA controller engines
    hda_clearEngines(hdaregs);

    return codecMask;
}

// enable global interrupt engine
void hda_enableStreamInterrupt(void* hdaregs, uint32_t stream) {
    // clear pending interrupts
    HDA_REG_WRITE32(hdaregs, HDA_REG_INTSTS, HDA_REG_FORCE_READ32(hdaregs, HDA_REG_INTSTS));
    // enable stream interrupt
    HDA_REG_MODIFY32(hdaregs, HDA_REG_INTCTL, (1 << stream) | HDA_INT_GLOBAL_MASK, (1 << stream) | HDA_INT_GLOBAL_MASK);
}

// disable global interrupt engine
void hda_disableStreamInterrupt(void* hdaregs, uint32_t stream) {
    HDA_REG_MODIFY32(hdaregs, HDA_REG_INTCTL, (1 << stream), 0);
}

// start stream
void hda_startStream(void* hdaregs, uint32_t stream) {
    uint32_t streamMask = (1 << stream);
    HDA_REG_MODIFY32(hdaregs, HDA_REG_SSYNC, streamMask, streamMask);
    HDA_STREAM_MODIFY8(hdaregs, stream, HDA_REG_STREAM_CTRL, HDA_STREAM_CTRL_RUN, HDA_STREAM_CTRL_RUN);
    uint32_t timeout = 100;
    while ((--timeout) && ((HDA_STREAM_READ8(hdaregs, stream, HDA_REG_STREAM_CTRL) & HDA_STREAM_CTRL_RUN) == 0)) hdaDelay_us(hdaregs, 10);
    HDA_REG_MODIFY32(hdaregs, HDA_REG_SSYNC, streamMask, 0);
}

// stop stream
void hda_stopStream(void* hdaregs, uint32_t stream) {
    uint32_t streamMask = (1 << stream);
    HDA_REG_MODIFY32(hdaregs, HDA_REG_SSYNC, streamMask, streamMask);
    HDA_STREAM_MODIFY8(hdaregs, stream, HDA_REG_STREAM_CTRL, HDA_STREAM_CTRL_RUN, 0);
    uint32_t timeout = 100;
    while ((--timeout) && ((HDA_STREAM_READ8(hdaregs, stream, HDA_REG_STREAM_CTRL) & HDA_STREAM_CTRL_RUN) != 0)) hdaDelay_us(hdaregs, 10);
}

// setup HDA controller PCI device, map MMIO registers
void hda_setupPCI(pciAddress pciaddr) {
    // enable PCI interrupts, bus master and memory space, clear pending errors
    tinypci::configWriteDword(pciaddr, 0x4, (tinypci::configReadDword(pciaddr, 0x4) & ~(1 << 10)) | (1 << 2) | (1 << 1));

    // intel-specific quirks
    if ((tinypci::configReadDword(pciaddr, 0) & 0xFFFF) == 0x8086) {
        // set traffic class to 0 (reg 0x44) for Intel HDA controllers - fixes static/DMA freezing/no IRQs
        tinypci::configWriteDword(pciaddr, 0x44, tinypci::configReadDword(pciaddr, 0x44) & ~7);
        // force HDA controller to reset "no snoop" flag for isochronous transfers
        tinypci::configWriteDword(pciaddr, 0x78, tinypci::configReadDword(pciaddr, 0x78) & ~(1 << 11));
    }
}

};

// ---------------------------------
// now for the usual stuff

sndHDAudio::sndHDAudio() : DmaBufferDevice("High Definition Audio", 512) {  // reserve more memory for private buffer
    hdaStreamFormat.raw = 0;
    hdaStreamTag = 0;
    hdaStreamIndex = -1;
    codecGraph.clear();
    memset(&pciInfo, 0, sizeof(pciInfo));
    memset(&bufferDescriptor, 0, sizeof(bufferDescriptor));
    
    bufferDescriptor.ptr = NULL; bufferDescriptor.dpmi.segment = bufferDescriptor.dpmi.selector = 0;
};

uint32_t sndHDAudio::resetCodec(SoundDevice::deviceInfo* info, bool setupPCI) {
    if (info == NULL) return SND_ERR_NULLPTR;

    // init PCI stuff
    if (setupPCI == true) hda_setupPCI(info->pci);

    // reset controller, check codec mask
    uint32_t codecMask = hda_reset(info->membase);
    if (codecMask == 0) return SND_ERR_NOTFOUND;

    // poke the codec, reuse info->iobase for codec id
    info->iobase = 0; while ((codecMask & 1) == 0) { codecMask >>= 1; info->iobase++; } 
    info->iobase <<= 8;

    // get vendor/device id
    hda_codecResponse resp = hdaSendVerb(info->membase, info->iobase, HDA_VERB_GET_PARAMETER);
    if (resp.value() == 0) return SND_ERR_NOTFOUND;

    // test if audio function group exists
    resp = hdaSendVerb(info->membase, info->iobase, HDA_VERB_GET_PARAMETER + 4);
    resp = hdaSendVerb(info->membase, info->iobase + (resp.value() >> 16), HDA_VERB_GET_PARAMETER + 5);
    if ((resp.value() & 0xFF) != 1) return SND_ERR_NOTFOUND;

    return SND_ERR_OK;
}

// detect HDA controller/codec
// returns 0 if failure, 1 if found
// several info fields are abused:
//  info->membase: mapped HDA codec registers;
//  info->iobase:  codec ID << 8

bool sndHDAudio::hdaDetect(SoundDevice::deviceInfo* info, bool manualDetect) {
    if (info == NULL) return false;
    info->membase = NULL; info->iobase = 0; info->iobase2 = 0;
    
    // start PCI scan
    pciDeviceList hdaDevs[64];      // enough
    pciClass hdaClass;
    hdaClass.baseClass      = 0x4;
    hdaClass.subClass       = 0x3;
    hdaClass.progInterface  = 0;
    uint32_t hdaDevsCount = tinypci::enumerateByClass(hdaDevs, arrayof(hdaDevs), hdaClass);
    if (hdaDevsCount == 0) return false;

    // test if overriden (TODO)

    // test each HDA controller
    pciDeviceList* it = hdaDevs; bool controllerFound = false;
    for (uint32_t i = 0; i < hdaDevsCount; i++, it++) {
        // test if codec in the blacklist
        bool blacklisted = false;
        uint32_t vendev = (it->vendorId | ((uint32_t)it->deviceId << 16));
        for (size_t i = 0; i < arrayof(hda_DeivceBlacklist); i++)
            if (vendev == hda_DeivceBlacklist[i]) {blacklisted = true; break;}
        if (blacklisted) continue;
        
        // map controller MMIO registers
        info->membase = dpmi_mapphysical(4096, (void*)(it->bar0 & ~0xF));
        if ((info->membase == NULL) || (dpmi_status)) continue;
        
        // test if WALLCLK increments
        uint32_t clock = HDA_REG_FORCE_READ32(info->membase, HDA_REG_WALLCLK), timeout = 0x100;
        while (--timeout) {if (clock != HDA_REG_FORCE_READ32(info->membase, HDA_REG_WALLCLK)) break;};
        if (timeout == 0) {
            // not an HDA controller, ditch it
            dpmi_unmapphysical(info->membase); info->membase = NULL; continue;
        }

        // reset controller, fill info about codec and first AFG
        if (resetCodec(info, false) != SND_ERR_OK) {
            // ditch it
            dpmi_unmapphysical(info->membase); info->membase = NULL; continue;
        }

        // indeed it's an HD audio device :) save info
        info->pci = it->address;
        info->irq = it->interruptLine;
        pciInfo   = *it;
        return true;
    }

    // not found!
    return false;
};

// scan codec DACs for supported formats, returns format mask
uint32_t sndHDAudio::getCodecCaps(SoundDevice::deviceInfo* info) {
    hda_codecResponse resp;
    // get AFG start
    resp = hdaSendVerb(info->membase, info->iobase, HDA_VERB_GET_PARAMETER + 4);
    resp = hdaSendVerb(info->membase, info->iobase + ((resp.value() >> 16) & 0xFF), HDA_VERB_GET_PARAMETER + 4);
    uint32_t firstChild = (resp.value() >> 16) & 0xFF, numChilds = resp.value() & 0xFF;
    uint32_t formatCaps = 0xFFFFFFFF;   // general format mask
    uint32_t codecIdNodeId = info->iobase + firstChild;

    // scan all widgets, find DACs, get DAC caps
    for (uint32_t n = firstChild; n < firstChild + numChilds; n++, codecIdNodeId++) {
        hda_codecResponse caps = hdaSendVerb(info->membase, codecIdNodeId, HDA_VERB_GET_PARAMETER + 9);
        if (((caps.value() >> 20) & 15) == HDA_WIDGET_AUDIO_OUTPUT) {
            formatCaps &= hdaSendVerb(info->membase, codecIdNodeId, HDA_VERB_GET_PARAMETER + 0xA).value();
        }
    }

    return formatCaps;
};

// fill codec info, assumes info->membase is valid
uint32_t sndHDAudio::fillCodecInfo(SoundDevice::deviceInfo* info) {
    if ((info == NULL) || (info->membase == NULL)) return SND_ERR_NULLPTR;

    // fill info
    info->name = "High Definition Audio";
    info->maxBufferSize = 32768;  // BYTES
    info->flags = 0;

    uint32_t pciVendorDeviceId   = tinypci::configReadDword(info->pci, 0);
    uint32_t codecVendorDeviceId = hdaSendVerb(info->membase, info->iobase, HDA_VERB_GET_PARAMETER).value();
    uint8_t* privateBufPtr = (uint8_t*)info->privateBuf;

    // get vendor/device info
    int stringLength = snprintf(
        info->privateBuf, info->privateBufSize, "PCI %02d:%02d.%01d [%04X:%04X], codec [%04X:%04X]",
        info->pci.bus, info->pci.device, info->pci.function,
        (pciVendorDeviceId & 0xFFFF), pciVendorDeviceId >> 16,
        (codecVendorDeviceId >> 16),  codecVendorDeviceId & 0xFFFF
    ) + 1;      // with trailing '\0'

    // link version string
    info->version = info->privateBuf; privateBufPtr += stringLength;

    // extract rates supported by the codec
    uint32_t              codecCaps = getCodecCaps(info);
    
    soundFormatCapability soundCaps;
    if (codecCaps & (1 << 5)) {
        soundCaps.ratesLength = 2; soundCaps.rates = hdaRates;
    } else {
        // alas, 48K only :(
        soundCaps.ratesLength = 1; soundCaps.rates = hdaRates48kOnly;
    }
    
    // extract bit depths
    soundFormatCapability *pCaps = (soundFormatCapability*)privateBufPtr;
    info->caps = pCaps; info->capsLen = 0;
    
    if (codecCaps & (1 << 16)) {
        pCaps->ratesLength = soundCaps.ratesLength;
        pCaps->rates = soundCaps.rates;
        pCaps->format = (SND_FMT_INT8 | SND_FMT_UNSIGNED | SND_FMT_MONO | SND_FMT_STEREO);
        pCaps++; info->capsLen++; privateBufPtr += sizeof(soundFormatCapability);
    };
    if (codecCaps & (1 << 17)) {
        pCaps->ratesLength = soundCaps.ratesLength;
        pCaps->rates = soundCaps.rates;
        pCaps->format = (SND_FMT_INT16 | SND_FMT_SIGNED | SND_FMT_MONO | SND_FMT_STEREO);
        pCaps++; info->capsLen++; privateBufPtr += sizeof(soundFormatCapability);
    };
    
    return SND_ERR_OK;
}

uint32_t sndHDAudio::detect(SoundDevice::deviceInfo* info) {

    // clear and fill device info
    this->devinfo.clear();
    if (hdaDetect(&this->devinfo, true) == false) return SND_ERR_NOTFOUND;

    // fill codec version
    fillCodecInfo(&this->devinfo);

    // UNMAP controller registers (these are mapped only during detection or after device is initialized)
    dpmi_unmapphysical(this->devinfo.membase); 
    this->devinfo.membase = NULL;
    this->devinfo.iobase  = 0;
    this->devinfo.iobase2 = 0;
    
    // copy info if not NULL
    if (info != NULL) *info = devinfo;

    return SND_ERR_OK;
}

uint32_t sndHDAudio::init(SoundDevice::deviceInfo *info) {
    uint32_t rtn = SND_ERR_OK;

    // deinit
    if (isInitialised) done();

    // validate fields
    SoundDevice::deviceInfo* p = (info != NULL ? info : &this->devinfo);

    // validate resources
    if ((p->pci.bus == -1) || (p->pci.device == -1) || (p->pci.function == -1)) return SND_ERR_INVALIDCONFIG;

    // copy resource fields
    if (info != NULL) {
        this->devinfo.pci = p->pci;
    }

    // extract PCI info
    if (tinypci::enumerateByAddress(&pciInfo, 1, devinfo.pci) == 0) return SND_ERR_INVALIDCONFIG;
    if ((pciInfo.deviceClass.baseClass != 0x4) || (pciInfo.deviceClass.subClass != 0x3) || (pciInfo.deviceClass.progInterface != 0x0)) return SND_ERR_INVALIDCONFIG;
    if ((pciInfo.bar0 & 1) != 0) return SND_ERR_INVALIDCONFIG;

    // map device registers
    devinfo.membase = dpmi_mapphysical(16384, (void*)(pciInfo.bar0 & ~0xF)); if (devinfo.membase == 0) return SND_ERR_NULLPTR;
    devinfo.irq = pciInfo.interruptLine;

    // reset controller and codec
    if ((rtn = resetCodec(&devinfo, true)) != SND_ERR_OK) return rtn;

    // build codec graph
    codecGraph.clear();
    if (hda_buildCodecGraph(devinfo.membase, devinfo.iobase, &codecGraph) == false) return SND_ERR_INVALIDCONFIG;

    // get first playback stream index
    hdaStreamIndex = (HDA_REG_READ32(devinfo.membase, HDA_REG_GCAP) >> 8) & 15;
    hdaStreamTag   = 1;

    isInitialised = true;

    return SND_ERR_OK;
}

uint32_t sndHDAudio::done() {
    if (isOpened) close();

    // unmap device
    if (devinfo.membase != NULL) dpmi_unmapphysical(devinfo.membase); devinfo.membase = NULL;

    isInitialised = false;
    return SND_ERR_OK;
}

uint32_t sndHDAudio::dmaBufferInit(uint32_t bufferSize, soundFormatConverterInfo *conv) {
    // premultiply bufferSize by bytesPerSample
    bufferSize *= conv->bytesPerSample;

    // check for bufsize
    if (bufferSize > devinfo.maxBufferSize) bufferSize = devinfo.maxBufferSize;
    
    // free stale DMA buffers
    if (dmaBufferFree() != SND_ERR_OK) return SND_ERR_MEMALLOC;

    // save dma info
    dmaBufferCount = 2;
    dmaBufferSize = bufferSize;
    dmaBlockSize = dmaBufferSize * 2;
    dmaCurrentPtr = dmaRenderPtr = 0;
    dmaBufferSamples = dmaBufferSize / conv->bytesPerSample;
    dmaBlockSamples  = dmaBlockSize  / conv->bytesPerSample;

#if 1
    // allocate DMA buffer
    dpmi_getdosmem((dmaBlockSize + 127) >> 4, &dmaBlock.dpmi);
    if (dpmi_status) return SND_ERR_MEMALLOC;
    dmaBlock.ptr = (void*)((((uint32_t)dmaBlock.dpmi.segment << 4) + 127) & ~127);

    // allocate buffer descriptor list
    dpmi_getdosmem(((sizeof(hda_bufferDescriptor) * dmaBufferCount) + 127) >> 4, &bufferDescriptor.dpmi);
    if (dpmi_status) return SND_ERR_MEMALLOC;
    bufferDescriptor.ptr = (hda_bufferDescriptor*)((((uint32_t)bufferDescriptor.dpmi.segment << 4) + 127) & ~127);
#else 
    // allocate DMA buffer
    *((void**)&dmaBlock.dpmi) = malloc(dmaBlockSize + 127);
    dmaBlock.ptr = (void*)((*((uint32_t*)&dmaBlock.dpmi) + 127) & ~127);

    // allocate buffer descriptor list
    *((void**)&bufferDescriptor.dpmi) = malloc((sizeof(hda_bufferDescriptor) * dmaBufferCount) + 127);
    bufferDescriptor.ptr = (hda_bufferDescriptor*)((*((uint32_t*)&bufferDescriptor.dpmi) + 127) & ~127);
#endif

    // fill buffer descriptor list
    for (uint32_t i = 0; i < dmaBufferCount; i++) {
        bufferDescriptor.ptr[i].ptr     = (void*)((uint8_t*)dmaBlock.ptr + (dmaBufferSize * i));
        bufferDescriptor.ptr[i].ptrHigh = NULL;
        bufferDescriptor.ptr[i].length  = dmaBufferSize;
        bufferDescriptor.ptr[i].status  = HDA_BDL_STATUS_IOC;
    }
    // lock DPMI memory for buffers
    dpmi_lockmemory(dmaBlock.ptr, dmaBlockSize);
    dpmi_lockmemory(bufferDescriptor.ptr, (sizeof(hda_bufferDescriptor) * dmaBufferCount));

    return SND_ERR_OK;
}

uint32_t sndHDAudio::dmaBufferFree() {
    // unlock DPMI memory

#if 1
    // free buffers
    if (dmaBlock.ptr != NULL) {
        dpmi_unlockmemory(dmaBlock.ptr, dmaBlockSize);
        dpmi_freedosmem(&dmaBlock.dpmi);
        dmaBlock.ptr = NULL;
    }
    if (bufferDescriptor.ptr != NULL) {
        dpmi_unlockmemory(bufferDescriptor.ptr, (sizeof(hda_bufferDescriptor) * dmaBufferCount));
        dpmi_freedosmem(&bufferDescriptor.dpmi);
        bufferDescriptor.ptr = NULL;
    }
#endif

    return SND_ERR_OK;
}

uint32_t sndHDAudio::open(uint32_t sampleRate, soundFormat fmt, uint32_t bufferSize, uint32_t flags, soundDeviceCallback callback, void *userdata, soundFormatConverterInfo *conv) {
    uint32_t result = SND_ERR_OK;
    if ((conv == NULL) || (callback == NULL)) return SND_ERR_NULLPTR;

    // stooop!
    if (isOpened) close();

    // clear converter info
    memset(conv, 0, sizeof(soundFormatConverterInfo));

    // test for conversion
    soundFormat newFormat = fmt;
   if ((flags & SND_OPEN_NOCONVERT) == 0) {
        // conversion is allowed
        // suggest 16bit mono/stereo, leave orig format for 8/16bit
        if ((fmt & SND_FMT_DEPTH_MASK) != SND_FMT_INT16) {
            newFormat = (fmt & (SND_FMT_CHANNELS_MASK)) | SND_FMT_INT16 | SND_FMT_SIGNED;
        }
    }
    if (isFormatSupported(sampleRate, newFormat, conv) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;

    // pass converter info
#ifdef DEBUG_LOG
    logdebug("src = 0x%x, dst = 0x%x\n", fmt, newFormat);
#endif
    if (getConverter(fmt, newFormat, conv) != SND_ERR_OK) return SND_ERR_UNKNOWN_FORMAT;
    conv->bytesPerSample = getBytesPerSample(conv->format);

    // we have all relevant info for opening sound device, do it now

    // allocate DMA buffer
    if ((result = dmaBufferInit(bufferSize, conv)) != SND_ERR_OK) return result;

    // install IRQ handler
    if ((result = installIrq()) != SND_ERR_OK) return result;

    // save callback info
    this->callback = callback;
    this->userdata = userdata;

    // pass coverter info
    memcpy(&convinfo, conv, sizeof(convinfo));

    // debug output
#ifdef DEBUG_LOG
    fprintf(stderr, __func__": requested format 0x%X, opened format 0x%X, rate %d hz, buffer %d bytes, flags 0x%X\n", fmt, newFormat, sampleRate, bufferSize, flags);
#endif

    isOpened = true;
    return SND_ERR_OK;
}

uint32_t sndHDAudio::close() {
    // stop playback
    if (isPlaying) stop();
    
    // deallocate DMA block
    dmaBufferFree();

    // unhook irq if hooked
    if (irq.hooked) irqUnhook(&irq, false);

    // reset codec (optinal but just in cause)
    if (resetCodec(&devinfo, false) == false) return SND_ERR_NOTFOUND;

    // fill with defaults
    isOpened = isPlaying = false;
    currentPos = irqs = 0;
    dmaChannel = dmaBlockSize = dmaBufferCount = dmaBufferSize = dmaBufferSamples = dmaBlockSamples = dmaCurrentPtr = dmaRenderPtr = 0;

    return SND_ERR_OK;
}

uint32_t sndHDAudio::resume()  {
    // resume playback
    hda_startStream(devinfo.membase, hdaStreamIndex);

    isPaused = false;
    return SND_ERR_RESUMED;
}

uint32_t sndHDAudio::start() {
    uint32_t rtn = SND_ERR_OK;
    if ((rtn = prefill()) != SND_ERR_OK) return rtn;

    // --------------------------------
    // device specific right now

    // build stream format
    hdaStreamFormat.baseRate = ((convinfo.sampleRate % 11025) == 0) ? 1 : 0;
    hdaStreamFormat.bitDepth = (convinfo.format & SND_FMT_INT8 ? 0 : 1);    // 8 or 16 bit
    hdaStreamFormat.channels = (convinfo.format & SND_FMT_MONO ? 0 : 1);    //  mono/stereo
    hdaStreamFormat.rateMultiplier = 0;
    hdaStreamFormat.rateDivider    = 0;
    hdaStreamFormat.type           = 0;         // PCM

    // reset stream
    hda_resetStream(devinfo.membase, hdaStreamIndex);

    // setup controller stream
    hda_setupStreamController(devinfo.membase, hdaStreamIndex, hdaStreamTag, hdaStreamFormat, dmaBlockSize, bufferDescriptor.ptr, dmaBufferCount);

    // parse codec to find appropriate paths
    // get all suitable output pins
    hda_widgetInfo* greenPin[16] = { 0 }; uint32_t greenPinCount = 0;
    for (uint32_t p = 0; (p < codecGraph.pinCount) && (greenPinCount < 16); p++) {
        uint32_t pinDevice = codecGraph.pin[p]->pinConfigDefault & HDA_PIN_DEFAULTDEVICE_MASK;
        if (((pinDevice == HDA_PIN_DEFAULTDEVICE_LINE_OUT) || (pinDevice == HDA_PIN_DEFAULTDEVICE_HP_OUT) || (pinDevice == HDA_PIN_DEFAULTDEVICE_SPDIF_OUT)) &&
            ((codecGraph.pin[p]->pinConfigDefault & HDA_PIN_CONNECTIVITY_MASK) != HDA_PIN_CONNECTIVITY_NONE)) {
            greenPin[greenPinCount++] = codecGraph.pin[p];
        }
    }
    // get closest path between each output pin and every DAC
    hda_widgetInfo *path[16];
    hda_widgetSearchStruct bfsTree[128];
    for (uint32_t p = 0; p < greenPinCount; p++)  {
        hdaBuildBFSTree(bfsTree, arrayof(bfsTree), greenPin[p]);
        for (uint32_t dac = 0; dac < codecGraph.dacCount; dac++) {
            uint32_t pathLength = hdaGetShortestPath(bfsTree, greenPin[p], codecGraph.dac[dac], path, arrayof(path));
            if (pathLength != 0) {
#ifdef DEBUG_LOG
                printf("  path from %02X to %02X: ", greenPin[p]->nodeId, codecGraph.dac[dac]->nodeId);
                for (uint32_t i = 0; i < pathLength; i++) {
                    if (path[i] != NULL) printf("%02X ", path[i]->nodeId);
                }
                printf("\n");
#endif
                // enable playback path
                hda_initPlaybackPath(devinfo.membase, devinfo.iobase, path, pathLength, hdaStreamFormat.raw, hdaStreamTag);
                break;      // pin is already serviced by DAC!
            }
        }
    }

    // enable controller/stream interrupts
    hda_enableStreamInterrupt(devinfo.membase, hdaStreamIndex);

    // enable stream!
    hda_startStream(devinfo.membase, hdaStreamIndex);

#ifdef DEBUG_LOG
    printf("playback started\n");
#endif

    // done! we're playing sound :)
    isPaused = false; isPlaying = true;

    return SND_ERR_OK;
}

uint32_t sndHDAudio::pause()
{
    // pause
    hda_stopStream(devinfo.membase, hdaStreamIndex);

    isPaused = true;
    return SND_ERR_OK;
}

uint32_t sndHDAudio::ioctl(uint32_t function, void* data, uint32_t len)
{
    return SND_ERR_UNSUPPORTED;
}

uint32_t sndHDAudio::stop() {
    if (isPlaying) {
        // stop stream
        hda_stopStream(devinfo.membase, hdaStreamIndex);

        // disable interrupts
        hda_disableStreamInterrupt(devinfo.membase, hdaStreamIndex);
    }

    isPlaying = false;

    // clear playing position
    currentPos = renderPos = irqs = 0;
    dmaCurrentPtr = dmaRenderPtr = 0;
    oldTotalPos = 0;

    return SND_ERR_OK;
}

// irq procedure
bool sndHDAudio::irqProc() {
    // test if HDA controller interrupt
    if ((HDA_REG_READ32(devinfo.membase, HDA_REG_INTSTS) & ((1 << 31) | (1 << hdaStreamIndex))) == 0)
        return true; // else chain to previous ISR
    
    // advance play pointers
    irqAdvancePos();

    // acknowledge HDA interrupt
    if (HDA_STREAM_READ8(devinfo.membase, hdaStreamIndex, HDA_REG_STREAM_STATUS) & HDA_STREAM_INT_BUFFER_COMPLETE)
        HDA_STREAM_WRITE8(devinfo.membase, hdaStreamIndex, HDA_REG_STREAM_STATUS, HDA_STREAM_INT_BUFFER_COMPLETE);

    // acknowledge interrupt
    outp(irq.info->picbase, 0x20); if (irq.info->flags & IRQ_SECONDARYPIC) outp(0x20, 0x20);

    // call callback
    irqCallbackCaller();
    
    return false;   // we're handling EOI by itself
}

// get play position in DMA buffer in bytes
uint32_t sndHDAudio::getPlayPos() {
    return HDA_STREAM_READ32(devinfo.membase, hdaStreamIndex, HDA_REG_STREAM_LINKPOS);
}

#define sndlib_min(a, b) ((a) < (b) ? (a) : (b))
#define sndlib_max(a, b) ((a) > (b) ? (a) : (b))
#define sndlib_clamp(a, l, h) (sndlib_max(sndlib_min(a, h), l))

void sndHDAudio::irqAdvancePos() {
    // blindly adjust play position as-is
    dmaRenderPtr += dmaBufferSize;
    if (dmaRenderPtr >= dmaBlockSize) dmaRenderPtr = 0;
    renderPos += dmaBufferSamples;

    // for play position i use a bit different approach
    int32_t playpos = getPlayPos() / convinfo.bytesPerSample;
    //playpos = sndlib_clamp(playpos, 0, dmaBlockSamples - 1);
    currentPos += (playpos < dmaCurrentPtr ? dmaBlockSamples + playpos - dmaCurrentPtr : playpos - dmaCurrentPtr);
    dmaCurrentPtr = playpos;
}

#undef sndlib_min
#undef sndlib_max
#undef sndlib_clamp

#endif
