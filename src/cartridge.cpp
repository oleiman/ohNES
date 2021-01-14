#include "cartridge.hpp"

#include <assert.h>
#include <iostream>
#include <iterator>
#include <sstream>

using std::array;
using std::cerr;
using std::endl;
using std::hex;
using std::istreambuf_iterator;
using std::ostream;
using std::string;
using std::vector;

// Flag 6
#define MIRRORING 0x01
#define PRGRAM 0x02
#define TRAINER 0x04
#define IGNOREMIR 0x08
#define MAPLSB 0xF0

// Flag 7
#define VSUNISYS 0x01
#define PC10 0x02
#define NES2 0x0C
#define MAPMSB 0xF0

#define B_RED(S) "\033[1;31m" + S + "\033[0m"
#define B_CYAN(S) "\033[1;36m" + S + "\033[0m"

namespace cart {

Cartridge::Cartridge(std::ifstream &infile) {
  array<uint8_t, 16> hdr;
  for (int i = 0; i < 16; ++i) {
    infile >> hdr[i];
  }
  parseHeader(hdr);
  std::istreambuf_iterator<char> start(infile), end;
  vector<uint8_t> data(start, end);

  cerr << +data.size() << endl;

  auto it = data.begin();

  if (hasTrainer) {
    trainer_ = vector<uint8_t>(it, it + 512);
    it += trainer_.size();
  }

  prg_rom_ = vector<uint8_t>(it, it + prgRomSize);
  it += prg_rom_.size();
  chr_rom_ = vector<uint8_t>(it, it + chrRomSize);
  it += chr_rom_.size();

  if (pc10) {
    pc_inst_rom_ = vector<uint8_t>(it, it + 8192);
    it += pc_inst_rom_.size();
    pc_prom_ = vector<uint8_t>(it, it + 32);
    it += pc_prom_.size();
  }
}

void Cartridge::parseHeader(array<uint8_t, 16> hdr) {
  for (int i = 0; i < 3; ++i) {
    cerr << hdr[i];
  }
  cerr // << +hdr[3]
      << endl;
  assert(hdr[3] == 0x1A);

  prgRomSize = hdr[4] * (1 << 14);
  chrRomSize = hdr[5] * (1 << 13);

  mirroring = hdr[6] & MIRRORING;
  hasPrgRam = hdr[6] & PRGRAM;
  hasTrainer = (hdr[6] & TRAINER) > 0;
  ignoreMirror = (hdr[6] & IGNOREMIR) > 0;

  vsSystem = hdr[7] & VSUNISYS;
  pc10 = hdr[7] & PC10;
  nes2 = (hdr[7] & NES2) >> 2 == 2;

  mapper = (hdr[7] & MAPMSB) | ((hdr[6] & MAPLSB) >> 4);

  prgRamSize = hdr[8];
  tvSystem = hdr[9] & 0x01;

  // NOTE(oren): ignore bytes 10 - 15 at this time
}

ostream &operator<<(ostream &os, Cartridge const &in) {
  std::stringstream ss;
  ss << std::hex;
  ss << B_RED(string("Cartridge description:")) << endl;
  ss << "\t" << B_CYAN(string("PRG ROM size")) << ": 0x" << +in.prg_rom_.size()
     << "B" << endl;
  ss << "\t" B_CYAN(string("CHR ROM size")) << ": 0x" << +in.chr_rom_.size()
     << "B" << endl;
  ss << "\t" << B_CYAN(string("Mapper: ")) << +in.mapper << endl;
  ss << "\t" << (in.mirroring == 0 ? "Horizontal" : "Vertical") << " mirroring"
     << endl;
  if (in.hasPrgRam) {
    ss << "\tPRG RAM size:"
       << ": 0x" << +in.prgRamSize << "B" << endl;
  }
  if (in.hasTrainer) {
    ss << "\tTrainer at $7000 - $71FF" << endl;
  }
  if (in.ignoreMirror) {
    ss << "\tIgnore mirroring control, whatever that means" << endl;
  }
  if (in.vsSystem) {
    ss << "\tCartridge for a VS Unisystem arcade console" << endl;
  }
  if (in.pc10) {
    ss << "\tPlayChoice-10 ROM is present" << endl;
  }
  if (in.nes2) {
    ss << "\tNES 2.0 cartridge" << endl;
  } else {
    ss << "\tiNES classic format cartridge" << endl;
  }

  os << ss.str();
  return os;
}

} // namespace cart