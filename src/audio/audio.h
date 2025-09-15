#ifndef AUDIO_H
#define AUDIO_H

#include "../shared_state.h"

#include "../../external/minimp3/minimp3.h"
#include "../../external/minimp3/minimp3_ex.h"

void audio_thread(mp3dec_t *dec, std::vector<uint8_t> audio_binary, int rate, SharedState *shared);

#endif

