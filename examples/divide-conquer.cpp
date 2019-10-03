#include <iostream>
#include <vector>

#include "caf/all.hpp"

#include "caf_pp/patterns.hpp"
#include "caf_pp/spawn.hpp"

using namespace std;
using namespace caf;
using namespace caf_pp;

struct Operand {
  vector<int64_t> &data;
  size_t left;
  size_t right;
};
using Result = Operand;
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(Operand)

template <class T> string con_to_string(const T &c) {
  string res{""};
  size_t i{0};
  for (auto el : c) {
    res += to_string(el) + (i < c.size() - 1 ? " " : "");
    i++;
  }
  return res;
}

inline size_t array_size(size_t left, size_t right) { return right - left + 1; }

struct config : actor_system_config {
  config() { opt_group{custom_options_, "global"}; }
};

void caf_main(actor_system &sys, const config &cfg) {
  cout << "CAF_VERSION=" << CAF_VERSION << endl;
  scoped_actor self{sys};

  auto div = [](Operand &op) -> vector<Operand> {
    // divide the input
    long pivot = op.data[(op.left + op.right) / 2];
    size_t i = op.left - 1, j = op.right + 1;
    long tmp;

    while (true) {
      do {
        i++;
      } while (op.data[i] < pivot);
      do {
        j--;
      } while (op.data[j] > pivot);

      if (i >= j)
        break;

      // swap
      tmp = op.data[i];
      op.data[i] = op.data[j];
      op.data[j] = tmp;
    }
    return {Operand{op.data, op.left, j}, Operand{op.data, j + 1, op.right}};
  };
  auto merge = [](vector<Result> &v_res) -> Result {
    // merge the output
    return {v_res[0].data, v_res[0].left, v_res[1].right};
  };
  auto seq = [=](Operand &op) -> Result {
    // sequential version
    if (array_size(op.left, op.right) > 2) {
      std::sort(op.data.begin() + op.left, op.data.begin() + op.right + 1);
    }
    return move(op);
  };
  auto cond = [](const Operand &op) -> bool {
    // sto recursion and execute sequential
    return array_size(op.left, op.right) < 2;
  };
  DivConq<Operand, Result> dac(div, merge, seq, cond);

  auto handle = spawn_pattern(sys, dac, actor_cast<actor>(self)).value();

  vector<int64_t> vec{3, 4, 1, 7, 8, 5, 2};
  Operand op{vec, 0, 7};
  aout(self) << "main_ (" << con_to_string(op.data) << ")" << endl;

  self->send(handle, op);
  self->receive(
      [&](up, Result &res) {
        aout(self) << "main_ (" << con_to_string(res.data) << ")" << endl;
      },
      [&self](error &_) { aout(self) << "error_" << _ << endl; });
}

CAF_MAIN()