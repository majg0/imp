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

inline f32 lerp(f32 a, f32 b, f32 t)
{
  return (1.f - t) * a + t * b; // precise lerp
}

inline f32 cerp(f32 a, f32 b, f32 t)
{
  return lerp(a, b, .5f - cos(t * PI) * .5f);
}

inline f32 lerp_array(f32 *values, u32 size, f32 t)
{
  f32 ixf = t * size;
  u32 ix = (u32)ixf;
  u32 max_ix = size - 1;
  return lerp(values[min(ix, max_ix)], values[min(ix + 1, max_ix)], ixf - ix);
}

inline f32 lerp_array_circular(f32 *values, u32 size, f32 t)
{
  while (t >= 1.f) {
    t -= 1.f;
  }
  f32 ixf = t * size;
  u32 ix = ((u32)ixf) % size;
  return lerp(values[ix], values[(ix + 1) % size], ixf - ix);
}

#endif
