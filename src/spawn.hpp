#pragma once

#include <caf/all.hpp>

#include "patterns.hpp"
#include "next.hpp"
#include "pp_impl/dac.hpp"
#include "pp_impl/emitter.hpp"
#include "pp_impl/map.hpp"
#include "pp_impl/map2.hpp"
#include "utils/next_tools.hpp"
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
  return spawn_pattern_with(sys, p, Runtime::actors, out);
}

template <class T>
caf::optional<actor>
spawn_pattern_with(actor_system &sys, T &p, Runtime m,
                   const caf::optional<actor> &out = caf::optional<actor>()) {
  caf::optional<Next> n = out ? Next(out.value()) : caf::optional<Next>();
  return next_to_actor(sys, _spawn_pattern(sys, p, n, m));
}

template <class T>
Next _spawn_pattern(actor_system &sys, T &p, caf::optional<Next> next,
                    Runtime m) {
  static_assert((is_base_of<Pattern, T>::value != 0),
                "Type parameter of this function must derive from Pattern");
  cout << "[DEBUG] "
       << "inside TOPFUN spawn" << endl;
  return next.value();
}

// SPAWN NODE
template <template <class> class P, typename T>
typename std::enable_if<std::is_same<P<T>, Seq<T>>::value, Next>::type
_spawn_pattern(actor_system &sys, P<T> &p, caf::optional<Next> next,
               Runtime m) {
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  actor a = spawn_actor<T>(sys, runtime, next);
  if (p.spawn_cb_) {
    p.spawn_cb_.value()(a);
  }
  p.instance_ = Next(a);
  return p.instance_.value();
}

// SPAWN FARM
template <template <class> class P, typename T>
typename std::enable_if<std::is_same<P<T>, Farm<T>>::value, Next>::type
_spawn_pattern(actor_system &sys, P<T> &p, caf::optional<Next> next,
               Runtime m) {
  // cout << "[DEBUG] " << "inside FARM spawn" << endl;
  auto replicas = p.replicas_ ? p.replicas_.value() : 4;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  auto spawn_fun = [&, runtime]() {
    // cout << "[DEBUG] " << "inside actor_pool spawn_fun" << endl;
    return next_to_actor(sys, _spawn_pattern(sys, p.stage_, next, runtime));
  };
  // cout << "[DEBUG] " << "make actor_pool" << endl;
  auto a = caf::actor_pool::make(sys.dummy_execution_unit(), replicas,
                                 spawn_fun, p.policy_);
  p.instance_ = Next(a);
  return a;
}

// SPAWN ALLFARM
template <template <class> class P, typename T>
typename std::enable_if<std::is_same<P<T>, AllFarm<T>>::value, Next>::type
_spawn_pattern(actor_system &sys, P<T> &p, caf::optional<Next> next,
               Runtime m) {
  // cout << "[DEBUG] "
  //      << "inside ALLFARM spawn" << endl;
  auto replicas = p.replicas_ ? p.replicas_.value() : 4;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;

  vector<actor> workers;
  for (auto _ = replicas; _ > 0; _--) {
    auto worker = _spawn_pattern(sys, p.stage_, next, runtime);
    workers.push_back(next_to_actor(sys, move(worker)));
  }
  auto n = Next(workers, p.policy_);
  p.instance_ = n;
  return n;
}

// SPAWN MAP
template <template <typename> typename P, typename Cnt>
typename std::enable_if<std::is_same<P<Cnt>, Map<Cnt>>::value, Next>::type
_spawn_pattern(actor_system &sys, P<Cnt> &p, caf::optional<Next> next,
               Runtime m) {
  // cout << "[DEBUG] " << "inside Map spawn" << endl;
  using namespace PartitionSched;
  using Fnc = typename Map<Cnt>::Fnc;
  actor map;
  auto replicas = p.replicas_ ? p.replicas_.value() : 4;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  if (holds_alternative<static_>(p.sched_)) {
    map = spawn_actor(sys, runtime, map_static_actor<Cnt, Fnc>, p.map_fun_,
                      replicas, next);
  } else {
    auto partition = get<dynamic_>(p.sched_).partition;
    map = spawn_actor(sys, runtime, map_dynamic_actor<Cnt, Fnc>, p.map_fun_,
                      replicas, partition, next);
  }
  p.instance_ = Next(map);
  return p.instance_.value();
}

