#ifndef PLAYER_H
#define PLAYER_H

#include <unistd.h>
#include <cstdio>
#include <stdexcept>


pid_t start_aplay_process(int rate, int pipefd[2])
{
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        throw std::runtime_error("pipe failed");
    }

    pid_t child_pid = fork();
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
        char rate_str[32];
        snprintf(rate_str, sizeof(rate_str), "%d", rate);
        char const *args[] = {
            "aplay",
            "-q",             // quiet (aplay prints errors only)
            "-D", "pipewire", // use pipewire output if available
            "-f", "S16_LE",   // 16-bit signed little-endian
            "-c", "2",
            "-r", rate_str,
            "-",    // stdin as input
            nullptr // end of args
        };
        execvp("aplay", (char *const *)args);
        std::fprintf(stderr, "Format: %d Hz, 2 ch, 16-bit signed (interleaved)\n", rate);
        perror("execvp aplay");
        _exit(1);
    }
    else
    {
        // Parent
        close(pipefd[0]); // parent writes to pipefd[1]
        return child_pid;
    }
};



#endif