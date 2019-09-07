#include "circular_rw_buffer.hpp"
#include "constants.hpp"
#include "wavetable.hpp"

#include <fmod.hpp>
#include <fmod_errors.h>

#ifdef WIN32
#  include <windows.h>
#  define SLEEP(ms) Sleep(ms)
#else
#  include <unistd.h>
#  define SLEEP(ms) usleep(ms * 1000)
#endif

#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

// General Defines ////////////////////////////////////////////////////////////

#define FMODERRCHECK(_result) FMODERRCHECK_fn(_result, __FILE__, __LINE__)
void FMODERRCHECK_fn(FMOD_RESULT result, const char *file, s32 line)
{
  if (result != FMOD_OK) {
    printf("%s(%d): FMOD error %d - %s\n", file, line, result, FMOD_ErrorString(result));
  }
}
#define FMODSOUNDERRCHECK(_result) FMODSOUNDERRCHECK_fn(_result, __FILE__, __LINE__)
void FMODSOUNDERRCHECK_fn(FMOD_RESULT result, const char *file, s32 line)
{
  if (result != FMOD_OK && result != FMOD_ERR_INVALID_HANDLE) {
    printf("%s(%d): FMOD error %d - %s\n", file, line, result, FMOD_ErrorString(result));
  }
}

