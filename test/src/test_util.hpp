#pragma once

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "system.hpp"

#include <gtest/gtest.h>

// TODO(oren): probably have some "stock" includes, using stmts, etc in a util
// header somewhere.
using namespace sys;

bool is_test_complete(NES &nes, uint16_t result_base) {
  static uint8_t gold[3] = {0xDE, 0xB0, 0x61};
  uint8_t status = nes.mapper().read(result_base, true);
  uint8_t mark[3] = {
      nes.mapper().read(result_base + 1, true),
      nes.mapper().read(result_base + 2, true),
      nes.mapper().read(result_base + 3, true),
  };
  return std::strncmp((char *)mark, (char *)gold, 3) == 0 && status < 0x80;
}

void check_reset(NES &nes, uint16_t result_base, bool &requested,
                 std::chrono::steady_clock::time_point &tp) {
  uint8_t status = nes.mapper().read(result_base, true);
  if (status == 0x81 && !requested) {
    requested = true;
    tp = std::chrono::steady_clock::now();
  } else if (requested && std::chrono::steady_clock::now() - tp >
                              std::chrono::milliseconds(200)) {
    nes.reset(true);
    requested = false;
  }
}

#define ROM_TEST(romfile, brk_pc, reg, val)                                    \
  {                                                                            \
    NES nes(romfile, false, true);                                             \
    while (nes.state().pc != brk_pc) {                                         \
      do {                                                                     \
        nes.step();                                                            \
      } while (nes.checkFrame());                                              \
    }                                                                          \
    EXPECT_EQ(nes.state().reg, val);                                           \
  }

#define BLARGG_TEST_MEM(romfile, result_addr, val)                             \
  {                                                                            \
    NES nes(romfile, false, true);                                             \
    uint8_t status = 0x00;                                                     \
    bool reset_requested = false;                                              \
    std::chrono::steady_clock::time_point req_time;                            \
    do {                                                                       \
      do {                                                                     \
        nes.step();                                                            \
      } while (!nes.checkFrame());                                             \
      status = nes.mapper().read(result_addr, true);                           \
      check_reset(nes, result_addr, reset_requested, req_time);                \
    } while (!is_test_complete(nes, result_addr));                             \
    EXPECT_EQ(nes.mapper().read(result_addr, true), val);                      \
  }

#define BLARGG_TEST(romfile) BLARGG_TEST_MEM(romfile, 0x6000, 0x00)
