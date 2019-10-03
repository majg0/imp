#ifndef IMP_ECS
#define IMP_ECS

#include "archive.hpp"
#include "constants.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <optional>
#include <type_traits>
#include <vector>

class Id {
public:
  using clock = std::chrono::high_resolution_clock;
  using precision = std::chrono::nanoseconds;
  using time = std::chrono::time_point<clock, precision>;

  Id() = default;
  explicit Id(time created) : _created(created.time_since_epoch().count()) {}

  const time created() const
  {
    return std::chrono::time_point<clock>(precision(_created));
  }
  const i64 raw_created() const { return _created; }

  friend bool operator<(const Id& l, const Id& r)
  {
    return l._created < r._created; // TODO 3. use std::tie to keep order
  }

  bool operator==(const Id& rhs) const { return _created == rhs._created; }

  template <typename T>
  void archive_impl(T& archive)
  {
    archive& _created;
  }

private:
  i64 _created{0};
  // TODO: 1. tests for id collision
  // TODO: 2. random number for uncolliding
};

class IdFactory {
public:
  Id create() { return Id{std::chrono::high_resolution_clock::now()}; }
};

struct PairIdLess {
  template <typename T>
  bool operator()(const std::pair<Id, T>& lhs, const Id& rhs) const
  {
    return lhs.first < rhs;
  }
  template <typename T>
  bool operator()(const Id& lhs, const std::pair<Id, T>& rhs) const
  {
    return lhs < rhs.first;
  }
};

template <typename T>
class ComponentStorage {
public:
  template <typename... Args>
  T& emplace_component(const Id& id, Args&&... args)
  {
    return components
      .emplace(
        std::lower_bound(
          components.begin(), components.end(), id, PairIdLess()),
        std::make_pair(id, T(args...)))
      ->second;
  }

  const std::optional<T&> get_component(const Id& id) const
  {
    auto end = components.end();
    auto it = std::lower_bound(components.begin(), end, id, PairIdLess());
    return it != end && it->first == id ? std::optional<T&>{it->second}
                                        : std::nullopt;
  }

  const bool has_component(const Id& id) const
  {
    auto end = components.end();
    auto it = std::lower_bound(components.begin(), end, id, PairIdLess());
    return it != end && it->first == id;
  }

  void remove_component(const Id& id)
  {
    auto end = components.end();
    components.erase(
      std::remove_if(
        components.begin(),
        end,
        [&](const auto& pair) -> bool { return pair.first == id; }),
      end);
  }

  const auto iterator_pair()
  {
    return std::make_pair(components.begin(), components.end());
  }

  template <typename Archive>
  void archive_impl(Archive& archive)
  {
    archive& components;
  }

  void clear() { components.clear(); }

private:
  std::vector<std::pair<Id, T>> components;
};

template <typename... Types>
struct SystemInput {
  using Components = std::tuple<Types...>;
  using Args = std::tuple<Types&...>;
};

template <class StorageTuple, class System>
struct SystemUpdater {
  using Components = typename System::Input::Components;

  SystemUpdater(System system) : system(system) {}

  template <typename ECS, typename... Types>
  void update_impl(const ECS& ecs, StorageTuple& storages, std::tuple<Types...>)
  {
    update_intersection(
      ecs, std::get<ComponentStorage<Types>>(storages).iterator_pair()...);
  }

  template <typename ECS>
  void update(const ECS& ecs, StorageTuple& storages)
  {
    // TODO (fix): achieve the same effect without constructing Components{}?
    update_impl(ecs, storages, Components{});
  }

private:
  template <typename ECS, typename It1, typename... Its>
  void update_intersection(const ECS& ecs, It1 it1, Its... its)
  {
    while (it1.first != it1.second && ((its.first != its.second) && ...)) {
      if (((it1.first->first < its.first->first) || ...)) {
        ++it1.first;
      }
      else if ((first_not_less_else_bump(its, it1) && ...)) {
        system.on_update(
          ecs,
          it1.first->first,
          std::forward_as_tuple(it1.first->second, its.first->second...));
        ++it1.first;
        (++its.first, ...);
      }
    }
  }

  template <typename PairItPair1, typename PairItPair2>
  const bool first_not_less_else_bump(PairItPair1& a, PairItPair2& b)
  {
    if (a.first->first < b.first->first) {
      ++a.first;
      return false;
    }
    return true;
  }

  System system;
};

template <typename... Types>
using Args = std::tuple<Types&...>;

template <typename StorageTuple, typename SystemUpdatersTuple>
struct ECS {
  ECS(SystemUpdatersTuple system_updaters) : system_updaters(system_updaters) {}

  template <typename T>
  ComponentStorage<T>& get_component_storage()
  {
    return std::get<ComponentStorage<T>>(storages);
  }

