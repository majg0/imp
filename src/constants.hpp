#ifndef IMP_CONSTANTS
#define IMP_CONSTANTS

#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

// Typedefs ///////////////////////////////////////////////////////////////////

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef float f32;
typedef double f64;

// Static data ////////////////////////////////////////////////////////////////

constexpr f32 IMP_SAMPLE_FREQ = 44100.f;
constexpr f32 IMP_INV_SAMPLE_FREQ = .000022675736961451248f;

constexpr size_t IMP_FILTER_SIZE = 32;
constexpr size_t IMP_MAX_FILTER_INDEX = 31;

constexpr size_t IMP_NUM_VOICES = 32;
constexpr size_t IMP_NUM_SYNTHS = 16;
constexpr size_t IMP_NUM_INSTRUMENT_INSTANCES = 64;

constexpr f64 PI = 3.14159265359;
constexpr f64 TWOPI = 6.283185307179586476925286766559;
constexpr f32 TWOPIF = 6.283185307179586476925286766559f;

#endif