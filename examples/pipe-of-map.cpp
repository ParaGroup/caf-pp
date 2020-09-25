#include <iostream>
#include <vector>

#include "all.hpp"

using std::cout;
using std::endl;
using namespace caf;
using namespace caf_pp;

template <class T> std::string con_to_string(const T &c) {
  std::string res{""};
  size_t i{0};
  for (auto el : c) {
    res += std::to_string(el) + (i < c.size() - 1 ? " " : "");
    i++;
  }
  return res;
}

struct config : actor_system_config {
  config() { opt_group{custom_options_, "global"}; }
};

void caf_main(actor_system &sys, const config &) {
  cout << "CAF_VERSION=" << CAF_VERSION << endl;
  scoped_actor self{sys};

  using Cnt = std::vector<int64_t>;
  auto mapStatic = Map<Cnt>([](auto range) {
                     for (auto &&el : range) {
                       el += 1;
                     }
                   })
                       .scheduler(PartitionSched::static_())
                       .replicas(3);

  auto mapDynamic = Map<Cnt>([](auto range) {
                      for (auto &&el : range) {
                        el += 1;
                      }
                    })
                        .scheduler(PartitionSched::dynamic_{1})
                        .replicas(3);

  Pipeline pipe(mapStatic, mapDynamic);
  cout << "    Pattern: " << pipe << endl;
  auto first = spawn_pattern(sys, pipe, actor_cast<actor>(self)).value();

  auto vec = Cnt();
  for (auto i = 10; i > 0; --i) {
    vec.push_back(0);
  }
  aout(self) << "main_ (" << con_to_string(vec) << ")" << endl;

  for (int i = 0; i < 1; i++) {
    self->send(first, std::move(vec));
    self->receive(
        [&](Cnt vec_back) {
          aout(self) << "main_ (" << con_to_string(vec_back) << ")" << endl;
          vec = std::move(vec_back);
        },
        [&self](error &_) { aout(self) << "error_" << _ << endl; });
  }
}
CAF_MAIN()
