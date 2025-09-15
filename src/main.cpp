#include <string>
#include <thread>
#include <iostream>
#include <cstdint>
#include <vector>

#include "audio/audio.h"
#include "audio/helpers.h"
#include "visual/visual.h"
#include "shared_state.h"

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include "../../external/minimp3/minimp3.h"
#include "../../external/minimp3/minimp3_ex.h"

#include <pthread.h>
#include <sched.h>

void pin_to_cpu(std::thread &t, int cpu)
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    pthread_setaffinity_np(t.native_handle(), sizeof(set), &set);
}

std::vector<uint8_t> read_binary(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        std::cerr << "Cannot open: " + path << '\n';
    f.seekg(0, std::ios::end);
    size_t len = (size_t)f.tellg();
    f.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(len);
    if (len && !f.read((char *)data.data(), (std::streamsize)len))
        std::cerr << "Read failed" + path << '\n';
    return data;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "Usage: %s <file.mp3>\n", argv[0]);
        return 1;
    }
    const std::string path = "assets/" + std::string(argv[1]);

    std::vector<uint8_t> audio_binary = read_binary(path);
    
    mp3dec_t dec;
    mp3dec_init(&dec);
    mp3dec_frame_info_t info{};
    int first_frame = mp3dec_decode_frame(&dec, audio_binary.data(), (int)(audio_binary.size()), nullptr, &info);

    if (info.channels < 2)
    {
        std::cerr << "Only stereo files are supported\n";
    }

    SharedState shared;
    std::thread visual_thread_handle(visual_thread, &shared);
    std::thread audio_thread_handle(audio_thread, &dec, audio_binary, info.hz, &shared);
    // pin_to_cpu(audio_thread_handle, 0); // audio on CPU 0
    // pin_to_cpu(visual_thread_handle, 1); // visual on CPU 1

    audio_thread_handle.join();
    visual_thread_handle.join();

    return 0;
}
