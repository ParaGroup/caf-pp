#pragma once

#include <caf/all.hpp>
#include <range/v3/all.hpp>

#include "policy2.hpp"

using namespace std;
using namespace caf;
using ranges::view::zip;

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
  virtual Next &operator=(const Next &other) {
    assign(other);
    return *this;
  }

  virtual void send(event_based_actor *a, message &&msg);
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

protected:
  vector<actor> nexts_;
  Policy policy_;
  void assign(const Next &other) {
    nexts_ = other.nexts_;
    policy_ = other.policy_;
  }
};

struct NextBatch : public Next {
  NextBatch(vector<actor> nexts, Policy policy, size_t batch)
      : Next(nexts, policy), messages_(nexts.size()), batch_(batch) {
    // nop
  }
  NextBatch(vector<actor> nexts, Policy policy) : NextBatch(nexts, policy, 1) {
    // nop
  }
  NextBatch(vector<actor> nexts) : NextBatch(nexts, RoundRobinPolicy()) {
    // nop
  }
  NextBatch(actor a) : NextBatch(vector<actor>({a})) {
    // nop
  }
  virtual ~NextBatch() {
    for (auto &&[n, m] : zip(nexts_, messages_)) {
      if (!m.empty()) {
        anon_send(n, move(m));
      }
    }
  }

  NextBatch(const NextBatch &other)
      : NextBatch(other.nexts_, other.policy_, other.batch_) {
    // nop
  }
  virtual NextBatch &operator=(const Next &other) {
    if (const NextBatch *b = dynamic_cast<const NextBatch *>(&other)) {
      assign(*b);
    } else {
      throw std::runtime_error("bad assignement");
    }
    return *this;
  }

  virtual void send(event_based_actor *a, message &&msg) override;

protected:
  vector<vector<message>> messages_;
  size_t batch_;
  void assign(const NextBatch &other) {
    Next::assign(other);
    batch_ = other.batch_;
    messages_ = vector<vector<message>>(nexts_.size());
  }
};

} // namespace caf_pp