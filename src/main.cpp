#include <string>
#include <thread>
#include <iostream>
#include <sys/wait.h>

#include "audio/audio.h"
#include "visual/visual.h"
#include "shared_state.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "Usage: %s <file.mp3>\n", argv[0]);
        return 1;
    }
    const std::string path = "assets/" + std::string(argv[1]);

    SharedState shared;
    std::thread audio_thread_handle(audio_thread, path, &shared);
    std::thread visual_thread_handle(visual_thread, &shared);
    audio_thread_handle.join();
    visual_thread_handle.join();

    return 0;
}
