#include <iostream>
#include <vector>

#include "all.hpp"

using std::cout;
using std::endl;
using namespace caf;
using namespace caf_pp;

class storage : public pp_actor {
public:
  storage(actor_config &cfg, caf::optional<Next> next) : pp_actor(cfg, next) {
    // nop
  }

  behavior make_behavior() override {
    return {[=,this](std::string key, std::tuple<int, int, int> values) {
      aout(this) << "\tworker_" << id() << "   key=" << key << endl;

      auto search = elements_.find(key);
      if (search == elements_.end()) {
        elements_.emplace(std::make_pair(key, std::make_tuple(0, 0, 0)));
      }
      auto &t = elements_[key];
      auto &t1 = get<0>(t);
      auto &t2 = get<1>(t);
      auto &t3 = get<2>(t);

      update0(t1, get<0>(values));
      update1(t2, get<1>(values));
      update2(t3, get<2>(values));

      send_next(key, std::to_string(1), t1);
      send_next(key, std::to_string(2), t2);
      send_next(key, std::to_string(3), t3);
    }};
  }

private:
  void update0(int &t, const int &in) { t += in; }
  void update1(int &t, const int &in) { t += in; }
  void update2(int &t, const int &in) { t += in; }
  std::unordered_map<std::string, std::tuple<int, int, int>> elements_;
};

class dispatcher : public pp_actor {
public:
  dispatcher(actor_config &cfg, caf::optional<Next> next)
      : pp_actor(cfg, next) {
    // nop
  }

  behavior make_behavior() override {
    return {[=,this](std::string &key, std::string &sigma, int &value) {
              aout(this) << "\tworker_" << id() << "   key=" << key + sigma
                         << "   value=" << value << endl;

              auto search = subscription_.find(key + sigma);
              if (search != subscription_.end()) {
                for (auto &c : search->second) {
                  send(c, key, sigma, value);
                }
              }
            },
            [=,this](std::string &key, std::string &sigma, actor &c) {
              subscription_[key + sigma].push_back(c);
            }};
  }

private:
  std::unordered_map<std::string, std::vector<actor>> subscription_;
};

class client : public event_based_actor {
public:
  client(actor_config &cfg) : event_based_actor(cfg) {
    // nop
  }

  behavior make_behavior() override {
    return {[=,this](std::string &element, std::string &sigma, int &value) {
      aout(this) << "\tclient_" << id() << "   element=" << element
                 << "   sigma=" + sigma << "   value=" << value << endl;
    }};
  }
};

struct config : actor_system_config {
  config() { opt_group{custom_options_, "global"}; }
};

void caf_main(actor_system &sys, const config &) {
  cout << "CAF_VERSION=" << CAF_VERSION << endl;

  Seq<storage> storage_seq;
  auto storage_farm = Farm(storage_seq)
                          .policy(by_key<std::string>([](type_erased_tuple &t) {
                            return t.get_as<std::string>(0);
                          }))
                          .replicas(3)
                          .runtime(Runtime::threads);
  Seq<dispatcher> dispatcher_seq(
      [](actor) { cout << "[DEBUG] init callback call" << endl; });
  auto dispatcher_farm = Farm(dispatcher_seq)
                             .policy(by_key<std::string>([](type_erased_tuple &t) {
                               return t.get_as<std::string>(0) + t.get_as<std::string>(1);
                             }))
                             .replicas(3)
                             .runtime(Runtime::actors);
  Pipeline pipe(storage_farm, dispatcher_farm);
  cout << "    Pattern: " << pipe << endl;
  auto first = spawn_pattern(sys, pipe).value();

  // subscribe 2 clients
  auto c1 = sys.spawn<client>();
  auto c2 = sys.spawn<client>();
  auto dispatcher_actor = dispatcher_farm.instance_.value();
  dispatcher_actor.send(make_message("asd", "1", c1));
  dispatcher_actor.send(make_message("asd", "2", c1));
  dispatcher_actor.send(make_message("asd", "3", c2));

  // send updates
  anon_send(first, "asd", std::make_tuple(1, 2, 3));
  anon_send(first, "asd", std::make_tuple(1, 2, 3));
}

CAF_MAIN()
