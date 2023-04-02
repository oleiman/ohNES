#include "test_util.hpp"

TEST(PpuTest, VblNmi) {
  BLARGG_TEST("rom/01-vbl_basics.nes");
  BLARGG_TEST("rom/02-vbl_set_time.nes");
  BLARGG_TEST("rom/03-vbl_clear_time.nes");
  BLARGG_TEST("rom/04-nmi_control.nes");
  BLARGG_TEST("rom/06-suppression.nes");
}

TEST(PpuTest, NmiTiming) { BLARGG_TEST("rom/05-nmi_timing.nes"); }

TEST(PpuTest, NmiOnOffTiming) {
  BLARGG_TEST("rom/07-nmi_on_timing.nes");
  BLARGG_TEST("rom/08-nmi_off_timing.nes");
}

TEST(PpuTest, VblNmiEvenOdd) {
  BLARGG_TEST("rom/09-even_odd_frames.nes");
  BLARGG_TEST("rom/10-even_odd_timing.nes");
}

TEST(PpuTest, PpuOpenBus) { BLARGG_TEST("rom/ppu_open_bus.nes"); }

TEST(PpuTest, PpuReadBuffer) { BLARGG_TEST("rom/test_ppu_read_buffer.nes"); }

TEST(PpuTest, Oam) {
  BLARGG_TEST("rom/oam_read.nes");
  BLARGG_TEST("rom/oam_stress.nes");
}
