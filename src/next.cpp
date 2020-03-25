#include "next.hpp"

#include "policy2.hpp"

namespace caf_pp {

void Next::send(event_based_actor *a, message &&msg) {
  auto ret = policy_(nexts_, msg);
  if (ret) {
    a->send(nexts_[ret.value()], move(msg));
  } else {
    for (const actor &n : nexts_) {
      a->send(n, msg);
    }
  }
}

void Next::send(message &&msg) {
  auto ret = policy_(nexts_, msg);
  if (ret) {
    anon_send(nexts_[ret.value()], move(msg));
  } else {
    for (const actor &n : nexts_) {
      anon_send(n, msg);
    }
  }
}

void NextBatch::send(event_based_actor *a, message &&msg) {
  auto ret = policy_(nexts_, msg);
  if (ret) {
    size_t index = ret.value();
    messages_[index].push_back(move(msg));
    if (messages_[index].size() == batch_) {
      a->send(nexts_[index], move(messages_[index]));
      messages_[index] = vector<message>();
    }
  } else {
    for (auto &&[m, n] : zip(messages_, nexts_)) {
      m.push_back(msg);
      if (m.size() == batch_) {
        a->send(n, move(m));
        m = vector<message>();
      }
    }
  }
}

} // namespace caf_pp