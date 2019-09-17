#ifndef IMP_VOICE
#define IMP_VOICE

#include "constants.hpp"
#include "math.hpp"
#include "time_state.hpp"

struct Synth;

class Voice {
public:
  enum class State { Off, On, Releasing }; // TODO (feat): remove Releasing?

  void strike(
    const f64 frequency,
    const TimeState& time_state,
    const f64 interpolation_duration,
    const Interpolation interpolation);

  void release(const Synth& synth, const TimeState& time_state);

  void proceed_phase(const Synth& synth, const TimeState& time_state);

  const bool has_state(const State target_state) const;

  const bool has_target_frequency(const f64 frequency) const;

  const f64 sample(const Synth& synth, const TimeState& time_state) const;

private:
  const f64 get_frequency(const TimeState& time_state) const;

  State state = State::Off;
  f64 last_strike_time = .0;  // TODO (feat): seconds type
  f64 last_release_time = .0; // TODO (feat): seconds type
  Interpolated frequency = .0;
  f64 phase = .0;
  f64 vol = .25;
};

#endif
