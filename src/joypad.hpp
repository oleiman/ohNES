#pragma once

#include <array>
#include <cstdint>

#include <iostream>

namespace ctrl {
enum class Button : uint8_t {
  A = 0,
  B,
  Select,
  Start,
  Up,
  Down,
  Left,
  Right,
  N_BUTTONS,
};

struct JoyPad {
  uint8_t readNext();
  void press(Button const b);
  void release(Button const b);
  void setStrobe(bool s);
  bool claim() {
    if (!claimed_) {
      claimed_ = true;
      return true;
    } else {
      return false;
    }
  }
  void unclaim() { claimed_ = false; }

  const uint8_t ID = NextId++;

private:
  static uint8_t NextId;
  std::array<uint8_t, static_cast<size_t>(Button::N_BUTTONS)> state_ = {};
  uint8_t curr_ = 0;
  bool strobe_ = false;
  bool claimed_ = false;
};
} // namespace ctrl
