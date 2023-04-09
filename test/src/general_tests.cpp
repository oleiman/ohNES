#include "util.hpp"

#include "ppu.hpp"

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

TEST(General, SpriteLayout) {
  //
  using Sprite = vid::PPU::Sprite;
  Sprite sprite;

  constexpr uint64_t DESCENDING = 0x0807060504030201u;

  EXPECT_EQ(sizeof(sprite), sizeof(uint64_t));

  sprite.v = DESCENDING;

  EXPECT_EQ(sprite.s.xpos, 0x01u);
  EXPECT_EQ(sprite.s.tile_lo, 0x02u);
  EXPECT_EQ(sprite.s.tile_hi, 0x03u);
  EXPECT_EQ(sprite.s.attrs.v, 0x04u);
  EXPECT_EQ(sprite.s.idx, 0x05u);
  EXPECT_EQ(sprite.s._, 0x06u);
  EXPECT_EQ(sprite.s.__, 0x0807u);

  sprite.s.attrs.v = 0b10100110u;
  EXPECT_EQ(sprite.s.attrs.s.palette_i, 0b10u);
  EXPECT_EQ(sprite.s.attrs.s._, 0b001u);
  EXPECT_EQ(sprite.s.attrs.s.priority, 0b1u);
  EXPECT_EQ(sprite.s.attrs.s.h_flip, 0b0u);
  EXPECT_EQ(sprite.s.attrs.s.v_flip, 0b1u);
}
