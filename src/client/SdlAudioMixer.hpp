#pragma once

#include "client/SoundEngine.hpp"

#include <SDL3/SDL.h>
#include <vector>

namespace mcpi::client {

class SdlAudioMixer {
public:
    SdlAudioMixer() = default;
    ~SdlAudioMixer();

    SdlAudioMixer(const SdlAudioMixer&) = delete;
    SdlAudioMixer& operator=(const SdlAudioMixer&) = delete;

    [[nodiscard]] bool play(const PendingSound& sound);
    void update();

private:
    std::vector<SDL_AudioStream*> streams_;
};

} // namespace mcpi::client
