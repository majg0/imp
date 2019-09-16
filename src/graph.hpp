#ifndef IMP_GRAPH
#define IMP_GRAPH

#include "constants.hpp"
#include "math.hpp"

#include <map>
#include <vector>

class Graph;
class Timing;

struct Context;

class IGraphNode {
public:
  using Inputs = std::vector<IGraphNode*>;
  virtual const f64 sample(
    const Graph& graph, const Timing& timing, const Inputs& inputs) const = 0;
  virtual void on_tick(const Timing& timing) = 0;
};

class Graph {
public:
  using Node = IGraphNode;

  void on_tick(const Timing& timing) const
  {
    for (const auto& [source, v] : map) {
      source->on_tick(timing);
    }
  }

  const f64 sample(const Timing& timing, const Node& sink) const
  {
    return sink.sample(*this, timing, map.at(const_cast<Node* const>(&sink)));
  }

  void connect(const Node& source, const Node& sink)
  {
    map.at(const_cast<Node* const>(&sink))
      .push_back(const_cast<Node* const>(&source));
  }

  void insert(const Node& node)
  {
    map.insert({const_cast<Node* const>(&node), {}});
  }

private:
  std::map<Node* const, std::vector<Node*>> map;
};

class Timing {
public:
  static constexpr f64 SAMPLE_FREQUENCY = 44100.;
  static constexpr f64 SAMPLE_DURATION = 1. / SAMPLE_FREQUENCY;

  const f64 get_scaled_delta_time() const { return scaled_delta_time; }
  const f64 get_scaled_time() const { return scaled_time; }

  // TODO (feat): interpolatable
  void set_time_scale(const f64 new_time_scale)
  {
    time_scale = new_time_scale;
    scaled_delta_time = time_scale * SAMPLE_DURATION;
  }

  void tick()
  {
    absolute_time += SAMPLE_DURATION;
    scaled_time += scaled_delta_time;
  }

private:
  f64 absolute_time = .0;
  f64 scaled_time = .0;
  f64 scaled_delta_time = SAMPLE_DURATION;
  f64 time_scale = 1.; // TODO (feat): interpolatable
};

class Interpolated {
public:
  Interpolated(const f64 value) : target_value(value) {}

  void set(
    const f64 target_value,
    const Timing&
      timing, // NOTE: takes a Timing in order to uphold the invariant that
              // interpolation must start at call time.
    const f64
      interpolation_duration, // TODO (feat): type safety - measure in seconds
    const Interpolation interpolation) noexcept
  {
    prev_value = get(timing);
    this->target_value = target_value;
    this->start_time = timing.get_scaled_time();
    this->interpolation_duration = interpolation_duration;
    this->interpolation = interpolation;
  }

  // NOTE: this could be unconstrained to take `const f64 time` instead, but we
  // are delaying that decision until confirmed necessary
  const f64 get(const Timing& timing) const noexcept
  {
    return interpolate(
      prev_value,
      target_value,
      interpolation_duration < DBL_EPSILON
        ? 1.
        : (timing.get_scaled_time() - start_time) / interpolation_duration,
      interpolation);
  }

private:
  f64 target_value = .0;
  f64 prev_value = .0;
  f64 start_time = .0; // TODO (feat): type safety - measure in seconds
  f64 interpolation_duration =
    .0; // TODO (feat): type safety - measure in seconds
  Interpolation interpolation = Interpolation::None;
};

class Oscillator : public IGraphNode {
public:
  Oscillator(const f64 frequency) : frequency(frequency) {}

  void set_frequency(
    const f64 target_value,
    const Timing& timing,
    const f64 interpolation_duration,
    const Interpolation interpolation)
  {
    frequency.set(target_value, timing, interpolation_duration, interpolation);
  }

  const f64 sample(const Graph&, const Timing& timing, const Inputs&) const
  {
    // TODO (feat): more waveform types
    return sin(frequency.get(timing) * phase * TWOPI);
  }

  void on_tick(const Timing& timing)
  {
    phase += timing.get_scaled_delta_time();
    if (phase > 1.) {
      --phase;
    }
  }

private:
  f64 phase = .0;
  Interpolated frequency = .0;
};

class Gain : public IGraphNode {
public:
  Gain(f64 gain) : gain(gain) {}

  const f64
  sample(const Graph& graph, const Timing& timing, const Inputs& inputs) const
  {
    f64 input = .0;
    for (const auto* node : inputs) {
      input += graph.sample(timing, *node);
    }
    return input * gain;
  }

  void on_tick(const Timing& timing) {}

private:
  f64 gain;
};

template <typename Node>
class Storage {
public:
  Storage(Graph& graph) : graph(graph) {}

  template <typename... Args>
  Node& create(const Args... args)
  {
    Node& node = items.emplace_back(args...);
    graph.insert(node);
    return node;
  }

private:
  Graph& graph;
  std::vector<Node> items;
};

struct Context {
  Graph graph;
  Timing timing;
  Storage<Oscillator> oscillator = {graph};
  Storage<Gain> gain = {graph};

  void tick()
  {
    timing.tick();
    graph.on_tick(timing);
  }
};

class Test {
public:
  void test()
  {
    auto context = Context();
    auto& osc = context.oscillator.create(441.);
    auto& gain = context.gain.create(0.25);

    context.graph.connect(osc, gain);
    for (u32 i = 0; i != 100; ++i) {
      std::cout << context.graph.sample(context.timing, gain) << std::endl;

      if (i == 50) {
        osc.set_frequency(882., context.timing, .0, Interpolation::None);
      }

      context.tick();
    }
  }
};

#endif
