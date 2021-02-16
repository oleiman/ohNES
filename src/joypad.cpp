#include "joypad.hpp"

namespace ctrl {
uint8_t JoyPad::readNext() {
  if (curr_ >= state_.size()) {
    return true;
  } else if (strobe_) {
    return state_[curr_];
  } else {
    return state_[curr_++];
  }
}

void JoyPad::press(Button const b) { state_[static_cast<uint8_t>(b)] = 0x01; }

void JoyPad::release(Button const b) { state_[static_cast<uint8_t>(b)] = 0x00; }

void JoyPad::toggleStrobe() {
  strobe_ = !strobe_;
  curr_ = static_cast<uint8_t>(Button::A);
}
} // namespace ctrl
