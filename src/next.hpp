#pragma once

#include <caf/all.hpp>

#include "policy2.hpp"

using namespace std;
using namespace caf;

namespace caf_pp {

struct Next {
  Next(vector<actor> nexts, Policy policy) : nexts_(nexts), policy_(policy) {
    // nop
  }
  Next(vector<actor> nexts) : Next(nexts, RoundRobinPolicy()) {
    // nop
  }
  Next(actor a) : Next(vector<actor>({a})) {
    // nop
  }
  Next(const Next &other) : Next(other.nexts_, other.policy_) {
    // nop
  }
  template <typename... Args> void send(event_based_actor *a, message &&msg) {
    if (!nexts_.empty()) {
      a->send(policy_(nexts_, msg), move(msg));
    }
  }
  void send_at(event_based_actor *a, size_t i, message &&msg) {
    if (!nexts_.empty()) {
      a->send(nexts_.at(i), move(msg));
    }
  }
  const vector<actor> &workers() { return nexts_; }

private:
  vector<actor> nexts_;
  Policy policy_;
};

} // namespace caf_pp