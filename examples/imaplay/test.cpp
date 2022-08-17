#include <stdio.h>
#include <ctype.h>
#include <conio.h>

#include "imados.h"

int main(int argc, char* argv[]) {
    imaplay_dos player;
    
    // parse options
    if (argc < 2) {
        printf("usage: imaplay.exe [filename.wav] [autosetup]\n");
        return 1;
    }
    
    bool downmix = false, mono = false, force8bit = false, autosetup = false;

    for (size_t i = 2; i < argc; i++) {
        char a = toupper(*argv[i]);
        if (a == 'A') autosetup = true;
        if (a == 'D') downmix = true;
        if (a == 'M') mono = true;
        if (a == '8') force8bit = true;
    }

    // set 64 frames in ring buffer (ca. 1.5 seconds of pre-render), 1024 samples DMA buffer
    if (!player.init(64, 1024, 0, autosetup)) return 1;
    
    // load wav file
    if (!player.load(argv[1])) return 1;
    
    // start playback
    if (!player.play()) return 1;
    
    uint64_t pos = player.getPos();
    bool isPause = false;
    printf("space to pause/resume, esc to exit\n");
    
    // loop
    while (1) {
        
        if (kbhit()) {
            char a = getch();
            if (a == ' ') {
                isPause = !isPause;
                if (isPause) player.pause(); else player.resume();
            }
            else if (a == 0x1B) break;
        }
        
        // get current position in samples, convert to mm:ss:ms
        uint64_t pos                = player.getPos();
        uint64_t posInMilliseconds  = (pos * 1000 / player.getSampleRate());
        uint32_t minutes = (posInMilliseconds / (1000 * 60));
        uint32_t seconds = (posInMilliseconds / 1000) % 60;
        uint32_t ms      = (posInMilliseconds % 1000);

        printf("\rplaypos = %8llu samples, %02d:%02d.%03d", player.getPos(), minutes, seconds, ms); fflush(stdout);
        while (inp(0x3da) & 8); while (!(inp(0x3da) & 8));

        // uncomment this to optionally decode wav in main thread
        //player.decode(0);     // how many frames per call you have to decode, 0 - until ring buffer full
    };
    
    // cleanup
    player.stop();
    player.done();
    
    return 0;
}