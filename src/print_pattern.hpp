#pragma once
#include <iostream>
#include <typeinfo>

#include "patterns.hpp"

using namespace caf;
using namespace std;

namespace caf_pp {

// PRINT SEQ
template <typename T> inline ostream &operator<<(ostream &o, Seq<T> &) {
  return o << "Seq[" << typeid(T).name() << "]";
}

// SPAWN FARM
template <typename T> inline ostream &operator<<(ostream &o, Farm<T> &p) {
  return o << "Farm[" << p.replicas_.value_or(0) << "](" << p.stage_ << ")";
}

// SPAWN ALLFARM
template <typename T> inline ostream &operator<<(ostream &o, AllFarm<T> &p) {
  // TODO: print policy
  return o << "AllFarm[" << p.replicas_.value_or(0) << "](" << p.stage_ << ")";
}

// SPAWN MAP
template <typename T> inline ostream &operator<<(ostream &o, Map<T> &p) {
  return o << "Map[" << p.replicas_.value_or(0) << "," << p.sched_ << ","
           << typeid(T).name() << "]";
}

// SPAWN MAP2
template <typename T1, typename T2>
inline ostream &operator<<(ostream &o, Map2<T1, T2> &p) {
  return o << "Map2[" << p.replicas_.value_or(0) << "," << p.sched_ << ","
           << typeid(T1).name() << "," << typeid(T2).name() << "]";
}

// SPAWN DIVCONQ
template <typename T> inline ostream &operator<<(ostream &o, DivConq<T> &) {
  return o << "DivConq[" << typeid(T).name() << "]";
}

// SPAWN FARMROUTER
template <typename... T>
inline ostream &operator<<(ostream &o, FarmRouter<T...> &p) {
  o << "FarmRouter(";
  print_pattern_tuple(o, p.stages_);
  return o << ")";
}

// SPAWN PIPELINE
template <typename... T>
inline ostream &operator<<(ostream &o, Pipeline<T...> &p) {
  o << "Pipeline(";
  print_pattern_tuple(o, p.stages_);
  return o << ")";
}

template <size_t I = 0, typename... T>
inline typename std::enable_if<(I >= sizeof...(T))>::type
print_pattern_tuple(ostream &, const std::tuple<T...> &) {}

template <size_t I = 0, typename... T>
inline typename std::enable_if<(I<sizeof...(T) && I> 0)>::type
print_pattern_tuple(ostream &o, const std::tuple<T...> &tp) {
  o << ", " << std::get<I>(tp);
  print_pattern_tuple<I + 1>(o, tp);
}

template <size_t I = 0, typename... T>
inline typename std::enable_if<(I == 0)>::type
print_pattern_tuple(ostream &o, const std::tuple<T...> &tp) {
  o << std::get<I>(tp);
  print_pattern_tuple<I + 1>(o, tp);
}

} // namespace caf_pp