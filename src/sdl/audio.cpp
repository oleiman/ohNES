#include "audio.hpp"
#include "gen_audio.hpp"

#include <SDL.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>

namespace sdl_internal {

Audio::Audio() { audio_spec_ = new SDL_AudioSpec(); }
Audio::~Audio() {
  SDL_CloseAudioDevice(audio_device_);
  delete audio_spec_;
}

void Audio::init() {
  SDL_AudioSpec want;
  SDL_zero(want);
  SDL_zero(*audio_spec_);

  want.freq = aud::SampleRate;
  want.format = AUDIO_S16LSB;
  want.channels = 1;
  want.samples = buffer_size_;

  want.callback = &Audio::audio_callback;

  audio_device_ = SDL_OpenAudioDevice(NULL, 0, &want, audio_spec_, 0);

  assert(audio_device_ != 0);
  assert(audio_spec_->format == want.format);

  buffer_size_ = audio_spec_->samples;

  SDL_PauseAudioDevice(audio_device_, 0);
}

void Audio::audio_callback(void *unused, uint8_t *byte_stream,
                           int byte_stream_length) {
  memset(byte_stream, 0, byte_stream_length);

  for (const auto &c : aud::Generator::Channels()) {
    c->write_stream(byte_stream, byte_stream_length);
  }
}

} // namespace sdl_internal
