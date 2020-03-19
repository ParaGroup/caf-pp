#pragma once
#include <iostream>

#include <caf/all.hpp>

#include "next.hpp"

using namespace caf;
using namespace std;

namespace caf_pp {

struct pp_actor : public event_based_actor {
  pp_actor(actor_config &cfg, caf::optional<Next> next)
      : event_based_actor(cfg), next_(next) {
    // nop
  }

  template <typename... Args> void send_next(Args&&... args) {
    if (next_) {
      next_.value().send(this, make_message(forward<Args>(args)...));
    }
  }

  template <typename... Args> void send_next(message&& msg) {
    if (next_) {
      next_.value().send(this, move(msg));
    }
  }

  template <typename... Args> void send_at(size_t i, Args... args) {
    if (next_) {
      next_.value().send_at(this, i, make_message(forward<Args>(args)...));
    }
  }

protected:
  caf::optional<Next> next_;
};

} // namespace caf_pp
