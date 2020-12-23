#pragma once

#include <array>
#include <cstring>
#include <cmath>

struct FPComponents {
  uint8_t sign;
  uint16_t exp;
  uint64_t frac;
};


inline uint64_t LShift(uint64_t val, int ls) {
  return val<<ls;
}

inline uint64_t RShift(uint64_t val, int ls) {
  return val>>ls;
}

inline FPComponents ParseDouble(double dv) {
  uint8_t buf[8];
  memcpy(buf, &dv, sizeof(dv));
  uint64_t ui;
  memcpy(&ui, buf, sizeof(ui));

  const uint8_t sign = (ui >> 63) != 0 ? 1 : 0;
  const uint16_t exp = (ui >> 52) & 0x7ff;
  const uint64_t frac = ui & 0xfffffffffffffULL;
#if 0
  double dfrac = static_cast<double>(LShift(1,52) | frac) * pow(2, int(exp)-1075);
  std::cout << "v="<<dv<< " sign=" << int(sign) << ",exp="<<exp<< ",frac="
            << std::hex<<frac<<std::dec << ",dfrac="<<dfrac<<"\n";
#endif
  return FPComponents { .sign = sign, .exp = exp, .frac = frac };
}

inline double ComposeDouble(const FPComponents&c) {
  const uint64_t ui = LShift(c.sign, 63) | LShift(c.exp, 52) | c.frac;
  uint8_t buf[8];
  memcpy(buf, &ui, sizeof(ui));
  double dv;
  memcpy(&dv, buf, sizeof(ui));
  return dv;
}
