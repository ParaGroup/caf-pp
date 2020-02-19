#pragma once

#include <caf/all.hpp>

#include "patterns.hpp"
#include "pp_impl/dac.hpp"
#include "pp_impl/map.hpp"
#include "pp_impl/map2.hpp"
#include "utils/spawn_actor.hpp"

using namespace caf;
using namespace std;

namespace caf_pp {

using namespace pp_impl;
using namespace utils;

template <class T>
caf::optional<actor>
spawn_pattern(actor_system &sys, T &p,
              const caf::optional<actor> &out = caf::optional<actor>()) {
  return _spawn_pattern(sys, p, out, Runtime::actors);
}

template <class T>
caf::optional<actor>
spawn_pattern_with(actor_system &sys, T &p, Runtime m,
                   const caf::optional<actor> &out = caf::optional<actor>()) {
  return _spawn_pattern(sys, p, out, m);
}

template <class T>
caf::optional<actor> _spawn_pattern(actor_system &sys, T &p,
                                    const caf::optional<actor> &out,
                                    Runtime m) {
  static_assert((is_base_of<Pattern, T>::value != 0),
                "Type parameter of this function must derive from Pattern");
  // cout << "[DEBUG] " << "inside TOPFUN spawn" << endl;
  return {};
}

// SPAWN NODE
template <template <class> class P, typename T>
typename std::enable_if<std::is_same<P<T>, Seq<T>>::value,
                        caf::optional<actor>>::type
_spawn_pattern(actor_system &sys, P<T> &p, const caf::optional<actor> &out,
               Runtime m) {
  // cout << "[DEBUG] " << "inside NODE spawn" << endl;
  // actor a = apply([&](auto&&... args) {
  //   return sys.spawn<T>(out, std::forward<decltype(args)>(args)...);
  // }, p.args_);
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  actor a = spawn_actor<T>(sys, runtime, out);
  if (p.spawn_cb_) {
    p.spawn_cb_.value()(a);
  }
  p.instance_ = caf::optional<actor>(a);
  return a;
}

// SPAWN FARM
template <template <class> class P, typename T>
typename std::enable_if<std::is_same<P<T>, Farm<T>>::value,
                        caf::optional<actor>>::type
_spawn_pattern(actor_system &sys, P<T> &p, const caf::optional<actor> &out,
               Runtime m) {
  // cout << "[DEBUG] " << "inside FARM spawn" << endl;
  auto replicas = p.replicas_ ? p.replicas_.value() : 4;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  auto spawn_fun = [&, runtime]() {
    // cout << "[DEBUG] " << "inside actor_pool spawn_fun" << endl;
    return _spawn_pattern(sys, p.stage_, out, runtime).value();
  };
  // cout << "[DEBUG] " << "make actor_pool" << endl;
  auto a = caf::actor_pool::make(sys.dummy_execution_unit(), replicas,
                                 spawn_fun, p.policy_);
  p.instance_ = caf::optional<actor>(a);
  return a;
}

// SPAWN MAP
template <template <typename> typename P, typename Cnt>
typename std::enable_if<std::is_same<P<Cnt>, Map<Cnt>>::value,
                        caf::optional<actor>>::type
_spawn_pattern(actor_system &sys, P<Cnt> &p, const caf::optional<actor> &out,
               Runtime m) {
  // cout << "[DEBUG] " << "inside Map spawn" << endl;
  using namespace PartitionSched;
  using Fnc = typename Map<Cnt>::Fnc;
  actor map;
  auto replicas = p.replicas_ ? p.replicas_.value() : 4;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  if (holds_alternative<static_>(p.sched_)) {
    map = spawn_actor(sys, runtime, map_static_actor<Cnt, Fnc>, p.map_fun_,
                      replicas, out);
  } else {
    auto partition = get<dynamic_>(p.sched_).partition;
    map = spawn_actor(sys, runtime, map_dynamic_actor<Cnt, Fnc>, p.map_fun_,
                      replicas, partition, out);
  }
  p.instance_ = caf::optional<actor>(map);
  return map;
}

// SPAWN MAP2
template <template <typename, typename> typename P, typename CntIn,
          typename CntOut>
typename std::enable_if<
    std::is_same<P<CntIn, CntOut>, Map2<CntIn, CntOut>>::value,
    caf::optional<actor>>::type
_spawn_pattern(actor_system &sys, P<CntIn, CntOut> &p,
               const caf::optional<actor> &out, Runtime m) {
  // cout << "[DEBUG] " << "inside Map spawn" << endl;
  using namespace PartitionSched;
  using Fnc = typename Map2<CntIn, CntOut>::Fnc;
  actor map;
  auto replicas = p.replicas_ ? p.replicas_.value() : 4;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  if (holds_alternative<static_>(p.sched_)) {
    map = spawn_actor(sys, runtime, map2_static_actor<CntIn, CntOut, Fnc>,
                      p.map_fun_, replicas, out);
  } else {
    auto partition = get<dynamic_>(p.sched_).partition;
    map = spawn_actor(sys, runtime, map2_dynamic_actor<CntIn, CntOut, Fnc>,
                      p.map_fun_, replicas, partition, out);
  }
  p.instance_ = caf::optional<actor>(map);
  return map;
}

// SPAWN DIVCONQ
template <template <class> class P, typename Cnt>
typename std::enable_if<std::is_same<P<Cnt>, DivConq<Cnt>>::value,
                        caf::optional<actor>>::type
_spawn_pattern(actor_system &sys, P<Cnt> &p, const caf::optional<actor> &out,
               Runtime) {
  // cout << "[DEBUG] " << "inside DIVCONQ spawn" << endl;
  auto dac = sys.spawn(dac_master_fun<Cnt>, p, out);
  p.instance_ = caf::optional<actor>(dac);
  return dac;
}

// SPAWN FARMROUTER
template <template <class> class P, typename... T>
typename std::enable_if<std::is_same<P<T...>, FarmRouter<T...>>::value,
                        caf::optional<actor>>::type
_spawn_pattern(actor_system &sys, P<T...> &p, const caf::optional<actor> &out,
               Runtime m) {
  cout << "[DEBUG] "
       << "inside FARMROUTER spawn" << endl;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  auto replicas = std::tuple_size<decltype(p.stages_)>::value;
  size_t i(0);
  auto spawn_fun = [&]() {
    auto a = for_index_pattern(i, p.stages_, sys, out, runtime).value();
    i += 1;
    return a;
  };
  auto a = caf::actor_pool::make(sys.dummy_execution_unit(), replicas,
                                 spawn_fun, p.policy_);

  p.instance_ = caf::optional<actor>(a);
  return a;
}

template <std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), caf::optional<actor>>::type
for_index_pattern(size_t, const std::tuple<Tp...> &, actor_system &,
                  const caf::optional<actor> &, const Runtime &) {
  return caf::optional<actor>();
}

