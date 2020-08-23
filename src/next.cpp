#include "next.hpp"

namespace caf_pp {

void Next::send(event_based_actor *a, message &&msg) {
  auto ret = policy_->select(nexts_, msg);
  if (ret) {
    a->send(ret.value(), move(msg));
  } else {
    for (const actor &n : nexts_) {
      a->send(n, msg);
    }
  }
}

void Next::send(message &&msg) {
  auto ret = policy_->select(nexts_, msg);
  if (ret) {
    anon_send(ret.value(), move(msg));
  } else {
    for (const actor &n : nexts_) {
      anon_send(n, msg);
    }
  }
}

} // namespace caf_pp