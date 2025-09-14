#ifndef PLAYER_H
#define PLAYER_H

#include <unistd.h>
#include <cstdio>

void start_aplay(mp3dec_frame_info_t info, int pipefd[2], pid_t &child_pid)
{
    int detected_rate = info.hz ? info.hz : 44100;
    int detected_channels = info.channels ? info.channels : 2;

    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        throw std::runtime_error("pipe failed");
    }

    child_pid = fork();
    if (child_pid < 0)
    {
        perror("fork");
        throw std::runtime_error("fork failed");
    }

    if (child_pid == 0)
    {
        // Child: read from pipe, exec aplay
        // stdin <- pipe read end
        if (dup2(pipefd[0], STDIN_FILENO) == -1)
        { // stdin will be read by aplay
            perror("dup2");
            _exit(1);
        }
        close(pipefd[0]);
        close(pipefd[1]); // not needed in child

        // Build args for aplay: aplay -f S16_LE -c <ch> -r <rate> -
        char ch_str[16], rate_str[32];
        snprintf(ch_str, sizeof(ch_str), "%d", detected_channels);
        snprintf(rate_str, sizeof(rate_str), "%d", detected_rate);
        char const *args[] = {
            "aplay",
            "-q",             // quiet (aplay prints errors only)
            "-D", "pipewire", // use pipewire output if available
            "-f", "S16_LE",   // 16-bit signed little-endian
            "-c", ch_str,
            "-r", rate_str,
            "-",    // stdin as input
            nullptr // end of args
        };
        execvp("aplay", (char *const *)args);
        std::fprintf(stderr, "Format: %d Hz, %d ch, 16-bit signed (interleaved)\n", detected_rate, detected_channels);
        perror("execvp aplay");
        _exit(1);
    }
    else
    {
        // Parent
        close(pipefd[0]); // parent writes to pipefd[1]
        return;
    }
};

inline void write_pcm_to_aplay(int pipefd[2], pid_t &child_pid, int16_t *pcm, int samples, int channels)
{
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