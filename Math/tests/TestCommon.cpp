
#include "ComplexNumbers.hpp"
#include "Testing.hpp"

using namespace tp;

TEST_DEF(ComplexNumber) {
  TEST((ComplexCart {3, 4}.mod() == 5));
  TEST((ComplexCart {1, 1}.arg() == 0.7853981633974483));
  TEST((ComplexCart {3, 4}.norm2() == 25));

  {
    ComplexCart cn {3, 4};
    ComplexCart expected {0.12, -0.16};
    auto val = cn.reciprocal();
    TEST((val == expected));
  }

  {
    ComplexCart cn {3, 4};
    ComplexCart expected {3, -4};
    TEST((cn.conjugate() == expected));
  }

  {
    ComplexCart cn1 {3, 4};
    ComplexCart cn2 {1, 2};
    ComplexCart expected {-5, 10};
    TEST((cn1 * cn2 == expected));
  }

  {
    ComplexRad c1 {5, 7};
    ComplexRad c2 {10, 3};
    ComplexCart tmp1 = (c1 * c2).cart();
    ComplexCart tmp2 = (c1.cart() * c2.cart());
    TEST((tmp1 - tmp2).mod() < 0.001);
  }
}

TEST_DEF(Math) { testComplexNumber(); }
