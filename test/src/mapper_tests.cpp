#include "test_util.hpp"

// TODO(oren): these never show the "0x80 in $6000" behavior to indicate that
// the test is started. No clue how to detect start/stop yet, so they just hang
// with the usual BLARGG_TEST harness.
TEST(MapperTest, MMC3) {
  BLARGG_TEST("rom/1-clocking.nes");
  BLARGG_TEST("rom/2-details.nes");
  BLARGG_TEST("rom/3-A12_clocking.nes");

  // off by one somehow
  // BLARGG_TEST("rom/4-scanline_timing.nes");

  BLARGG_TEST("rom/5-MMC3.nes");

  // Alternate IRQ behavior around reloads. Don't know how to differentiate at
  // cart load time, so leaving this as is.
  /* BLARGG_TEST("rom/6-MMC3_alt.nes"); */
}
