#ifndef IMP_WAVETABLE
#define IMP_WAVETABLE

#include "constants.hpp"
#include "math.hpp"

#include <iostream>
#include <vector>

class HarmonicsWavetable {
public:
  HarmonicsWavetable() {}
  HarmonicsWavetable(std::vector<f64> harmonics) { fill(harmonics); }
  HarmonicsWavetable(std::initializer_list<f64> harmonics) { fill(harmonics); }

  void fill(std::vector<f64> harmonics)
  {
    // Precompute harmonics normalization factor n
    f64 n = 0.;
    for (f64 amplitude : harmonics) {
      n += amplitude;
    }
    n = 1. / n;

    auto N = harmonics.size();

    // Fill the wavetable
    for (u32 i = 0; i != BUF_SIZE; ++i) {
      for (u32 k = 0; k != N; ++k) {
        buffer[i] += n * harmonics[k] * sin(C * i * (k + 1));
      }
    }
  }

  const f64 sample(const f64 t) const
  {
    const f64 ixf = (t - u32(t)) * BUF_SIZE;
    const u32 ix = u32(ixf) % BUF_SIZE;
    return lerp(buffer[ix], buffer[(ix + 1) % BUF_SIZE], ixf - ix);
  }

  void dbg_print()
  {
    constexpr static int height = 80;
    constexpr static int width = 130;
    for (int k = 0; k != height; ++k) {
      for (int i = width - 1; i != 0; --i) {
        f64 val = .5 + .5 * sample(f64(i) / f64(width));

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
  constexpr static f64 C = TWOPI / BUF_SIZE;

  f64 buffer[BUF_SIZE] = {0};
};

#endif