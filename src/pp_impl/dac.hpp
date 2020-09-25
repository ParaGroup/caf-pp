#pragma once

#include <caf/all.hpp>
#include <range/v3/all.hpp>

#include "../patterns.hpp"
#include "../utils/ns_type.hpp"
#include "../next.hpp"

using namespace caf;

namespace caf_pp {
namespace pp_impl {

using namespace utils;

using up = atom_constant<caf::atom("up")>;
using down = atom_constant<caf::atom("down")>;

template <typename Cnt> struct dac_state {
  using Itr = typename Cnt::iterator;
  using Rng = ranges::subrange<Itr>;
  int num_fragments;
  std::vector<Rng> partial_res;
};
template <typename Cnt>
behavior dac_worker_fun(stateful_actor<dac_state<Cnt>> *self, uint32_t id_,
                        DivConq<Cnt> p_, actor parent_) {
  self->state.num_fragments = 0;
  using Itr = typename Cnt::iterator;
  using Rng = ranges::subrange<Itr>;
  // utility functions
  auto create_range = [=](ns_type<Cnt> &ns_c, size_t start, size_t end) -> Rng {
    assert(start >= 0 && end <= ns_c->size());
    return *ns_c | ranges::views::slice(start, end);
  };
  auto create_index = [=](ns_type<Cnt> &ns_c,
                          Rng range) -> std::tuple<size_t, size_t> {
    size_t start = range.begin() - ns_c->begin();
    size_t end = range.end() - ns_c->begin();
    return {start, end};
  };
  auto send_parent_and_terminate = [=](ns_type<Cnt> &ns_c, Rng &res) {
    std::tuple<size_t, size_t> t = create_index(ns_c, res);
    self->send(parent_, up::value, id_, ns_c, size_t(get<0>(t)),
               size_t(get<1>(t)));
    self->quit();
  };
  return {[=](down, ns_type<Cnt> &ns_c, size_t start, size_t end) {
            // divide
            auto &s = self->state;
            Rng op = create_range(ns_c, start, end);
            if (p_.cond_fun_(op)) {
              Rng res = p_.seq_fun_(op);
              send_parent_and_terminate(ns_c, res);
            } else {
              auto ops = p_.div_fun_(op);
              s.partial_res.resize(ops.size());
              int32_t id = 0;
              for (auto &op : ops) {
                auto a = self->spawn(dac_worker_fun<Cnt>, id, p_,
                                     actor_cast<actor>(self));
                auto t = create_index(ns_c, op);
                self->send(a, down::value, ns_c, get<0>(t), get<1>(t));
                s.num_fragments += 1;
                id += 1;
              }
            }
          },
          [=](up, uint32_t id, ns_type<Cnt> &ns_c, size_t start, size_t end) {
            // merge
            auto &s = self->state;
            s.num_fragments -= 1;
            auto res = create_range(ns_c, start, end);
            s.partial_res[id] = std::move(res);
            if (s.num_fragments == 0) {
              Rng res = p_.merg_fun_(s.partial_res);
              send_parent_and_terminate(ns_c, res);
            }
          }};
}

struct dac_master_state {
  response_promise promis;
};
template <typename Cnt>
behavior dac_master_fun(stateful_actor<dac_master_state> *self, DivConq<Cnt> p_,
                        caf::optional<Next> out_) {
  return {[=](Cnt &c) mutable {
            // divide
            ns_type<Cnt> ns_c(std::move(c));

            auto a = self->spawn(dac_worker_fun<Cnt>, 0, p_,
                                 actor_cast<actor>(self));

            self->state.promis = self->make_response_promise();
            self->send(a, down::value, ns_c, size_t(0), ns_c->size());
            return self->state.promis;
          },
          [=](up, uint32_t, ns_type<Cnt> ns_c, size_t, size_t) mutable {
            Cnt c = ns_c.release();
            if (out_) {
              out_.value().send(self, make_message(std::move(c)));
              self->state.promis.deliver(0);
            } else {
              self->state.promis.deliver(std::move(c));
            }
          }};
}
} // namespace pp_impl
} // namespace caf_pp
