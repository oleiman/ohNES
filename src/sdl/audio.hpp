#pragma once

#include <SDL.h>

#include <array>
#include <cmath>
#include <deque>
#include <iostream>
#include <memory>
#include <vector>

namespace sdl_internal {

class Generator;

class Audio {
public:
  // static constexpr int SampleRate = 44100;
  // NOTE(oren): this is entirely a hack to make DMC playback sound reasonable
  // without too much effort. Would like to restore 44100Hz sample rate then do
  // the upsampling/interpolation at the output stage. However, this setting
  // actually sounds pretty nice across the board, so maybe not worth the
  // effort.
  static constexpr int SampleRate = 33143;
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

  void set_pitch(double p) {
    // TODO(oren): might not be a good idea to assert on this path
    assert(p > 0);
    pitch_ = p;
  }
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
    OutputLevel lvl(level, rate);

    std::lock_guard<std::mutex> lg(m_);
    for (int i = 0; i < lvl.rate; ++i) {
      out_buf_[write_head_] = lvl.level;
      write_head_ = (write_head_ + 1) % out_buf_.size();
    }
  }

private:
  struct OutputLevel {
    OutputLevel(uint8_t l, uint16_t r)
        : level(project(l, 0, 127, 0, INT16_MAX)),
          rate(std::roundf(r / 40.646)) {}
    int16_t level;
    uint16_t rate;

  private:
    int16_t project(int16_t val, int16_t old_min, int16_t old_max,
                    int16_t new_min, int16_t new_max) {
      if (val == 0xFF) {
        return 0;
      }
      return (val - 0x40) << 8;
    }
  };
  std::array<int16_t, Audio::TableLength * 2> out_buf_;
  int read_head_ = 0;
  int write_head_ = 0;
};

} // namespace sdl_internal
