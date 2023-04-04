#pragma once

#include "dbg/nes_debugger.hpp"
#include "debugger.hpp"
#include "joypad.hpp"

#include <SDL.h>

#include <cassert>
#include <memory>
#include <unordered_map>

namespace sys {
class NES;
}

namespace sdl_internal {

template <class Derived, class KeyT> class InputHandler {
  using KeyMap = std::unordered_map<KeyT, ctrl::Button>;

public:
  InputHandler(ctrl::JoyPad &pad, const KeyMap &map, sys::NESDebugger &dbg)
      : joypad_(pad), map_(map), debugger_(dbg) {}

  virtual ~InputHandler() = default;

  void handleEvent(SDL_Event &e, bool focused) {
    if (!focused || !static_cast<Derived &>(*this).accept(e)) {
      return;
    }
    KeyT key = static_cast<Derived &>(*this).get_key(e);
    update(key, static_cast<Derived &>(*this).get_state(e));
  }

protected:
  ctrl::JoyPad &joypad_;

private:
  void update(KeyT k, uint8_t state) {
    auto it = map_.find(k);
    if (it != map_.end()) {
      auto btn = it->second;
      if (state == SDL_PRESSED) {
        joypad_.press(btn);
      } else if (state == SDL_RELEASED) {
        joypad_.release(btn);
      } else {
        assert(false);
      }
      debugger_.processInput(joypad_.ID, static_cast<uint8_t>(btn), state);
    }
  }
  const KeyMap &map_;
  sys::NESDebugger &debugger_;
};

class KeyboardInputHandler
    : public InputHandler<KeyboardInputHandler, SDL_KeyCode> {
  using KeyT = SDL_KeyCode;
  using Parent = InputHandler<KeyboardInputHandler, KeyT>;
  using KeyMap = std::unordered_map<KeyT, ctrl::Button>;
  static bool Enabled;

public:
  KeyboardInputHandler(ctrl::JoyPad &jp, sys::NESDebugger &dbg)
      : Parent(jp, ToJoyPad, dbg) {}
  ~KeyboardInputHandler() = default;

  static void Init();
  static void HandleEvent(SDL_Event &e, sys::NES &nes, bool focused);
  bool accept(SDL_Event &e) { return true; }
  KeyT get_key(SDL_Event &e) { return static_cast<KeyT>(e.key.keysym.sym); }
  uint8_t get_state(SDL_Event &e) { return e.key.state; }

private:
  static const KeyMap ToJoyPad;
};

class ControllerInputHandler
    : public InputHandler<ControllerInputHandler, SDL_GameControllerButton> {
  using KeyT = SDL_GameControllerButton;
  using Parent = InputHandler<ControllerInputHandler, KeyT>;
  using KeyMap = std::unordered_map<KeyT, ctrl::Button>;
  static std::unordered_map<SDL_JoystickID,
                            std::unique_ptr<ControllerInputHandler>>
      Instances;
  static bool Enabled;

public:
  ControllerInputHandler(ctrl::JoyPad &jp, int handle, sys::NESDebugger &dbg)
      : Parent(jp, ToJoyPad, dbg) {
    auto controller = SDL_GameControllerOpen(handle);
    auto joystick = SDL_GameControllerGetJoystick(controller);
    js_id_ = SDL_JoystickInstanceID(joystick);
  }

  ~ControllerInputHandler() {
    joypad_.unclaim();
    // NOTE(oren): seems that closing the controller and/or underlying Joystick
    // causes unpredictable crashes? No clue why really, but it's vaguely
    // mentioned in this thread:
    // https://discourse.libsdl.org/t/memory-corruption-problem-with-sdl2-and-joysticks/20030/14
    // The crashes are usually some form of "double free", so maybe it's the
    // case that some memory associated with the controller is freed
    // automatically before SDL generates CONTROLLERDEVICEREMOVED, then the
    // Close function tries to free that memory again?
    // Bit of evidence: In the CONTROLLERDEVICEREMOVED handler, NumJoysticks
    // returns 0 before calling this destructor and regatdless of whether we
    // call close.
    // SDL_GameControllerClose(controller_);
  }

  static void Init();
  static void HandleEvent(SDL_Event &e, sys::NES &nes, bool focused);
  bool accept(SDL_Event &e) { return e.cbutton.which == js_id_; }
  KeyT get_key(SDL_Event &e) { return static_cast<KeyT>(e.cbutton.button); }
  uint8_t get_state(SDL_Event &e) { return e.cbutton.state; }

private:
  SDL_JoystickID js_id_;
  static const KeyMap ToJoyPad;
};

class RecordingInputHandler
    : public InputHandler<RecordingInputHandler, uint8_t> {
  using KeyT = uint8_t;
  using Parent = InputHandler<RecordingInputHandler, KeyT>;
  using KeyMap = std::unordered_map<KeyT, ctrl::Button>;
  static bool Enabled;
  static uint32_t REC_EVENT;

public:
  RecordingInputHandler(ctrl::JoyPad &jp, sys::NESDebugger &dbg)
      : Parent(jp, ToJoyPad, dbg) {}
  ~RecordingInputHandler() = default;

  static void Init();
  static void HandleEvent(SDL_Event &e, sys::NES &nes, bool focused);
  static uint32_t EventType() { return REC_EVENT; }
  bool accept(SDL_Event &e) { return true; }
  KeyT get_key(SDL_Event &e) {
    return static_cast<KeyT>((e.user.code >> 8) & 0xFF);
  }
  uint8_t get_state(SDL_Event &e) {
    return static_cast<uint8_t>(e.user.code & 0xFF);
  }

private:
  static const KeyMap ToJoyPad;
};

} // namespace sdl_internal
