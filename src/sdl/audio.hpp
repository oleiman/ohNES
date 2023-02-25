#pragma once

#include <SDL.h>

#include <array>
#include <iostream>
#include <memory>
#include <vector>

namespace sdl_internal {

class Generator;

class Audio {
public:
  static constexpr int SampleRate = 44100;
  static constexpr int TableLength = 1024;
  Audio();
  ~Audio();
  template <typename T> static T &MakeGenerator() {
    channels_.emplace_back(std::make_unique<T>());
    return static_cast<T &>(*channels_.back());
  }

  void init();

private:
  static void audio_callback(void *unused, uint8_t *byte_stream,
                             int byte_stream_length);
  // must be a power of two, decrease to allow for a lower latency,
  // increase to reduce risk of underrun.
  // uint16_t buffer_size_ = 4096;
  uint16_t buffer_size_ = 512;
  SDL_AudioDeviceID audio_device_;
  SDL_AudioSpec audio_spec_;
  // SDL_Event event;
  static std::vector<std::unique_ptr<Generator>> channels_;
};

class Generator {
public:
  virtual void write_stream(uint8_t *stream, int len) const;
  virtual ~Generator() = default;

  void set_pitch(double p) { pitch_ = p; }
  void set_volume(double v) { volume_ = v; }
  void on() { enabled_ = true; }
  void off() { enabled_ = false; }

protected:
  Generator() = default;
  static constexpr long chunk_size = 64;
  void write_chunk(int16_t *stream, long begin, long end, long len,
                   double pitch) const;
  std::array<int16_t, Audio::TableLength> table_ = {};
  bool enabled_ = false;

private:
  double pitch_ = 220;
  double volume_ = 1.0;
  mutable double phase = 0.0;

protected:
  mutable std::mutex m_;
};

class Triangle : public Generator {
public:
  Triangle() { init_table(); }

private:
  void init_table();
  int32_t mod(int32_t x, int32_t p);
  int16_t tri(int32_t t);
};

class Pulse : public Generator {
public:
  Pulse() { init_table(); }

  void set_duty_cycle(double d) {
    auto prev = duty_cycle_;
    duty_cycle_ = d;
    if (duty_cycle_ != prev) {
      init_table();
    }
  }

private:
  void init_table();
  double duty_cycle_ = 0.5;
};

class Noise : public Generator {
public:
  Noise() { init_table(); }

private:
  void init_table();
};

} // namespace sdl_internal
