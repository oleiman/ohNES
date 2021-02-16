#include "mappers/mapper_factory.hpp"
#include "sdl/display.hpp"
#include "sdl/pad_maps.hpp"
#include "system.hpp"

#include "time.h"
#include <SDL.h>
#include <iostream>

using cart::Cartridge;
using cpu::M6502;
using ctrl::Button;

using mapper::MapperFactory;
using sdl_int::Display;
using sdl_int::Kbd2JoyPad;
using sys::NES;

#define SCALE 4
#define SCREEN_DELAY 2000

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "USAGE: ohNES /path/to/rom/file" << std::endl;
    exit(1);
  }

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL_Init Error",
                             SDL_GetError(), NULL);
    SDL_Delay(2000);
    exit(1);
  }
  {
    Display<vid::WIDTH, vid::HEIGHT, SCALE> display;
    NES nes(argv[1]);

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
