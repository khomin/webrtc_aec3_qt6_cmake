#include <string>
#include <algorithm>
#include <memory>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "io_audio.h"

int main(int argc, char *argv[]) {
    std::cout << "starting..." << std::endl;

    auto audio = new IoAudio();
    audio->startAudio();

    while(audio->isStarted()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    delete audio;
    audio = NULL;

    std::cout << "end" << std::endl;

    return 0;
}
