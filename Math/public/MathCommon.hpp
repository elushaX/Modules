#pragma once

#include "Utils.hpp"

#define EPSILON 0.0000001

#define PI double(3.1415926535897932384626433832795)
#define PI2 (PI * 2)
#define PI13 (PI / 3)
#define PI23 (PI2 / 3)
#define PI43 (2 * PI2 / 3)

#define SQRT2 1.4142135623730950488016887242
#define EXP 2.7182818284590452353602874714

#include <cmath>

#define RAD(val) (val * (PI / 180.f))
#define DEG(val) (val * (180.f / PI))