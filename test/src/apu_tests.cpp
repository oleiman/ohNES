#include "test_util.hpp"

TEST(ApuTest, LengthCounter) {
  BLARGG_TEST_MEM("rom/1-len_ctr.nes", 0x6000, 0x00);
}

TEST(ApuTest, LengthTable) {
  BLARGG_TEST_MEM("rom/2-len_table.nes", 0x6000, 0x00);
}

TEST(ApuTest, IrqFlag) { BLARGG_TEST_MEM("rom/3-irq_flag.nes", 0x6000, 0x00); }

TEST(ApuTest, Jitter) { BLARGG_TEST_MEM("rom/4-jitter.nes", 0x6000, 0x00); }

TEST(ApuTest, LengthTiming) {
  BLARGG_TEST_MEM("rom/5-len_timing.nes", 0x6000, 0x00);
}

TEST(ApuTest, IrqTiming) {
  BLARGG_TEST_MEM("rom/6-irq_flag_timing.nes", 0x6000, 0x00);
}

TEST(ApuTest, Dmc) {
  BLARGG_TEST("rom/7-dmc_basics.nes");
  BLARGG_TEST("rom/8-dmc_rates.nes");
}

TEST(ApuTest, Reset) {
  BLARGG_TEST("rom/4015_cleared.nes");
  BLARGG_TEST("rom/4017_written.nes");
  BLARGG_TEST("rom/4017_timing.nes");
  BLARGG_TEST("rom/len_ctrs_enabled.nes");
  BLARGG_TEST("rom/irq_flag_cleared.nes");
  BLARGG_TEST("rom/works_immediately.nes");
}
