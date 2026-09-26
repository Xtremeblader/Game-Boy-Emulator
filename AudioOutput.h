#pragma once
#include <string>
#include <vector>
struct SDL_AudioStream;
class AudioOutput {
public:
    AudioOutput() = default;
    ~AudioOutput();
    AudioOutput(const AudioOutput&) = delete;
    AudioOutput& operator=(const AudioOutput&) = delete;
    bool open();
    bool queue(const std::vector<float>& samples);
    std::string error() const;
private:
    SDL_AudioStream* stream = nullptr;
    bool initialized = false;
};