// The frequencies for all MIDI notes
static f32 imp_note_freqs[] = {
  8.17579891564368f, // C-1 (First MIDI note)
  8.66195721802722f, // Db-1
  9.17702399741896f, // D-1
  9.722718241315f,   // Eb-1
  10.3008611535272f, // E-1
  10.9133822322813f, // F-1
  11.5623257097385f, // Gb-1
  12.2498573744296f, // G-1
  12.9782717993732f, // Ab-1
  13.75f,            // A-1
  14.5676175474403f, // Bb-1
  15.4338531642538f, // B-1
  16.3515978312874f, // C0
  17.3239144360545f, // Db0
  18.6540479948379f, // D0
  19.44543648263f,   // Eb0
  20.6017223070543f, // E0
  21.8567644645627f, // F0
  23.1246514194771f, // Gb0
  24.4997147488593f, // G0
  25.9565435987465f, // Ab0
  27.5f,             // A0 (Piano low A)
  29.1352350948806f, // Bb0
  30.8677063285077f, // B0 (5-string bass low open B)
  32.7031956625748f, // C1
  34.6478288721089f, // Db1
  36.7080959896759f, // D1
  38.89087296526f,   // Eb1
  41.2034446141087f, // E1 (4-string bass low open E)
  43.6535289291254f, // F1
  46.2493028389542f, // Gb1
  48.9994294977186f, // G1
  51.913087197493f,  // Ab1
  55.f,              // A1
  58.2704701897611f, // B1
  61.7354126570154f, // B1
  65.4063913251495f, // C2
  69.2956577442179f, // Db2
  73.4161919793518f, // D2
  77.7817459305201f, // Eb2
  82.4068892282174f, // E2 (Guitar low open E)
  87.3070578582508f, // F2
  92.4986056779085f, // Gb2
  97.9988589954372f, // G2 (4 or 5 string bass, high open G)
  103.826174394986f, // Ab2
  110.f,             // A2 (Guitar 5th string open)
  116.540940379522f, // Bb2
  123.470825314031f, // B2
  130.812782650299f, // C3 (6-string bass, high open C)
  138.591315488436f, // Db3
  146.832383958704f, // D3 (Guitar 4th string open)
  155.56349186104f,  // Eb3
  164.813778456435f, // E3
  174.614115716502f, // F3
  184.997211355817f, // Gb3
  195.997717990874f, // G3 (Guitar 3rd string open) (Violin low open G)
  207.652348789972f, // Ab3
  220.f,             // A3
  233.081880759045f, // Bb3
  246.941650628062f, // B3 (Guitar 3rd string open)
  261.625565300599f, // C4 (Piano middle C)
  277.182630976872f, // Db4
  293.664767917407f, // D4 (Violin 3rd string open)
  311.126983722081f, // Eb4
  329.62755691287f,  // E4 (Guitar 1st string open)
  349.228231433004f, // F4
  369.994422711634f, // Gb4
  391.995435981749f, // G4
  415.304697579945f, // Ab4
  440.f,             // A4 (Tuning fork A) (Violin 2nd string open)
  466.16376151809f,  // Bb4
  493.883301256124f, // B4
  523.251130601197f, // C5
  554.365261953744f, // Db5
  587.329535834815f, // D5
  622.253967444162f, // Eb5
  659.25511382574f,  // E5 (Guitar 1st string 12 fret) (Violin 1st string open)
  698.456462866008f, // F5
  739.988845423269f, // Gb5
  783.990871963499f, // G5
  830.609395159891f, // Ab5
  880.f,             // A5
  932.32752303618f,  // Bb5
  987.766602512249f, // B5
  1046.50226120239f, // C6
  1108.73052390749f, // Db6
  1174.65907166963f, // D6
  1244.50793488832f, // Eb6
  1318.51022765148f, // E6 (Guitar 1st string 24 fret)
  1396.91292573202f, // F6
  1479.97769084654f, // Gb6
  1567.981743927f,   // G6
  1661.21879031978f, // Ab6
  1760.f,            // A6
  1864.65504607236f, // Bb6
  1975.5332050245f,  // B6
  2093.00452240479f, // C7
  2217.46104781498f, // Db7
  2349.31814333926f, // D7
  2489.01586977665f, // Eb7
  2637.02045530296f, // E7
  2793.82585146403f, // F7
  2959.95538169308f, // Gb7
  3135.963487854f,   // G7
  3322.43758063956f, // Ab7
  3520.f,            // A7
  3729.31009214472f, // Bb7
  3951.066410049f,   // B7
  4186.00904480958f, // C8 (Piano upper C)
  4434.92209562996f, // Db8
  4698.63628667853f, // D8
  4978.0317395533f,  // Eb8
  5274.04091060593f, // E8
  5587.65170292807f, // F8
  5919.91076338616f, // Gb8
  6271.926975708f,   // G8
  6644.87516127913f, // Ab8
  7040.00000000001f, // A8
  7458.62018428945f, // Bb8
  7902.132820098f,   // B8
  8372.01808961917f, // C9
  8869.84419125992f, // Db9
  9397.27257335706f, // D9
  9956.06347910661f, // Eb9
  10548.0818212119f, // E9
  11175.3034058561f, // F9
  11839.8215267723f, // Gb9
  12543.853951416f,  // G9 (Last MIDI note)
};

// Enums //////////////////////////////////////////////////////////////////////

enum IMP_OCTAVE : u8 {
  IMP_OCTAVE_MINUS_5,
  IMP_OCTAVE_MINUS_4,
  IMP_OCTAVE_MINUS_3,
  IMP_OCTAVE_MINUS_2,
  IMP_OCTAVE_MINUS_1,
  IMP_OCTAVE_ZERO,
  IMP_OCTAVE_PLUS_1,
  IMP_OCTAVE_PLUS_2,
  IMP_OCTAVE_PLUS_3,
  IMP_OCTAVE_PLUS_4,
  IMP_OCTAVE_PLUS_5,
};

enum IMP_NOTE : u8 {
  IMP_NOTE_C,
  IMP_NOTE_CSHARP,
  IMP_NOTE_D,
  IMP_NOTE_DSHARP,
  IMP_NOTE_E,
  IMP_NOTE_F,
  IMP_NOTE_FSHARP,
  IMP_NOTE_G,
  IMP_NOTE_GSHARP,
  IMP_NOTE_A,
  IMP_NOTE_ASHARP,
  IMP_NOTE_B,
};

