#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <ostream>

namespace util {
static constexpr uint8_t BIT0 = 0b00000001;
static constexpr uint8_t BIT1 = 0b00000010;
static constexpr uint8_t BIT2 = 0b00000100;
static constexpr uint8_t BIT3 = 0b00001000;
static constexpr uint8_t BIT4 = 0b00010000;
static constexpr uint8_t BIT5 = 0b00100000;
static constexpr uint8_t BIT6 = 0b01000000;
static constexpr uint8_t BIT7 = 0b10000000;

void reverseByte(uint8_t &b);

template <typename T, size_t Cap,
          typename StoreT = std::array<std::unique_ptr<T>, Cap>>
class RingBuf {
public:
  RingBuf(){};
  ~RingBuf() = default;
  RingBuf(const RingBuf &other) {
    for (int i = 0; i < Cap; ++i) {
      auto &oit = other.store_[i];
      if (oit != nullptr) {
        store_[i] = std::make_unique<T>(oit);
      } else {
        store_[i] = nullptr;
      }
    }
    tail_ = other.tail_;
    head_ = other.head_;
    size_ = other.size_;
  }

  RingBuf &operator=(const RingBuf &other) {
    if (this == &other) {
      return *this;
    }

    for (int i = 0; i < Cap; ++i) {
      auto &oit = other.store_[i];
      if (oit != nullptr) {
        store_[i] = std::make_unique<T>(oit);
      } else {
        store_[i] = nullptr;
      }
    }
    tail_ = other.tail_;
    head_ = other.head_;
    size_ = other.size_;
    return *this;
  }

  void insert(const T &item) {
    store_[tail_] = std::make_unique<T>(item);
    ++tail_;
    if (tail_ == Cap) {
      tail_ = 0;
    }
    if (size_ < Cap) {
      ++size_;
    } else {
      ++head_;
      if (head_ == Cap) {
        head_ = 0;
      }
    }
  }

  size_t size() const { return size_; }

  const T &back() const {
    assert(size_ > 0);
    if (tail_ == 0) {
      return *store_[size_ - 1];
    } else {
      return *store_[tail_ - 1];
    }
  }

  const T &operator[](size_t i) const {
    assert(i < size_);
    size_t idx = (head_ + i) % store_.size();
    return *store_[idx];
  }

  friend std::ostream &operator<<(std::ostream &os, const RingBuf &rb) {
    auto idx = rb.head_;
    int rel_i = 0;
    os << "Size: " << +rb.size_ << std::endl;
    while ((idx != rb.tail_ || rel_i == 0) && rb.store_[idx] != nullptr) {
      os << "[" << +rel_i << "] " << idx << ": " << *rb.store_[idx] << "\n";
      ++idx;
      if (idx == Cap) {
        idx = 0;
      }
      ++rel_i;
    }
    return os;
  }

private:
  StoreT store_ = {};
  size_t head_ = 0;
  size_t tail_ = head_;
  size_t size_ = 0;
};

} // namespace util
