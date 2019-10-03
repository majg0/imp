#include "composition/circular_rw_buffer.hpp"
#include "constants.hpp"
// #include "synthesis/graph.hpp"
#include "ecs.hpp"
#include "synthesis/synth.hpp"
#include "synthesis/voice.hpp"
#include "synthesis/wavetable.hpp"
#include "time_state.hpp"

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
#include <vector>

// General Defines ////////////////////////////////////////////////////////////

#define FMODERRCHECK(_result) FMODERRCHECK_fn(_result, __FILE__, __LINE__)
void FMODERRCHECK_fn(FMOD_RESULT result, const char* file, i32 line)
{
  if (result != FMOD_OK) {
    printf(
      "%s(%d): FMOD error %d - %s\n",
      file,
      line,
      result,
      FMOD_ErrorString(result));
  }
}
#define FMODSOUNDERRCHECK(_result) \
  FMODSOUNDERRCHECK_fn(_result, __FILE__, __LINE__)
void FMODSOUNDERRCHECK_fn(FMOD_RESULT result, const char* file, i32 line)
{
  if (result != FMOD_OK && result != FMOD_ERR_INVALID_HANDLE) {
    printf(
      "%s(%d): FMOD error %d - %s\n",
      file,
      line,
      result,
      FMOD_ErrorString(result));
  }
}

