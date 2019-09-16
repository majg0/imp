#ifndef IMP_ADSR
#define IMP_ADSR

#include "constants.hpp"
#include "math.hpp"
#include "voice.hpp"

struct AdsrParams {
  // TODO (feat): seconds type
  f64 attack_duration = 0;
  f64 decay_duration = 0;
  f64 release_duration = 0;

  // TODO (feat): amplitude type?
  f64 attack_amplitude = 0;
  f64 sustain_amplitude = 0;

  Interpolation attack_interpolation = Interpolation::Cosine;
  Interpolation decay_interpolation = Interpolation::Cosine;
  Interpolation release_interpolation = Interpolation::Cosine;

  const f64 sample(
    const Voice::State state,
    const f64 last_strike_time,
    const f64 last_release_time,
    const f64 song_time) const
  {

    if (state == Voice::State::Off) {
      return .0;
    }

    if (state == Voice::State::On) {
      f64 duration = song_time - last_strike_time;
      if (duration < attack_duration) {
        return interpolate(
          .0,
          attack_amplitude,
          max(.0, duration / attack_duration),
          attack_interpolation);
      }
      if (duration < attack_duration + decay_duration) {
        return interpolate(
          attack_amplitude,
          sustain_amplitude,
          (duration - attack_duration) / decay_duration,
          decay_interpolation);
      }
      return sustain_amplitude;
    }

    if (state == Voice::State::Releasing) {
      f64 duration = song_time - last_release_time;
      if (duration < release_duration) {
        f64 adsr_release_amount = sample(
          Voice::State::On,
          last_strike_time,
          last_release_time,
          last_release_time);
        return interpolate(
          adsr_release_amount,
          .0,
          duration / release_duration,
          release_interpolation);
      }
      return .0;
    }

    // TODO (fix): is there an STL exception ?
    throw "invariant broken: assumed state to be handled";
  }
};

#endif
