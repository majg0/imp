#ifndef IMP_ADSR
#define IMP_ADSR

#include "constants.hpp"
#include "math.hpp"
#include "voice.hpp"

struct AdsrParams {
  // TODO (feat): seconds type
  f32 attack_duration = 0;
  f32 decay_duration = 0;
  f32 release_duration = 0;

  // TODO (feat): amplitude type?
  f32 attack_amplitude = 0;
  f32 sustain_amplitude = 0;

  Interpolation attack_interpolation = Cosine;
  Interpolation decay_interpolation = Cosine;
  Interpolation release_interpolation = Cosine;

  const f32 sample(
    const Voice::State state,
    const f32 last_strike_time,
    const f32 last_release_time,
    const f32 song_time) const
  {

    if (state == Voice::Off) {
      return 0.f;
    }

    if (state == Voice::On) {
      f32 duration = song_time - last_strike_time;
      if (duration < attack_duration) {
        return interpolate(
          0.f,
          attack_amplitude,
          max(0.f, duration / attack_duration),
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

    if (state == Voice::Releasing) {
      f32 duration = song_time - last_release_time;
      if (duration < release_duration) {
        f32 adsr_release_amount = sample(
          Voice::On, last_strike_time, last_release_time, last_release_time);
        return interpolate(
          adsr_release_amount,
          0.f,
          duration / release_duration,
          release_interpolation);
      }
      return 0.f;
    }

    // TODO (fix): is there an STL exception ?
    throw "invariant broken: assumed state to be handled";
  }
};

#endif
