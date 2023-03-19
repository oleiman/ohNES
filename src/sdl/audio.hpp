#pragma once

#include <array>
#include <deque>
#include <iostream>
#include <memory>
#include <vector>

struct SDL_AudioSpec;

namespace aud {
class Generator;
}

namespace sdl_internal {

class Audio {

public:
  Audio();
  ~Audio();

  void init();

private:
  static void audio_callback(void *unused, uint8_t *byte_stream,
                             int byte_stream_length);
  // must be a power of two, decrease to allow for a lower latency,
  // increase to reduce risk of underrun.
  // uint16_t buffer_size_ = 4096;
  uint16_t buffer_size_ = 512;
  // SDL_AudioDeviceID audio_device_;
  uint32_t audio_device_;
  SDL_AudioSpec *audio_spec_;
};

} // namespace sdl_internal
