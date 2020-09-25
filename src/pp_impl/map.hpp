#pragma once

#include <caf/all.hpp>
#include <range/v3/all.hpp>

#include "../utils/ns_type.hpp"
#include "../next.hpp"

using namespace caf;
namespace caf_pp {
namespace pp_impl {

using namespace utils;

using ok = atom_constant<caf::atom("ok")>;

template <class Cnt, class Fnc>
behavior map_static_worker_actor(event_based_actor *, Fnc fun_) {
  return {[=](ns_type<Cnt> &ns_c, size_t start, size_t end) {
    // if (__verbose__)
    //   caf::aout(self) << "actor" << self->id() << "_ (" <<
    //   con_to_string(*ns_c)
    //                   << ") [" << start << ":" << end << "]" << endl;
    auto r = ranges::views::slice(*ns_c, start, end);
    fun_(r);
    return ok::value;
  }};
}

struct map_state {
  std::vector<actor> worker;
};
template <class Cnt, class Fnc>
behavior map_static_actor(stateful_actor<map_state> *self, Fnc fun_,
                          uint32_t nw_, caf::optional<Next> out_) {
  for (auto i = 0u; i < nw_; i++) {
    self->state.worker.push_back(
        self->spawn(map_static_worker_actor<Cnt, Fnc>, fun_));
  }

  return {[=](Cnt c) mutable {
    // if (__verbose__)
    //   caf::aout(self) << "map_static_actor_ (" << con_to_string(c) << ")" <<
    //   endl;
    ns_type<Cnt> ns_c(std::move(c));

    size_t nv = ns_c->size();
    size_t chunk = nv / nw_;
    size_t plus = nv % nw_;

    auto promis = self->make_response_promise();
    auto n_res = std::make_shared<size_t>(nw_);
    auto update_cb = [=](ok) mutable {
      if (--(*n_res) == 0) {
        Cnt c = ns_c.release();
        if (out_) {
          out_.value().send(self, make_message(std::move(c)));
          promis.deliver(0);
        } else {
          promis.deliver(std::move(c));
        }
      }
    };

    size_t p = 0;
    for (size_t iw = 0; iw < nw_; iw++) {
      size_t len = chunk;
      if (plus > 0) {
        len += 1;
        plus--;
      }
      size_t pend = p + len;
      self->request(self->state.worker[iw], caf::infinite, ns_c, p, pend)
          .then(update_cb);
      p = pend;
    }
    return promis;
  }};
}

template <class Cnt, class Fnc>
behavior map_dynamic_worker_actor(event_based_actor *, Fnc fun_,
                                  size_t partition_,
                                  std::shared_ptr<std::atomic<size_t>> atomic_i_) {
  return {[=](ns_type<Cnt> &ns_c) {
    size_t i;
    size_t size = ns_c->size();
    while ((i = atomic_i_->fetch_add(partition_)) < size) {
      auto r = ranges::views::slice(*ns_c, i, i + partition_);
      fun_(r);
    }
    return ok::value;
  }};
}

struct map_dynamic_state {
  std::vector<actor> worker;
  std::shared_ptr<std::atomic<size_t>> atomic_i;
};
template <class Cnt, class Fnc>
behavior map_dynamic_actor(stateful_actor<map_dynamic_state> *self, Fnc fun_,
                           uint32_t nw_, size_t partition_,
                           caf::optional<Next> out_) {
  self->state.atomic_i = std::make_shared<std::atomic<size_t>>(0);
  for (auto i = 0u; i < nw_; i++) {
    self->state.worker.push_back(self->spawn(map_dynamic_worker_actor<Cnt, Fnc>,
                                             fun_, partition_,
                                             self->state.atomic_i));
  }

  return {[=](Cnt c) mutable {
    ns_type<Cnt> ns_c(std::move(c));
    self->state.atomic_i->store(0);

    auto promis = self->make_response_promise();
    auto n_res = std::make_shared<uint64_t>(nw_);
    for (auto w : self->state.worker) {
      self->request(w, caf::infinite, ns_c).then([=](ok) mutable {
        if (--(*n_res) == 0) {
          Cnt c = ns_c.release();
          if (out_) {
            out_.value().send(self, make_message(std::move(c)));
          } else {
            promis.deliver(std::move(c));
          }
        }
      });
    }
    return promis;
  }};
}
} // namespace pp_impl
} // namespace caf_pp
