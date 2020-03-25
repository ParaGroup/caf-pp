#pragma once
#include <memory>

#include <caf/all.hpp>

#include "next.hpp"
#include "patterns.hpp"
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

using next_ptr = std::unique_ptr<Next>;

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
  auto n = out ? std::make_unique<Next>(out.value()) : caf::optional<next_ptr>();
  return next_to_actor(sys, _spawn_pattern(sys, p, move(n), m));
}

template <class T>
next_ptr _spawn_pattern(actor_system &sys, T &p, caf::optional<next_ptr> next,
                    Runtime m) {
  static_assert((is_base_of<Pattern, T>::value != 0),
                "Type parameter of this function must derive from Pattern");
  cout << "[DEBUG] "
       << "inside TOPFUN spawn" << endl;
  return next.value();
}

// SPAWN NODE
template <template <class> class P, typename T>
typename std::enable_if<std::is_same<P<T>, Seq<T>>::value, next_ptr>::type
_spawn_pattern(actor_system &sys, P<T> &p, caf::optional<next_ptr> next,
               Runtime m) {
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  actor a = spawn_actor<T>(sys, runtime, move(next));
  if (p.spawn_cb_) {
    p.spawn_cb_.value()(a);
  }
  p.instance_ = std::make_unique<Next>(a);
  return std::make_unique<Next>(a);
}

// SPAWN FARM
template <template <class> class P, typename T>
typename std::enable_if<std::is_same<P<T>, Farm<T>>::value, next_ptr>::type
_spawn_pattern(actor_system &sys, P<T> &p, caf::optional<next_ptr> next,
               Runtime m) {
  // cout << "[DEBUG] " << "inside FARM spawn" << endl;
  auto replicas = p.replicas_ ? p.replicas_.value() : 4;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  auto spawn_fun = [&, runtime]() {
    // cout << "[DEBUG] " << "inside actor_pool spawn_fun" << endl;
    return next_to_actor(sys, move(_spawn_pattern(sys, p.stage_, std::make_unique<Next>(*next), runtime)));
  };
  // cout << "[DEBUG] " << "make actor_pool" << endl;
  auto a = caf::actor_pool::make(sys.dummy_execution_unit(), replicas,
                                 spawn_fun, p.policy_);
  p.instance_ = std::make_unique<Next>(a);
  return std::make_unique<Next>(a);
}

// SPAWN ALLFARM
template <template <class> class P, typename T>
typename std::enable_if<std::is_same<P<T>, AllFarm<T>>::value, next_ptr>::type
_spawn_pattern(actor_system &sys, P<T> &p, caf::optional<next_ptr> next,
               Runtime m) {
  // cout << "[DEBUG] "
  //      << "inside ALLFARM spawn" << endl;
  auto replicas = p.replicas_ ? p.replicas_.value() : 4;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;

  vector<actor> workers;
  for (auto _ = replicas; _ > 0; _--) {
    auto worker = _spawn_pattern(sys, p.stage_, std::make_unique<Next>(*next), runtime);
    workers.push_back(next_to_actor(sys, move(worker)));
  }

  auto n = p.batch_
               ? std::make_unique<NextBatch>(workers, p.policy_, p.batch_.value())
               : std::make_unique<Next>(workers, p.policy_);
  p.instance_ = std::make_unique<Next>(*n);
  return n;
}

// SPAWN MAP
template <template <typename> typename P, typename Cnt>
typename std::enable_if<std::is_same<P<Cnt>, Map<Cnt>>::value, next_ptr>::type
_spawn_pattern(actor_system &sys, P<Cnt> &p, caf::optional<next_ptr> next,
               Runtime m) {
  // cout << "[DEBUG] " << "inside Map spawn" << endl;
  using namespace PartitionSched;
  using Fnc = typename Map<Cnt>::Fnc;
  actor map;
  auto replicas = p.replicas_ ? p.replicas_.value() : 4;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  if (holds_alternative<static_>(p.sched_)) {
    map = spawn_actor(sys, runtime, map_static_actor<Cnt, Fnc>, p.map_fun_,
                      replicas, move(next));
  } else {
    auto partition = get<dynamic_>(p.sched_).partition;
    map = spawn_actor(sys, runtime, map_dynamic_actor<Cnt, Fnc>, p.map_fun_,
                      replicas, partition, move(next));
  }
  p.instance_ = std::make_unique<Next>(map);
  return std::make_unique<Next>(map);
}

// SPAWN MAP2
template <template <typename, typename> typename P, typename CntIn,
          typename CntOut>
