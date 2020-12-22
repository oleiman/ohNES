
// TODO(oren): does this need a path?
#include "cpu.hpp"
#include "memory.hpp"

using cpu::M6502;
using mem::Ram;

constexpr int memory_size = 0x10000;

int main(int argc, char **argv) {
  Ram<memory_size, M6502::AddressT, M6502::DataT> cpuVRAM;
  return 0;
}
