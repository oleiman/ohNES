#include "test_util.hpp"

TEST(CpuTest, Nestest) {
  NES nes("rom/nestest.nes", false, true);
  nes.reset(static_cast<uint16_t>(0xc000));

  while (nes.state().pc != 0xC66E) {
    do {
      nes.step();
    } while (nes.checkFrame());
  }
  EXPECT_EQ(nes.state().cycle, 26554);
  EXPECT_EQ(nes.mapper().read(0x0002), 0x00);
  EXPECT_EQ(nes.mapper().read(0x0003), 0x00);
}

TEST(CpuTest, InstTestV5_Basics) { BLARGG_TEST("rom/01-basics.nes"); }
TEST(CpuTest, InstTestV5_Implied) { BLARGG_TEST("rom/02-implied.nes"); }
TEST(CpuTest, InstTestV5_Immediate) { BLARGG_TEST("rom/03-immediate.nes"); }
TEST(CpuTest, InstTestV5_ZeroPage) {
  BLARGG_TEST("rom/04-zero_page.nes");
  BLARGG_TEST("rom/05-zp_xy.nes");
}
TEST(CpuTest, InstTestV5_Absolute) {
  BLARGG_TEST("rom/06-absolute.nes");
  BLARGG_TEST("rom/07-abs_xy.nes");
}
TEST(CpuTest, InstTestV5_IdxX) { BLARGG_TEST("rom/08-ind_x.nes"); }
TEST(CpuTest, InstTestV5_IdxY) { BLARGG_TEST("rom/09-ind_y.nes"); }
TEST(CpuTest, InstTestV5_Branches) { BLARGG_TEST("rom/10-branches.nes"); }
TEST(CpuTest, InstTestV5_Stack) { BLARGG_TEST("rom/11-stack.nes"); }
TEST(CpuTest, InstTestV5_JmpJsr) { BLARGG_TEST("rom/12-jmp_jsr.nes"); }
TEST(CpuTest, InstTestV5_Rts) { BLARGG_TEST("rom/13-rts.nes"); }
TEST(CpuTest, InstTestV5_Rti) { BLARGG_TEST("rom/14-rti.nes"); }
TEST(CpuTest, InstTestV5_Brk) { BLARGG_TEST("rom/15-brk.nes"); }
TEST(CpuTest, InstTestV5_Special) { BLARGG_TEST("rom/16-special.nes"); }

TEST(CpuTest, InstrTiming) {
  BLARGG_TEST_MEM("rom/1-instr_timing.nes", 0x6000, 0x00);
}

TEST(CpuTest, BranchTiming) {
  BLARGG_TEST_MEM("rom/2-branch_timing.nes", 0x6000, 0x00);
}

TEST(CpuTest, DummyWrites) {
  BLARGG_TEST("rom/cpu_dummy_writes_oam.nes");

  // NOTE(oren): requires open bus behavior to work correctly
  // BLARGG_TEST("rom/cpu_dummy_writes_ppumem.nes");
}

TEST(CpuTest, CpuReset) {
  BLARGG_TEST("rom/registers.nes");
  BLARGG_TEST("rom/ram_after_reset.nes");
}
