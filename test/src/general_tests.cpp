#include "util.hpp"

#include <gtest/gtest.h>

constexpr size_t RB_CAP = 32;
constexpr size_t EXTRA = 45;

using IntRingBuf = util::RingBuf<size_t, RB_CAP>;

TEST(General, RingBuf) {
  IntRingBuf rb;

  for (size_t i = 0; i < RB_CAP; ++i) {
    EXPECT_EQ(rb.size(), i);
    rb.insert(i);
    EXPECT_EQ(rb.back(), i);
  }

  EXPECT_EQ(rb.size(), RB_CAP);

  for (size_t i = 0; i < RB_CAP; ++i) {
    EXPECT_EQ(rb[i], i);
  }

  for (size_t i = RB_CAP; i < RB_CAP + EXTRA; ++i) {
    EXPECT_EQ(rb.size(), RB_CAP);
    rb.insert(i);
    EXPECT_EQ(rb.back(), i);
  }

  for (size_t i = 0; i < rb.size(); ++i) {
    EXPECT_EQ(rb[i], i + EXTRA);
  }
}
