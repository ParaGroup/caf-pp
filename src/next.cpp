#include "next.hpp"

#include <caf/all.hpp>

#include "policy2.hpp"

using namespace std;
using namespace caf;

namespace caf_pp {

void Next::send(event_based_actor *a, message &&msg) {
  // if (!nexts_.empty()) {
  auto ret = policy_(nexts_, msg);
  if (ret) {
    a->send(ret.value(), move(msg));
  } else {
    send_all(a, move(msg));
  }
  // }
}
void Next::send_at(event_based_actor *a, size_t i, message &&msg) {
  if (!nexts_.empty()) {
    a->send(nexts_.at(i), move(msg));
  }
}
void Next::send_all(event_based_actor *a, message &&msg) {
  for (const actor &n : nexts_) {
    a->send(n, msg);
  }
}
const vector<actor> &Next::workers() { return nexts_; }
Policy &Next::policy() { return policy_; }

} // namespace caf_pp