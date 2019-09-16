#ifndef IMP_MATH
#define IMP_MATH

#include "constants.hpp"

template <typename T>
inline T max(T a, T b)
{
  return a > b ? a : b;
}

template <typename T>
inline T min(T a, T b)
{
  return a < b ? a : b;
}

inline const f32 lerp(const f32 a, const f32 b, const f32 t)
{
  return (1.f - t) * a + t * b; // precise lerp
}

inline const f32 cerp(const f32 a, const f32 b, const f32 t)
{
  return lerp(a, b, .5f - cos(t * PI) * .5f);
}

template <typename T>
inline const T clamp(const T a, const T b, const T val)
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

inline const f32 interpolate(
  const f32 a, const f32 b, const f32 t, const Interpolation interpolation)
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

// inline f32 lerp_array(f32 *values, u32 size, f32 t)
// {
//   f32 ixf = t * size;
//   u32 ix = (u32)ixf;
//   u32 max_ix = size - 1;
//   return lerp(values[min(ix, max_ix)], values[min(ix + 1, max_ix)], ixf -
//   ix);
// }

#endif
