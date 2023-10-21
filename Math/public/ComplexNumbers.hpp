#pragma once

#include "MathCommon.hpp"

namespace tp {

  struct ComplexCart {
    typedef alnf Number;
    Number r = 0;
    Number i = 0;

  public:
    [[nodiscard]] ComplexCart operator*(const ComplexCart& in) const { return {in.r * r - in.i * i, in.r * i + in.i * r}; }

    [[nodiscard]] ComplexCart operator*(Number in) const { return {in * r, in * i}; }

    [[nodiscard]] ComplexCart operator+(const ComplexCart& in) const { return {in.r + r, in.i + i}; }

    [[nodiscard]] ComplexCart operator-(const ComplexCart& in) const { return {in.r - r, in.i - i}; }

    [[nodiscard]] Number mod() const { return tp::sqrt(i * i + r * r); }

    [[nodiscard]] Number arg() const { return atan2(r, i); }

    [[nodiscard]] Number norm2() const { return this->operator*(conjugate()).r; }

    [[nodiscard]] ComplexCart reciprocal() const { return {r / norm2(), -i / norm2()}; }

    void set(Number mod, Number arg) {
      r = mod * cos(arg);
      i = mod * sin(arg);
    }

    bool operator==(const ComplexCart& in) const { return in.r == r && in.i == i; }

    [[nodiscard]] ComplexCart conjugate() const { return {r, -i}; }
  };

  struct ComplexRad {
    typedef alnf Number;
    Number r = 0;
    Number a = 0;

  public:
    [[nodiscard]] ComplexRad operator*(const ComplexRad& in) const { return {r * in.r, a + in.a}; }

    [[nodiscard]] ComplexRad operator*(Number in) const { return {r * in, a}; }

    [[nodiscard]] ComplexCart cart() const {
      ComplexCart out;
      out.set(r, a);
      return out;
    }

    bool operator==(const ComplexRad& in) const { return in.r == r && in.a == a; }
  };
}
