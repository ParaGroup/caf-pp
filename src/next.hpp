#pragma once
#include <variant>

#include <caf/all.hpp>
#include <range/v3/all.hpp>

#include "policy2.hpp"

using namespace std;
using namespace caf;
using ranges::view::zip;

namespace caf_pp {

struct Next {
  using ActorVar = std::variant<actor, Next>;

  Next(vector<ActorVar> nexts, Policy policy, size_t batch)
      : nexts_(nexts), policy_(policy), messages_(nexts.size()), batch_(batch) {
    // nop
  }
  Next(vector<ActorVar> nexts, Policy policy) : Next(nexts, policy, 1) {
    // nop
  }
  Next(vector<ActorVar> nexts) : Next(nexts, RoundRobinPolicy()) {
    // nop
  }
  Next(actor a) : Next(vector<ActorVar>({a})) {
    // nop
  }
  ~Next() { flush(); }

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
  void flush(event_based_actor *a);
  void flush();

  inline void send_at(event_based_actor *a, size_t i, message &&msg) {
    auto next = nexts_.at(i);
    if (holds_alternative<Next>(next)) {
      get<Next>(next).send_at(a, i, move(msg));
    } else {
      a->send(get<actor>(next), move(msg));
    }
  }
  caf::optional<const actor &> get_next(const message &msg);
  inline const vector<ActorVar> &actors() { return nexts_; }
  inline size_t size() { return nexts_.size(); }
  inline Policy &policy() { return policy_; }

private:
  void send_no_batch(event_based_actor *a, message &&msg);
  void send_batch(event_based_actor *a, message &&msg);

  inline void send_or_forward(ActorVar &next, event_based_actor *a,
                              message &&msg) {
    if (holds_alternative<Next>(next)) {
      get<Next>(next).send(a, move(msg));
    } else {
      a->send(get<actor>(next), move(msg));
    }
  }

  inline void send_or_forward(ActorVar &next, message &&msg) {
    if (holds_alternative<Next>(next)) {
      get<Next>(next).send(move(msg));
    } else {
      anon_send(get<actor>(next), move(msg));
    }
  }

  vector<ActorVar> nexts_;
  Policy policy_;
  vector<vector<message>> messages_;
  size_t batch_;
};

} // namespace caf_pp