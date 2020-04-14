#pragma once

#include <caf/all.hpp>

#include "../next.hpp"
#include "../pp_impl/emitter.hpp"

using namespace caf;

namespace caf_pp {
namespace utils {

actor next_to_actor(actor_system &sys, Next &&next) {
  if (next.size() == 1) {
    return next.actors()[0];
  } else {
    return sys.spawn<pp_impl::Emitter>(next);
  }
}

} // namespace utils
} // namespace caf_pp
