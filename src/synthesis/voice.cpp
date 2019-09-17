#include "voice.hpp"
#include "synth.hpp"

#include "constants.hpp"
#include "math.hpp"

void Voice::strike(
  const f64 frequency,
  const TimeState& time_state,
  const f64 interpolation_duration,
  const Interpolation interpolation)
{
  this->frequency.set(
    frequency,
    time_state,
    interpolation_duration,
    interpolation);
  state = State::On;
}

void Voice::release(const Synth& synth, const TimeState& time_state)
{
  // Calculate current adsr release volume (if still in attack / decay phase, a
  // sudden jump down to sustain level would cause an audible discontinuity)
  // last_frequency = get_frequency(time_state);
  last_release_time = time_state.get_scaled_time();
  state = State::Releasing;
}

void Voice::proceed_phase(const Synth& synth, const TimeState& time_state)
{
  const f64 time = time_state.get_scaled_time();
  phase += time_state.get_scaled_delta_time() *
    (synth.vibrato.amp * sin(TWOPI * synth.lfo * synth.vibrato.freq) +
     get_frequency(time_state));

  if (phase < .0) {
    ++phase;
  }
  else if (phase >= 1.) {
    --phase;
  }

  if (
    has_state(State::Releasing) &&
    (time - last_release_time) > synth.adsr_params.release_duration) {
    state = State::Off;
  }
}

const bool Voice::has_state(const State target_state) const
{
  return state == target_state;
}

const bool Voice::has_target_frequency(const f64 frequency) const
{
  return this->frequency.get_target_value() == frequency;
}

const f64 Voice::sample(const Synth& synth, const TimeState& time_state) const
{
  const f64 adsr = synth.adsr_params.sample(
    state, last_strike_time, last_release_time, time_state.get_scaled_time());
  return adsr * vol * synth.wavetable.sample(phase);
}

const f64 Voice::get_frequency(const TimeState& time_state) const
{
  return frequency.get(time_state);
}
