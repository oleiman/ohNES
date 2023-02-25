#include "audio.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <random>

namespace sdl_internal {

std::vector<std::unique_ptr<Generator>> Audio::channels_;

Audio::Audio() {}

void Audio::init() {
  SDL_AudioSpec want;
  SDL_zero(want);
  SDL_zero(audio_spec_);

  want.freq = SampleRate;
  want.format = AUDIO_S16LSB;
  want.channels = 1;
  want.samples = buffer_size_;

  want.callback = &Audio::audio_callback;

  audio_device_ = SDL_OpenAudioDevice(NULL, 0, &want, &audio_spec_, 0);

  assert(audio_device_ != 0);
  assert(audio_spec_.format == want.format);

  buffer_size_ = audio_spec_.samples;

  SDL_PauseAudioDevice(audio_device_, 0);
}

Audio::~Audio() { SDL_CloseAudioDevice(audio_device_); }

void Audio::audio_callback(void *unused, uint8_t *byte_stream,
                           int byte_stream_length) {
  memset(byte_stream, 0, byte_stream_length);

  for (const auto &c : channels_) {
    c->write_stream(byte_stream, byte_stream_length);
  }
}

int32_t Triangle::mod(int32_t x, int32_t p) { return ((x % p) + p) % p; }
int16_t Triangle::tri(int32_t t) {
  static constexpr int32_t m = 4 * INT16_MAX / (Audio::TableLength);
  return static_cast<int16_t>(
      m * std::abs(static_cast<int32_t>(
              mod(t - (Audio::TableLength / 4), Audio::TableLength) -
              (Audio::TableLength / 2))) -
      INT16_MAX);
}

void Triangle::init_table() {
  for (int16_t i = 0; i < table_.size(); ++i) {
    table_[i] = tri(i);
  }
}

void Generator::write_stream(uint8_t *byte_stream, int len) const {
  if (!enabled_) {
    return;
  }

  // std::cout << volume_ << std::endl;

  std::lock_guard<std::mutex> lg(m_);

  int16_t *stream = reinterpret_cast<int16_t *>(byte_stream);
  long remain = len / 2;

  long iters = remain / chunk_size;
  for (long i = 0; i < iters; ++i) {
    long begin = i * chunk_size;
    long end = (i * chunk_size) + chunk_size;
    write_chunk(stream, begin, end, chunk_size, pitch_);
  }
}

void Generator::write_chunk(int16_t *stream, long begin, long end, long len,
                            double pitch) const {
  double d_SR = Audio::SampleRate;
  double d_TL = Audio::TableLength;

  double phase_inc = (pitch / d_SR) * d_TL;
  // std::cout << "phase inc " << phase_inc << std::endl;

  for (int i = 0; i < len; ++i) {
    phase += phase_inc;
    int phase_int = static_cast<int>(phase);
    if (phase >= table_.size()) {
      double diff = phase - table_.size();
      phase = diff;
      phase_int = static_cast<int>(phase);
    }

    if (phase_int < table_.size() && phase_int >= 0) {
      if (stream != nullptr) {
        auto sample = table_[phase_int];
        sample *= (0.25 * volume_);
        stream[i + begin] += sample;
      }
    }
  }
}

// TODO(oren): this isn't the right shape here.
// see https://www.nesdev.org/wiki/APU_Pulse#Sequencer_behavior
void Pulse::init_table() {

  double abs_duty = std::abs(duty_cycle_);

  int16_t pulse_width = abs_duty * (table_.size());
  std::lock_guard<std::mutex> lg(m_);
  for (int16_t i = 0; i <= pulse_width; ++i) {
    table_[i] = INT16_MAX;
    // table_[i] = (i < pulse_width ? INT16_MAX : 0);
  }

  std::fill(table_.begin() + pulse_width + 1, table_.end(), INT16_MIN);
}

void Noise::init_table() {
  std::random_device dev;
  std::uniform_int_distribution<int16_t> dist(INT16_MIN, INT16_MAX);

  table_[0] = dist(dev);

  for (int i = 1; i < table_.size(); ++i) {
    // table_[i] = dist(dev);
    auto val = dist(dev);
    int32_t next = static_cast<int32_t>(table_[i - 1]) + val;

    if (next > INT16_MAX) {
      auto diff = next - INT16_MAX;
      // diff should be > 0
      next = INT16_MIN + diff;
    } else if (next < INT16_MIN) {
      auto diff = next - INT16_MIN;
      // diff should be < 0
      next = INT16_MAX + diff;
    }

    table_[i] = static_cast<int16_t>(next);
  }

  // std::transform(table_.cbegin(), table_.cend(), table_.begin(),
  //                [](auto &e) { return e / 2; });
}

} // namespace sdl_internal
