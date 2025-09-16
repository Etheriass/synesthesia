#include <string>
#include <thread>
#include <iostream>
#include <sys/wait.h>

#include "audio/audio.h"
#include "visual/visual.h"
#include "shared_state.h"

#include <pthread.h>
#include <sched.h>

void pin_to_cpu(std::thread &t, int cpu)
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    pthread_setaffinity_np(t.native_handle(), sizeof(set), &set);
}

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
    // pin_to_cpu(audio_thread_handle, 0); // audio on CPU 0
    // pin_to_cpu(visual_thread_handle, 1); // visual on CPU 1
    
    audio_thread_handle.join();
    visual_thread_handle.join();

    return 0;
}
