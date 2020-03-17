#pragma once

#include <vector>

#include <caf/all.hpp>

using namespace std;
using namespace caf;

namespace caf_pp {

using RoundRobinPolicy = function<actor &(vector<actor> &)>;
RoundRobinPolicy round_robin_factory() {
  auto i = make_shared<size_t>(0);
  return [=](vector<actor> &nexts) -> actor & {
    actor &next = nexts.at(*i % nexts.size());
    *i += 1;
    return next;
  };
}

struct Next {
  Next(vector<actor> nexts) : nexts_(nexts), policy_(round_robin_factory()) {
    // nop
  }
  Next(actor a) : Next(vector<actor>({a})) {
    // nop
  }
  Next(const Next &n) : Next(n.nexts_) {
    // nop
  }
  template <typename... Args> void send(event_based_actor* a, Args... args) {
    if (!nexts_.empty()) {
      a->send(policy_(nexts_), forward<Args>(args)...);
    }
  }
  template <typename... Args>
  void send_at(event_based_actor* a, size_t i, Args... args) {
    if (!nexts_.empty()) {
      a->send(nexts_.at(i), forward<Args>(args)...);
    }
  }
  vector<actor> &workers() { return nexts_; }

private:
  vector<actor> nexts_;
  RoundRobinPolicy policy_;
};

} // namespace caf_pp