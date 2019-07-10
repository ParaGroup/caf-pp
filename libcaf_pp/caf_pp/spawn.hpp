#pragma once

#include "caf/all.hpp"

#include "patterns.hpp"

using namespace caf;
using namespace std;

namespace caf_pp {

template <class T>
caf::optional<actor> spawn_pattern(actor_system &sys, T &p,
                                   const caf::optional<actor> &out) {
  static_assert((is_base_of<Pattern, T>::value != 0),
                "Type parameter of this function must derive from Pattern");
  return {};
}

template <>
caf::optional<actor> spawn_pattern(actor_system &sys, Node &p,
                                   const caf::optional<actor> &out) {
  // is node
  cout << "[DEBUG] "
       << "inside NODE spawn" << endl;
  actor a = p.spawn_fun_(out);
  p.instance_ = caf::optional<actor>(a);
  return a;
}

template <template <class> class Farm, typename T>
caf::optional<actor> spawn_pattern(actor_system &sys, Farm<T> &p,
                                   const caf::optional<actor> &out) {
  // is Farm
  cout << "[DEBUG] "
       << "inside FARM spawn" << endl;
  auto spawn_fun = [&]() {
    cout << "[DEBUG] "
         << "inside actor_pool spawn_fun" << endl;
    return spawn_pattern(sys, p.stage_, out).value();
  };
  cout << "[DEBUG] "
       << "make actor_pool" << endl;
  auto a = caf::actor_pool::make(sys.dummy_execution_unit(), p.replicas_,
                                 spawn_fun, p.policy_);
  p.instance_ = caf::optional<actor>(a);
  return a;
}

template <template <class> class Pipeline, typename... T>
caf::optional<actor> spawn_pattern(actor_system &sys, Pipeline<T...> &p,
                                   const caf::optional<actor> &out) {
  // is Pipeline
  cout << "[DEBUG] "
       << "inside PIPELINE spawn" << endl;
  auto a = for_each_tuple(sys, p.stages_, out).value();
  p.instance_ = caf::optional<actor>(a);
  return a;
}

template <size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), caf::optional<actor>>::type
for_each_tuple(actor_system &sys, std::tuple<Tp...> &t,
               const caf::optional<actor> &out) {
  return out;
}

template <size_t I = 0, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), caf::optional<actor>>::type
    for_each_tuple(actor_system &sys, std::tuple<Tp...> &t,
                   const caf::optional<actor> &out) {
  auto last = spawn_pattern(sys, std::get<(sizeof...(Tp)) - 1 - I>(t), out);
  return for_each_tuple<I + 1, Tp...>(sys, t, caf::optional<actor>(last));
}

} // namespace caf_pp
