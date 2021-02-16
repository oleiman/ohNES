#pragma once

#include <SDL.h>

#include <array>

namespace sdl_int {

template <int W, int H, int S> class Display {
public:
  Display() {

    SDL_CreateWindowAndRenderer(W * S, H * S, SDL_WINDOW_SHOWN, &window,
                                &renderer);

    if (window == nullptr || renderer == nullptr) {
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                               "SDL_CreateWindowAndRenderer Error",
                               SDL_GetError(), NULL);
      SDL_Delay(2000);
      exit(1);
    }

    SDL_SetWindowTitle(window, "ohNES v0.1");

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, W, H);

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                                SDL_TEXTUREACCESS_STREAMING, W, H);
  }
  ~Display() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
  }

  void update() {
    SDL_UpdateTexture(texture, nullptr, renderBuf.data(), W * 3);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
    ++frames_;
  }

  unsigned long long frames() { return frames_; }

  std::array<std::array<uint8_t, 3>, W *H> renderBuf = {};

private:
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  unsigned long long frames_;
};
} // namespace sdl_int
