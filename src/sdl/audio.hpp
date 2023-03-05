#pragma once

#include <SDL.h>

#include <array>
#include <deque>
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
  static std::vector<std::unique_ptr<Generator>> channels_;
};

class Generator {
public:
  virtual void write_stream(uint8_t *stream, int len);
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
  double volume_ = 1.0;

private:
  double pitch_ = 220;
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

class DMC : public Generator {
public:
  DMC() {}
  void write_stream(uint8_t *stream, int len) override;

  void change_output_level(uint8_t level, uint16_t rate) {
    // this might put way too much pressure on the lock
    std::lock_guard<std::mutex> lg(m_);
    output_q_.emplace_back(level, rate);
  }

private:
  struct OutputLevel {
    // output level is always kept in [0,127], so we can safely multiply by 100h
    OutputLevel(uint8_t l, uint16_t r)
        : level(project(l, 0, 127, 0, INT16_MAX)), rate(r / 40.646) {}
    int16_t level;
    uint16_t rate;

  private:
    int16_t project(int16_t val, int16_t old_min, int16_t old_max,
                    int16_t new_min, int16_t new_max) {
      if (val == 0) {
        return 0;
      }
      double old_range = old_max - old_min;
      double new_range = new_max - new_min;
      return static_cast<int16_t>((((val - old_min) * new_range) / old_range) +
                                  new_min);
    }
  };
  std::deque<OutputLevel> output_q_;
};

} // namespace sdl_internal
