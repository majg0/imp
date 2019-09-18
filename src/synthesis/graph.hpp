#ifndef IMP_GRAPH
#define IMP_GRAPH

#include "constants.hpp"
#include "math.hpp"
#include "time_state.hpp"
#include "wavetable.hpp"

#include <map>
#include <vector>

struct PhaseRecord {
  void on_tick(const TimeState& time_state)
  {
    phase += time_state.get_scaled_delta_time();
    if (phase > 1.) {
      --phase;
    }
  }

  f64 phase = .0;
};

struct SineRecord {
  void set_frequency(
    const f64 target_value,
    const TimeState& time_state,
    const f64 interpolation_duration,
    const Interpolation interpolation)
  {
    frequency.set(
      target_value, time_state, interpolation_duration, interpolation);
  }

  const f64 sample(const TimeState& time_state, const f64 phase) const
  {
    // TODO (feat): more waveform types
    return sin(frequency.get(time_state) * phase * TWOPI);
  }

private:
  Interpolated frequency = .0;
};

#include <algorithm> // lower_bound
#include <cstring>   // memcpy
#include <optional>  // optional

struct Range {
  size_t start;
  size_t end;

  friend bool operator<(const Range& l, const size_t r) { return l.start < r; }

  void debug_print() const noexcept
  {
    std::cout << "[" << start << "," << end << "]";
  }
};

template <typename T>
void dbg(const T& t)
{
  t.debug_print();
  std::cout << std::endl;
}

// TODO (feat): tests
template <size_t NUM_ITEMS>
class Ranges {
public:
  struct TakeResult {
    bool success;
    size_t index;
  };

  static Ranges full()
  {
    Ranges result;
    result.ranges[0].start = 0;
    result.ranges[0].end = NUM_ITEMS;
    result.num_ranges = 1;
    return result;
  }

  static Ranges empty()
  {

    Ranges result;
    result.ranges[0].start = NUM_ITEMS;
    result.ranges[0].end = NUM_ITEMS;
    result.num_ranges = 0;
    return result;
  }

  const std::optional<const size_t> take_first() noexcept
  {
    if (num_ranges == 0) {
      return std::nullopt;
    }

    const auto [start, end] = ranges[0];
    const auto new_start = start + 1;

    // 1. range now empty
    if (new_start == end) {
      if (--num_ranges == 0) {
        ranges[0].start = NUM_ITEMS;
      }
      else {
        std::memcpy(&ranges[0], &ranges[1], sizeof(Range) * num_ranges);
      }
    }
    // 2. range still not empty
    else {
      ranges[0].start = new_start;
    }

    return {start};
  }

  const auto find_overlapping(const size_t index) const noexcept
  {
    const auto end = &ranges[num_ranges];
    // NOTE: find it such that `index <= it->start`
    const auto it = std::lower_bound(ranges, end, index);
    if (it == end) {
      const auto last = end - 1;
      return index < last->end ? std::optional<decltype(it)>{last}
                               : std::nullopt;
    }
    return index >= it->start ? std::optional<decltype(it)>{it} : std::nullopt;
  }

  void leave(const size_t index) noexcept
  {
    if (index >= NUM_ITEMS) {
      return;
    }

    const auto end = &ranges[num_ranges];

    // NOTE: find it such that `index <= it->start`
    const auto it = std::lower_bound(ranges, end, index);

    if (it == end) {
      // NOTE: here `r.start < index` for every Range r in `ranges`.
      // NOTE: here it's impossibe for `it` to be out of bounds in `ranges`.

      if (num_ranges > 0) {
        const auto last = it - 1;

        if (index < last->end) {
          // 0. overlap; ignore
          return;
        }

        // 1. expand last forwards
        if (index == last->end) {
          ++last->end;
          return;
        }
      }

      // 2. create new range after
      it->start = index;
      it->end = index + 1;
      ++num_ranges;
      return;
    }

    if (index == it->start) {
      // 0. overlap; ignore
      return;
    }

    const auto expand_backwards = index == it->start - 1;
    const auto expand_previous_forwards =
      it != ranges && index == (it - 1)->end;

    if (expand_backwards && expand_previous_forwards) {
      // 3. merge ranges
      const auto offset = it - ranges;
      const auto num_ranges_rightwards = num_ranges - offset;
      it->start = (it - 1)->start;
      std::memcpy(it - 1, it, sizeof(Range) * num_ranges_rightwards);
      --num_ranges;
      return;
    }

    if (expand_backwards) {
      // 4. expand backwards
      it->start = index;
      return;
    }

    if (expand_previous_forwards) {
      // 5. expand previous forwards
      ++(it - 1)->end;
      return;
    }

    // 6. create new range before
    const auto offset = it - ranges;
    const auto num_ranges_rightwards = num_ranges - offset;
    std::memcpy(it + 1, it, sizeof(Range) * num_ranges_rightwards);
    it->start = index;
    it->end = index + 1;
    ++num_ranges;
  }

  void debug_print() const noexcept
  {
    if (num_ranges == 0) {
      std::cout << "[]";
    }
    else {
      for (auto it = ranges; it != &ranges[num_ranges]; ++it) {
        it->debug_print();
      }
    }
    std::cout << std::endl;
  }

private:
  Ranges() {}
  static constexpr size_t MAX_NUM_RANGES = (NUM_ITEMS + 1) / 2;
  size_t num_ranges = 1;
  Range ranges[MAX_NUM_RANGES] = {0};
};

class Database;

// class ID {
// public:
//   // TODO: how to establish invariant that ID may not outlive graph?
//   ID(Graph2& graph, const size_t index) : graph(graph), index(index) {}
//   ~ID();

// private:
//   Graph2& graph;
//   size_t index = 0;
// };

class Database {
public:
  static constexpr size_t SIZE = 1024;
  Ranges<SIZE> remaining = Ranges<SIZE>::full();
  Ranges<SIZE> used = Ranges<SIZE>::empty();

  // const ID get_id()
  // {
  //   auto [success, index] = ranges.take_first();
  //   if (!success) {
  //     throw "no more space in graph";
  //   }
  // }

  template <typename T>
  const T get_record(const size_t id);

private:
  PhaseRecord phase_records[SIZE]{};
  SineRecord sine_records[SIZE]{};
};

// ID : ~ID()
// {
//   graph.ranges.leave(index);
// }

template <>
const PhaseRecord Database::get_record(const size_t id)
{
  return phase_records[id];
}

template <>
const SineRecord Database::get_record(const size_t id)
{
  return sine_records[id];
}

class Test {
public:
  void test()
  {
    Database db;

    // auto context = Context();
    // auto& osc = context.oscillator.create(441.);
    // auto& gain = context.gain.create(0.25);

    // context.graph.connect(osc, gain);
    // for (u32 i = 0; i != 100; ++i) {
    //   std::cout << context.graph.sample(context.time_state, gain) <<
    //   std::endl;

    //   if (i == 50) {
    //     osc.set_frequency(882., context.time_state, .0, Interpolation::None);
    //   }

    //   context.tick();
    // }
  }
};

#endif
