#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <ostream>

namespace util {
void reverseByte(uint8_t &b);

template <typename T, unsigned Cap,
          typename StoreT = std::array<std::unique_ptr<T>, Cap>>
class RingBuf {
public:
  RingBuf(){};
  ~RingBuf() = default;
  void insert(const T &item) {
    store_[tail_] = std::make_unique<T>(item);
    ++tail_;
    if (tail_ == Cap) {
      tail_ = 0;
    }
    if (tail_ == head_) {
      ++head_;
    }
    if (head_ == Cap) {
      head_ = 0;
    }
    if (size_ < Cap) {
      ++size_;
    }
  }

  friend std::ostream &operator<<(std::ostream &os, RingBuf &rb) {
    auto idx = rb.head_;
    while (idx != rb.tail_ && rb.store_[idx] != nullptr) {
      os << idx << ": " << *rb.store_[idx] << "\n";
      ++idx;
      if (idx == Cap) {
        idx = 0;
      }
    }
    return os;
  }

private:
  StoreT store_ = {};
  unsigned head_ = 0;
  unsigned tail_ = head_;
  unsigned size_ = 0;
};

} // namespace util
