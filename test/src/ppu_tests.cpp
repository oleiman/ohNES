#include "util.hpp"

TEST(PpuTest, VblNmi) {
  BLARGG_TEST_MEM("rom/01-vbl_basics.nes", 0x6000, 0x00);
  BLARGG_TEST_MEM("rom/02-vbl_set_time.nes", 0x6000, 0x00);
  BLARGG_TEST_MEM("rom/03-vbl_clear_time.nes", 0x6000, 0x00);
  BLARGG_TEST_MEM("rom/04-nmi_control.nes", 0x6000, 0x00);
  BLARGG_TEST_MEM("rom/06-suppression.nes", 0x6000, 0x00);
}

TEST(PpuTest, NmiTiming) {
  // Failing test
  BLARGG_TEST_MEM("rom/05-nmi_timing.nes", 0x6000, 0x01);
  BLARGG_TEST_MEM("rom/07-nmi_on_timing.nes", 0x6000, 0x00);
  BLARGG_TEST_MEM("rom/08-nmi_off_timing.nes", 0x6000, 0x00);
}

TEST(PpuTest, VblNmiEvenOdd) {
  BLARGG_TEST_MEM("rom/09-even_odd_frames.nes", 0x6000, 0x00);
  BLARGG_TEST_MEM("rom/10-even_odd_timing.nes", 0x6000, 0x00);
}

TEST(PpuTest, DISABLED_PpuReadBuffer) {
  BLARGG_TEST_MEM("rom/test_ppu_read_buffer.nes", 0x6000, 0x35);
}
