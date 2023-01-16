#pragma once

#include <array>
#include <cstdint>

#include <iostream>

namespace ctrl {
enum class Button {
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
  void toggleStrobe();

private:
  std::array<uint8_t, static_cast<size_t>(Button::N_BUTTONS)> state_ = {};
  uint8_t curr_ = 0;
  bool strobe_ = false;
};
} // namespace ctrl
