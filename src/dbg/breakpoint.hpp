#pragma once

#include "instruction.hpp"

#include <iomanip>
#include <sstream>
#include <string>

namespace sys {

class Breakpoint {
public:
  Breakpoint() = default;
  virtual ~Breakpoint() = default;
  // TODO(oren): this doesn't need to be virtual
  virtual void enable(bool e) = 0;
  virtual bool isEnabled() const = 0;
  virtual bool check(const instr::Instruction &in) const = 0;
  virtual std::string str() const = 0;
};

template <typename Derived> class BpBase : public Breakpoint {
protected:
  BpBase(bool enabled) : enabled_(enabled) {}

public:
  virtual ~BpBase() = default;

  void enable(bool e) override { enabled_ = e; }
  bool isEnabled() const override { return enabled_; }

  bool check(const instr::Instruction &in) const override {
    return enabled_ && static_cast<const Derived &>(*this).check_impl(in);
  }

  virtual std::string str() const override { return ""; }

private:
  bool enabled_;
};

class PcBreakpoint : public BpBase<PcBreakpoint> {
  using Parent = BpBase<PcBreakpoint>;

public:
  PcBreakpoint(bool e, uint16_t pc) : Parent(e), pc_(pc) {}
  bool check_impl(const instr::Instruction &in) const { return in.pc == pc_; }

  std::string str() const override {
    std::stringstream ss;
    ss << "0x" << std::hex << std::setw(4) << pc_;
    return ss.str();
  }

private:
  uint16_t pc_;
};

class OpcodeBreakpoint : public BpBase<OpcodeBreakpoint> {
  using Parent = BpBase<OpcodeBreakpoint>;

public:
  OpcodeBreakpoint(bool e, uint8_t opcode) : Parent(e), opcode_(opcode) {}

  bool check_impl(const instr::Instruction &in) const {
    return in.opcode == opcode_;
  }

private:
  uint8_t opcode_;
};

} // namespace sys
