#pragma once
#include <iostream>
#include <memory>

#include <caf/all.hpp>

#include "next.hpp"

using namespace caf;
using namespace std;

namespace caf_pp {

struct pp_actor : public event_based_actor {
  pp_actor(actor_config &cfg, caf::optional<unique_ptr<Next>> next)
      : event_based_actor(cfg), next_(move(next)) {
    // nop
  }

  template <typename... Args> inline void send_next(Args &&... args) {
    if (next_) {
      next_.value()->send(this, make_message(forward<Args>(args)...));
    }
  }

  inline void send_next(message &&msg) {
    if (next_) {
      next_.value()->send(this, move(msg));
    }
  }

  template <typename... Args> inline void send_at(size_t i, Args &&... args) {
    if (next_) {
      next_.value()->send_at(this, i, make_message(forward<Args>(args)...));
    }
  }

  inline void send_at(size_t i, message &&msg) {
    if (next_) {
      next_.value()->send_at(this, i, move(msg));
    }
  }

  inline caf::optional<const actor &> get_next(message &msg) {
    if (next_) {
      return next_.value()->get_next(msg);
    }
    return caf::optional<const actor &>();
  }

  inline caf::optional<const vector<actor> &> nexts() {
    if (next_) {
      return next_.value()->actors();
    }
    return caf::optional<const vector<actor> &>();
  }

protected:
  caf::optional<unique_ptr<Next>> next_;
};

} // namespace caf_pp
