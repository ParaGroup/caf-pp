#pragma once

#include <iostream>
#include <typeinfo>

#include "caf/all.hpp"

#include "pp_actor.hpp"

using namespace caf;
using namespace std;

namespace caf_pp {

using SpawnCb = function<void(actor)>;
enum PartitionSched { static_, dynamic_ };

struct Pattern {};

template <typename Actor> struct Seq : public Pattern {

  Seq() {}
  Seq(SpawnCb spawn_cb) : spawn_cb_(spawn_cb) {
    static_assert(is_base_of<pp_actor, Actor>::value,
                  "Actor parameter of Seq must derive from 'pp_actor'");
  }
  // TODO: add constructor that forward parameters
  // read: https://stackoverflow.com/questions/57563594/partial-class-template-argument-deduction-in-c17
  //       https://stackoverflow.com/questions/41833630/c17-class-template-partial-deduction
  // In order to implement the forwarding parameters features it is probabily
  // necessary to implement a method 'make_seq' like the std::tuple class

  caf::optional<SpawnCb> spawn_cb_;
  caf::optional<actor> instance_;
};

template <typename Cont> struct Map : public Pattern {
  // TODO: check that Cont is a container
  using Iter = typename Cont::iterator;
  using MapFunc = function<void(Iter begin, Iter end)>;
  // using MapFunc = function<void(Iter begin, Iter end, const Cont &c)>;
  Map(MapFunc map_fun, PartitionSched sched, uint64_t replicas)
      : map_fun_(map_fun), sched_(sched), replicas_(replicas) {}

  MapFunc map_fun_;
  PartitionSched sched_;
  uint64_t replicas_;
  caf::optional<actor> instance_;
};

template <typename Op, typename Res> struct DivConq : public Pattern {
  using DivFun = function<vector<Op>(Op&)>;
  using MergFun = function<Res(vector<Res>&)>;
  using SeqFun = function<Res(Op&)>;
  using CondFun = function<bool(const Op &)>;
  DivConq(DivFun div_fun, MergFun merg_fun, SeqFun seq_fun, CondFun cond_fun)
    : div_fun_(div_fun), merg_fun_(merg_fun), seq_fun_(seq_fun), cond_fun_(cond_fun) {}

  DivFun div_fun_;
  MergFun merg_fun_;
  SeqFun seq_fun_;
  CondFun cond_fun_;
};

template <typename T> struct Farm : public Pattern {
  Farm(T &stage, actor_pool::policy policy,  uint64_t replicas)
      : stage_(stage), policy_(policy), replicas_(replicas){
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
    // std::cout << std::is_same<T, Pattern>::value <<  "\n";
    static_assert(conjunction_v<is_base_of<Pattern, T>...>,
                  "Type parameter of this class must derive from Pattern");
  }

  tuple<T &...> stages_;
  caf::optional<actor> instance_;
};

} // namespace caf_pp
