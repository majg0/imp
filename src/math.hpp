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

class TimeState;

class Interpolated {
public:
  Interpolated();
  Interpolated(const f64 value);

  // NOTE: takes a Timing in order to uphold the invariant that interpolation
  // must start at call time.
  void set(
    const f64 target_value,
    const TimeState& timing,
    const f64 interpolation_duration,
    const Interpolation interpolation) noexcept;

  // NOTE: this could be unconstrained to take `const f64 time` instead, but we
  // are delaying that decision until confirmed necessary
  const f64 get(const TimeState& timing) const noexcept;
  const f64 get_target_value() const noexcept;

private:
  f64 target_value = .0;
  f64 prev_value = .0;
  // TODO (feat): type safety - measure in seconds
  f64 start_time = .0;
  // TODO (feat): type safety - measure in seconds
  f64 interpolation_duration = .0;
  Interpolation interpolation = Interpolation::None;
};

#endif
