#include "math.hpp"
#include "time_state.hpp"

Interpolated::Interpolated()
{
}

Interpolated::Interpolated(const f64 value) : target_value(value)
{
}

void Interpolated::set(
  const f64 target_value,
  const TimeState& time_state,
  const f64 interpolation_duration,
  const Interpolation interpolation) noexcept
{
  prev_value = get(time_state);
  this->target_value = target_value;
  this->start_time = time_state.get_scaled_time();
  this->interpolation_duration = interpolation_duration;
  this->interpolation = interpolation;
}

const f64 Interpolated::get(const TimeState& time_state) const noexcept
{
  return interpolate(
    prev_value,
    target_value,
    interpolation_duration < DBL_EPSILON
      ? 1.
      : (time_state.get_scaled_time() - start_time) / interpolation_duration,
    interpolation);
}

const f64 Interpolated::get_target_value() const noexcept
{
  return target_value;
}
