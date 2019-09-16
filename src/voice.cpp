#include "voice.hpp"
#include "constants.hpp"
#include "math.hpp"
#include "synth.hpp"

void Voice::strike(
  const f64 frequency,
  const f64 current_time,
  const f64 interpolation_duration,
  const Interpolation interpolation)
{
  target_frequency = frequency;
  last_strike_time = current_time;
  frequency_interpolation_duration = interpolation_duration;
  frequency_interpolation = interpolation;
  state = State::On;
}

void Voice::release(const Synth& synth, const f64 current_time)
{
  // Calculate current adsr release volume (if still in attack / decay phase, a
  // sudden jump down to sustain level would cause an audible discontinuity)
  last_frequency = get_frequency(current_time);
  last_release_time = current_time;
  state = State::Releasing;
}

void Voice::proceed_phase(
  const Synth& synth, const f64 current_time, const f64 dt)
{
  phase += dt *
    (synth.vibrato.amp * sin(TWOPI * synth.lfo * synth.vibrato.freq) +
     get_frequency(current_time));

  if (phase < .0) {
    ++phase;
  }
  else if (phase >= 1.) {
    --phase;
  }

  if (
    has_state(State::Releasing) &&
    (current_time - last_release_time) > synth.adsr_params.release_duration) {
    state = State::Off;
  }
}

const f64 Voice::get_frequency(const f64 song_time) const
{
  return interpolate(
    last_frequency,
    target_frequency,
    (song_time - last_strike_time) / frequency_interpolation_duration,
    frequency_interpolation);
}

const bool Voice::has_state(const State target_state) const
{
  return state == target_state;
}
const bool Voice::has_target_frequency(const f64 frequency) const
{
  return frequency == target_frequency;
}

const f64 Voice::sample(const Synth& synth, const f64 song_time) const
{
  const f64 adsr = synth.adsr_params.sample(
    state, last_strike_time, last_release_time, song_time);
  return adsr * vol * synth.wavetable.sample(phase);
}
