#ifndef IMP_TIME_STATE
#define IMP_TIME_STATE

#include "constants.hpp"

constexpr f64 SAMPLE_FREQUENCY = 44100.;
constexpr f64 SAMPLE_DURATION = 1. / SAMPLE_FREQUENCY;

// TODO (style): remove get_ naming
class TimeState {
public:
  const f64 get_absolute_time() const { return absolute_time; }
  const f64 get_scaled_time() const { return scaled_time; }
  const f64 get_scaled_delta_time() const { return scaled_delta_time; }
  const f64 get_time_scale() const { return time_scale; }

  // TODO (feat): interpolatable
  void set_time_scale(const f64 new_time_scale)
  {
    time_scale = new_time_scale;
    scaled_delta_time = time_scale * SAMPLE_DURATION;
  }

  void tick()
  {
    absolute_time += SAMPLE_DURATION;
    scaled_time += scaled_delta_time;
  }

private:
  f64 absolute_time = .0;
  f64 scaled_time = .0;
  f64 scaled_delta_time = SAMPLE_DURATION;
  f64 time_scale = 1.; // TODO (feat): interpolatable
};

#endif
