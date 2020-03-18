#pragma once
#include <caf/all.hpp>

#include "../pp_actor.hpp"

using namespace caf;

namespace caf_pp {

struct Emitter : public pp_actor {
  Emitter(actor_config &cfg, caf::optional<Next> next) : pp_actor(cfg, next) {
    // nop
  }

  behavior make_behavior() override {
    set_default_handler([=](scheduled_actor *, message_view &msg) -> result<message> {
      // aout(this) << "default handler " << msg.content().stringify() << endl;
      this->send_next(msg.move_content_to_message());
      return sec::none;
    });
    return {[=](){}};
  }
};

} // namespace caf_pp