// enum IMP_ABSTRACT_INTERVAL : u8 {
//     IMP_ABSTRACT_INTERVAL_UNISON,
//     IMP_ABSTRACT_INTERVAL_SECOND,
//     IMP_ABSTRACT_INTERVAL_THIRD,
//     IMP_ABSTRACT_INTERVAL_FOURTH,
//     IMP_ABSTRACT_INTERVAL_FIFTH,
//     IMP_ABSTRACT_INTERVAL_SIXTH,
//     IMP_ABSTRACT_INTERVAL_SEVENTH,
//     IMP_ABSTRACT_INTERVAL_OCTAVE,
//     IMP_ABSTRACT_INTERVAL_NINTH,
//     IMP_ABSTRACT_INTERVAL_TENTH,
//     IMP_ABSTRACT_INTERVAL_ELEVENTH,
//     IMP_ABSTRACT_INTERVAL_TWELVTH,
//     IMP_ABSTRACT_INTERVAL_THIRTEENTH,
//     IMP_ABSTRACT_INTERVAL_FOURTEENTH,
//     IMP_ABSTRACT_INTERVAL_DOUBLE_OCTAVE,
// };

// enum IMP_INTERVAL : u8 {
//     IMP_INTERVAL_UNISON,
//     IMP_INTERVAL_MINOR_SECOND,
//     IMP_INTERVAL_MAJOR_SECOND,
//     IMP_INTERVAL_MINOR_THIRD,
//     IMP_INTERVAL_MAJOR_THIRD,
//     IMP_INTERVAL_PERFECT_FOURTH,
//     IMP_INTERVAL_TRITONE,
//     IMP_INTERVAL_PERFECT_FIFTH,
//     IMP_INTERVAL_MINOR_SIXTH,
//     IMP_INTERVAL_MAJOR_SIXTH,
//     IMP_INTERVAL_MINOR_SEVENTH,
//     IMP_INTERVAL_MAJOR_SEVENTH,
// };

enum IMP_EVENT_TYPE : u8 {
  IMP_EVENT_TYPE_STRIKE,  // note
  IMP_EVENT_TYPE_RELEASE, // note
  // IMP_EVENT_TYPE_VIBRATO, // amp, period
  IMP_EVENT_TYPE_WAIT, // wait, subdiv
  // IMP_EVENT_TYPE_SET_TIME_SIGNATURE, // top, bottom
  // IMP_EVENT_TYPE_CHORD, // note, type
};

enum IMP_VOICE_STATE : u8 {
  IMP_VOICE_STATE_OFF,
  IMP_VOICE_STATE_ATTACK,
  IMP_VOICE_STATE_DECAY,
  IMP_VOICE_STATE_SUSTAIN,
  IMP_VOICE_STATE_RELEASE,
};

// Structs ////////////////////////////////////////////////////////////////////

struct imp_voice {
  f32 phase;
  f32 freq;
  f32 vol;
  f32 start_time;
  f32 release_time;
  f32 adsr_release_vol;
  IMP_VOICE_STATE state;
};

struct imp_adsr {
  /// attack time in seconds
  f32 at;
  /// decay time in seconds
  f32 dt;
  /// release time in seconds
  f32 rt;
  /// peak attack level, bounds [0,1]
  f32 a;
  /// sustain level, bounds [0,1]
  f32 s;
  /// attack filter shape, length IMP_FILTER_SIZE
  f32 *af;
  /// decay filter shape, length IMP_FILTER_SIZE
  f32 *df;
  /// release filter shape, length IMP_FILTER_SIZE
  f32 *rf;
};

struct imp_vibrato {
  f32 amp;  // in hz
  f32 freq; // in samples
};

struct imp_scale {
  u8 size;
  u8 *ixs;
};

// struct imp_time_sig {
//     u8 top;
//     u8 btm;
// };

