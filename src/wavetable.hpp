#ifndef IMP_WAVETABLE
#define IMP_WAVETABLE

#include "constants.hpp"
#include "math.hpp"

#include <iostream>
#include <vector>

class HarmonicsWavetable {
public:
  HarmonicsWavetable() {}
  HarmonicsWavetable(std::vector<f32> harmonics) { fill(harmonics); }
  HarmonicsWavetable(std::initializer_list<f32> harmonics) { fill(harmonics); }

  void fill(std::vector<f32> harmonics)
  {
    // Precompute harmonics normalization factor n
    f32 n = 0.f;
    for (f32 amplitude : harmonics) {
      n += amplitude;
    }
    n = 1.f / n;

    auto N = harmonics.size();

    // Fill the wavetable
    for (u32 i = 0; i != BUF_SIZE; ++i) {
      for (u32 k = 0; k != N; ++k) {
        buffer[i] += n * harmonics[k] * sin(C * i * (k + 1));
      }
    }
  }

  const f32 sample(f32 t) const
  {
    while (t >= 1.f) {
      t -= 1.f;
    }
    f32 ixf = t * BUF_SIZE;
    u32 ix = ((u32)ixf) % BUF_SIZE;
    return lerp(buffer[ix], buffer[(ix + 1) % BUF_SIZE], ixf - ix);
  }

  void dbg_print()
  {
    constexpr static int height = 80;
    constexpr static int width = 130;
    for (int k = 0; k != height; ++k) {
      for (int i = width - 1; i != 0; --i) {
        f32 val = .5f + .5f * sample(f32(i) / f32(width));

        if (val * height >= k && val * height < k + 1) {
          std::cout << "x";
        }
        else {
          std::cout << " ";
        }
      }
      std::cout << std::endl;
    }
  }

private:
  constexpr static size_t BUF_SIZE = 1024;
  constexpr static f32 C = TWOPIF / (f32)BUF_SIZE;

  f32 buffer[BUF_SIZE] = {0};
};

#endif
