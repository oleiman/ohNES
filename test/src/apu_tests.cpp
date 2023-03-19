#include "util.hpp"

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

TEST(ApuTest, DmcFailing) {
  BLARGG_TEST_MEM("rom/7-dmc_basics.nes", 0x6000, 0x13);
  BLARGG_TEST_MEM("rom/8-dmc_rates.nes", 0x6000, 0x2);
}
