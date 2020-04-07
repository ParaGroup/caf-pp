#pragma once
#include <iostream>

#include <caf/all.hpp>

using namespace caf;
using namespace std;

namespace caf_pp {
using Policy = function<caf::optional<size_t>(size_t, const message &)>;

struct RoundRobinPolicy {
  RoundRobinPolicy() : index_(0) {
    // nop
  }
  RoundRobinPolicy(const RoundRobinPolicy &) : RoundRobinPolicy() {
    // nop
  }
  caf::optional<size_t> operator()(size_t num, const message &) {
    // cout << "[DEBUG] " << "Policy RoundRobinPolicy called with " <<
    // msg.content().stringify() << endl;
    auto i = index_ % num;
    index_ += 1;
    return i;
  }

private:
  size_t index_;
};

struct BroadcastPolicy {
  BroadcastPolicy() {
    // nop
  }
  BroadcastPolicy(const BroadcastPolicy &) : BroadcastPolicy() {
    // nop
  }
  caf::optional<size_t> operator()(size_t, const message &) {
    return caf::optional<size_t>();
  }
};

template <typename T> using GetKey = function<const T &(const message &)>;
using KeyRouter = function<size_t(size_t, size_t)>;
template <typename T> struct ByKeyPolicy {
  ByKeyPolicy(GetKey<T> get_key, KeyRouter router)
      : get_key_(get_key), router_(router) {
    // nop
  }
  ByKeyPolicy(GetKey<T> get_key)
      : ByKeyPolicy(get_key, [](size_t i, size_t n) { return i % n; }) {
    // nop
  }
  ByKeyPolicy(const ByKeyPolicy &other)
      : ByKeyPolicy(other.get_key_, other.router_) {
    // nop
  }
  caf::optional<size_t> operator()(size_t num, const message &msg) {
    // cout << "[DEBUG] " <<"ByKeyPolicy called with " <<
    // msg.content().stringify() << endl;
    const T &key = get_key_(msg);
    size_t hashcode = hash_fun_(key);
    size_t index = router_(hashcode, num);
    // cout << "[DEBUG] hashcode=" << hashcode << " select=" << idx << endl;
    return index;
  }

private:
  GetKey<T> get_key_;
  hash<T> hash_fun_;
  KeyRouter router_;
};

} // namespace caf_pp
