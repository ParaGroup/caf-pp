#include <iostream>
#include <vector>

#include "caf/all.hpp"

#include "caf_pp/patterns.hpp"
#include "caf_pp/policy.hpp"
#include "caf_pp/spawn.hpp"

using namespace std;
using namespace caf;
using namespace caf_pp;

// class my_general_worker : public event_based_actor {
// public:
//   my_general_worker(actor_config &cfg, caf::optional<actor> next, int _,
//                     string __)
//       : event_based_actor(cfg), next_(next) {
//     // nop
//   }

//   behavior make_behavior() override {
//     return {[=](string value) {
//       aout(this) << "\tworker_" << id() << "   value=" << value << endl;
//       if (next_)
//         send(next_.value(), value + "[" + to_string(id()) + "]");
//     }};
//   }

// private:
//   caf::optional<actor> next_;
// };

class security : public event_based_actor {
public:
  security(actor_config &cfg, caf::optional<actor> next)
      : event_based_actor(cfg), next_(next) {
    // nop
  }

  behavior make_behavior() override {
    return {[=](string key, tuple<int, int, int> values) {
      aout(this) << "\tworker_" << id() << "   key=" << key << endl;

      auto search = instruments_.find(key);
      if (search == instruments_.end()) {
        instruments_.emplace(make_pair(key, make_tuple(0, 0, 0)));
      }
      auto &tuple = instruments_[key];
      auto &t1 = get<0>(tuple);
      auto &t2 = get<1>(tuple);
      auto &t3 = get<2>(tuple);

      update0(t1, get<0>(values));
      update1(t2, get<1>(values));
      update2(t3, get<2>(values));

      if (next_) {
        send(next_.value(), key, to_string(1), t1);
        send(next_.value(), key, to_string(2), t2);
        send(next_.value(), key, to_string(3), t3);
      }
      cout << "[DEBUG] HERE" << endl;
    }};
  }

private:
  void update0(int &t, const int &in) { t += in; }
  void update1(int &t, const int &in) { t += in; }
  void update2(int &t, const int &in) { t += in; }
  caf::optional<actor> next_;
  unordered_map<string, tuple<int, int, int>> instruments_;
};

class dispatcher : public event_based_actor {
public:
  dispatcher(actor_config &cfg) : event_based_actor(cfg) {
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
    return {[=](string &instrument, string &sigma, int &value) {
      aout(this) << "\tclient_" << id() << "   instrument=" << instrument
                 << "   sigma=" + sigma << "   value=" << value << endl;
    }};
  }
};

struct config : actor_system_config {
  config() { opt_group{custom_options_, "global"}; }
};

void caf_main(actor_system &sys, const config &cfg) {
  cout << "CAF_VERSION=" << CAF_VERSION << endl;

  Node security_node(
      [&](caf::optional<actor> next) {
        return sys.spawn<security>(next);
      });
  Node dispatcher_node(
      [&](caf::optional<actor> _) { return sys.spawn<dispatcher>(); });

  Farm security_farm(security_node, 3, by_key<string>([](type_erased_tuple &t) {
                       return t.get_as<string>(0);
                     }));

  Farm dispatcher_farm(dispatcher_node, 3,
                       by_key<string>([](type_erased_tuple &t) {
                         return t.get_as<string>(0) + t.get_as<string>(1);
                       }));

  Pipeline pipe(security_farm, dispatcher_farm);

  auto first = spawn_pattern(sys, pipe, caf::optional<actor>()).value();

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
