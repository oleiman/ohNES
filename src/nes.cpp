
#include "cartridge.hpp"
#include "cpu.hpp"
#include "debugger.hpp"
#include "joypad.hpp"
#include "mappers/mapper_factory.hpp"
#include "mappers/ppu_map.hpp"
#include "ppu.hpp"
#include "system.hpp"

#include "time.h"

#include <SDL.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using cart::Cartridge;
using cpu::M6502;
using ctrl::Button;
using ctrl::JoyPad;
using dbg::Debugger;

using mapper::MapperFactory;
using mapper::PPUMap;
using std::string;
using sys::NES;
using vid::PPU;
using vid::Registers;

using std::filesystem::current_path;

#define SCALE 4
#define SCREEN_DELAY 2000

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "USAGE: ohNES /path/to/rom/file" << std::endl;
    exit(1);
  }

  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  std::array<std::array<uint8_t, 3>, vid::WIDTH * vid::HEIGHT> fb;

  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL_Init Error",
                             SDL_GetError(), NULL);
    SDL_Delay(2000);
    exit(1);
  }

  SDL_CreateWindowAndRenderer(vid::WIDTH * SCALE, vid::HEIGHT * SCALE,
                              SDL_WINDOW_SHOWN, &window, &renderer);

  if (window == nullptr || renderer == nullptr) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                             "SDL_CreateWindowAndRenderer Error",
                             SDL_GetError(), NULL);
    SDL_Delay(2000);
    exit(1);
  }

  SDL_SetWindowTitle(window, "ohNES v0.1");

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_RenderSetLogicalSize(renderer, vid::WIDTH, vid::HEIGHT);

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                        SDL_TEXTUREACCESS_STREAMING, vid::WIDTH, vid::HEIGHT);

  NES nes(argv[1]);

  SDL_Event event;

  bool quit = false;
  auto clockStart = clock();
  int frames = 0;
  while (!quit) {

    while (SDL_PollEvent(&event) != 0) {
      switch (event.type) {
      case SDL_QUIT:
        std::cerr << "Quitting: "
                  << frames / ((double)(clock() - clockStart) / CLOCKS_PER_SEC)
                  << "fps" << std::endl;
        quit = true;
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_a:
          nes.joypad_1.press(Button::A);
          break;
        case SDLK_s:
          nes.joypad_1.press(Button::B);
          break;
        case SDLK_q:
          nes.joypad_1.press(Button::Select);
          break;
        case SDLK_w:
          nes.joypad_1.press(Button::Start);
          break;
        case SDLK_UP:
          nes.joypad_1.press(Button::Up);
          break;
        case SDLK_DOWN:
          nes.joypad_1.press(Button::Down);
          break;
        case SDLK_LEFT:
          nes.joypad_1.press(Button::Left);
          break;
        case SDLK_RIGHT:
          nes.joypad_1.press(Button::Right);
          break;
        default:
          std::cout << +event.key.keysym.sym << std::endl;
          break;
        }
        break;
      case SDL_KEYUP:
        switch (event.key.keysym.sym) {
        case SDLK_a:
          nes.joypad_1.release(Button::A);
          break;
        case SDLK_s:
          nes.joypad_1.release(Button::B);
          break;
        case SDLK_q:
          nes.joypad_1.release(Button::Select);
          break;
        case SDLK_w:
          nes.joypad_1.release(Button::Start);
          break;
        case SDLK_UP:
          nes.joypad_1.release(Button::Up);
          break;
        case SDLK_DOWN:
          nes.joypad_1.release(Button::Down);
          break;
        case SDLK_LEFT:
          nes.joypad_1.release(Button::Left);
          break;
        case SDLK_RIGHT:
          nes.joypad_1.release(Button::Right);
          break;
        default:
          std::cout << +event.key.keysym.sym << std::endl;
          break;
        }
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

    // maybe some stuff about performance counter to drive a certain number of
    // cpu cycles we're still technically instruction stepped, so this might get
    // a little weird and rendering may suffer

    if (nes.render(fb)) {
      SDL_UpdateTexture(texture, nullptr, fb.data(), vid::WIDTH * 3);
      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, nullptr, nullptr);
      SDL_RenderPresent(renderer);
      ++frames;
      // SDL_Delay(10);
      // SDL_Delay(SCREEN_DELAY);
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyTexture(texture);
  SDL_DestroyWindow(window);
  SDL_Quit();
  std::cout << +nes.state().cycle << std::endl;
  std::cout << "Frames: " << frames << std::endl;

  return 0;
}
