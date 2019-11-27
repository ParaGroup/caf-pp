#pragma once

#include <iostream>

using namespace caf;
using namespace std;

namespace caf_pp {

struct pp_actor : public event_based_actor {
  pp_actor(actor_config &cfg, caf::optional<actor> next)
      : event_based_actor(cfg), next_(next) {
    // nop
  }

  template <typename... Args> void send_next(Args... args) {
    if (next_) {
      send(next_.value(), forward<Args>(args)...);
    }
  }

private:
  caf::optional<actor> next_;
};

} // namespace caf_pp