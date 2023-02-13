#pragma once

#include "cpu.hpp"
#include "dbg/breakpoint.hpp"
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
  static constexpr int I_RINGBUF_SZ = 64;

public:
  enum class Mode {
    RUN,
    PAUSE,
    STEP,
    BREAK,
  };

  using InstructionCache = util::RingBuf<instr::Instruction, I_RINGBUF_SZ>;

  explicit NESDebugger(NES &console);
  ~NESDebugger() = default;
  instr::Instruction const &step(const instr::Instruction &in,
                                 const cpu::CpuState &state,
                                 mem::Mapper &mapper) override;
  void render(RenderBuffer &buf);
  void selectNametable(uint8_t s) { nt_select = s; }
  void cyclePalete() { ptable_pidx = (ptable_pidx + 1) & 0b111; }

  void pause(bool p) {
    if (p) {
      setMode(Mode::PAUSE);
    } else {
      if (mode() == Mode::BREAK) {
        resume_ = true;
      }
      setMode(Mode::RUN);
    }
  }

  bool paused() const { return mode() == Mode::PAUSE || mode() == Mode::BREAK; }

  void stepOne(int i = 1) {
    if (mode() == Mode::BREAK) {
      resume_ = true;
    }
    setMode(Mode::STEP);
  }

  Mode mode() const { return dbg_mode; }

  const InstructionCache &cache() const { return instr_cache_; }

  instr::Instruction decode(AddressT pc);
  std::string InstrToStr(const instr::Instruction &instr);

  void setLogging(bool l) {
    if (l && !logging_) {
      log_stream_.open("ohNES.log");
      assert(log_stream_);
    }
    logging_ = l;
  }

  template <typename T, typename... Args> void setBreakpoint(Args... args) {
    if (breakpoints_.size() >= 32) {
      std::cerr << "Exceeded 32 Breakpoint max..." << std::endl;
      return;
    }
    breakpoints_.push_back(std::make_unique<T>(args...));
  }

  void disableBreakpoint(int i) {
    if (0 <= i && i < breakpoints_.size()) {
      breakpoints_[i]->enable(false);
    }
  }

  const std::vector<std::unique_ptr<Breakpoint>> &breakpoints() const {
    return breakpoints_;
  }

private:
  void setMode(Mode s) { dbg_mode = s; }
  void set_pixel(int x, int y, std::array<uint8_t, 3> const &rgb);
  uint16_t calc_nt_base(int x, int y);
  void draw_nametable();
  void draw_sprites();
  void draw_ptables();
  void draw_palettes();
  void draw_scroll_region();

  uint8_t palette_idx() { return (ptable_pidx << 2); }

  uint8_t ppu_read(uint16_t addr);
  uint8_t cpu_read(uint16_t addr);

  NES &console_;
  InstructionCache instr_cache_;
  AddressT curr_pc_;
  FrameBuffer frameBuffer = {};
  uint8_t nt_select = 0;
  uint8_t ptable_pidx = 0;
  std::atomic<Mode> dbg_mode{Mode::RUN};
  bool logging_ = false;
  std::ofstream log_stream_;

  std::vector<std::unique_ptr<Breakpoint>> breakpoints_;
  bool resume_ = false;

  friend std::ostream &operator<<(std::ostream &os, NESDebugger &dbg);
};

} // namespace sys
