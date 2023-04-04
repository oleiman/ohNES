#include "joypad.hpp"

namespace ctrl {
uint8_t JoyPad::NextId = 0;

uint8_t JoyPad::readNext() {
  if (curr_ >= state_.size()) {
    return 0x01;
  } else if (strobe_) {
    return state_[curr_];
  } else {
    return state_[curr_++];
  }
}

void JoyPad::press(Button const b) { state_[static_cast<uint8_t>(b)] = 0x01; }

void JoyPad::release(Button const b) { state_[static_cast<uint8_t>(b)] = 0x00; }

void JoyPad::setStrobe(bool s) {
  strobe_ = s;
  if (s) {
    curr_ = static_cast<uint8_t>(Button::A);
  }
}
} // namespace ctrl
