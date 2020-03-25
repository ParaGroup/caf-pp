#include <iostream>
#include <vector>

#include "all.hpp"

using namespace std;
using namespace caf;
using namespace caf_pp;

class Act1 : public pp_actor {
public:
  Act1(actor_config &cfg, caf::optional<unique_ptr<Next>> next) : pp_actor(cfg, move(next)) {
    // nop
  }

  behavior make_behavior() override {
    return {[=](string &a, int b) {
      string res;
      for (int i = b; i > 0; i--) {
        res += a;
      }
      send_next(res);
    }};
  }
};

class Act2 : public pp_actor {
public:
  Act2(actor_config &cfg, caf::optional<unique_ptr<Next>> next) : pp_actor(cfg, move(next)) {
    // nop
  }

  behavior make_behavior() override {
    return {[=](string &a) {
              aout(this) << "[" << current_sender()->id() << "->" << id()
                         << "] " << a << endl;
            },
            [=](vector<message> &in) {
              aout(this) << endl << "batch " << in.size() << endl;
              for (auto &msg : in) {
                auto a = msg.get_as<string>(0);
                aout(this) << "[" << (current_sender() ? to_string(current_sender()->id()) : "anon") << "->" << id()
                           << "] " << a << endl;
              }
            }};
  }
};

void caf_main(actor_system &sys) {
  cout << "CAF_VERSION=" << CAF_VERSION << endl;

  Seq<Act1> seq1;
  auto farm1 = AllFarm(seq1).replicas(5).runtime(Runtime::actors);
  Seq<Act2> seq2;
  auto farm2 =
      AllFarm(seq2)
          .policy(ByKeyPolicy<string>([](const message &msg) -> const string & {
            return msg.get_as<string>(0);
          }))
          .replicas(3)
          .batch(5)
          .runtime(Runtime::actors);
  Pipeline pipe(farm1, farm2);
  cout << "    Pattern: " << pipe << endl;
  auto first = spawn_pattern(sys, pipe).value();

  anon_send(first, "Halo", 1);
  anon_send(first, "Halo", 2);
  anon_send(first, "Halo", 3);

  anon_send(first, "Halo", 1);
  anon_send(first, "Halo", 2);
  anon_send(first, "Halo", 3);
}

CAF_MAIN()
