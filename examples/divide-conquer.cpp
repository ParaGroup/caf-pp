#include "caf_pp/patterns.hpp"

using namespace caf_pp;

struct Operand{};
struct Result{};

int main() {
  auto div = [](Operand &op) -> vector<Operand> {
    // divide the input
    return {};
  };
  auto merge = [](vector<Result> &v_res) -> Result {
    // merge the output
    return {};
  };
  auto seq = [](Operand &op) -> Result {
    // sequential version
    return {};
  };
  auto cond = [](const Operand &op) -> bool {
    // sto recursion and execute sequential
    return {};
  };
  DivConq<Operand, Result> dac(div, merge, seq, cond);
}