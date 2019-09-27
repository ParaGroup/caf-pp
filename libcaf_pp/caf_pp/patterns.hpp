#pragma once

#include <iostream>
#include <typeinfo>

#include "caf/all.hpp"

using namespace caf;
using namespace std;

namespace caf_pp {

using SpawnFunc = function<actor(caf::optional<actor>)>;

struct Pattern {};

struct Node : public Pattern {
  Node(SpawnFunc spawn_fun) : spawn_fun_(spawn_fun) {}

  SpawnFunc spawn_fun_;
  caf::optional<actor> instance_;
};

enum PartitionSched { static_, dynamic_ };
template <typename Cont> struct Map : public Pattern {
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
