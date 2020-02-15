#include <iostream>
#include <vector>

#include "all.hpp"

using namespace std;
using namespace caf;
using namespace caf_pp;
using namespace ranges;

template <class T> string con_to_string(const T &c) {
  string res{""};
  size_t i{0};
  for (auto el : c) {
    res += to_string(el) + (i < c.size() - 1 ? " " : "");
    i++;
  }
  return res;
}

struct config : actor_system_config {
  config() { opt_group{custom_options_, "global"}; }
};

void caf_main(actor_system &sys, const config &cfg) {
  cout << "CAF_VERSION=" << CAF_VERSION << endl;
  scoped_actor self{sys};

  using Cnt = vector<int64_t>;
  using Itr = typename Cnt::iterator;
  using Rng = subrange<Itr>;
  auto div = [](Rng &op) -> vector<Rng> {
    // divide the input
    size_t last = op.size() - 1;
    auto pivot = op[last];
    size_t i = -1;

    for (size_t j = 0; j < last; j++) {
      if (op[j] <= pivot) {
        i = i + 1;
        auto tmp = op[i];
        op[i] = op[j];
        op[j] = tmp;
      }
    }
    auto tmp = op[i + 1];
    op[i + 1] = op[last];
    op[last] = tmp;

    // pivot is in position i+1
    return {op | views::slice(size_t{0}, i + 1),
            op | views::slice(i + 2, ranges::end)};
  };
  auto merge = [](vector<Rng> &v_res) -> Rng {
    // merge the output
    return {v_res[0].begin(), v_res[1].end()};
  };
  auto seq = [=](Rng &op) -> Rng {
    // sequential version
    if (op.size() > 2) {
      actions::stable_sort(op);
    }
    return op;
  };
  auto cond = [](Rng &op) -> bool {
    // stop recursion and execute sequential
    return op.size() < 2;
  };
  DivConq<Cnt> dac(div, merge, seq, cond);
  auto handle = spawn_pattern(sys, dac).value();

  vector<int64_t> vec{3, 4, 1, 7, 8, 5, 2};
  aout(self) << "main_ (" << con_to_string(vec) << ")" << endl;
  self->request(handle, caf::infinite, std::move(vec))
      .receive(
          [&](vector<int64_t> &vec) {
            aout(self) << "main_ (" << con_to_string(vec) << ")" << endl;
          },
          [&self](error &_) { aout(self) << "error_" << _ << endl; });
}

CAF_MAIN()
