#include "mp2play.h"

bool mp2play::init(uint32_t decodedPool, uint32_t bufferSamples, bool autosetup, bool mono, bool downsample2to1) {return false;}

bool mp2play::load(const char *filename) {return false;}
bool mp2play::loadmem(void * ptr, uint32_t size) {return false;}

bool mp2play::play(uint64_t pos) {return false;}

bool mp2play::decode(uint32_t frames) {return false;}

bool mp2play::pause() {return false;}

bool mp2play::resume() {return false;}

uint64_t mp2play::getPos() {return 0;}

uint32_t mp2play::getSampleRate() {return 0;}

bool mp2play::stop() {return false;}

bool mp2play::done() {return false;}