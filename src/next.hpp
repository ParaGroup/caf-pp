#pragma once

#include <caf/all.hpp>

#include "policy2.hpp"

using namespace caf;

namespace caf_pp {

struct Next {
  Next(std::vector<actor> nexts, Policy policy) : nexts_(std::move(nexts)), policy_(std::move(policy)) {
    // nop
  }
  Next(std::vector<actor> nexts) : Next(nexts, RoundRobinPolicy()) {
    // nop
  }
  Next(actor a) : Next(std::vector<actor>({a})) {
    // nop
  }
  Next(const Next &other) : Next(other.nexts_, other.policy_) {
    // nop
  }
  Next &operator=(const caf_pp::Next &other) {
    nexts_ = other.nexts_;
    policy_ = other.policy_;
    return *this;
  }

  void send(event_based_actor *a, message &&msg);
  void send(message &&msg);

  inline void send_at(event_based_actor *a, size_t i, message &&msg) {
    a->send(nexts_.at(i), std::move(msg));
  }
  inline caf::optional<const actor &> get_next(const message &msg) {
    return policy_->select(nexts_, msg);
  }
  inline const std::vector<actor> &actors() { return nexts_; }
  inline size_t size() { return nexts_.size(); }
  inline Policy &policy() { return policy_; }

private:
  std::vector<actor> nexts_;
  Policy policy_;
};

} // namespace caf_pp