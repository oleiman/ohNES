
#include "cartridge.hpp"
#include "cpu.hpp"
#include "debugger.hpp"
#include "joypad.h"
#include "mapper.hpp"
#include "memory.hpp"
#include "ppu.hpp"

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
using mapper::NROM;
using mapper::PPUMap;
using std::string;
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

  string f(argv[1]);
  std::ifstream infile(f, std::ios::in | std::ios::binary);
  if (!infile) {
    std::cerr << "BAD FILE" << std::endl;
    exit(1);
  }
  Cartridge c(infile);
  infile.close();
  std::cerr << c << std::endl;

  // TODO(oren): ugh, initialization
  // cpu mapper needs ref to ppu_reg
  // ppu mapper nees ref to cpu_mem
  // both vrams need ref to respectie mappers
  // both cpu and ppu need refs to respective busses
  // On top of all this, VRam template precludes any runtime
  // mapper selection. That's probably the first thing to address.
  // I expect everything else will fall into place after that.
  // With current design, only solution is to have regs
  // and cpu internal memory owned by the system, which makes
  // absolutely no sense.
  // Quickest mitigation would be to just store the busses as pointers to
  // be updated after construction, but this leaves the CPU/PPU in an unuseable
  // state, which just feels a little gross.
  Registers ppu_reg;

  JoyPad joypad;
  // std::array<uint8_t, 0x800> cpu_mem{};
  NROM cpu_map(c, ppu_reg, ppu_reg.oam(), joypad);
  PPUMap ppu_map(c, ppu_reg.oam());

  // TODO(oren): I think I'd actually like to move this inside the cpu, giving
  // access to debugger and potentially logging. Would still need the callback
  // interface, but that's only to drive other components. The counter itself
  // could just get incremented in a wrapper or something
  M6502 cpu(cpu_map, false);

  auto ppu = PPU(ppu_map, cpu.nmiPin(), ppu_reg);
  ppu.showPatternTable();

  cpu.reset();

  // automation mode, do the reset things, but then immediately set PC to the
  // beginning of the tests
  // cpu.reset(0xC000);

  Debugger d(true, false);

  SDL_Event event;

  bool quit = false;
  while (!quit) {

    while (SDL_PollEvent(&event) != 0) {
      switch (event.type) {
      case SDL_QUIT:
        std::cerr << "Quitting" << std::endl;
        quit = true;
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_a:
          joypad.pressButton(Button::A);
          break;
        case SDLK_s:
          joypad.pressButton(Button::B);
          break;
        case SDLK_q:
          joypad.pressButton(Button::Select);
          break;
        case SDLK_w:
          joypad.pressButton(Button::Start);
          break;
        case SDLK_UP:
          joypad.pressButton(Button::Up);
          break;
        case SDLK_DOWN:
          joypad.pressButton(Button::Down);
          break;
        case SDLK_LEFT:
          joypad.pressButton(Button::Left);
          break;
        case SDLK_RIGHT:
          joypad.pressButton(Button::Right);
          break;
        default:
          std::cout << +event.key.keysym.sym << std::endl;
          break;
        }
        break;
      case SDL_KEYUP:
        switch (event.key.keysym.sym) {
        case SDLK_a:
          joypad.releaseButton(Button::A);
          break;
        case SDLK_s:
          joypad.releaseButton(Button::B);
          break;
        case SDLK_q:
          joypad.releaseButton(Button::Select);
          break;
        case SDLK_w:
          joypad.releaseButton(Button::Start);
          break;
        case SDLK_UP:
          joypad.releaseButton(Button::Up);
          break;
        case SDLK_DOWN:
          joypad.releaseButton(Button::Down);
          break;
        case SDLK_LEFT:
          joypad.releaseButton(Button::Left);
          break;
        case SDLK_RIGHT:
          joypad.releaseButton(Button::Right);
          break;
        default:
          std::cout << +event.key.keysym.sym << std::endl;
          break;
        }
        break;
      }
    }

    try {
      // auto c = cpu.debugStep(d);
      auto c = cpu.step();

      // (void)c;
      ppu.step(c * 3);
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
      std::cerr << "Cycles: " << cpu.state().cycle << std::endl;
      std::cerr << std::hex << "PC: 0x" << +cpu.state().pc << std::endl;
      quit = true;
      SDL_Delay(SCREEN_DELAY);
    }

    // maybe some stuff about performance counter to drive a certain number of
    // cpu cycles we're still technically instruction stepped, so this might get
    // a little weird and rendering may suffer

    if (cpu.nmiPending() && ppu_reg.showBackground()) {
      // std::cout << "UPDATE SCREEN" << std::endl;
      ppu.renderBackground();
      ppu.renderSprites();
      SDL_UpdateTexture(texture, nullptr, ppu.frameBuffer().data(),
                        vid::WIDTH * 3);
      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, nullptr, nullptr);
      SDL_RenderPresent(renderer);
      // SDL_Delay(2);
      // SDL_Delay(SCREEN_DELAY);
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyTexture(texture);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
