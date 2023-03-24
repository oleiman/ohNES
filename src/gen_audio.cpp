#include "gen_audio.hpp"

#include <random>

namespace aud {
std::vector<std::unique_ptr<Generator>> Generator::channels_;

int32_t Triangle::mod(int32_t x, int32_t p) { return ((x % p) + p) % p; }
int16_t Triangle::tri(int32_t t) {
  static constexpr int32_t m = 4 * INT16_MAX / (TableLength);
  return static_cast<int16_t>(
      m * std::abs(static_cast<int32_t>(
              mod(t - (TableLength / 4), TableLength) - (TableLength / 2))) -
      INT16_MAX);
}

void Triangle::init_table() {
  for (size_t i = 0; i < table_.size(); ++i) {
    table_[i] = tri(static_cast<int32_t>(i));
  }
}

void Generator::write_stream(uint8_t *byte_stream, int len) {
  if (!enabled_) {
    return;
  }

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
  double d_SR = SampleRate;
  double d_TL = TableLength;

  double phase_inc = (pitch / d_SR) * d_TL;
  for (int i = 0; i < len; ++i) {
    phase += phase_inc;
    size_t phase_int = static_cast<size_t>(phase);
    if (phase >= static_cast<double>(table_.size())) {
      double diff = phase - table_.size();
      phase = diff;
      phase_int = static_cast<int>(phase);
    }

    if (phase_int < table_.size() && phase_int >= 0) {
      if (stream != nullptr) {
        auto sample = table_[phase_int];
        sample *= (0.2 * volume_);
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
  }

  std::fill(table_.begin() + pulse_width + 1, table_.end(), 0);
}

void Noise::init_table() {
  std::random_device dev;
  std::uniform_int_distribution<int16_t> dist(INT16_MIN, INT16_MAX);

  table_[0] = dist(dev);

  for (size_t i = 1; i < table_.size(); ++i) {
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

  std::transform(table_.cbegin(), table_.cend(), table_.begin(),
                 [](auto &e) { return e / 2; });
}

void DMC::write_stream(uint8_t *byte_stream, int len) {
  if (!enabled_) {
    return;
  }

  std::lock_guard<std::mutex> ul(m_);
  int16_t *stream = reinterpret_cast<int16_t *>(byte_stream);

  int remain = len / 2;
  int idx = 0;
  while (idx < remain && read_head_ != write_head_) {
    int16_t level = out_buf_[read_head_];
    stream[idx] += level;
    ++idx;
    read_head_ = (read_head_ + 1) % out_buf_.size();
  }
}
} // namespace aud
