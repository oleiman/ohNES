#pragma once

#include <SDL.h>

#include <array>
#include <string>

namespace sdl_internal {

template <int W, int H, int S = 1> class Display {
public:
  Display(const std::string &name, const std::string &title) {

    window = SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, W * S, H * S,
                              SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

    if (window == nullptr || renderer == nullptr) {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                               "SDL_CreateWindowAndRenderer Error",
                               SDL_GetError(), NULL);
      SDL_Delay(2000);
      exit(1);
    }

    // Grab window identifier
    window_id = SDL_GetWindowID(window);

    // Flag as opened
    shown = true;
    minimized = false;

    SDL_SetWindowTitle(window, title.c_str());

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    SDL_RenderSetLogicalSize(renderer, W, H);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                SDL_TEXTUREACCESS_STREAMING, W, H);
  }
  ~Display() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
  }

  void handleEvent(SDL_Event &e) {
    if (e.type == SDL_WINDOWEVENT && e.window.windowID == window_id) {
      switch (e.window.event) {
      case SDL_WINDOWEVENT_SHOWN:
        shown = true;
        break;
      case SDL_WINDOWEVENT_HIDDEN:
        shown = false;
        break;
      case SDL_WINDOWEVENT_ENTER:
        mouse_focus = true;
        break;
      case SDL_WINDOWEVENT_LEAVE:
        mouse_focus = false;
        break;
      case SDL_WINDOWEVENT_FOCUS_GAINED:
        keyboard_focus = true;
        break;
      case SDL_WINDOWEVENT_FOCUS_LOST:
        keyboard_focus = false;
        break;
      case SDL_WINDOWEVENT_MINIMIZED:
        minimized = true;
        break;
      case SDL_WINDOWEVENT_RESTORED:
        minimized = false;
        break;
      case SDL_WINDOWEVENT_CLOSE:
        SDL_HideWindow(window);
        break;
      default:
        break;
      }
    }
  }

  void update() {
    if (shown) {
      SDL_UpdateTexture(texture, nullptr, renderBuf.data(), W * 3);
      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, nullptr, nullptr);
      SDL_RenderPresent(renderer);
      ++frames_;
    }
  }

  void focus() {
    if (!shown) {
      SDL_ShowWindow(window);
    }
    SDL_RaiseWindow(window);
  }

  bool hasKbdFocus() { return keyboard_focus; }
  bool hasMouseFocus() { return mouse_focus; }
  bool isShown() { return shown; }

  unsigned long frames() { return frames_; }

  std::array<std::array<uint8_t, 3>, W *H> renderBuf = {};

private:
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  int window_id;

  bool mouse_focus;
  bool keyboard_focus;
  bool shown;
  bool minimized;
  unsigned long frames_ = 0;
};
} // namespace sdl_internal
