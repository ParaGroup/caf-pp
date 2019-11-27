#pragma once

#include "caf/all.hpp"

#include "patterns.hpp"

using namespace caf;

namespace caf_pp {
namespace utils {

template <class... Args>
actor spawn_actor(actor_system &sys, Runtime runtime, Args... args) {
  if (runtime == Runtime::actors) {
    return sys.spawn(std::forward<Args>(args)...);
  } else {
    return sys.spawn<detached>(std::forward<Args>(args)...);
  }
}

template <class T, class... Args>
actor spawn_actor(actor_system &sys, Runtime runtime, Args... args) {
  if (runtime == Runtime::actors) {
    return sys.spawn<T>(std::forward<Args>(args)...);
  } else {
    return sys.spawn<T, detached>(std::forward<Args>(args)...);
  }
}

} // namespace utils
} // namespace caf_pp