template <std::size_t I = 0, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), caf::optional<actor>>::type
    for_index_pattern(size_t i, const std::tuple<Tp...> &tp, actor_system &sys,
                      const caf::optional<actor> &out, const Runtime &m) {
  if (i == 0u) {
    return _spawn_pattern(sys, std::get<I>(tp), out, m);
  } else {
    return for_index_pattern<I + 1, Tp...>(i - 1, tp, sys, out, m);
  }
}

// SPAWN PIPELINE
template <template <class> class P, typename... T>
typename std::enable_if<std::is_same<P<T...>, Pipeline<T...>>::value,
                        caf::optional<actor>>::type
_spawn_pattern(actor_system &sys, P<T...> &p, const caf::optional<actor> &out,
               Runtime m) {
  // cout << "[DEBUG] " << "inside PIPELINE spawn" << endl;
  auto a = for_each_pattern(sys, p.stages_, out, m).value();
  p.instance_ = caf::optional<actor>(a);
  return a;
}

template <size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), caf::optional<actor>>::type
for_each_pattern(actor_system &, const std::tuple<Tp...> &,
                 const caf::optional<actor> &out, const Runtime &) {
  return out;
}

template <size_t I = 0, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), caf::optional<actor>>::type
    for_each_pattern(actor_system &sys, const std::tuple<Tp...> &tp,
                     const caf::optional<actor> &out, const Runtime &m) {
  auto last =
      _spawn_pattern(sys, std::get<(sizeof...(Tp)) - 1 - I>(tp), out, m);
  return for_each_pattern<I + 1, Tp...>(sys, tp, last, m);
}

} // namespace caf_pp
