#ifndef AUDIO_H
#define AUDIO_H

#include <string>
#include "../shared_state.h"

void audio_thread(const std::string path, SharedState *shared);

#endif