typename std::enable_if<
    std::is_same<P<CntIn, CntOut>, Map2<CntIn, CntOut>>::value, next_ptr>::type
_spawn_pattern(actor_system &sys, P<CntIn, CntOut> &p, caf::optional<next_ptr> next,
               Runtime m) {
  // cout << "[DEBUG] " << "inside Map spawn" << endl;
  using namespace PartitionSched;
  using Fnc = typename Map2<CntIn, CntOut>::Fnc;
  actor map;
  auto replicas = p.replicas_ ? p.replicas_.value() : 4;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  if (holds_alternative<static_>(p.sched_)) {
    map = spawn_actor(sys, runtime, map2_static_actor<CntIn, CntOut, Fnc>,
                      p.map_fun_, replicas, move(next));
  } else {
    auto partition = get<dynamic_>(p.sched_).partition;
    map = spawn_actor(sys, runtime, map2_dynamic_actor<CntIn, CntOut, Fnc>,
                      p.map_fun_, replicas, partition, move(next));
  }
  p.instance_ = std::make_unique<Next>(map);
  return std::make_unique<Next>(map);
}

// SPAWN DIVCONQ
template <template <class> class P, typename Cnt>
typename std::enable_if<std::is_same<P<Cnt>, DivConq<Cnt>>::value, next_ptr>::type
_spawn_pattern(actor_system &sys, P<Cnt> &p, caf::optional<next_ptr> next,
               Runtime) {
  // cout << "[DEBUG] " << "inside DIVCONQ spawn" << endl;
  auto dac = sys.spawn(dac_master_fun<Cnt>, p, move(next));
  p.instance_ = std::make_unique<Next>(dac);
  return std::make_unique<Next>(dac);
}

// SPAWN FARMROUTER
template <template <class> class P, typename... T>
typename std::enable_if<std::is_same<P<T...>, FarmRouter<T...>>::value,
                        next_ptr>::type
_spawn_pattern(actor_system &sys, P<T...> &p, caf::optional<next_ptr> next,
               Runtime m) {
  // cout << "[DEBUG] "
  //      << "inside FARMROUTER spawn" << endl;
  auto runtime = p.runtime_ ? p.runtime_.value() : m;
  auto replicas = std::tuple_size<decltype(p.stages_)>::value;
  size_t i(0);
  auto spawn_fun = [&]() {
    auto a =
        next_to_actor(sys, move(for_index_pattern(i, p.stages_, sys, std::make_unique<Next>(*next), runtime)));
    i += 1;
    return a;
  };
  auto a = caf::actor_pool::make(sys.dummy_execution_unit(), replicas,
                                 spawn_fun, p.policy_);
  p.instance_ = std::make_unique<Next>(a);
  return std::make_unique<Next>(a);
}

template <std::size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), next_ptr>::type
for_index_pattern(size_t, const std::tuple<Tp...> &, actor_system &,
                  caf::optional<next_ptr> next, const Runtime &) {
  return next.value();
}

template <std::size_t I = 0, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), next_ptr>::type
    for_index_pattern(size_t i, const std::tuple<Tp...> &tp, actor_system &sys,
                      caf::optional<next_ptr> next, const Runtime &m) {
  if (i == 0u) {
    return _spawn_pattern(sys, std::get<I>(tp), move(next), m);
  } else {
    return for_index_pattern<I + 1, Tp...>(i - 1, tp, sys, move(next), m);
  }
}

// SPAWN PIPELINE
template <template <class> class P, typename... T>
typename std::enable_if<std::is_same<P<T...>, Pipeline<T...>>::value,
                        next_ptr>::type
_spawn_pattern(actor_system &sys, P<T...> &p, caf::optional<next_ptr> next,
               Runtime m) {
  // cout << "[DEBUG] " << "inside PIPELINE spawn" << endl;
  auto a = for_each_pattern(sys, p.stages_, move(next), m);
  p.instance_ = std::make_unique<Next>(*a);
  return move(a);
}

template <size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), next_ptr>::type
for_each_pattern(actor_system &, const std::tuple<Tp...> &,
                 caf::optional<next_ptr> next, const Runtime &) {
  return next.value();
}

template <size_t I = 0, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), next_ptr>::type
    for_each_pattern(actor_system &sys, const std::tuple<Tp...> &tp,
                     caf::optional<next_ptr> next, const Runtime &m) {
  auto last =
      _spawn_pattern(sys, std::get<(sizeof...(Tp)) - 1 - I>(tp), move(next), m);
  return for_each_pattern<I + 1, Tp...>(sys, tp, move(last), m);
}

} // namespace caf_pp
