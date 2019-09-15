#include "voice.hpp"
#include "constants.hpp"
#include "math.hpp"
#include "synth.hpp"

void Voice::strike(
  const f32 frequency,
  const f32 current_time,
  const f32 interpolation_duration,
  const Interpolation interpolation)
{
  target_frequency = frequency;
  last_strike_time = current_time;
  frequency_interpolation_duration = interpolation_duration;
  frequency_interpolation = interpolation;
  state = On;
}

void Voice::release(const Synth& synth, const f32 current_time)
{
  // Calculate current adsr release volume (if still in attack / decay phase, a
  // sudden jump down to sustain level would cause an audible discontinuity)
  last_frequency = get_frequency(current_time);
  last_release_time = current_time;
  state = Releasing;
}

void Voice::proceed_phase(
  const Synth& synth, const f32 current_time, const f32 dt)
{
  phase += dt *
    (synth.vibrato.amp * sin(TWOPIF * synth.lfo * synth.vibrato.freq) +
     get_frequency(current_time));

  if (phase < 0.f) {
    ++phase;
  }
  else if (phase >= 1.f) {
    --phase;
  }

  if (
    has_state(Voice::Releasing) &&
    (current_time - last_release_time) > synth.adsr_params.release_duration) {
    state = Voice::Off;
  }
}

const f32 Voice::get_frequency(const f32 song_time) const
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
const bool Voice::has_target_frequency(const f32 frequency) const
{
  return frequency == target_frequency;
}

const f32 Voice::sample(const Synth& synth, const f32 song_time) const
{
  const f32 adsr = synth.adsr_params.sample(
    state, last_strike_time, last_release_time, song_time);
  return adsr * vol * synth.wavetable.sample(phase);
}
