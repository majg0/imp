#ifndef IMP_SYNTH
#define IMP_SYNTH

#include "adsr_params.hpp"
#include "voice.hpp"
#include "wavetable.hpp"

// TODO
struct imp_vibrato {
  f64 amp;  // TODO (feat): type to ensure in hz
  f64 freq; // TODO (feat): type to ensure in samples
};

struct Synth {
  static constexpr size_t NUM_VOICES = 32;
  f64 lfo;
  HarmonicsWavetable wavetable;
  Voice voices[NUM_VOICES];
  AdsrParams adsr_params;
  imp_vibrato vibrato;
};

#endif
