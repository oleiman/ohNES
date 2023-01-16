#include "ppu.hpp"
#include "sdl/display.hpp"
#include "sdl/pad_maps.hpp"
#include "system.hpp"

#include <SDL.h>
#include <args.hxx>
#include <nfd.h>

#include "time.h"
#include <chrono>
#include <iostream>
#include <string>

using vid::LoadSystemPalette;

using sdl_internal::Display;
using sdl_internal::Kbd2JoyPad;
using sys::NES;

#define SCALE 2
#define SCREEN_DELAY 2000

const char DEFAULT_PALETTE[] = "../data/2c02.palette";

int main(int argc, char **argv) {
  std::string file;
  if (argc >= 2) {
    file = argv[1];
  } else {
    // TODO(oren): appkit version of NFD doesn't seem to work. I'd guess this
    // has something to do with arm/x64 compatibility?
    nfdchar_t *outPath = nullptr;
    nfdresult_t result = NFD_OpenDialog(nullptr, nullptr, &outPath);
    if (result == NFD_OKAY) {
      std::cerr << "Rom Path: " << outPath << std::endl;
      file = outPath;
      free(outPath);
    } else if (result == NFD_CANCEL) {
      std::cerr << "Fun Police" << std::endl;
      exit(0);
    } else {
      std::cerr << "File select error: " << NFD_GetError() << std::endl;
      exit(1);
    }
  }

  LoadSystemPalette(DEFAULT_PALETTE);

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL_Init Error",
                             SDL_GetError(), NULL);
    SDL_Delay(2000);
    exit(1);
  }
  {
    Display<vid::WIDTH, vid::HEIGHT, SCALE> display;
    NES nes(file, false);

    SDL_Event event;
    bool quit = false;
    auto clockStart = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point clockEnd;
    while (!quit) {
      while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
          clockEnd = std::chrono::steady_clock::now();
          quit = true;
          break;
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
          auto sym = event.key.keysym.sym;
          if (Kbd2JoyPad.count(sym) > 0) {
            auto b = Kbd2JoyPad.find(sym)->second;
            if (event.type == SDL_KEYDOWN) {
              nes.joypad_1.press(b);
            } else {
              nes.joypad_1.release(b);
            }
          }
        } break;

        default:
          break;
        }
      }

      // Render a whole frame before checking keyboard events. This effectively
      // locks the polling loop to vsync. It's responsive enough.
      do {
        try {
          nes.step();
        } catch (std::exception &e) {
          clockEnd = std::chrono::steady_clock::now();
          std::cerr << e.what() << std::endl;
          std::cerr << "Cycle: " << nes.state().cycle << std::endl;
          std::cerr << std::hex << "PC: 0x" << +nes.state().pc << std::dec
                    << std::endl;
          quit = true;
          SDL_Delay(SCREEN_DELAY);
          break;
        }
      } while (!nes.render(display.renderBuf));

      display.update();
    }

    std::cerr << "Quitting: "
              << static_cast<double>(display.frames()) /
                     (std::chrono::duration_cast<std::chrono::seconds>(
                          clockEnd - clockStart)
                          .count())
              << "fps" << std::endl;

    std::cout << "Cycles: " << +nes.state().cycle << std::endl;
    std::cout << "Frames: " << display.frames() << std::endl;
  }
  SDL_Quit();

  return 0;
}
