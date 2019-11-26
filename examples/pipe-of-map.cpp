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

  using Cnt = vector<int64_t>;
  auto mapStatic = Map<Cnt>([](auto range) {
                     for (auto &el : range) {
                       el += 1;
                     }
                   })
                       .scheduler(PartitionSched::static_())
                       .replicas(3);

  auto mapDynamic = Map<Cnt>([](auto range) {
                      for (auto &el : range) {
                        el += 1;
                      }
                    })
                        .scheduler(PartitionSched::dynamic_{1})
                        .replicas(3);

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
