#pragma once

#include <caf/all.hpp>

using namespace caf;
using namespace std;

namespace caf_pp {

// TODO: deduct KEY parameter from GetKey function
template <class Key, class GetKey, class Router> struct by_key_policy {
  by_key_policy(GetKey gf, Router rf) : gf_(gf), rf_(rf) {
    // nop
  }
  void operator()(actor_system &, actor_pool::uplock &guard,
                  const actor_pool::actor_vec &vec, mailbox_element_ptr &ptr,
                  execution_unit *host) {
    CAF_ASSERT(!vec.empty());
    type_erased_tuple &tp = ptr->content();
    Key key = gf_(tp);
    size_t hashcode = hf_(key);
    size_t idx = rf_(hashcode, vec.size());
    CAF_ASSERT(idx > 0 && idx < vec.size());
    cout << "[DEBUG] hashcode=" << hashcode << " select=" << idx << endl;
    actor selected = vec[idx];
    guard.unlock();
    selected->enqueue(std::move(ptr), host);
  }
  GetKey gf_;
  hash<Key> hf_;
  Router rf_;
};

template <class Key, class GetKey, class Router>
actor_pool::policy by_key(GetKey gf, Router rf) {
  return by_key_policy<Key, GetKey, Router>{gf, rf};
}

template <class Key, class GetKey> actor_pool::policy by_key(GetKey gf) {
  return by_key<Key>(gf, [](size_t i, size_t n) { return i % n; });
}

} // namespace caf_pp
