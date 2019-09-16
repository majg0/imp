#ifndef IMP_VOICE
#define IMP_VOICE

#include "constants.hpp"
#include "math.hpp"

struct Synth;

// TODO ?
// template<typename T>
// struct InterpolatedValue {
//   T last_value;
//   T target_value;
//   Interpolation interpolation;
//   f32 interpolation_duration; // TODO (feat): seconds type or STL type?
//   f32 start_time;
//   f32 prev_start_time;
// };

class Voice {
public:
  enum class State { Off, On, Releasing }; // TODO (feat): remove Releasing?

  f32 phase = 0;
  f32 vol = 0;

  void strike(
    const f32 frequency,
    const f32 current_time,
    const f32 interpolation_duration,
    const Interpolation interpolation);
  void release(const Synth& synth, const f32 current_time);
  void proceed_phase(const Synth& synth, const f32 current_time, const f32 dt);

  const bool has_state(const State target_state) const;
  const bool has_target_frequency(const f32 frequency) const;

  const f32 sample(const Synth& synth, const f32 song_time) const;

private:
  const f32 get_frequency(const f32 song_time) const;

  State state = State::Off;
  f32 last_strike_time = 0;                 // TODO (feat): seconds type
  f32 last_release_time = 0;                // TODO (feat): seconds type
  f32 frequency_interpolation_duration = 0; // TODO (feat): seconds type
  Interpolation frequency_interpolation = Interpolation::None;
  f32 last_frequency = 0;   // TODO (feat): frequency type
  f32 target_frequency = 0; // TODO (feat): frequency type
  // InterpolatedValue<f32> frequency;
};

#endif
