
#include "cartridge.hpp"
#include "cpu.hpp"
#include "debugger.hpp"
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
using dbg::Debugger;
using mapper::NROM;
using mapper::PPUMap;
using mem::VRam;
using ppu::PPU;
using ppu::Registers;
using std::string;

using std::filesystem::current_path;

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

  SDL_CreateWindowAndRenderer(ppu::WIDTH * 2, ppu::HEIGHT * 2, SDL_WINDOW_SHOWN,
                              &window, &renderer);

  if (window == nullptr || renderer == nullptr) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                             "SDL_CreateWindowAndRenderer Error",
                             SDL_GetError(), NULL);
    SDL_Delay(2000);
    exit(1);
  }

  SDL_SetWindowTitle(window, "ohNES v0.1");

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_RenderSetLogicalSize(renderer, ppu::WIDTH, ppu::HEIGHT);

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
                        SDL_TEXTUREACCESS_STREAMING, ppu::WIDTH, ppu::HEIGHT);

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

  // TODO(oren): this interface is broken. No way at all to select
  // a mapper at runtime :(
  // there shouldn't be templates here LOL
  std::array<uint8_t, 0x800> cpu_mem{};
  VRam cpu_vram(NROM(c, cpu_mem, ppu_reg));
  VRam ppu_vram(PPUMap(c, cpu_mem));

  // TODO(oren): I think I'd actually like to move this inside the cpu, giving
  // access to debugger and potentially logging. Would still need the callback
  // interface, but that's only to drive other components. The counter itself
  // could just get incremented in a wrapper or something
  int cycles = 0;
  M6502 cpu(cpu_vram.addressBus(), cpu_vram.dataBus(), false);

  auto pixProc =
      PPU(ppu_vram.addressBus(), ppu_vram.dataBus(), cpu.nmiPin(), ppu_reg);
  // pixProc.initFramebuf();
  pixProc.showPatternTable();
  auto tick = [&]() {
    pixProc.step(3);
    ++cycles;
  };
  cpu.registerClockCallback(tick);

  // automation mode, skips over the ppu check in nestest
  // cpu.initPc(0xC000);

  // full mode. most roms just loop on status. PPU not yet implemented
  cpu.reset();

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
      }
    }

    try {
      // cpu.debugStep(d);
      cpu.step();
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
      std::cerr << "Cycles: " << +cycles << std::endl;
      std::cerr << std::hex << "PC: 0x" << +cpu.pc() << std::endl;
      quit = true;
    }

    // maybe some stuff about performance counter to drive a certain number of
    // cpu cycles we're still technically instruction stepped, so this might get
    // a little weird and rendering may suffer

    // try {
    //   do {
    //     cpu.debugStep(d);
    //     // std::cerr << +cycles << std::endl;
    //   } while (cpu.pc() != 0xC66E);
    // } catch (std::exception &e) {
    //   std::cerr << e.what() << std::endl;
    //   std::cerr << "Cycles: " << +cycles << std::endl;
    //   std::cerr << std::hex << "PC: 0x" << +cpu.pc() << std::endl;
    //   quit = true;
    // }

    SDL_UpdateTexture(texture, nullptr, pixProc.frameBuffer().data(),
                      ppu::WIDTH * 3);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyTexture(texture);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
