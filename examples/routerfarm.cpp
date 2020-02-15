#include <iostream>
#include <vector>

#include "all.hpp"

using namespace std;
using namespace caf;
using namespace caf_pp;

using msg = atom_constant<caf::atom("msg")>;

class Act1 : public pp_actor {
public:
  Act1(actor_config &cfg, caf::optional<actor> next) : pp_actor(cfg, next) {
    // nop
  }

  behavior make_behavior() override {
    return {[=](msg) { aout(this) << "this is Act1" << endl; }};
  }
};

class Act2 : public pp_actor {
public:
  Act2(actor_config &cfg, caf::optional<actor> next) : pp_actor(cfg, next) {
    // nop
  }

  behavior make_behavior() override {
    return {[=](msg) { aout(this) << "this is Act2" << endl; }};
  }
};

void caf_main(actor_system &sys) {
  cout << "CAF_VERSION=" << CAF_VERSION << endl;

  Seq<Act1> act1;
  Seq<Act2> act2;
  auto farm = FarmRouter(act1, act2).policy(actor_pool::random());
  auto first = spawn_pattern_with(sys, farm, Runtime::threads).value();

  anon_send(first, msg::value);
  anon_send(first, msg::value);
  anon_send(first, msg::value);
}

CAF_MAIN()
