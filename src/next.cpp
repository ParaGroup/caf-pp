#include "next.hpp"

#include "policy2.hpp"

namespace caf_pp {

void Next::send(event_based_actor *a, message &&msg) {
  // TODO: do this test statically to improve performance
  if (batch_ == 1) {
    send_no_batch(a, move(msg));
  } else {
    send_batch(a, move(msg));
  }
}

void Next::flush(event_based_actor *a) {
  if (batch_ != 1) {
    for (auto &&[n, m] : zip(nexts_, messages_)) {
      if (!m.empty()) {
        send_or_forward(n, a, make_message(move(m)));
      }
    }
  }
}

void Next::flush() {
  if (batch_ != 1) {
    for (auto &&[n, m] : zip(nexts_, messages_)) {
      if (!m.empty()) {
        send_or_forward(n, make_message(move(m)));
      }
    }
  }
}

void Next::send_no_batch(event_based_actor *a, message &&msg) {
  auto ret = policy_(nexts_.size(), msg);
  if (ret) {
    send_or_forward(nexts_[ret.value()], a, move(msg));
  } else {
    for (auto &n : nexts_) {
      send_or_forward(n, a, move(message(msg)));
    }
  }
}

void Next::send_batch(event_based_actor *a, message &&msg) {
  auto ret = policy_(nexts_.size(), msg);
  if (ret) {
    size_t index = ret.value();
    messages_[index].push_back(move(msg));
    if (messages_[index].size() == batch_) {
      send_or_forward(nexts_[index], a, make_message(move(messages_[index])));
      messages_[index] = vector<message>();
    }
  } else {
    for (auto &&[m, n] : zip(messages_, nexts_)) {
      m.push_back(msg);
      if (m.size() == batch_) {
        send_or_forward(n, a, make_message(move(m)));
        m = vector<message>();
      }
    }
  }
}

void Next::send(message &&msg) {
  auto ret = policy_(nexts_.size(), msg);
  if (ret) {
    send_or_forward(nexts_[ret.value()], move(msg));
  } else {
    for (auto &n : nexts_) {
      send_or_forward(n, move(message(msg)));
    }
  }
}

caf::optional<const actor &> Next::get_next(const message &msg) {
  auto ret = policy_(nexts_.size(), msg);
  if (ret) {
    auto next = nexts_[ret.value()];
    if (holds_alternative<Next>(next)) {
      return get<Next>(next).get_next(msg);
    } else {
      return get<actor>(next);
    }
  } else {
    return caf::optional<const actor &>();
  }
}

} // namespace caf_pp