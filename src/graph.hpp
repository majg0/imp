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
  virtual const f64 sample(const Graph& graph, const Inputs& inputs) const = 0;
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

  const f64 sample(const Node& sink) const
  {
    return sink.sample(*this, map.at(const_cast<Node* const>(&sink)));
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

// class Interpolated {
// public:

//   void on_tick(const Timing& timing) {

//   }

// private:
//   Interpolation interpolation;
//   f64 target_time;
//   f64 started_time;
//   f64 value;
// };

class Timing {
public:
  static constexpr f64 SAMPLE_FREQUENCY = 44100.;
  static constexpr f64 INVERSE_SAMPLE_FREQUENCY = 1. / SAMPLE_FREQUENCY;

  const f64 get_scaled_delta_time() const { return scaled_delta_time; }

  void set_time_scale(const f64 new_time_scale)
  {
    time_scale = new_time_scale;
    scaled_delta_time = time_scale * INVERSE_SAMPLE_FREQUENCY;
  }

  void tick()
  {
    absolute_time += INVERSE_SAMPLE_FREQUENCY;
    scaled_time += scaled_delta_time;
  }

private:
  f64 absolute_time = .0;
  f64 scaled_time = .0;
  f64 scaled_delta_time = INVERSE_SAMPLE_FREQUENCY;
  f64 time_scale = 1.; // TODO (feat): interpolatable
};

class Oscillator : public IGraphNode {
public:
  Oscillator(const f64 frequency) : frequency(frequency) {}

  const f64 sample(const Graph&, const Inputs&) const
  {
    // TODO (feat): more waveform types
    return sin(frequency * phase * TWOPI);
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
  f64 frequency = .0;
};

class Gain : public IGraphNode {
public:
  Gain(f64 gain) : gain(gain) {}

  const f64 sample(const Graph& graph, const Inputs& inputs) const
  {
    f64 input = .0;
    for (const auto* node : inputs) {
      input += graph.sample(*node);
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
      std::cout << context.graph.sample(gain) << std::endl;
      context.tick();
    }
  }
};

#endif
