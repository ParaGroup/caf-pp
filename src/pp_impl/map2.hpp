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

template <class CntIn, class CntOut, class Fnc>
behavior map2_static_worker_actor(event_based_actor *self, Fnc fun_) {
  return {[=](ns_type<CntIn> &ns_c_in, ns_type<CntOut> &ns_c_out, size_t start,
              size_t end) {
    for (auto i = start; i < end; i++) {
      (*ns_c_out)[i] = fun_((*ns_c_in)[i]);
    }
    return ok::value;
  }};
}

struct map2_state {
  std::vector<actor> worker;
};
template <class CntIn, class CntOut, class Fnc>
behavior map2_static_actor(stateful_actor<map2_state> *self, Fnc fun_,
                           uint32_t nw_, caf::optional<Next> out_) {
  for (auto i = 0u; i < nw_; i++) {
    self->state.worker.push_back(
        self->spawn(map2_static_worker_actor<CntIn, CntOut, Fnc>, fun_));
  }

  return {[=](CntIn c) mutable {
    ns_type<CntIn> ns_c_in(std::move(c));
    ns_type<CntOut> ns_c_out(std::move(CntOut(ns_c_in->size())));
    size_t nv = ns_c_in->size();
    size_t chunk = nv / nw_;
    size_t plus = nv % nw_;

    auto promis = self->make_response_promise();
    auto n_res = std::make_shared<size_t>(nw_);
    size_t p = 0;
    for (auto w : self->state.worker) {
      size_t len = chunk;
      if (plus > 0) {
        len += 1;
        plus--;
      }
      size_t pend = p + len;
      self->request(w, caf::infinite, ns_c_in, ns_c_out, p, pend)
          .then([=](ok) mutable {
            if (--(*n_res) == 0) {
              ns_c_in.release();
              CntOut c = ns_c_out.release();
              if (out_) {
                out_.value().send(self, make_message(std::move(c)));
                // promis.deliver(0);
              } else {
                promis.deliver(std::move(c));
              }
            }
          });
      p = pend;
    }
    return promis;
  }};
}

template <class CntIn, class CntOut, class Fnc>
behavior map2_dynamic_worker_actor(event_based_actor *self, Fnc fun_,
                                   size_t partition_,
                                   std::shared_ptr<std::atomic<size_t>> atomic_i_) {
  return {[=](ns_type<CntIn> &ns_c_in, ns_type<CntOut> &ns_c_out) {
    size_t size = ns_c_in->size();
    size_t i;
    while ((i = atomic_i_->fetch_add(partition_)) < size) {
      for (auto j = i; j < i + partition_; j++) {
        (*ns_c_out)[j] = fun_((*ns_c_in)[j]);
      }
    }
    return ok::value;
  }};
}

struct map2_dynamic_state {
  std::vector<actor> worker;
  std::shared_ptr<std::atomic<size_t>> atomic_i;
};

template <class CntIn, class CntOut, class Fnc>
behavior map2_dynamic_actor(stateful_actor<map2_dynamic_state> *self, Fnc fun_,
                            uint32_t nw_, size_t partition_,
                            caf::optional<Next> out_) {
  self->state.atomic_i = std::make_shared<std::atomic<size_t>>(0);
  for (auto i = 0u; i < nw_; i++) {
    self->state.worker.push_back(
        self->spawn(map2_dynamic_worker_actor<CntIn, CntOut, Fnc>, fun_,
                    partition_, self->state.atomic_i));
  }

  return {[=](CntIn c) mutable {
    ns_type<CntIn> ns_c_in(std::move(c));
    ns_type<CntOut> ns_c_out(std::move(CntOut(ns_c_in->size())));
    self->state.atomic_i->store(0);

    auto promis = self->make_response_promise();
    auto n_res = std::make_shared<uint64_t>(nw_);
    for (auto w : self->state.worker) {
      self->request(w, caf::infinite, ns_c_in, ns_c_out).then([=](ok) mutable {
        if (--(*n_res) == 0) {
          ns_c_in.release();
          CntOut c = ns_c_out.release();
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
