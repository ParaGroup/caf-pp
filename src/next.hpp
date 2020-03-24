#pragma once

#include <caf/all.hpp>
#include <range/v3/all.hpp>

#include "policy2.hpp"

using namespace std;
using namespace caf;
using ranges::view::zip;

namespace caf_pp {

struct Next {
  Next(vector<actor> nexts, Policy policy, size_t batch)
      : nexts_(nexts), policy_(policy), messages_(nexts.size()), batch_(batch) {
    // nop
  }
  Next(vector<actor> nexts, Policy policy) : Next(nexts, policy, 1) {
    // nop
  }
  Next(vector<actor> nexts) : Next(nexts, RoundRobinPolicy()) {
    // nop
  }
  Next(actor a) : Next(vector<actor>({a})) {
    // nop
  }
  ~Next() {
    for (auto &&[n, m] : zip(nexts_, messages_)) {
      if (!m.empty()) {
        anon_send(n, move(m));
      }
    }
  }

  Next(const Next &other) : Next(other.nexts_, other.policy_, other.batch_) {
    // nop
  }
  Next &operator=(const caf_pp::Next &other) {
    nexts_ = other.nexts_;
    policy_ = other.policy_;
    batch_ = other.batch_;
    messages_ = vector<vector<message>>(nexts_.size());
    return *this;
  }

  void send(event_based_actor *a, message &&msg);
  void send(message &&msg);

  inline void send_at(event_based_actor *a, size_t i, message &&msg) {
    a->send(nexts_.at(i), move(msg));
  }
  inline caf::optional<const actor &> get_next(const message &msg) {
    auto ret = policy_(nexts_, msg);
    if (ret) {
      return nexts_[ret.value()];
    } else {
      return caf::optional<const actor &>();
    }
  }
  inline const vector<actor> &actors() { return nexts_; }
  inline size_t size() { return nexts_.size(); }
  inline Policy &policy() { return policy_; }

private:
  void send_no_batch(event_based_actor *a, message &&msg);
  void send_batch(event_based_actor *a, message &&msg);
  
  vector<actor> nexts_;
  Policy policy_;
  vector<vector<message>> messages_;
  size_t batch_;
};

} // namespace caf_pp