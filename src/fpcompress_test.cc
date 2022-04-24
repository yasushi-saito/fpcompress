#include "bit_cast.h"
#include "gtest/gtest.h"
#include "parse.h"
#include "rotating_encoder.h"

namespace fpcompress {
TEST(Rotate, Basic) {
  EXPECT_EQ(RotateLeft<uint32_t>(0x7fffffff, 1), 0xfffffffe);
  EXPECT_EQ(RotateRight<uint32_t>(0x7fffffff, 1), 0xbfffffff);
}

TEST(RotatingEncoder, Basic) {
  RotatingEncoder<float> enc;
  RotatingDecoder<float> dec;

  for (float f = 0.0; f < 100; f += 0.1) {
    ASSERT_EQ(dec.Decode(enc.Encode(f)), f);
  }
}

TEST(Cast, Basic) {
  using Cast = Bitcast<double>;
  double v = 1.0123;
  EXPECT_EQ(v, Cast::ToFloat(Cast::ToInt(v)));
  v = 1234.2368;
  EXPECT_EQ(v, Cast::ToFloat(Cast::ToInt(v)));

  uint8_t buf[8];
  memcpy(buf, &v, 8);
  EXPECT_EQ(v, Cast::ToFloat(buf));
}

TEST(Compose, Basic) {
  const double v = 1.234;
  const auto fp = ParseDouble(v);
  EXPECT_EQ(ComposeDouble(fp), v);
}
}  // namespace fpcompress
