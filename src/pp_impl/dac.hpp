#pragma once

#include "caf/all.hpp"

#include "../utils/ns_type.hpp"

using namespace caf;
using namespace std;

namespace caf_pp {
namespace pp_impl {

using namespace utils;

using up = atom_constant<caf::atom("up")>;
template <typename Res> struct dac_state {
  int num_fragments;
  vector<Res> partial_res;
};
template <typename Op, typename Res>
behavior dac_fun(stateful_actor<dac_state<Res>> *self, actor parent_,
                 function<vector<Op>(Op &)> div_,
                 function<Res(vector<Res> &)> merg_, function<Res(Op &)> seq_,
                 function<bool(const Op &)> cond_) {
  self->state.num_fragments = 0;
  auto send_parent_and_terminate = [=](Res &res) {
    self->send(parent_, up::value, res);
    self->quit();
  };
  return {[=](Op &op) {
            // divide
            auto &s = self->state;
            if (cond_(op)) {
              Res res = seq_(op);
              send_parent_and_terminate(res);
            } else {
              auto ops = div_(op);

              for (auto &op : ops) {
                auto a = self->spawn(dac_fun<Op, Res>, actor_cast<actor>(self),
                                     div_, merg_, seq_, cond_);
                self->send(a, move(op));
                s.num_fragments += 1;
              }
            }
          },
          [=](up, Res &res) {
            // merge
            auto &s = self->state;
            s.num_fragments -= 1;
            s.partial_res.push_back(move(res));
            if (s.num_fragments == 0) {
              Res res = merg_(s.partial_res);
              send_parent_and_terminate(res);
            }
          }};
}
} // namespace pp_impl
} // namespace caf_pp
