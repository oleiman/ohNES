
#include "cartridge.hpp"
#include "cpu.hpp"
#include "debugger.hpp"
#include "mapper.hpp"
#include "memory.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using cart::Cartridge;
using cpu::M6502;
using dbg::Debugger;
using mapper::NROM;
using mem::VRam;
using std::string;

using std::filesystem::current_path;

int main(int argc, char **argv) {
  if (argc == 1) {
    std::cerr << "USAGE: ohNES /path/to/rom/file" << std::endl;
    exit(1);
  }
  string f(argv[1]);
  std::ifstream infile(f, std::ios::in | std::ios::binary);
  if (!infile) {
    std::cerr << "BAD FILE" << std::endl;
    exit(1);
  }
  Cartridge c(infile);
  infile.close();
  std::cerr << c << std::endl;

  std::array<uint8_t, 8> ppu_reg;

  // TODO(oren): this interface is broken. No way at all to select
  // a mapper at runtime :(
  // there shouldn't be templates here LOL
  std::array<uint8_t, 0x800> cpu_mem;
  VRam cpu_vram(NROM(c, cpu_mem, ppu_reg));

  int cycles = 0;
  auto tick = [&cycles]() { ++cycles; };
  M6502 cpu(cpu_vram.addressBus(), cpu_vram.dataBus(), tick);
  cpu.initPc(0xC000);

  Debugger d(true, false);

  try {
    do {
      cpu.debugStep(d);
      // std::cerr << +cycles << std::endl;
    } while (cpu.pc() != 0xC66E);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << "Cycles: " << +cycles << std::endl;
    std::cerr << std::hex << "PC: 0x" << +cpu.pc() << std::endl;
  }

  cpu_vram.addressBus().put(0x000);
  std::cerr << std::hex << "0x" << +cpu_vram.dataBus().get() << std::endl;

  return 0;
}
