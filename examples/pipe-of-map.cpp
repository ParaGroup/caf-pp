#include <iostream>
#include <vector>

#include "caf/all.hpp"

#include "patterns.hpp"
#include "policy.hpp"
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

  using Container = vector<int64_t>;
  using Iterator = typename Container::iterator;
  Map<Container> mapStatic(
      [=](Iterator begin, Iterator end) {
        for (auto it = begin; it != end; ++it) {
          *it += 1;
        }
      },
      PartitionSched::static_(), 3);

  Map<Container> mapDynamic(
      [=](Iterator begin, Iterator end) {
        for (auto it = begin; it != end; ++it) {
          *it += 1;
        }
      },
      PartitionSched::dynamic_{1}, 3);

  Pipeline pipe(mapStatic, mapDynamic);
  auto first =
      spawn_pattern(sys, pipe, actor_cast<actor>(self), Runtime::actors)
          .value();

  auto vec = vector<int64_t>();
  for (auto i = 10; i > 0; --i) {
    vec.push_back(0);
  }
  aout(self) << "main_ (" << con_to_string(vec) << ")" << endl;

  for (int i = 0; i < 1; i++) {
    self->send(first, move(vec));
    self->receive(
        [&](vector<int64_t> vec_back) {
          aout(self) << "main_ (" << con_to_string(vec_back) << ")" << endl;
          vec = move(vec_back);
        },
        [&self](error &_) { aout(self) << "error_" << _ << endl; });
  }
}
CAF_MAIN()