// The frequencies for all MIDI notes
static f64 imp_note_freqs[] = {
  8.17579891564368, // C-1 (First MIDI note)
  8.66195721802722, // Db-1
  9.17702399741896, // D-1
  9.722718241315,   // Eb-1
  10.3008611535272, // E-1
  10.9133822322813, // F-1
  11.5623257097385, // Gb-1
  12.2498573744296, // G-1
  12.9782717993732, // Ab-1
  13.75,            // A-1
  14.5676175474403, // Bb-1
  15.4338531642538, // B-1
  16.3515978312874, // C0
  17.3239144360545, // Db0
  18.6540479948379, // D0
  19.44543648263,   // Eb0
  20.6017223070543, // E0
  21.8567644645627, // F0
  23.1246514194771, // Gb0
  24.4997147488593, // G0
  25.9565435987465, // Ab0
  27.5,             // A0 (Piano low A)
  29.1352350948806, // Bb0
  30.8677063285077, // B0 (5-string bass low open B)
  32.7031956625748, // C1
  34.6478288721089, // Db1
  36.7080959896759, // D1
  38.89087296526,   // Eb1
  41.2034446141087, // E1 (4-string bass low open E)
  43.6535289291254, // F1
  46.2493028389542, // Gb1
  48.9994294977186, // G1
  51.913087197493,  // Ab1
  55.,              // A1
  58.2704701897611, // B1
  61.7354126570154, // B1
  65.4063913251495, // C2
  69.2956577442179, // Db2
  73.4161919793518, // D2
  77.7817459305201, // Eb2
  82.4068892282174, // E2 (Guitar low open E)
  87.3070578582508, // F2
  92.4986056779085, // Gb2
  97.9988589954372, // G2 (4 or 5 string bass, high open G)
  103.826174394986, // Ab2
  110.,             // A2 (Guitar 5th string open)
  116.540940379522, // Bb2
  123.470825314031, // B2
  130.812782650299, // C3 (6-string bass, high open C)
  138.591315488436, // Db3
  146.832383958704, // D3 (Guitar 4th string open)
  155.56349186104,  // Eb3
  164.813778456435, // E3
  174.614115716502, // F3
  184.997211355817, // Gb3
  195.997717990874, // G3 (Guitar 3rd string open) (Violin low open G)
  207.652348789972, // Ab3
  220.,             // A3
  233.081880759045, // Bb3
  246.941650628062, // B3 (Guitar 3rd string open)
  261.625565300599, // C4 (Piano middle C)
  277.182630976872, // Db4
  293.664767917407, // D4 (Violin 3rd string open)
  311.126983722081, // Eb4
  329.62755691287,  // E4 (Guitar 1st string open)
  349.228231433004, // F4
  369.994422711634, // Gb4
  391.995435981749, // G4
  415.304697579945, // Ab4
  440.,             // A4 (Tuning fork A) (Violin 2nd string open)
  466.16376151809,  // Bb4
  493.883301256124, // B4
  523.251130601197, // C5
  554.365261953744, // Db5
  587.329535834815, // D5
  622.253967444162, // Eb5
  659.25511382574,  // E5 (Guitar 1st string 12 fret) (Violin 1st string open)
  698.456462866008, // F5
  739.988845423269, // Gb5
  783.990871963499, // G5
  830.609395159891, // Ab5
  880.,             // A5
  932.32752303618,  // Bb5
  987.766602512249, // B5
  1046.50226120239, // C6
  1108.73052390749, // Db6
  1174.65907166963, // D6
  1244.50793488832, // Eb6
  1318.51022765148, // E6 (Guitar 1st string 24 fret)
  1396.91292573202, // F6
  1479.97769084654, // Gb6
  1567.981743927,   // G6
  1661.21879031978, // Ab6
  1760.,            // A6
  1864.65504607236, // Bb6
  1975.5332050245,  // B6
  2093.00452240479, // C7
  2217.46104781498, // Db7
  2349.31814333926, // D7
  2489.01586977665, // Eb7
  2637.02045530296, // E7
  2793.82585146403, // F7
  2959.95538169308, // Gb7
  3135.963487854,   // G7
  3322.43758063956, // Ab7
  3520.,            // A7
  3729.31009214472, // Bb7
  3951.066410049,   // B7
  4186.00904480958, // C8 (Piano upper C)
  4434.92209562996, // Db8
  4698.63628667853, // D8
  4978.0317395533,  // Eb8
  5274.04091060593, // E8
  5587.65170292807, // F8
  5919.91076338616, // Gb8
  6271.926975708,   // G8
  6644.87516127913, // Ab8
  7040.00000000001, // A8
  7458.62018428945, // Bb8
  7902.132820098,   // B8
  8372.01808961917, // C9
  8869.84419125992, // Db9
  9397.27257335706, // D9
  9956.06347910661, // Eb9
  10548.0818212119, // E9
  11175.3034058561, // F9
  11839.8215267723, // Gb9
  12543.853951416,  // G9 (Last MIDI note)
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
  IMP_EVENT_TYPE_SLIDE,   // note
  // IMP_EVENT_TYPE_VIBRATO, // amp, period
  IMP_EVENT_TYPE_WAIT, // wait, subdiv
  // IMP_EVENT_TYPE_SET_TIME_SIGNATURE, // top, bottom
  // IMP_EVENT_TYPE_CHORD, // note, type
};

// Structs ////////////////////////////////////////////////////////////////////

struct imp_scale {
  u8 size;
  u8* ixs;
};

// struct imp_time_sig {
//     u8 top;
//     u8 btm;
// };

struct imp_instrument_instance {
  bool active;
  Synth* synth;
  imp_scale scale;
  u8 scale_root;
  f64 e_countdown;
  CircularRWBuffer queue;
};

struct imp_song {
  f64 bpm;
  // u8 *form;
  imp_instrument_instance* instrument_instances;
  TimeState time_state;
};

// Helper functions ///////////////////////////////////////////////////////////

inline u8 imp_note(IMP_NOTE note, IMP_OCTAVE octave)
{
  return octave * 12 + note;
}