struct imp_synth {
  f32 lfo;
  HarmonicsWavetable wavetable;
  imp_voice *voices;
  imp_adsr adsr;
  imp_vibrato vibrato;
};

struct imp_instrument_instance {
  bool active;
  imp_synth *synth;
  imp_scale scale;
  u8 scale_root;
  f64 e_countdown;
  CircularRWBuffer<u8> queue;
};

struct imp_song {
  f32 bpm;
  // u8 *form;
  imp_instrument_instance *instrument_instances;
  f64 time_scale;
  f64 absolute_time;
  f64 time;
};

// Helper functions ///////////////////////////////////////////////////////////

inline u8 imp_note(IMP_NOTE note, IMP_OCTAVE octave)
{
  return octave * 12 + note;
}

// TODO prefix with imp_
// TODO scale_rel_ix_up / scale_rel_ix_down instead, like in ground ??
/// Find relative index in scale
/// example:
/// scale_note 5
/// scale_ix 0  1  2
/// scale    2  4  6
/// offsets -3 -1 +1
/// returns 1
inline bool scale_rel_ix(imp_scale scale, u8 scale_root, u8 note, u8 *scale_ix, s8 *offset)
{
  s8 scale_note = (note + 12 - scale_root) % 12;
  *offset = 100; // init with unreasonably high offset
  for (u32 i = 0; i != scale.size; ++i) {
    s8 curr_offset = (s8)scale.ixs[i] - scale_note;
    if (curr_offset == 0) {
      *offset = 0;
      *scale_ix = i;
      return true;
    }
    // no equality check, in order to prefer lower note (unless scale ixs goes down for stupid
    // reason)
    else if ((curr_offset * curr_offset < *offset * *offset)) {
      *offset = curr_offset;
      *scale_ix = i;
    }
  }
  return false;
}

inline u8 scale_descend(imp_scale scale, u8 scale_root, u8 origin)
{
  u8 root_rel = (origin + 12 - scale_root) % 12;

  for (s32 i = scale.size - 1; i >= 0; --i) {
    if (scale.ixs[i] < root_rel) {
      return origin + scale.ixs[i] - root_rel; // TODO bound
    }
  }

  return origin + scale.ixs[scale.size] - 12 - root_rel; // TODO bound
}

inline u8 scale_ascend(imp_scale scale, u8 scale_root, u8 origin)
{
  u8 root_rel = (origin + 12 - scale_root) % 12;

  for (u32 i = 0; i != scale.size; ++i) {
    if (scale.ixs[i] > root_rel) {
      return origin + scale.ixs[i] - root_rel; // TODO bound
    }
  }

  return origin + 12 - root_rel; // TODO bound
}

inline u8 scale_rand(imp_scale scale, u8 scale_root)
{
  return (scale_root + scale.ixs[rand() % scale.size]) % 12;
}

// Application ////////////////////////////////////////////////////////////////

// u8 form[] = {
//     {root} {chord} wait 5/8 {root} {chord} wait 3/8
//     {root} {chord} wait 2/4 {root} {chord} wait 1/4 {root} {chord} wait 1/4
//     ... etc
//     "repeat twice",
//     eof
// };

