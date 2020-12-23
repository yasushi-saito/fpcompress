#include "gtest/gtest.h"

#include "type.h"
#include "parse.h"

TEST(Cast, Basic) {
  using Cast = Bitcast<double>;
  double v =1.0123;
  EXPECT_EQ(v, Cast::ToFloat(Cast::ToInt(v)));
  v =1234.2368;
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
