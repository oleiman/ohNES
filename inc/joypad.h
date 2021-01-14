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
};

struct JoyPad {
  std::array<uint8_t, 8> state{};

  uint8_t readNext() {
    if (curr_ >= state.size()) {
      return true;
    } else if (strobe_) {
      return state[curr_];
    } else {
      return state[curr_++];
    }
  }

  void pressButton(Button b) { state[static_cast<uint8_t>(b)] = 0x01; }

  void releaseButton(Button b) { state[static_cast<uint8_t>(b)] = 0x00; }

  void toggleStrobe() {
    strobe_ = !strobe_;
    curr_ = static_cast<uint8_t>(Button::A);
  }

private:
  uint8_t curr_ = 0;
  bool strobe_ = false;
};
} // namespace ctrl