FMOD_RESULT F_CALLBACK pcmreadcallback(FMOD_SOUND *sound, void *data, u32 datalen)
{
  imp_song *song_ptr;
  ((FMOD::Sound *)sound)->getUserData((void **)&song_ptr);
  imp_song &song = *song_ptr;

  // Fill sound buffer
  s16 *stereo16bitbuffer = (s16 *)data;
  for (u32 count = 0; count < (datalen >> 2); count++) // >>2 = 4 bytes per sample (16bit stereo)
  {
    f32 amplitude_sum = 0;
    f64 dt = IMP_INV_SAMPLE_FREQ * song.time_scale;

    // update song

    for (int instrument_instance_ix = 0; instrument_instance_ix < IMP_NUM_INSTRUMENT_INSTANCES;
         ++instrument_instance_ix) {
      // Get active instrument instance
      imp_instrument_instance &instrument_instance =
        song.instrument_instances[instrument_instance_ix];
      if (!instrument_instance.active) {
        continue;
      }

      imp_synth &synth = *instrument_instance.synth;
      imp_scale scale = instrument_instance.scale;
      u8 scale_root = instrument_instance.scale_root;

      // Handle events
      while (instrument_instance.e_countdown <= 0) {
        auto &events = instrument_instance.queue;

        if (events.getState() == CircularRWBufferBase::State::Empty) {
          // Generate future events from plan

          u32 subdiv = pow(2, rand() % 3); // ∈ { 1, 2, 4 }
          u8 note = imp_note(
            (IMP_NOTE)scale_rand(scale, scale_root), (IMP_OCTAVE)(IMP_OCTAVE_MINUS_2 + rand() % 2));
          for (u32 i = 0; i < subdiv; ++i) {
            if (rand() % 2) {
              u32 subdiv2 = pow(2, rand() % 3); // ∈ { 1, 2, 4 }
              using F = u8 (*)(imp_scale, u8, u8);
              F move_func = rand() % 2 == 0 ? scale_ascend : scale_descend;
              for (u32 j = 0; j < subdiv2; ++j) {
                if (rand() % 4) {
                  note = move_func(scale, scale_root, note);
                  events.write(
                    IMP_EVENT_TYPE_STRIKE,
                    note,
                    IMP_EVENT_TYPE_WAIT,
                    1,
                    subdiv * subdiv2,
                    IMP_EVENT_TYPE_RELEASE,
                    note);
                }
                else {
                  events.write(IMP_EVENT_TYPE_WAIT);
                  events.write(1);
                  events.write(subdiv * subdiv2);
                }
              }
            }
            else {
              note = imp_note(
                (IMP_NOTE)scale_rand(scale, scale_root),
                (IMP_OCTAVE)(IMP_OCTAVE_MINUS_1 + rand() % 2));
              events.write(IMP_EVENT_TYPE_STRIKE);
              events.write(note);
              events.write(IMP_EVENT_TYPE_WAIT);
              events.write(1);
              events.write(subdiv);
              events.write(IMP_EVENT_TYPE_RELEASE);
              events.write(note);
            }
          }
        }

        u8 event = events.read();

        if (event == IMP_EVENT_TYPE_STRIKE) {
          f32 freq = imp_note_freqs[events.read()];
          for (u32 i = 0; i < IMP_NUM_VOICES; ++i) {
            imp_voice &voice = synth.voices[i];
            if (voice.state == IMP_VOICE_STATE_OFF) {
              voice.freq = freq;
              voice.start_time = song.time;
              voice.state = IMP_VOICE_STATE_ATTACK;
              break;
              // NOTE do we need to release other voices on the same note? I think they should be
              // automatically released...
            }
          }
        }
        else if (event == IMP_EVENT_TYPE_RELEASE) {
          f32 freq = imp_note_freqs[events.read()];
          for (u32 i = 0; i < IMP_NUM_VOICES; ++i) {
            imp_voice &voice = synth.voices[i];

            if (
              voice.freq == freq && voice.state != IMP_VOICE_STATE_OFF &&
              voice.state != IMP_VOICE_STATE_RELEASE) {
              // Calculate current adsr release volume (if still in attack / decay phase, a sudden
              // jump down to sustain level would cause an audible discontinuity)
              if (voice.state == IMP_VOICE_STATE_ATTACK) {
                voice.adsr_release_vol = synth.adsr.a *
                  lerp_array(synth.adsr.af,
                             IMP_FILTER_SIZE,
                             (song.time - voice.start_time) / synth.adsr.at);
              }
              else if (voice.state == IMP_VOICE_STATE_DECAY) {
                voice.adsr_release_vol = synth.adsr.s +
                  (synth.adsr.a - synth.adsr.s) *
                    lerp_array(
                      synth.adsr.df,
                      IMP_FILTER_SIZE,
                      (song.time - voice.start_time - synth.adsr.at) / synth.adsr.dt);
              }
              else { // voice.state == IMP_VOICE_STATE_SUSTAIN
                voice.adsr_release_vol = synth.adsr.s;
              }

              voice.release_time = song.time;
              voice.state = IMP_VOICE_STATE_RELEASE;
            }
          }
        }
        else if (event == IMP_EVENT_TYPE_WAIT) {
          u8 wait = events.read();
          u8 div = events.read();
          f32 beats_wait = wait * 4.f / div;
          instrument_instance.e_countdown = 60.f * beats_wait / song.bpm;
        }
      }

      // Countdown to next event
      instrument_instance.e_countdown -= dt;

      // Sum voice amplitudes
      for (u32 i = 0; i < IMP_NUM_VOICES; ++i) {
        // Get active voice
        imp_voice &voice = synth.voices[i];
        if (voice.state == IMP_VOICE_STATE_OFF) {
          continue;
        }

        // Calculate ADSR filter modifier
        f32 adsr;
        if (voice.state == IMP_VOICE_STATE_ATTACK) {
          adsr = synth.adsr.a *
            lerp_array(
                   synth.adsr.af, IMP_FILTER_SIZE, (song.time - voice.start_time) / synth.adsr.at);
        }
        else if (voice.state == IMP_VOICE_STATE_DECAY) {
          adsr = synth.adsr.s +
            (synth.adsr.a - synth.adsr.s) *
              lerp_array(
                synth.adsr.df,
                IMP_FILTER_SIZE,
                (song.time - voice.start_time - synth.adsr.at) / synth.adsr.dt);
        }
        else if (voice.state == IMP_VOICE_STATE_SUSTAIN) {
          adsr = synth.adsr.s;
        }
        else { // voice.state == IMP_VOICE_STATE_RELEASE
          adsr =
            voice.adsr_release_vol *
            lerp_array(
              synth.adsr.rf, IMP_FILTER_SIZE, (song.time - voice.release_time) / synth.adsr.rt);
        }

        // Interpolate and sum amplitude
        amplitude_sum += adsr * voice.vol * synth.wavetable.sample(voice.phase);

        // Proceed phase
        voice.phase +=
          dt * (synth.vibrato.amp * sin(TWOPIF * synth.lfo * synth.vibrato.freq) + voice.freq);
        if (voice.phase < 0.f) {
          ++voice.phase;
        }
        else if (voice.phase >= 1.f) {
          --voice.phase;
        }

        // Age
        if (
          voice.state == IMP_VOICE_STATE_ATTACK &&
          (song.time - voice.start_time) >= synth.adsr.at) {
          voice.state = IMP_VOICE_STATE_DECAY;
        }
        else if (
          voice.state == IMP_VOICE_STATE_DECAY &&
          (song.time - voice.start_time) >= synth.adsr.at + synth.adsr.dt) {
          voice.state = IMP_VOICE_STATE_SUSTAIN;
        }
        else if (
          voice.state == IMP_VOICE_STATE_RELEASE &&
          (song.time - voice.release_time) >= synth.adsr.rt) {
          voice.state = IMP_VOICE_STATE_OFF;
        }
      }

      // Update oscillator
      synth.lfo += dt;
      if (synth.lfo >= 1.f) {
        --synth.lfo;
      }
    }

    song.time += dt;
    song.absolute_time += IMP_INV_SAMPLE_FREQ;

    f32 time_lerp_start_time = 5.f;
    f32 time_lerp_duration = 3.f;
    if (song.absolute_time > time_lerp_start_time) {
      f32 t = (song.absolute_time - time_lerp_start_time) / time_lerp_duration;
      song.time_scale = cerp(1.f, 0.f, min(t, 1.f));
    }

    // Clamp amplitude
    if (amplitude_sum >= 1.f) {
      amplitude_sum = 1.f;
      puts("clip+");
    }
    else if (amplitude_sum <= -1.f) {
      amplitude_sum = -1.f;
      puts("clip-");
    }

    // Write channel data
    s16 val = (s16)(amplitude_sum * 32767.f);
    *stereo16bitbuffer++ = val; // left channel
    *stereo16bitbuffer++ = val; // right channel
  }

  return FMOD_OK;
}

