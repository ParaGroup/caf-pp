#include <iostream>
#include <vector>

#include "caf/all.hpp"

#include "patterns.hpp"
#include "policy.hpp"
#include "pp_actor.hpp"
#include "spawn.hpp"

using namespace std;
using namespace caf;
using namespace caf_pp;

class storage : public pp_actor {
public:
  storage(actor_config &cfg, caf::optional<actor> next) : pp_actor(cfg, next) {
    // nop
  }

  behavior make_behavior() override {
    return {[=](string key, tuple<int, int, int> values) {
      aout(this) << "\tworker_" << id() << "   key=" << key << endl;

      auto search = elements_.find(key);
      if (search == elements_.end()) {
        elements_.emplace(make_pair(key, make_tuple(0, 0, 0)));
      }
      auto &tuple = elements_[key];
      auto &t1 = get<0>(tuple);
      auto &t2 = get<1>(tuple);
      auto &t3 = get<2>(tuple);

      update0(t1, get<0>(values));
      update1(t2, get<1>(values));
      update2(t3, get<2>(values));

      send_next(key, to_string(1), t1);
      send_next(key, to_string(2), t2);
      send_next(key, to_string(3), t3);
    }};
  }

private:
  void update0(int &t, const int &in) { t += in; }
  void update1(int &t, const int &in) { t += in; }
  void update2(int &t, const int &in) { t += in; }
  unordered_map<string, tuple<int, int, int>> elements_;
};

class dispatcher : public pp_actor {
public:
  dispatcher(actor_config &cfg, caf::optional<actor> next)
      : pp_actor(cfg, next) {
    // nop
  }

  behavior make_behavior() override {
    return {[=](string &key, string &sigma, int &value) {
              aout(this) << "\tworker_" << id() << "   key=" << key + sigma
                         << "   value=" << value << endl;

              auto search = subscription_.find(key + sigma);
              if (search != subscription_.end()) {
                for (auto &c : search->second) {
                  send(c, key, sigma, value);
                }
              }
            },
            [=](string &key, string &sigma, actor &c) {
              subscription_[key + sigma].push_back(c);
            }};
  }

private:
  unordered_map<string, vector<actor>> subscription_;
};

class client : public event_based_actor {
public:
  client(actor_config &cfg) : event_based_actor(cfg) {
    // nop
  }

  behavior make_behavior() override {
    return {[=](string &element, string &sigma, int &value) {
      aout(this) << "\tclient_" << id() << "   element=" << element
                 << "   sigma=" + sigma << "   value=" << value << endl;
    }};
  }
};

struct config : actor_system_config {
  config() { opt_group{custom_options_, "global"}; }
};

void caf_main(actor_system &sys, const config &cfg) {
  cout << "CAF_VERSION=" << CAF_VERSION << endl;

  Seq<storage> storage_seq;
  Farm storage_farm(storage_seq, by_key<string>([](type_erased_tuple &t) {
                      return t.get_as<string>(0);
                    }),
                    3);
  Seq<dispatcher> dispatcher_seq(
      [](actor _a) { cout << "[DEBUG] init callback call" << endl; });
  Farm dispatcher_farm(dispatcher_seq, by_key<string>([](type_erased_tuple &t) {
                         return t.get_as<string>(0) + t.get_as<string>(1);
                       }),
                       3);
  Pipeline pipe(storage_farm, dispatcher_farm);

  auto first =
      spawn_pattern(sys, pipe, caf::optional<actor>(), Runtime::threads)
          .value();

  // subscribe 2 clients
  auto c1 = sys.spawn<client>();
  auto c2 = sys.spawn<client>();
  auto dispatcher_actor = dispatcher_farm.instance_.value();
  anon_send(dispatcher_actor, "asd", "1", c1);
  anon_send(dispatcher_actor, "asd", "2", c1);
  anon_send(dispatcher_actor, "asd", "3", c2);

  // send updates
  anon_send(first, "asd", make_tuple(1, 2, 3));
  anon_send(first, "asd", make_tuple(1, 2, 3));
}

CAF_MAIN()
