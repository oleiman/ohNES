#include "sdl/display.hpp"
#include "sdl/pad_maps.hpp"
#include "system.hpp"

#include <SDL.h>
#include <nfd.h>

#include "time.h"
#include <iostream>
#include <string>

using cart::Cartridge;
using cpu::M6502;
using ctrl::Button;

using sdl_internal::Display;
using sdl_internal::Kbd2JoyPad;
using sys::NES;

#define SCALE 4
#define SCREEN_DELAY 2000

int main(int argc, char **argv) {
  std::string file;
  if (argc == 2) {
    file = argv[1];
  } else {
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

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL_Init Error",
                             SDL_GetError(), NULL);
    SDL_Delay(2000);
    exit(1);
  }
  {
    Display<vid::WIDTH, vid::HEIGHT, SCALE> display;
    NES nes(file);

    SDL_Event event;
    bool quit = false;
    auto clockStart = clock();
    while (!quit) {
      while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
          std::cerr << "Quitting: "
                    << display.frames() /
                           ((double)(clock() - clockStart) / CLOCKS_PER_SEC)
                    << "fps" << std::endl;
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

      try {
        nes.step();
      } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Cycles: " << nes.state().cycle << std::endl;
        std::cerr << std::hex << "PC: 0x" << +nes.state().pc << std::endl;
        quit = true;
        SDL_Delay(SCREEN_DELAY);
      }

      // TODO(oren): limit framerate to 60fps

      if (nes.render(display.renderBuf)) {
        display.update();
      }
    }

    std::cout << "Cycles: " << +nes.state().cycle << std::endl;
    std::cout << "Frames: " << display.frames() << std::endl;
  }
  SDL_Quit();

  return 0;
}
