#pragma once

#include <iostream>
#include <typeinfo>
#include <variant>

#include "caf/all.hpp"
#include <range/v3/all.hpp>

#include "pp_actor.hpp"

using namespace caf;
using namespace std;

namespace caf_pp {
enum Runtime { threads, actors };

struct Pattern {};

using SpawnCb = function<void(actor)>;
template <typename Actor> struct Seq : public Pattern {

  Seq() {}
  Seq(SpawnCb spawn_cb) : spawn_cb_(spawn_cb) {
    static_assert(is_base_of<pp_actor, Actor>::value,
                  "Actor parameter of Seq must derive from 'pp_actor'");
  }
  // TODO: add constructor that forward parameters
  // read:
  // https://stackoverflow.com/questions/57563594/partial-class-template-argument-deduction-in-c17
  // https://stackoverflow.com/questions/41833630/c17-class-template-partial-deduction
  // In order to implement the forwarding parameters features it is probabily
  // necessary to implement a method 'make_seq' like the std::tuple class

  caf::optional<SpawnCb> spawn_cb_;
  caf::optional<actor> instance_;
};

namespace PartitionSched {
struct static_ {};
struct dynamic_ {
  size_t partition = 1;
};
} // namespace PartitionSched

namespace Replicas {
struct auto_ {};
} // namespace Replicas

template <typename Cnt> struct Map : public Pattern {
  // TODO: check that Cont is a container
  using Itr = typename Cnt::iterator;
  using Rng = ranges::subrange<Itr>;
  using Fnc = function<void(Rng)>;
  using PartitionVar =
      std::variant<PartitionSched::static_, PartitionSched::dynamic_>;
  using ReplicasVar = std::variant<Replicas::auto_, uint32_t>;
  Map(Fnc map_fun)
      : map_fun_(map_fun), sched_(PartitionSched::static_()),
        replicas_(Replicas::auto_()), runtime_(Runtime::actors) {}

  Map<Cnt>& scheduler(PartitionVar sched){
    sched_ = sched;
    return *this;
  };
  Map<Cnt>& replicas(ReplicasVar replicas){
    replicas_ = replicas;
    return *this;
  };
  Map<Cnt>& runtime(Runtime runtime) {
    runtime_ = runtime_;
    return *this;
  };

  Fnc map_fun_;
  PartitionVar sched_;
  ReplicasVar replicas_;
  Runtime runtime_;
  caf::optional<actor> instance_;
};

template <typename Cnt> struct DivConq : public Pattern {
  // TODO: check that Cont is a container
  using Itr = typename Cnt::iterator;
  using Rng = ranges::subrange<Itr>;
  using DivFun = function<vector<Rng>(Rng &)>;
  using MergFun = function<Rng(vector<Rng> &)>;
  using SeqFun = function<Rng(Rng &)>;
  using CondFun = function<bool(Rng &)>;
  DivConq(DivFun div_fun, MergFun merg_fun, SeqFun seq_fun, CondFun cond_fun)
      : div_fun_(div_fun), merg_fun_(merg_fun), seq_fun_(seq_fun),
        cond_fun_(cond_fun) {}

  DivFun div_fun_;
  MergFun merg_fun_;
  SeqFun seq_fun_;
  CondFun cond_fun_;
  caf::optional<actor> instance_;
};

template <typename T> struct Farm : public Pattern {
  Farm(T &stage, actor_pool::policy policy, uint64_t replicas)
      : stage_(stage), policy_(policy), replicas_(replicas) {
    static_assert(is_base_of<Pattern, T>::value,
                  "Type parameter of this class must derive from Pattern");
  }

  T &stage_;
  actor_pool::policy policy_;
  uint64_t replicas_;
  caf::optional<actor> instance_;
};

template <class... T> struct Pipeline : public Pattern {
  Pipeline(T &... stages) : stages_(stages...) {
    static_assert(conjunction_v<is_base_of<Pattern, T>...>,
                  "Type parameter of this class must derive from Pattern");
  }

  tuple<T &...> stages_;
  caf::optional<actor> instance_;
};

} // namespace caf_pp
