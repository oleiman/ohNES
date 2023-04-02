#include "input.hpp"
#include "joypad.hpp"
#include "system.hpp"

namespace sdl_internal {

const std::unordered_map<SDL_KeyCode, ctrl::Button>
    KeyboardInputHandler::ToJoyPad{{
        {SDLK_a, ctrl::Button::A},
        {SDLK_s, ctrl::Button::B},
        {SDLK_q, ctrl::Button::Select},
        {SDLK_w, ctrl::Button::Start},
        {SDLK_UP, ctrl::Button::Up},
        {SDLK_DOWN, ctrl::Button::Down},
        {SDLK_LEFT, ctrl::Button::Left},
        {SDLK_RIGHT, ctrl::Button::Right},
    }};
bool KeyboardInputHandler::Enabled = false;

void KeyboardInputHandler::Init() { Enabled = true; }
void KeyboardInputHandler::HandleEvent(SDL_Event &e, sys::NES &nes,
                                       bool focused) {
  if (!Enabled) {
    return;
  }
  static KeyboardInputHandler instance(nes.joypad_1);
  instance.handleEvent(e, focused);
}

const std::unordered_map<SDL_GameControllerButton, ctrl::Button>
    ControllerInputHandler::ToJoyPad{{
        {SDL_CONTROLLER_BUTTON_B, ctrl::Button::A},
        {SDL_CONTROLLER_BUTTON_A, ctrl::Button::B},
        {SDL_CONTROLLER_BUTTON_BACK, ctrl::Button::Select},
        {SDL_CONTROLLER_BUTTON_START, ctrl::Button::Start},
        {SDL_CONTROLLER_BUTTON_DPAD_UP, ctrl::Button::Up},
        {SDL_CONTROLLER_BUTTON_DPAD_DOWN, ctrl::Button::Down},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT, ctrl::Button::Left},
        {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, ctrl::Button::Right},
    }};

std::unordered_map<SDL_JoystickID, std::unique_ptr<ControllerInputHandler>>
    ControllerInputHandler::Instances;

bool ControllerInputHandler::Enabled = false;

void ControllerInputHandler::Init() {
  Enabled = true;
  assert(SDL_GameControllerEventState(SDL_ENABLE) == SDL_ENABLE);
}

void ControllerInputHandler::HandleEvent(SDL_Event &e, sys::NES &nes,
                                         bool focused) {
  if (!Enabled) {
    return;
  }
  int id = e.cdevice.which;
  switch (e.type) {
  case SDL_CONTROLLERDEVICEADDED: {
    try {
      auto inst =
          std::make_unique<ControllerInputHandler>(nes.getAvailablePad(), id);
      auto js_id = inst->js_id_;
      Instances.emplace(js_id, std::move(inst));
      std::cerr << "Controller " << +js_id << " ADDED... ("
                << +SDL_NumJoysticks() << " total in system)" << std::endl;
    } catch (std::runtime_error &e) {
      std::cerr << e.what() << std::endl;
    }
  } break;
  case SDL_CONTROLLERDEVICEREMOVED: {
    auto it = Instances.find(id);
    if (it != Instances.end()) {
      Instances.erase(id);
      std::cerr << "Controller " << +id << " REMOVED..." << std::endl;
    }
  } break;
  case SDL_CONTROLLERBUTTONDOWN:
  case SDL_CONTROLLERBUTTONUP: {
    auto it = Instances.find(id);
    if (it != Instances.end()) {
      it->second->handleEvent(e, true);
    }
  } break;
  default:
    std::cerr << "Unhandled: " << +e.type << std::endl;
    break;
  }
}

} // namespace sdl_internal
