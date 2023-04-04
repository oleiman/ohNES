#include "dbg/nes_debugger.hpp"
#include "instruction.hpp"
#include "system.hpp"
#include "util.hpp"

#include <algorithm>
#include <bitset>
#include <chrono>

using vid::PPU;

namespace sys {

NESDebugger::NESDebugger(NES &console)
    : dbg::Debugger(false, false), console_(console),
      init_time_(std::chrono::system_clock::now()) {}

void NESDebugger::setLogging(bool l) {
  if (l && !logging_) {
    auto logfile = get_romfile();
    logfile += ".log";
    std::cerr << "LOG: " << logfile << std::endl;
    log_stream_.open(logfile);
    assert(log_stream_);
  }
  logging_ = l;
}

void NESDebugger::setRecording(bool r) {
  if (r && !recording_) {
    auto recordfile = get_romfile();
    recordfile += ".rec";
    std::cerr << "RECORD: " << recordfile << std::endl;
    recording_stream_.open(recordfile, std::ios::out | std::ios::binary);
    assert(recording_stream_);
  }
  recording_ = r;
}

std::string NESDebugger::get_romfile() {
  auto &full_path = console_.cart().romfile;
  std::string romfile = full_path;
  auto last_slash = romfile.find_last_of('/');
  if (last_slash != std::string::npos) {
    romfile =
        "." + full_path.substr(full_path.find_last_of('/'), full_path.size());
  }
  return romfile + "_" + std::to_string(init_time_.time_since_epoch().count());
}

const instr::Instruction &NESDebugger::step(const instr::Instruction &in,
                                            const cpu::CpuState &cpu_state,
                                            mem::Mapper &mapper) {
  curr_pc_ = in.pc;

  auto match = std::any_of(
      std::begin(breakpoints_), std::end(breakpoints_),
      [&](const std::unique_ptr<Breakpoint> &bp) { return bp->check(in); });

  if (match && !resume_) {
    std::cout << "BREAK! " << InstrToStr(in) << std::endl;
    in.discard = true;
    setMode(Mode::BREAK);
  } else if (mode() == Mode::STEP) {
    setMode(Mode::PAUSE);
  }

  if (!in.discard) {
    instr_cache_.insert(in);
  }

  if (isLogging()) {
    log_stream_ << std::left << std::setw(40) << InstrToStr(in) << CpuStateStr()
                << " (C: " << in.issueCycle << ")\n";
  }

  resume_ = false;

  return in;
}

void NESDebugger::processInput(uint8_t joy_id, uint8_t btn, uint8_t state) {
  if (isRecording()) {
    // TODO(oren): process through a struct/union
    uint32_t v = (static_cast<uint32_t>(joy_id) << 16) |
                 (static_cast<uint32_t>(btn) << 8) |
                 (static_cast<uint32_t>(state));
    recording_stream_.write(reinterpret_cast<char *>(&v), sizeof(v));
  }
}

std::ostream &operator<<(std::ostream &os, NESDebugger &dbg) {
  os << dbg.instr_cache_;
  return os;
}

void NESDebugger::draw_nametable() {
  for (int y = 0; y < 240; ++y) {
    uint8_t pattern_lo = 0;
    uint8_t pattern_hi = 0;
    std::array<uint8_t, 4> palette = {};
    for (int x = 0; x < 256; ++x) {
      int pixel_x = x;
      int pixel_y = y;

      // for each slice of 8 pixels, fetch
      //   - a nametable entry
      //   - an attribute
      //   - two bytes of pattern data (hi & lo)
      //   - a palette
      // TODO(oren): easier to write tile-wise? like the others...
      if ((pixel_x & 0b111) == 0) {
        // nametable fetch
        uint16_t nt_base = console_.ppu_registers_.BaseNTAddr[nt_select];
        int tile_x = pixel_x >> 3;
        int tile_y = pixel_y >> 3;
        int nt_i = tile_y * 32 + tile_x;
        uint8_t nt = ppu_read(nt_base + nt_i);

        // attribute fetch
        int block_x = tile_x >> 2;
        int block_y = tile_y >> 2;
        uint16_t attr_base = nt_base + 0x3c0;
        uint8_t at_entry = ppu_read(attr_base + (block_y * 8 + block_x));
        uint8_t meta_x = (tile_x & 0x03) >> 1;
        uint8_t meta_y = (tile_y & 0x03) >> 1;
        uint8_t shift = (meta_y * 2 + meta_x) << 1;
        uint8_t val = (at_entry >> shift) & 0b11;
        uint8_t attr = val & 0b11;

        // pattern fetches
        uint8_t y_fine = pixel_y & 0b111;
        auto tile = static_cast<uint16_t>(nt);
        auto tile_base =
            console_.ppu_registers_.backgroundPTableAddr() + tile * 16;
        pattern_lo = ppu_read(tile_base + y_fine);
        pattern_hi = ppu_read(tile_base + y_fine + 8);
        util::reverseByte(pattern_lo);
        util::reverseByte(pattern_hi);

        // palette fetch
        auto pidx = attr << 2;
        palette[0] = palette_read(0x3f00);
        palette[1] = palette_read(0x3f01 + pidx);
        palette[2] = palette_read(0x3f02 + pidx);
        palette[3] = palette_read(0x3f03 + pidx);
      }

      // render pixel
      auto fine_x = pixel_x & 0b111;
      auto lower = pattern_lo >> fine_x;
      auto upper = pattern_hi >> fine_x;
      auto value = ((upper & 0b1) << 1) | (lower & 0b1);
      auto c = palette[value] & 0x3F;
      set_pixel(x, y, PPU::SystemPalette[c]);
    }
  }
}

void NESDebugger::draw_sprites() {

  // evaluate sprites (primary OAM)
  const auto &oam = console_.ppu_oam_;
  uint8_t sprite_size = console_.ppu_registers_.spriteSize();
  uint8_t n_sprites = static_cast<uint8_t>(oam.size() >> 2);
  std::array<uint8_t, 4> palette = {};

  uint16_t vert_offset = 272;

  for (uint8_t i = 0; i < n_sprites; ++i) {
    uint16_t x = (i & 0b1111) * 8 * 2;
    uint16_t y = vert_offset + (i >> 4) * 16 * 2;
    uint8_t sprite_base = i << 2;
    // uint8_t sprite_y = oam[sprite_base];
    uint8_t tile_idx = oam[sprite_base + 1];
    PPU::Sprite sprite; // = {.s.attrs.v = oam[sprite_base + 2]};
    sprite.s.attrs.v = oam[sprite_base + 2];
    uint8_t pidx = sprite.s.attrs.s.palette_i;
    palette[0] = palette_read(0x3f00);
    palette[1] = palette_read(0x3f11 + pidx);
    palette[2] = palette_read(0x3f12 + pidx);
    palette[3] = palette_read(0x3f13 + pidx);
    auto bank = console_.ppu_registers_.spritePTableAddr(tile_idx);
    if (sprite_size == 16) {
      tile_idx &= 0xFE;
    }
    auto tile_base = bank + (tile_idx * 16);
    for (int tile_y = 0; tile_y < sprite_size; ++tile_y) {
      int y_tmp = tile_y;
      uint16_t base_tmp = tile_base;
      if (y_tmp >= 8) {
        y_tmp -= 8;
        base_tmp += 16;
      }
      uint8_t tile_lo = ppu_read(base_tmp + y_tmp);
      uint8_t tile_hi = ppu_read(base_tmp + y_tmp + 8);
      if (sprite.s.attrs.s.h_flip) {
        util::reverseByte(tile_lo);
        util::reverseByte(tile_hi);
      }
      for (int tile_x = 0; tile_x < 8; ++tile_x) {
        uint16_t value = ((tile_hi & 0b1) << 1) | (tile_lo & 0b1);
        tile_lo >>= 1;
        tile_hi >>= 1;
        auto c = palette[value] & 0x3f;
        set_pixel(x + tile_x, y + tile_y, PPU::SystemPalette[c]);
      }
    }
  }
}

void NESDebugger::draw_ptables() {
  //
  std::array<uint8_t, 4> palette = {
      palette_read(0x3f00),
      palette_read(0x3f01 + palette_idx()),
      palette_read(0x3f02 + palette_idx()),
      palette_read(0x3f03 + palette_idx()),
  };
  uint16_t horiz_off = 256 + 8;
  uint16_t vert_off = 0;
  for (uint16_t base = 0x0000; base < 0x2000; base += 0x1000) {
    uint16_t x_off = horiz_off + (base >> 5);
    for (int i = 0; i < 16 * 16; ++i) {
      int x = (i & 0b1111) * 8 + x_off;
      int y = (i >> 4) * 8 + vert_off;
      uint16_t tile_base = base + i * 16;
      for (uint8_t tile_y = 0; tile_y < 8; ++tile_y) {
        uint8_t pattern_lo = ppu_read(tile_base + tile_y);
        uint8_t pattern_hi = ppu_read(tile_base + tile_y + 8);
        util::reverseByte(pattern_lo);
        util::reverseByte(pattern_hi);
        for (uint8_t tile_x = 0; tile_x < 8;
             ++tile_x, pattern_lo >>= 1, pattern_hi >>= 1) {
          auto value = ((pattern_hi & 0b1) << 1) | (pattern_lo & 0b1);
          auto c = palette[value] & 0x3F;
          set_pixel(x + tile_x, y + tile_y, PPU::SystemPalette[c]);
        }
      }
    }
  }
}

void NESDebugger::draw_palettes() {
  uint16_t horiz_off = 256 + 8;
  uint16_t vert_off = 128 + 48;

  for (int i = 0; i < 32; ++i) {
    auto c = palette_read(0x3f00 + i);
    int x = (i & 0b1111) * 16 + horiz_off;
    int y = (i >> 4) * 16 + vert_off;
    for (int cx = 0; cx < 16; ++cx) {
      for (int cy = 0; cy < 16; ++cy) {
        set_pixel(x + cx, y + cy, PPU::SystemPalette[c]);
      }
    }
  }
}

void NESDebugger::render(RenderBuffer &buf) {
  std::fill(&frameBuffer[0][0], &frameBuffer[0][0] + frameBuffer.size() * 3,
            0xCF);

  draw_nametable();
  draw_sprites();
  draw_ptables();
  draw_palettes();

  // TODO(oren): draw a box around the nametable region currently in frame
  // draw_scroll_region();

  std::copy(frameBuffer.begin(), frameBuffer.end(), buf.begin());
}

void NESDebugger::set_pixel(int x, int y, std::array<uint8_t, 3> const &rgb) {
  // add a little space around the edge
  x += 16;
  y += 16;
  size_t pi = y * DBG_W + x;
  if (pi < frameBuffer.size()) {
    std::copy(std::begin(rgb), std::end(rgb), std::begin(frameBuffer[pi]));
  }
}

uint16_t NESDebugger::calc_nt_base(int x, int y) {
  if (y < 240) {
    if (x < 256) {
      return console_.ppu_registers_.BaseNTAddr[0];
    } else {
      return console_.ppu_registers_.BaseNTAddr[1];
    }
  } else {
    if (x < 256) {
      return console_.ppu_registers_.BaseNTAddr[2];
    } else {
      return console_.ppu_registers_.BaseNTAddr[3];
    }
  }
}

uint8_t NESDebugger::ppu_read(uint16_t addr) {
  // debug read (no address bus activity)
  return console_.mapper_->ppu_read(addr, true);
}

uint8_t NESDebugger::cpu_read(uint16_t addr) {
  // debug flag to avoid affecting PPU registers
  return console_.mapper_->read(addr, true);
}

uint8_t NESDebugger::palette_read(uint16_t addr) {
  return console_.mapper_->palette_read(addr);
}

instr::Instruction NESDebugger::history(int idx) const {
  if (std::abs(idx) > instr_cache_.size()) {
    return instr::Instruction(0, 0, 0);
  }
  if (idx < 0) {
    idx = instr_cache_.size() - std::abs(idx);
  }

  return instr_cache_[idx];
}

instr::Instruction NESDebugger::decode(AddressT offset) {
  auto pc = console_.state().pc;
  pc += offset;

  return instr::Instruction(cpu_read(pc), pc, 0);
  // TODO(oren): better representation of the Address mode and stuff
}

uint16_t make_addr(uint8_t lo, uint8_t hi) {
  return (static_cast<uint16_t>(hi) << 8) | lo;
}

// TODO(oren): would be nicer to have this in the Instruction module in the
// CPU, but the memory access logic is all tangled up. When actually decoding
// instructions in the CPU, reads have a cost, so do carries, and there are
// several special cases to penalize address boundary crossing and the like.
// it's convenient then to just construct a nice string representation here.
std::string NESDebugger::InstrToStr(const instr::Instruction &in) {
  using AMode = instr::AddressMode;
  std::stringstream ss;

  if (in.discard) {
    ss << "***BREAK***";
  }

  ss << "0x" << std::hex << std::setfill('0') << std::setw(4) << +in.pc << "\t";
  std::array<int, 3> data = {-1, -1, -1};
  for (size_t i = 0; i < in.size; ++i) {
    data[i] = (cpu_read(in.pc + i));
  }

  ss << std::uppercase;

  for (size_t i = 0; i < data.size(); ++i) {
    ss << " ";
    auto b = data[i];
    if (b >= 0) {
      ss << std::setw(2) << +cpu_read(in.pc + i);
    } else {
      ss << "  ";
    }
  }

  ss << "  " << instr::GetMnemonic(in.operation) << "  ";
  auto am = in.addressMode;

  switch (am) {
  case AMode::implicit:
    break;
  case AMode::accumulator:
    ss << "A";
    break;
  case AMode::immediate:
    ss << "#$" << data[1];
    break;
  case AMode::absolute: {
    auto addr = make_addr(data[1], data[2]);
    ss << "$" << std::setw(4) << addr;
    if (in.operation != instr::Operation::JMP) {
      ss << " = " << std::setw(2) << +cpu_read(addr);
    }
  } break;
  case AMode::absoluteX: {
    auto addr = make_addr(data[1], data[2]);
    ss << "$" << std::setw(4) << addr << ",X @ ";
    addr += console_.state().rX;
    ss << std::setw(4) << addr << " = " << std::setw(2) << +cpu_read(addr);
  } break;
  case AMode::absoluteY: {
    auto addr = make_addr(data[1], data[2]);
    ss << "$" << std::setw(4) << addr << ",Y @ ";
    addr += console_.state().rY;
    ss << std::setw(4) << addr << " = " << std::setw(2) << +cpu_read(addr);
  } break;
  case AMode::zeroPage:
  case AMode::zeroPageX:
  case AMode::zeroPageY: {
    auto addr = static_cast<uint16_t>(data[1]);
    ss << "$" << std::setw(2) << addr;
    if (am == AMode::zeroPageX) {
      addr += console_.state().rX;
      ss << ",X @ " << std::setw(4) << addr;
    } else if (am == AMode::zeroPageY) {
      addr += console_.state().rY;
      ss << ",Y @ " << std::setw(4) << addr;
    }
    if (in.operation != instr::Operation::JMP) {
      ss << " = " << std::setw(2) << +cpu_read(addr);
    }
  } break;
  case AMode::relative: {
    int8_t offset = static_cast<int8_t>(data[1]);
    auto target = in.pc + 2 - (-offset);
    ss << "$" << std::setw(4) << target;
  } break;
  case AMode::indirect: {
    auto addr = make_addr(data[1], data[2]);
    auto target = make_addr(cpu_read(addr), cpu_read(addr + 1));
    ss << "($" << std::setw(4) << addr << ") @ " << std::setw(4) << target;
  } break;
  case AMode::indexedIndirect: {
    // x,indexed
    // zero page address with X offset, without carry (wraparound)
    uint8_t zp_addr = data[1] + console_.state().rX;
    uint16_t target = make_addr(cpu_read(zp_addr), cpu_read(zp_addr + 1));
    ss << "($" << std::setw(2) << +data[1] << ",X) @ " << std::setw(2)
       << +zp_addr << " = " << std::setw(4) << target << " = " << std::setw(2)
       << +cpu_read(target);
  } break;
  case AMode::indirectIndexed: {
    // indexed,y
    uint8_t zp_addr = data[1];
    uint16_t base = make_addr(cpu_read(zp_addr), cpu_read(zp_addr + 1));
    uint16_t target = base + console_.state().rY;
    ss << "($" << std::setw(2) << +zp_addr << "),Y = " << std::setw(4) << base
       << " @ " << std::setw(4) << target << " = " << std::setw(2)
       << +cpu_read(target);
  } break;
  default:
    ss << "(" << instr::GetAModeMnemonic(am) << ")";
    break;
  }

  return ss.str();
}

std::string NESDebugger::CpuStateStr() const {
  std::stringstream ss;
  ss << std::hex << "A: " << std::setw(2) << +console_.state().rA
     << " X: " << std::setw(2) << +console_.state().rX << " Y: " << std::setw(2)
     << +console_.state().rY
     << " S: " << std::bitset<8>(console_.state().status)
     << " PPU: " << std::dec << console_.currScanline() << ", "
     << console_.currPpuCycle();
  return ss.str();
}

void NESDebugger::muteApuChannel(int cid, bool e) {
  console_.apu_.mute(cid, e);
}

} // namespace sys
