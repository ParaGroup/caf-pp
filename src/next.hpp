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
  void send(event_based_actor *a, message &&msg);
  void send_at(event_based_actor *a, size_t i, message &&msg);
  void send_all(event_based_actor *a, message &&msg);
  const vector<actor> &workers();
  Policy &policy();

private:
  vector<actor> nexts_;
  Policy policy_;
};

} // namespace caf_pp