#pragma once

#include <iostream>
#include <typeinfo>

#include "caf/all.hpp"

#include "pp_actor.hpp"

using namespace caf;
using namespace std;

namespace caf_pp {

using SpawnCb = function<void(actor)>;

struct Pattern {};

template <typename Actor> struct Seq : public Pattern {

  Seq() {}
  Seq(SpawnCb spawn_cb) : spawn_cb_(spawn_cb) {
    static_assert(is_base_of<pp_actor, Actor>::value,
                  "Actor parameter of Seq must derive from 'pp_actor'");
  }
  // TODO: add constructor that forward parameters

  caf::optional<SpawnCb> spawn_cb_;
  caf::optional<actor> instance_;
};

enum PartitionSched { static_, dynamic_ };
template <typename Cont> struct Map : public Pattern {
  // TODO: check that Cont is a container
  using Iter = typename Cont::iterator;
  using MapFunc = function<void(Iter begin, Iter end)>;
  // using MapFunc = function<void(Iter begin, Iter end, const Cont &c)>;
  Map(MapFunc map_fun, uint64_t replicas, PartitionSched sched = static_)
      : map_fun_(map_fun), replicas_(replicas), sched_(sched) {}

  MapFunc map_fun_;
  uint64_t replicas_;
  PartitionSched sched_;
  caf::optional<actor> instance_;
};

template <typename T> struct Farm : public Pattern {
  Farm(T &stage, uint64_t replicas, actor_pool::policy policy)
      : stage_(stage), replicas_(replicas), policy_(policy) {
    static_assert(is_base_of<Pattern, T>::value,
                  "Type parameter of this class must derive from Pattern");
  }

  T &stage_;
  uint64_t replicas_;
  actor_pool::policy policy_;
  caf::optional<actor> instance_;
};

template <class... T> struct Pipeline : public Pattern {
  Pipeline(T &... stages) : stages_(stages...) {
    // std::cout << std::is_same<T, Pattern>::value <<  "\n";
    static_assert(conjunction_v<is_base_of<Pattern, T>...>,
                  "Type parameter of this class must derive from Pattern");
  }

  tuple<T &...> stages_;
  caf::optional<actor> instance_;
};

} // namespace caf_pp
