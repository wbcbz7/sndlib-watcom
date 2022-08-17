#include "imaplay.h"

bool imaplay::init(uint32_t decodedPool, uint32_t bufferSamples, uint32_t resampleMode, bool autosetup) {return false;}

bool imaplay::load(const char *filename, uint32_t) {return false;}
bool imaplay::loadmem(void * ptr, uint32_t size) {return false;}

bool imaplay::play(uint64_t pos) {return false;}

bool imaplay::decode(uint32_t frames) {return false;}

bool imaplay::pause() {return false;}

bool imaplay::resume() {return false;}

uint64_t imaplay::getPos() {return 0;}

uint32_t imaplay::getSampleRate() {return 0;}

bool imaplay::stop() {return false;}

bool imaplay::done() {return false;}