// TODO scale_rel_ix_up / scale_rel_ix_down instead, like in ground ??
/// Find relative index in scale
/// example:
/// scale_note 5
/// scale_ix 0  1  2
/// scale    2  4  6
/// offsets -3 -1 +1
/// returns 1
inline bool
scale_rel_ix(imp_scale scale, u8 scale_root, u8 note, u8* scale_ix, i8* offset)
{
  i8 scale_note = (note + 12 - scale_root) % 12;
  *offset = 100; // init with unreasonably high offset
  for (u32 i = 0; i != scale.size; ++i) {
    i8 curr_offset = i8(scale.ixs[i]) - scale_note;
    if (curr_offset == 0) {
      *offset = 0;
      *scale_ix = i;
      return true;
    }
    // no equality check, in order to prefer lower note (unless scale ixs goes
    // down for stupid reason)
    else if ((curr_offset * curr_offset < *offset * *offset)) {
      *offset = curr_offset;
      *scale_ix = i;
    }
  }
  return false;
}

inline u8 scale_descend(imp_scale scale, u8 scale_root, u8 origin)
{
  u32 last_index = scale.size - 1;

  u8 root_rel = (origin + 12 - scale_root) % 12;

  if (root_rel == 0) {
    return origin + scale.ixs[last_index] - 12 - root_rel; // TODO bound
  }

  for (i32 i = last_index; i >= 0; --i) {
    if (scale.ixs[i] < root_rel) {
      return origin + scale.ixs[i] - root_rel; // TODO bound
    }
  }

  throw "invariant broken: logic faulty"; // TODO (fix): STL exception?
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

FMOD_RESULT F_CALLBACK
pcmreadcallback(FMOD_SOUND* sound, void* data, u32 datalen)
{
  imp_song* song_ptr;
  ((FMOD::Sound*)sound)->getUserData((void**)&song_ptr);
  imp_song& song = *song_ptr;

  // Fill sound buffer
  i16* stereo16bitbuffer = static_cast<i16*>(data);
  for (u32 count = 0; count < (datalen >> 2);
       count++) // >>2 = 4 bytes per sample (16bit stereo)
  {
    f64 amplitude_sum = .0;
    f64 dt = song.time_state.get_scaled_delta_time();

    // update song

    for (size_t instrument_instance_ix = 0;
         instrument_instance_ix < IMP_NUM_INSTRUMENT_INSTANCES;
         ++instrument_instance_ix) {
      // Get active instrument instance
      imp_instrument_instance& instrument_instance =
        song.instrument_instances[instrument_instance_ix];
      if (!instrument_instance.active) {
        continue;
      }

      Synth& synth = *instrument_instance.synth;
      imp_scale scale = instrument_instance.scale;
      u8 scale_root = instrument_instance.scale_root;

      // Handle events
      while (instrument_instance.e_countdown <= 0) {
        auto& events = instrument_instance.queue;

        if (events.get_state() == CircularRWBufferBase::State::Empty) {
          // Generate future events from plan

          u8 note = imp_note(
            IMP_NOTE(scale_rand(scale, scale_root)),
            IMP_OCTAVE(IMP_OCTAVE_MINUS_1 + rand() % 2));
          u8 subdiv1 = pow(2, rand() % 2); // ∈ { 1, 2 }
          for (u32 i = 0; i < subdiv1; ++i) {
            u8 subdiv2 = pow(2, rand() % 4); // ∈ { 1, 2, 4, 8 }
            using F = u8 (*)(imp_scale, u8, u8);
            F move_func = rand() % 2 == 0 ? scale_ascend : scale_descend;
            for (u32 j = 0; j < subdiv2; ++j) {

              u8 subdiv = subdiv1 * subdiv2;
              if (rand() % 3) {
                note = move_func(scale, scale_root, note);
                events.write(
                  rand() % 2 ? IMP_EVENT_TYPE_STRIKE : IMP_EVENT_TYPE_SLIDE,
                  // IMP_EVENT_TYPE_STRIKE,
                  note,
                  1,
                  subdiv,
                  IMP_EVENT_TYPE_RELEASE,
                  note);
              }
              else {
                events.write(IMP_EVENT_TYPE_WAIT, 1, subdiv);
              }
            }
          }
        }

        u8 event = events.read();

        if (event == IMP_EVENT_TYPE_STRIKE) {
          f64 freq = imp_note_freqs[events.read()];
          u8 wait = events.read();
          u8 div = events.read();
          f64 duration = 60. * (wait * 4. / div) / song.bpm;

          std::find_if(
            synth.voices,
            synth.voices + Synth::NUM_VOICES,
            [](const Voice& voice) {
              return voice.has_state(Voice::State::Off);
            })
            ->strike(freq, song.time_state, duration, Interpolation::None);

          instrument_instance.e_countdown = duration;
        }
        else if (event == IMP_EVENT_TYPE_SLIDE) {
          f64 freq = imp_note_freqs[events.read()];
          u8 wait = events.read();
          u8 div = events.read();
          f64 duration = 60. * (wait * 4. / div) / song.bpm;

          std::find_if(
            synth.voices,
            synth.voices + Synth::NUM_VOICES,
            [](const Voice& voice) {
              return voice.has_state(Voice::State::Off);
            })
            ->strike(
              freq, song.time_state, duration / 4., Interpolation::Linear);

          instrument_instance.e_countdown = duration;
        }
        else if (event == IMP_EVENT_TYPE_RELEASE) {
          f64 freq = imp_note_freqs[events.read()];
          std::find_if(
            synth.voices,
            synth.voices + Synth::NUM_VOICES,
            [freq](const Voice& voice) {
              return voice.has_state(Voice::State::On) &&
                voice.has_target_frequency(freq);
            })
            ->release(synth, song.time_state);
        }
        else if (event == IMP_EVENT_TYPE_WAIT) {
          u8 wait = events.read();
          u8 div = events.read();
          f64 beats_wait = wait * 4. / div;
          instrument_instance.e_countdown = 60. * beats_wait / song.bpm;
        }
      }

      // Countdown to next event
      instrument_instance.e_countdown -= dt;

      // Sum voice amplitudes
      for (u32 i = 0; i < Synth::NUM_VOICES; ++i) {
        // Get active voice
        Voice& voice = synth.voices[i];
        if (voice.has_state(Voice::State::Off)) {
          continue;
        }

        amplitude_sum += voice.sample(synth, song.time_state);

        voice.proceed_phase(synth, song.time_state);
      }

      // Update oscillator
      synth.lfo += dt;
      if (synth.lfo >= 1.) {
        --synth.lfo;
      }
    }

    song.time_state.tick();

    f64 time_lerp_start_time = 100.;
    f64 time_lerp_duration = 3.;
    const f64 time_since_start =
      song.time_state.get_absolute_time() - time_lerp_start_time;
    if (time_since_start > 0) {
      f64 t = time_since_start / time_lerp_duration;
      song.time_state.set_time_scale(cerp(1., .0, min(t, 1.)));
    }

    // Clamp amplitude
    if (amplitude_sum >= 1.) {
      amplitude_sum = 1.;
      std::cout << "clip+" << std::endl;
    }
    else if (amplitude_sum <= -1.) {
      amplitude_sum = -1.;
      std::cout << "clip-" << std::endl;
    }

    // Write channel data
    i16 val = i16(amplitude_sum * 32767.);
    *stereo16bitbuffer++ = val; // left channel
    *stereo16bitbuffer++ = val; // right channel
  }

  return FMOD_OK;
}

FMOD_RESULT F_CALLBACK pcmsetposcallback(
  FMOD_SOUND* /*sound*/,
  i32 /*subsound*/,
  u32 /*position*/,
  FMOD_TIMEUNIT /*postype*/)
{
  // This is useful if the user calls Channel::setPosition and you want to seek
  // your data accordingly.
  return FMOD_OK;
}

i32 main()
{
  Test test;
  test.test();
  return 0;
}

i32 main2()
{
  int seed = 0;
  std::cout << "Please input a seed:";
  std::cin >> seed;
  srand(seed);

  auto sine_wavetable = {1.};
  auto violin_wavetable = {
    1., .75, .65, .55, .5, .45, .4, .35, .3, .25, .25, .2};
  auto random_wavetable = ([]() {
    std::vector<f64> harmonics;
    for (u32 i = 0; i != 32; ++i) {
      f64 div = i + 1;
      harmonics.push_back(f64(1 + (rand() % 5)) / (div * div));
    }
    return HarmonicsWavetable(harmonics);
  })();

  // Setup synths
  Synth synths[IMP_NUM_SYNTHS] = {0};
  for (i32 i = 0; i != IMP_NUM_SYNTHS; ++i) {
    synths[i].wavetable = violin_wavetable;
    synths[i].adsr_params.attack_duration = .068;
    synths[i].adsr_params.decay_duration = .014;
    synths[i].adsr_params.release_duration = .045;
    synths[i].adsr_params.attack_amplitude = .7;
    synths[i].adsr_params.sustain_amplitude = .5;
    synths[i].vibrato.amp = .5;
    synths[i].vibrato.freq = 3.;
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
  imp_instrument_instance instrument_instances[IMP_NUM_INSTRUMENT_INSTANCES] = {
    0};
  for (i32 i = 0; i != 4; ++i) {
    instrument_instances[i].active = true;
    instrument_instances[i].e_countdown = 0;
    instrument_instances[i].synth = &synths[i % IMP_NUM_SYNTHS];
    instrument_instances[i].scale = penta;
    instrument_instances[i].scale_root = IMP_NOTE_A;
  }

  // Setup song
  imp_song song;
  {
    song.bpm = 130.;
    song.instrument_instances = instrument_instances;
  }

  // Init FMOD
  FMOD::System* system = nullptr;
  FMOD::Channel* channel = nullptr;
  FMOD::Sound* sound;

  FMODERRCHECK(FMOD::System_Create(&system));
  FMODERRCHECK(system->init(512, FMOD_INIT_NORMAL, nullptr));

  // Create sound stream
  {
    FMOD_CREATESOUNDEXINFO exinfo;
    FMOD_MODE mode = FMOD_OPENUSER | FMOD_LOOP_NORMAL | FMOD_CREATESTREAM;
    memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO); /* Required. */
    exinfo.numchannels = 2; /* Number of channels in the sound. */
    exinfo.defaultfrequency =
      IMP_SAMPLE_FREQ; /* Default playback rate of sound. */
    exinfo.decodebuffersize =
      2048; /* Chunk size of stream update in samples. This will be the
               amount of data passed to the user callback. */
    exinfo.length = exinfo.defaultfrequency * exinfo.numchannels *
      sizeof(i16); /* Length of PCM data in bytes of whole song (for
                      Sound::getLength) */
    exinfo.format = FMOD_SOUND_FORMAT_PCM16;  /* Data format of sound. */
    exinfo.pcmreadcallback = pcmreadcallback; /* User callback for reading. */
    exinfo.pcmsetposcallback =
      pcmsetposcallback; /* User callback for seeking. */
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

  std::chrono::
    time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds>
      t, t0 = std::chrono::high_resolution_clock::now();
  f64 dt, prev_elapsed, elapsed = .0;

  bool isPlaying = true;
  while (isPlaying && song.time_state.get_time_scale() > DBL_EPSILON) {
    // time
    {
      t = std::chrono::high_resolution_clock::now();
      prev_elapsed = elapsed;
      elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(t - t0).count() /
        1000.;
      dt = elapsed - prev_elapsed;
    }

    FMODERRCHECK(system->update());

    FMODSOUNDERRCHECK(channel->isPlaying(&isPlaying));

    SLEEP(1);
  }

  FMODERRCHECK(sound->release());

  FMODERRCHECK(system->close());

  FMODERRCHECK(system->release());

  return 0;
}