FMOD_RESULT F_CALLBACK pcmsetposcallback(
  FMOD_SOUND * /*sound*/, s32 /*subsound*/, u32 /*position*/, FMOD_TIMEUNIT /*postype*/)
{
  // This is useful if the user calls Channel::setPosition and you want to seek your data
  // accordingly.
  return FMOD_OK;
}

s32 main()
{
  int seed = 0;
  std::cout << "Please input a seed:";
  std::cin >> seed;
  srand(seed);

  auto random_wavetable = ([]() {
    std::vector<f32> harmonics;
    for (u32 i = 0; i != 32; ++i) {
      f32 div = (f32)(i + 1);
      harmonics.push_back((rand() % 5) / (div * div));
    }
    return HarmonicsWavetable(harmonics);
  })();

  auto sine_wavetable = HarmonicsWavetable({1});

  auto violin_wavetable = {1.f, .75f, .65f, .55f, .5f, .45f, .4f, .35f, .3f, .25f, .25f, .2f};

  // Fill AD(S)R filters
  f32 attack_filter[IMP_FILTER_SIZE] = {0};
  f32 decay_filter[IMP_FILTER_SIZE] = {0};
  f32 release_filter[IMP_FILTER_SIZE] = {0};
  {
    // Linear functions for now
    f32 fsize = IMP_FILTER_SIZE - 1.f;
    for (s32 i = 0; i != IMP_FILTER_SIZE; ++i) {
      attack_filter[i] = i / fsize;
      decay_filter[i] = 1.f - (i / fsize);
      release_filter[i] = 1.f - (i / fsize);
    }
  }

  // Initialize synth voices
  imp_voice voices[IMP_NUM_VOICES * IMP_NUM_SYNTHS] = {0};
  for (s32 i = 0; i < IMP_NUM_VOICES * IMP_NUM_SYNTHS; ++i) {
    voices[i].vol = .25f;
  }

  // Setup synths
  imp_synth synths[IMP_NUM_SYNTHS] = {0};
  for (s32 i = 0; i != IMP_NUM_SYNTHS; ++i) {
    synths[i].voices = voices + i * IMP_NUM_VOICES;
    synths[i].wavetable = violin_wavetable;
    synths[i].adsr.at = .068f;
    synths[i].adsr.dt = .814f;
    synths[i].adsr.rt = .045f;
    synths[i].adsr.a = .7f;
    synths[i].adsr.s = .5f;
    synths[i].adsr.af = attack_filter;
    synths[i].adsr.df = decay_filter;
    synths[i].adsr.rf = release_filter;
    synths[i].vibrato.amp = .5f;
    synths[i].vibrato.freq = 3.f;
  }

  // Setup scales
  u8 penta_ixs[] = {
    0,
    2,
    5,
    7,
    10,
  };
  imp_scale penta;
  penta.size = sizeof(penta_ixs);
  penta.ixs = penta_ixs;

  u8 major_ixs[] = {
    0,
    2,
    4,
    5,
    7,
    9,
    11,
  };
  imp_scale major;
  major.size = sizeof(major_ixs);
  major.ixs = major_ixs;

  u8 harm_min_ixs[] = {
    0,
    2,
    3,
    5,
    7,
    8,
    11,
  };
  imp_scale harm_min;
  harm_min.size = sizeof(harm_min_ixs);
  harm_min.ixs = harm_min_ixs;

  // Setup instrument instances
  imp_instrument_instance instrument_instances[IMP_NUM_INSTRUMENT_INSTANCES] = {0};
  for (s32 i = 0; i != 4; ++i) {
    instrument_instances[i].active = true;
    instrument_instances[i].e_countdown = 0;
    instrument_instances[i].synth = &synths[i % IMP_NUM_SYNTHS];
    instrument_instances[i].scale = penta;
    instrument_instances[i].scale_root = IMP_NOTE_A;
  }

  // Setup song
  imp_song song;
  {
    song.bpm = 130;
    song.instrument_instances = instrument_instances;
    song.time_scale = 1.f;
    song.time = 0.f;
    song.absolute_time = 0.f;
  }

  // Init FMOD
  FMOD::System *system = nullptr;
  FMOD::Channel *channel = nullptr;
  FMOD::Sound *sound;

  FMODERRCHECK(FMOD::System_Create(&system));
  FMODERRCHECK(system->init(512, FMOD_INIT_NORMAL, nullptr));

  // Create sound stream
  {
    FMOD_CREATESOUNDEXINFO exinfo;
    FMOD_MODE mode = FMOD_OPENUSER | FMOD_LOOP_NORMAL | FMOD_CREATESTREAM;
    memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO); /* Required. */
    exinfo.numchannels = 2;                         /* Number of channels in the sound. */
    exinfo.defaultfrequency = IMP_SAMPLE_FREQ;      /* Default playback rate of sound. */
    exinfo.decodebuffersize = 2048; /* Chunk size of stream update in samples. This will be the
                                       amount of data passed to the user callback. */
    exinfo.length = exinfo.defaultfrequency * exinfo.numchannels *
      sizeof(s16); /* Length of PCM data in bytes of whole song (for Sound::getLength) */
    exinfo.format = FMOD_SOUND_FORMAT_PCM16;      /* Data format of sound. */
    exinfo.pcmreadcallback = pcmreadcallback;     /* User callback for reading. */
    exinfo.pcmsetposcallback = pcmsetposcallback; /* User callback for seeking. */
    exinfo.userdata = &song;

    FMODERRCHECK(system->createSound(nullptr, mode, &exinfo, &sound));
    FMODERRCHECK(system->playSound(sound, nullptr, false, &channel));
  }

  // FMOD::DSP* dsp_echo;
  // FMOD::DSP* dsp_lowpass;
  // FMOD::ChannelGroup* channelgroup;

  // // Add echo to the sound.
  // FMODERRCHECK(system->createDSPByType(FMOD_DSP_TYPE_ECHO, &dsp_echo));
  // FMODERRCHECK(channel->addDSP(1, dsp_echo));

  // // Add the channel that the sound is playing on to a new channel group.
  // FMODERRCHECK(system->createChannelGroup("my channelgroup", &channelgroup));
  // FMODERRCHECK(channel->setChannelGroup(channelgroup));

  // // Add a lowpass filter to that channel group.
  // FMODERRCHECK(system->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &dsp_lowpass));
  // FMODERRCHECK(channelgroup->addDSP(1, dsp_lowpass));

  std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> t,
    t0 = std::chrono::high_resolution_clock::now();
  f32 dt, prev_elapsed, elapsed = 0.f;

  bool isPlaying = true;
  while (isPlaying && song.time_scale > FLT_EPSILON) {
    // time
    {
      t = std::chrono::high_resolution_clock::now();
      prev_elapsed = elapsed;
      elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t - t0).count() / 1000.f;
      dt = elapsed - prev_elapsed;
    }

    FMODERRCHECK(system->update());

    FMODSOUNDERRCHECK(channel->isPlaying(&isPlaying));

    SLEEP(1);
  }

  FMODERRCHECK(sound->release());

  FMODERRCHECK(system->close());

  FMODERRCHECK(system->release());
}
