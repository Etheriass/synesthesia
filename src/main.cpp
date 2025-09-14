#include <string>
#include <thread>

#include "audio/audio.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "Usage: %s <file.mp3>\n", argv[0]);
        return 1;
    }
    const std::string path = "assets/" + std::string(argv[1]);

    std::thread audio_thread_handle(audio_thread, path);
    audio_thread_handle.join();

    return 0;
}
