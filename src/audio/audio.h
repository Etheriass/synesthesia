#ifndef AUDIO_H
#define AUDIO_H

#include "../shared_state.h"

#include "../../external/minimp3/minimp3.h"
#include "../../external/minimp3/minimp3_ex.h"

void audio_thread(const std::string path, SharedState *shared);


inline void write_pcm_to_pipe(int pipefd[2], int16_t *pcm, int samples)
{
    int channels = 2;
    size_t bytes = (size_t)samples * channels * sizeof(int16_t);
    const uint8_t *p = reinterpret_cast<const uint8_t *>(pcm);
    while (bytes > 0)
    {
        ssize_t n = write(pipefd[1], p, bytes);
        if (n < 0)
        {
            perror("write to aplay");
            break;
        }
        p += n;
        bytes -= (size_t)n;
    }
    return;
}

#endif