  void clear_storages()
  {
    std::apply([&](auto&... storage) { (storage.clear(), ...); }, storages);
  }

  void update_systems()
  {
    std::apply(
      [&](auto&... system_updater) {
        ((system_updater.update(*this, storages)), ...);
      },
      system_updaters);
  }

  template <typename Archive>
  void archive_impl(Archive& archive)
  {
    archive& storages;
  }

  IdFactory ids;
  StorageTuple storages;
  SystemUpdatersTuple system_updaters;
};

template <typename StorageTuple, typename... SystemUpdaters>
struct ECSSystemInjector {
  ECSSystemInjector() = default;
  ECSSystemInjector(std::tuple<SystemUpdaters...>&& updaters)
      : updaters(updaters)
  {
  }

  template <typename System, typename... Types>
  ECSSystemInjector<
    StorageTuple,
    SystemUpdaters...,
    SystemUpdater<StorageTuple, System>>
  with_system(Types... args)
  {
    return ECSSystemInjector<
      StorageTuple,
      SystemUpdaters...,
      SystemUpdater<StorageTuple, System>>(std::tuple_cat(
      std::move(updaters),
      std::make_tuple(SystemUpdater<StorageTuple, System>(System(args...)))));
  }

  ECS<StorageTuple, std::tuple<SystemUpdaters...>> construct()
  {
    return ECS<StorageTuple, std::tuple<SystemUpdaters...>>(
      std::move(updaters));
  }

  std::tuple<SystemUpdaters...> updaters;
};

struct ECSBuilder {
  template <typename... Types>
  static ECSSystemInjector<std::tuple<ComponentStorage<Types>...>>
  with_components()
  {
    return ECSSystemInjector<std::tuple<ComponentStorage<Types>...>>();
  }
};

struct AB {
  AB() = default;
  AB(int a, int b) : a(a), b(b) {}

  template <typename Archive>
  void archive_impl(Archive& archive)
  {
    archive& a& b;
  }

  int a = 0;
  int b = 0;
};

struct AddSystem {
  // TODO (feat): Not<T> inputs
  using Input = SystemInput<AB, int, float>;
  // TODO (feat): async execution and system dependencies

  AddSystem(int add) : add(add) {}

  template <typename ECS>
  void on_update(const ECS& ecs, const Id& id, Input::Args args)
  {
    auto& [ab, i, f] = args;
    ab.a += add;
    ab.b += add;
    i += add;
    f += add;
  }

  int add = 0;
};

struct PrintSystem {
  using Input = SystemInput<AB, int, float>;

  template <typename ECS>
  void on_update(const ECS& ecs, const Id& id, Input::Args args)
  {
    auto& [ab, i, f] = args;
    using namespace std;
    cout << "Print System" << endl;
    cout << "id: " << id.raw_created() << endl;
    cout << "a: " << ab.a << endl;
    cout << "b: " << ab.b << endl;
    cout << "i: " << i << endl;
    cout << "f: " << f << endl;
    cout << endl;
  }
};

struct Test {
  void test()
  {
    ECS ecs = ECSBuilder::with_components<AB, int, float>()
                .with_system<AddSystem>(5)
                .with_system<PrintSystem>()
                .construct();

    {
      auto& ids = ecs.ids;
      Id x = ids.create();
      Id y = ids.create();
      Id z = ids.create();
      Id r = ids.create();

      {
        using namespace std;
        cout << "x:" << x.raw_created() << endl;
        cout << "y:" << y.raw_created() << endl;
        cout << "z:" << z.raw_created() << endl;
        cout << "r:" << r.raw_created() << endl;
        cout << endl;
      }

      {
        auto& ab_storage = ecs.get_component_storage<AB>();
        auto& int_storage = ecs.get_component_storage<int>();
        auto& float_storage = ecs.get_component_storage<float>();

        ab_storage.emplace_component(x, 1, 2);
        ab_storage.emplace_component(y, 4, 5);
        ab_storage.emplace_component(r, 6, 7);

        int_storage.emplace_component(x, 8);
        int_storage.emplace_component(z, 9);
        int_storage.emplace_component(r, 10);

        float_storage.emplace_component(r, 11.5);
      }
    }

    {
      using namespace std;

      auto w = ofstream("save.imp", ios::binary);
      auto archive = Archive(w);

      archive << ecs;
    }

    ecs.update_systems();
    ecs.update_systems();
    ecs.update_systems();

    {
      using namespace std;

      auto r = std::ifstream("save.imp", std::ios::binary);
      auto archive = Archive(r);

      try {
        ecs.clear_storages();
        archive >> ecs;
        cout << "loaded state from file\n" << endl;
      }
      catch (runtime_error e) {
        cout << "caught exception: \"" << e.what() << '\"' << endl;
      }
    }

    ecs.update_systems();
  }
};

#endif
