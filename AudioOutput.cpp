#include "AudioOutput.h"
#include "APU.h"
#ifdef GB_WITH_SDL
#include <SDL3/SDL.h>
AudioOutput::~AudioOutput(){
    if(stream) SDL_DestroyAudioStream(stream);
    if(initialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
bool AudioOutput::open(){
    if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;
    initialized=true;
    SDL_AudioSpec spec{};
    spec.format=SDL_AUDIO_F32; spec.channels=2; spec.freq=APU::SampleRate;
    stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,nullptr,nullptr);
    return stream && SDL_ResumeAudioStreamDevice(stream);
}
bool AudioOutput::queue(const std::vector<float>& samples){
    if(!stream) return false;
    int queued=SDL_GetAudioStreamQueued(stream);
    if(queued<0) return false;
    // Drop stale audio after a host stall instead of building unbounded latency.
    if(queued>APU::SampleRate*2*static_cast<int>(sizeof(float))/5)
        if(!SDL_ClearAudioStream(stream)) return false;
    return samples.empty() || SDL_PutAudioStreamData(stream,samples.data(),
                                         static_cast<int>(samples.size()*sizeof(float)));
}
std::string AudioOutput::error() const { return SDL_GetError(); }
#else
AudioOutput::~AudioOutput() = default;
bool AudioOutput::open(){ return false; }
bool AudioOutput::queue(const std::vector<float>&){ return false; }
std::string AudioOutput::error() const { return "Built without SDL3 audio support"; }
#endif
