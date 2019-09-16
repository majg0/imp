#ifndef IMP_MATH
#define IMP_MATH

#include "constants.hpp"

template <typename T>
inline T max(T a, T b) noexcept
{
  return a > b ? a : b;
}

template <typename T>
inline T min(T a, T b) noexcept
{
  return a < b ? a : b;
}

inline const f64 lerp(const f64 a, const f64 b, const f64 t) noexcept
{
  return (1. - t) * a + t * b; // precise lerp
}

inline const f64 cerp(const f64 a, const f64 b, const f64 t) noexcept
{
  return lerp(a, b, .5 - cos(t * PI) * .5);
}

template <typename T>
inline const T clamp(const T a, const T b, const T val) noexcept
{
  // a <= val <= b
  if (a <= b) {
    if (val > b) {
      return b;
    }
    if (val < a) {
      return a;
    }
    return val;
  }

  // b >= val >= a
  if (val < b) {
    return b;
  }
  if (val > a) {
    return a;
  }
  return val;
}

enum class Interpolation {
  None,
  Linear,
  Cosine,
};

inline const f64 interpolate(
  const f64 a,
  const f64 b,
  const f64 t,
  const Interpolation interpolation) noexcept
{
  switch (interpolation) {
  case Interpolation::None:
    return b;
  case Interpolation::Linear:
    return clamp(a, b, lerp(a, b, t));
  case Interpolation::Cosine:
    return clamp(a, b, cerp(a, b, t));
  default:
    throw "unsupported Interpolation";
  }
}

// inline f64 lerp_array(f64 *values, u32 size, f64 t)
// {
//   f64 ixf = t * size;
//   u32 ix = u32(ixf);
//   u32 max_ix = size - 1;
//   return lerp(values[min(ix, max_ix)], values[min(ix + 1, max_ix)], ixf -
//   ix);
// }

#endif
