#include <algorithm>
#include <optional>
#include <vector>

struct PairFirstLess {
  template <class K, class Vec>
  bool operator()(const std::pair<K, Vec>& lhs, const K& rhs) const
  {
    return lhs.first < rhs;
  }
  template <class K, class Vec>
  bool operator()(const K& lhs, const std::pair<K, Vec>& rhs) const
  {
    return lhs < rhs.first;
  }
};

// template <class K, class V>
// class registry {
// public:
//   template <typename... Args>
//   V& emplace(const K& key, Args&&... args)
//   {
//     return _map
//       .emplace(
//         std::lower_bound(begin(), end(), key, KeyValuePairLess()),
//         std::make_pair(key, V(args...)))
//       ->second;
//   }

//   V& operator[](const K& key)
//   {
//     auto it = std::lower_bound(begin(), end(), key, KeyValuePairLess());
//     if (it != end() && it->first == key) {
//       return it->second;
//     }
//     return _map.emplace(it, std::make_pair(key, V()))->second;
//   }

//   const bool has_component(const K& key) const
//   {
//     auto it = std::lower_bound(cbegin(), cend(), key, KeyValuePairLess());
//     return it != cend() && it->first == key;
//   }

//   void remove_component(const K& key)
//   {
//     _map.erase(
//       std::remove_if(
//         begin(),
//         end(),
//         [&](const auto& pair) -> bool { return pair.first == key; }),
//       end());
//   }

//   auto begin() { return _map.begin(); }
//   auto end() { return _map.end(); }
//   const auto cbegin() { return _map.cbegin(); }
//   const auto cend() { return _map.cend(); }

//   // template <typename Archive>
//   // void archive_impl(Archive& archive)
//   // {
//   //   archive& _map;
//   // }

//   void clear() { _map.clear(); }

// private:
//   std::vector<std::pair<K, V>> _map;
// };

template <class T>
struct Vec3 {
  T x, y, z;

  Vec3() = default;
  Vec3(const T x, const T y, const T z) : x(x), y(y), z(z) {}
};

template <typename Predicate, typename Container>
auto constexpr filter(Predicate predicate, const Container& container)
  -> Container
{
  Container result;
  std::copy_if(
    container.begin(), container.end(), std::back_inserter(result), predicate);
  return result;
}

struct Test {
  void test()
  {
    using namespace std;

    vector<pair<int, Vec3<double>>> reg;

    reg.emplace_back(make_pair(0, Vec3(.12, 1.2, 12.)));
    reg.emplace_back(make_pair(0, Vec3(.32, 3.2, 32.)));
    reg.emplace_back(make_pair(1, Vec3(.42, 3.2, 22.)));
    reg.emplace_back(make_pair(2, Vec3(.52, 3.2, 02.)));

    // std::function<bool(const pair<int, Vec3<double>>&)> p = ;

    auto key0 = [](const auto& kv) { return kv.first == 0; };
    auto y3p2 = [](const auto& kv) { return kv.second.y == 3.2; };

    cout << "a" << endl;
    auto result = filter(key0, reg);
    for (const auto [k, v] : result) {
      cout << k << " " << v.x << " " << v.y << " " << v.z << endl;
    }

    cout << "b" << endl;
    result = filter(y3p2, reg);
    for (const auto [k, v] : result) {
      cout << k << " " << v.x << " " << v.y << " " << v.z << endl;
    }
  }
};
