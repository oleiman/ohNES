#pragma once

#include "joypad.hpp"

#include <SDL.h>

#include <unordered_map>

namespace sdl_internal {

const std::unordered_map<SDL_Keycode, ctrl::Button> Kbd2JoyPad{{
    {SDLK_a, ctrl::Button::A},
    {SDLK_s, ctrl::Button::B},
    {SDLK_q, ctrl::Button::Select},
    {SDLK_w, ctrl::Button::Start},
    {SDLK_UP, ctrl::Button::Up},
    {SDLK_DOWN, ctrl::Button::Down},
    {SDLK_LEFT, ctrl::Button::Left},
    {SDLK_RIGHT, ctrl::Button::Right},
}};
} // namespace sdl_internal