// SPAWN MAP2
template <template <typename, typename> typename P, typename CntIn,
          typename CntOut>
typename std::enable_if<
    std::is_same<P<CntIn, CntOut>, Map2<CntIn, CntOut>>::value, Next>::type
_spawn_pattern(actor_system &sys, P<CntIn, CntOut> &p, caf::optional<Next> next,
               Runtime m) {
  // cout << "[DEBUG] " << "inside Map spawn" << endl;
  using namespace PartitionSched;
  using Fnc = typename Map2<CntIn, CntOut>::Fnc;
  actor map;
  auto replicas = p.replicas_ ? p.replicas_.value() : 4;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  if (holds_alternative<static_>(p.sched_)) {
    map = spawn_actor(sys, runtime, map2_static_actor<CntIn, CntOut, Fnc>,
                      p.map_fun_, replicas, next);
  } else {
    auto partition = get<dynamic_>(p.sched_).partition;
    map = spawn_actor(sys, runtime, map2_dynamic_actor<CntIn, CntOut, Fnc>,
                      p.map_fun_, replicas, partition, next);
  }
  p.instance_ = Next(map);
  return  p.instance_.value();
}

// SPAWN DIVCONQ
template <template <class> class P, typename Cnt>
typename std::enable_if<std::is_same<P<Cnt>, DivConq<Cnt>>::value, Next>::type
_spawn_pattern(actor_system &sys, P<Cnt> &p, caf::optional<Next> next,
               Runtime) {
  // cout << "[DEBUG] " << "inside DIVCONQ spawn" << endl;
  auto dac = sys.spawn(dac_master_fun<Cnt>, p, next);
  p.instance_ = Next(dac);
  return p.instance_.value();
}

// SPAWN FARMROUTER
template <template <class> class P, typename... T>
typename std::enable_if<std::is_same<P<T...>, FarmRouter<T...>>::value,
                        Next>::type
_spawn_pattern(actor_system &sys, P<T...> &p, caf::optional<Next> next,
               Runtime m) {
  // cout << "[DEBUG] "
  //      << "inside FARMROUTER spawn" << endl;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  auto replicas = std::tuple_size<decltype(p.stages_)>::value;
  size_t i(0);
  auto spawn_fun = [&]() {
    auto a =
        next_to_actor(sys, for_index_pattern(i, p.stages_, sys, next, runtime));
    i += 1;
    return a;
  };
  auto a = caf::actor_pool::make(sys.dummy_execution_unit(), replicas,
                                 spawn_fun, p.policy_);
  p.instance_ = Next(a);
  return p.instance_.value();
}

template <std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), Next>::type
for_index_pattern(size_t, const std::tuple<Tp...> &, actor_system &,
                  caf::optional<Next> next, const Runtime &) {
  return next.value();
}

template <std::size_t I = 0, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), Next>::type
    for_index_pattern(size_t i, const std::tuple<Tp...> &tp, actor_system &sys,
                      caf::optional<Next> next, const Runtime &m) {
  if (i == 0u) {
    return _spawn_pattern(sys, std::get<I>(tp), next, m);
  } else {
    return for_index_pattern<I + 1, Tp...>(i - 1, tp, sys, next, m);
  }
}

// SPAWN PIPELINE
template <template <class> class P, typename... T>
typename std::enable_if<std::is_same<P<T...>, Pipeline<T...>>::value,
                        Next>::type
_spawn_pattern(actor_system &sys, P<T...> &p, caf::optional<Next> next,
               Runtime m) {
  // cout << "[DEBUG] " << "inside PIPELINE spawn" << endl;
  auto a = for_each_pattern(sys, p.stages_, next, m);
  p.instance_ = Next(a);
  return p.instance_.value();
}

template <size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), Next>::type
for_each_pattern(actor_system &, const std::tuple<Tp...> &,
                 caf::optional<Next> next, const Runtime &) {
  return next.value();
}

template <size_t I = 0, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), Next>::type
    for_each_pattern(actor_system &sys, const std::tuple<Tp...> &tp,
                     caf::optional<Next> next, const Runtime &m) {
  auto last =
      _spawn_pattern(sys, std::get<(sizeof...(Tp)) - 1 - I>(tp), next, m);
  return for_each_pattern<I + 1, Tp...>(sys, tp, last, m);
}

} // namespace caf_pp
