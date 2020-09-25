#include <chrono>
#include <iostream>
#include <vector>

#include "all.hpp"

using std::cout;
using std::endl;
using namespace std::chrono_literals;
using namespace std::chrono;
using namespace caf;
using namespace caf_pp;

using init = caf::atom_constant<caf::atom("init")>;
using gen = caf::atom_constant<caf::atom("gen")>;

const auto APP_RUN_TIME = 2s;
const auto MAX_QUEUE = 5ul;

struct tuple_t {
  int64_t a, b, c;
};
CAF_ALLOW_UNSAFE_MESSAGE_TYPE(tuple_t)

void active_delay(nanoseconds waste_time) {
  auto start_time = chrono::high_resolution_clock::now();
  bool end = false;
  while (!end) {
    auto end_time = chrono::high_resolution_clock::now();
    end = (end_time - start_time) >= waste_time;
  }
}

struct Source : public pp_actor {
  Source(actor_config &cfg, caf::optional<Next> next) : pp_actor(cfg, next) {}

  behavior make_behavior() override {
    return {[=](init) {
              generated_tuples_ = 0;
              num_wait_ = 0;
            },
            [=](gen) {
              auto app_start_time = chrono::high_resolution_clock::now();
              while (true) {
                auto msg = make_message(tuple_t{0, 0, 0});
                generated_tuples_++;

                while (!send_next_if(MAX_QUEUE, msg)) {
                  active_delay(5ns);
                  num_wait_++;
                }

                if ((chrono::high_resolution_clock::now() - app_start_time) >=
                    APP_RUN_TIME) {
                  quit();
                  return;
                }
              }
            }};
  }

  void on_exit() override {
    aout(this) << "[Source" << id() << "] "
               << "generated_tuples=" << generated_tuples_ << " "
               << "num_wait=" << num_wait_ << endl;
  }

private:
  uint64_t generated_tuples_;
  uint64_t num_wait_;
};

class Sink : public pp_actor {
public:
  Sink(actor_config &cfg, caf::optional<Next> next) : pp_actor(cfg, next) {}

  behavior make_behavior() override {
    return {[=](init) { received_tuples_ = 0; },
            [=](tuple_t &) {
              received_tuples_++;
              active_delay(duration_cast<nanoseconds>(1ms));
            }};
  }

  void on_exit() override {
    aout(this) << "[Sink" << id() << "] received_tuples=" << received_tuples_
               << endl;
  }

private:
  uint64_t received_tuples_;
};

void caf_main(actor_system &sys) {
  cout << "CAF_VERSION=" << CAF_VERSION << endl;

  Seq<Source> source{[](actor a) { anon_send(a, init::value); }};
  Seq<Sink> sink{[](actor a) { anon_send(a, init::value); }};
  auto sink_farm = AllFarm{sink}.replicas(3);

  Pipeline pipe(source, sink_farm);

  cout << "    Pattern: " << pipe << endl;
  auto first = spawn_pattern(sys, pipe).value();

  cout << "Start..." << endl;
  anon_send(first, gen::value);
}

CAF_MAIN()
