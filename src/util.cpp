#include "util.hpp"

#include <array>

namespace util {
void reverseByte(uint8_t &b) {
  static const std::array<uint8_t, 16> lookup{
      0b0000, 0b1000, 0b0100, 0b1100, 0b0010, 0b1010, 0b0110, 0b1110,
      0b0001, 0b1001, 0b0101, 0b1101, 0b0011, 0b1011, 0b0111, 0b1111};
  b = (lookup[b & 0b1111] << 4) | (lookup[b >> 4]);
}
} // namespace util
