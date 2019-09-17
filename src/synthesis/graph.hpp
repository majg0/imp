#ifndef IMP_GRAPH
#define IMP_GRAPH

#include "constants.hpp"
#include "math.hpp"
#include "time_state.hpp"
#include "wavetable.hpp"

#include <map>
#include <vector>

class Graph;

struct Context;

class IGraphNode {
public:
  using Inputs = std::vector<IGraphNode*>;
  virtual const f64 sample(
    const Graph& graph,
    const TimeState& time_state,
    const Inputs& inputs) const = 0;
  virtual void on_tick(const TimeState& time_state) = 0;
};

class Graph {
public:
  using Node = IGraphNode;

  void on_tick(const TimeState& time_state) const
  {
    for (const auto& [source, v] : map) {
      source->on_tick(time_state);
    }
  }

  const f64 sample(const TimeState& time_state, const Node& sink) const
  {
    return sink.sample(
      *this, time_state, map.at(const_cast<Node* const>(&sink)));
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

class Oscillator : public IGraphNode {
public:
  Oscillator(const f64 frequency) : frequency(frequency) {}

  void set_frequency(
    const f64 target_value,
    const TimeState& time_state,
    const f64 interpolation_duration,
    const Interpolation interpolation)
  {
    frequency.set(
      target_value, time_state, interpolation_duration, interpolation);
  }

  const f64
  sample(const Graph&, const TimeState& time_state, const Inputs&) const
  {
    // TODO (feat): more waveform types
    return sin(frequency.get(time_state) * phase * TWOPI);
  }

  void on_tick(const TimeState& time_state)
  {
    phase += time_state.get_scaled_delta_time();
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

  const f64 sample(
    const Graph& graph, const TimeState& time_state, const Inputs& inputs) const
  {
    f64 input = .0;
    for (const auto* node : inputs) {
      input += graph.sample(time_state, *node);
    }
    return input * gain.get(time_state);
  }

  void on_tick(const TimeState& time_state) {}

private:
  Interpolated gain;
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
  TimeState time_state;
  Storage<Oscillator> oscillator = {graph};
  Storage<Gain> gain = {graph};

  void tick()
  {
    time_state.tick();
    graph.on_tick(time_state);
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
      std::cout << context.graph.sample(context.time_state, gain) << std::endl;

      if (i == 50) {
        osc.set_frequency(882., context.time_state, .0, Interpolation::None);
      }

      context.tick();
    }
  }
};

#endif
