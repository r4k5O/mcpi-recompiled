#include "client/SdlAudioMixer.hpp"

#include <algorithm>
#include <limits>

namespace mcpi::client {

SdlAudioMixer::~SdlAudioMixer() {
    for (SDL_AudioStream* stream : streams_) {
        SDL_DestroyAudioStream(stream);
    }
}

bool SdlAudioMixer::play(const PendingSound& sound) {
    if (sound.encoded.empty() || sound.encoded.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }

    SDL_IOStream* source = SDL_IOFromConstMem(sound.encoded.data(), sound.encoded.size());
    if (source == nullptr) {
        return false;
    }

    SDL_AudioSpec spec{};
    Uint8* decoded = nullptr;
    Uint32 decoded_length = 0;
    if (!SDL_LoadWAV_IO(source, true, &spec, &decoded, &decoded_length)) {
        return false;
    }

    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (stream == nullptr) {
        SDL_free(decoded);
        return false;
    }

    const bool configured = SDL_SetAudioStreamGain(stream, sound.gain) &&
                            SDL_PutAudioStreamData(stream, decoded, static_cast<int>(decoded_length)) &&
                            SDL_FlushAudioStream(stream) &&
                            SDL_ResumeAudioStreamDevice(stream);
    SDL_free(decoded);
    if (!configured) {
        SDL_DestroyAudioStream(stream);
        return false;
    }

    streams_.push_back(stream);
    return true;
}

void SdlAudioMixer::update() {
    const auto first_finished = std::remove_if(streams_.begin(), streams_.end(), [](SDL_AudioStream* stream) {
        const int queued = SDL_GetAudioStreamQueued(stream);
        if (queued > 0) {
            return false;
        }
        SDL_DestroyAudioStream(stream);
        return true;
    });
    streams_.erase(first_finished, streams_.end());
}

} // namespace mcpi::client
