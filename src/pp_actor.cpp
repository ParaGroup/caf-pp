#include "pp_actor.hpp"

namespace caf_pp {

#ifdef SIZED_QUEUE
bool pp_actor::send_next_if(size_t size, message &msg) {
  if (next_) {
    auto &next = next_.value();
    if (next.policy()->type() == typeid(RoundRobinPolicy)) {
      for (auto n = next.size(); n > 0; --n) {
        actor a = next.get_next(msg).value();
        auto qsize = receiver_queue_size(a);
        if (qsize < size) {
          send(a, move(msg));
          return true;
        }
      }
    } else if (next.policy()->type() == typeid(BroadcastPolicy)) {
      next.send(this, move(msg));
      return true;
    } else {
      actor a = next.get_next(msg).value();
      auto qsize = receiver_queue_size(a);
      if (qsize < size) {
        send(a, move(msg));
        return true;
      }
    }
    return false;
  }
  return true;
}
#endif

} // namespace caf_pp