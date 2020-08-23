#pragma once
#include <caf/all.hpp>
#include <iostream>
#include <memory>
#include <typeinfo>

using namespace caf;
using namespace std;

namespace caf_pp {

struct PolicyBase {
  virtual ~PolicyBase() = default;
  virtual PolicyBase &operator=(const PolicyBase &) = delete;
  virtual std::unique_ptr<PolicyBase> clone() const = 0;
  virtual const std::type_info &type() const = 0;
  virtual caf::optional<const actor &> select(const vector<actor> &nexts,
                                              const message &) = 0;
};

/**
 * Polimorphic value wrapper for PolicyBase
 */
struct Policy {
  template <typename T, typename = enable_if_t<is_base_of_v<PolicyBase, T>>>
  Policy(T obj) : ptr_(make_unique<T>(move(obj))) {}
  Policy(const Policy &other) : ptr_{other->clone()} {}
  Policy(Policy &&other) : ptr_{other.ptr_.release()} {}

  PolicyBase *operator->() const { return ptr_.get(); }
  PolicyBase &operator*() const { return *ptr_; }

  Policy &operator=(const Policy &other) {
    ptr_ = other->clone();
    return *this;
  }

  // cast the underlying type at run-time
  template <typename T> T &cast() { return dynamic_cast<T &>(*ptr_); }
  template <typename T> const T &cast() const {
    return dynamic_cast<T &>(*ptr_);
  }

private:
  std::unique_ptr<PolicyBase> ptr_;
};

struct RoundRobinPolicy : public PolicyBase {
  RoundRobinPolicy() : PolicyBase{}, index_{0} {
    // nop
  }
  RoundRobinPolicy(const RoundRobinPolicy &) : RoundRobinPolicy{} {
    // nop
  }
  unique_ptr<PolicyBase> clone() const override {
    return make_unique<RoundRobinPolicy>();
  }
  const std::type_info &type() const override { return typeid(*this); }

  caf::optional<const actor &> select(const vector<actor> &nexts,
                                      const message &) {
    // cout << "[DEBUG] " << "PolicyBase RoundRobinPolicy called with " <<
    // msg.content().stringify() << endl;
    const actor &next = nexts[index_ % nexts.size()];
    index_ += 1;
    return next;
  }

private:
  size_t index_;
};

struct BroadcastPolicy : public PolicyBase {
  BroadcastPolicy() : PolicyBase{} {
    // nop
  }
  BroadcastPolicy(const BroadcastPolicy &) : BroadcastPolicy{} {
    // nop
  }
  unique_ptr<PolicyBase> clone() const override {
    return make_unique<BroadcastPolicy>();
  }
  const std::type_info &type() const override { return typeid(*this); }

  caf::optional<const actor &> select(const vector<actor> &, const message &) {
    return caf::optional<const actor &>();
  }
};

template <typename T> using GetKey = function<const T &(const message &)>;
using KeyRouter = function<size_t(size_t, size_t)>;
template <typename T> struct ByKeyPolicy : public PolicyBase {
  ByKeyPolicy(GetKey<T> get_key, KeyRouter router)
      : PolicyBase{}, get_key_{get_key}, router_{router} {
    // nop
  }
  ByKeyPolicy(GetKey<T> get_key)
      : ByKeyPolicy{get_key, [](size_t i, size_t n) { return i % n; }} {
    // nop
  }
  ByKeyPolicy(const ByKeyPolicy &other)
      : ByKeyPolicy{other.get_key_, other.router_} {
    // nop
  }
  unique_ptr<PolicyBase> clone() const override {
    return make_unique<ByKeyPolicy<T>>(get_key_, router_);
  }
  const std::type_info &type() const override { return typeid(*this); }

  caf::optional<const actor &> select(const vector<actor> &nexts,
                                      const message &msg) {
    // cout << "[DEBUG] " <<"ByKeyPolicy called with " <<
    // msg.content().stringify() << endl;
    const T &key = get_key_(msg);
    size_t hashcode = hash_fun_(key);
    size_t index = router_(hashcode, nexts.size());
    // cout << "[DEBUG] hashcode=" << hashcode << " select=" << idx << endl;
    return nexts[index];
  }

private:
  GetKey<T> get_key_;
  hash<T> hash_fun_;
  KeyRouter router_;
};

} // namespace caf_pp
