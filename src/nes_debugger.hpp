#pragma once

#include "cpu.hpp"
#include "debugger.hpp"
#include "ppu_registers.hpp"
#include "util.hpp"

namespace sys {
class NES;

constexpr int DBG_W = 552;
constexpr int DBG_H = 480;

class NESDebugger : public dbg::Debugger {
  using RenderBuffer = std::array<std::array<uint8_t, 3>, DBG_W * DBG_H>;
  using FrameBuffer = std::array<std::array<uint8_t, 3>, DBG_W * DBG_H>;

public:
  explicit NESDebugger(NES &console);
  ~NESDebugger() = default;
  instr::Instruction const &step(const instr::Instruction &in,
                                 const cpu::CpuState &state,
                                 mem::Mapper &mapper) override;
  void render(RenderBuffer &buf);
  void selectNametable(uint8_t s) { nt_select = s; }
  void cyclePalete() { ptable_pidx = (ptable_pidx + 1) & 0b111; }

private:
  void set_pixel(int x, int y, std::array<uint8_t, 3> const &rgb);
  uint16_t calc_nt_base(int x, int y);
  void draw_nametable();
  void draw_sprites();
  void draw_ptables();
  void draw_palettes();
  void draw_scroll_region();

  uint8_t palette_idx() { return (ptable_pidx << 2); }

  uint8_t ppu_read(uint16_t addr);

  static constexpr int I_RINGBUF_SZ = 64;
  // TODO(oren): const?
  NES &console_;
  util::RingBuf<instr::Instruction, I_RINGBUF_SZ> instr_cache_;
  FrameBuffer frameBuffer = {};
  uint8_t nt_select = 0;
  uint8_t ptable_pidx = 0;
  friend std::ostream &operator<<(std::ostream &os, NESDebugger &dbg);
};

} // namespace sys
