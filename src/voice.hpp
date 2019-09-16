#ifndef IMP_VOICE
#define IMP_VOICE

#include "constants.hpp"
#include "math.hpp"

struct Synth;

class Voice {
public:
  enum class State { Off, On, Releasing }; // TODO (feat): remove Releasing?

  void strike(
    const f64 frequency,
    const f64 current_time,
    const f64 interpolation_duration,
    const Interpolation interpolation);
  void release(const Synth& synth, const f64 current_time);
  void proceed_phase(const Synth& synth, const f64 current_time, const f64 dt);

  const bool has_state(const State target_state) const;
  const bool has_target_frequency(const f64 frequency) const;

  const f64 sample(const Synth& synth, const f64 song_time) const;

private:
  const f64 get_frequency(const f64 song_time) const;

  State state = State::Off;
  f64 last_strike_time = 0;                 // TODO (feat): seconds type
  f64 last_release_time = 0;                // TODO (feat): seconds type
  f64 frequency_interpolation_duration = 0; // TODO (feat): seconds type
  Interpolation frequency_interpolation = Interpolation::None;
  f64 last_frequency = 0;   // TODO (feat): frequency type
  f64 target_frequency = 0; // TODO (feat): frequency type
  // InterpolatedValue<f64> frequency;
  f64 phase = 0;
  f64 vol = .25;
};

#endif
