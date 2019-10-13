#include <iostream>
#include <vector>

#include "caf/all.hpp"

#include "patterns.hpp"
#include "spawn.hpp"

using namespace std;
using namespace caf;
using namespace caf_pp;

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

  using Cont = vector<int64_t>;
  using I = Cont::iterator;
  auto div = [](Range<I> &op) -> vector<Range<I>> {
    // divide the input
    auto start = op.begin();
    auto end = op.end();
    long pivot = *(start + (distance(start, end) / 2));
    auto i = start - 1, j = end + 1;
    long tmp;

    while (true) {
      do {
        i++;
      } while (*i < pivot);
      do {
        j--;
      } while (*j > pivot);

      if (i >= j)
        break;

      // swap
      tmp = *i;
      *i = *j;
      *j = tmp;
    }
    return {{start, j}, {j + 1, end}};
  };
  auto merge = [](vector<Range<I>> &v_res) -> Range<I> {
    // merge the output
    return {v_res[0].begin(), v_res[1].end()};
  };
  auto seq = [=](Range<I> &op) -> Range<I> {
    // sequential version
    if (distance(op.begin(), op.end()) > 2) {
      std::sort(op.begin(), op.end());
    }
    return move(op);
  };
  auto cond = [](Range<I> &op) -> bool {
    // stop recursion and execute sequential
    return distance(op.begin(), op.end()) < 2;
  };
  DivConq<Cont> dac(div, merge, seq, cond);
  auto handle = spawn_pattern(sys, dac, caf::optional<actor>()).value();

  vector<int64_t> vec{3, 4, 1, 7, 8, 5, 2};
  aout(self) << "main_ (" << con_to_string(vec) << ")" << endl;
  self->request(handle, caf::infinite, move(vec))
      .receive(
          [&](vector<int64_t> &vec) {
            aout(self) << "main_ (" << con_to_string(vec) << ")" << endl;
          },
          [&self](error &_) { aout(self) << "error_" << _ << endl; });
}

CAF_MAIN()