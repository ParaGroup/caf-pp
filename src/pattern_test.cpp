#include <iostream>
#include <typeinfo>
#include <vector>

#include "caf/all.hpp"

using namespace caf;
using namespace std;

using SpawnFunc = function<actor(caf::optional<actor>)>;

struct Pattern {};

struct Node : public Pattern {
  Node(SpawnFunc spawn_fun) : spawn_fun_(spawn_fun) {}

  SpawnFunc spawn_fun_;
  caf::optional<actor> instance_;
};

template <typename T> struct Farm : public Pattern {
  Farm(T &stage, uint64_t replicas, actor_pool::policy policy)
      : stage_(stage), replicas_(replicas), policy_(policy) {
    static_assert(is_base_of<Pattern, T>::value,
                  "Type parameter of this class must derive from Pattern");
  }

  T &stage_;
  uint64_t replicas_;
  actor_pool::policy policy_;
  caf::optional<actor> instance_;
};

template <class... T> struct Pipeline : public Pattern {
  Pipeline(T &... stages) : stages_(stages...) {
    // std::cout << std::is_same<T, Pattern>::value <<  "\n";
    static_assert(conjunction_v<is_base_of<Pattern, T>...>,
                  "Type parameter of this class must derive from Pattern");
  }

  tuple<T &...> stages_;
  caf::optional<actor> instance_;
};

template <class T>
caf::optional<actor> spawn_pattern(actor_system &sys, T &p,
                                   const caf::optional<actor> &out) {
  static_assert((is_base_of<Pattern, T>::value != 0),
                "Type parameter of this function must derive from Pattern");
  return {};
}

template <>
caf::optional<actor> spawn_pattern(actor_system &sys, Node &p,
                                   const caf::optional<actor> &out) {
  // is node
  cout << "[DEBUG] "
       << "inside NODE spawn" << endl;
  actor a = p.spawn_fun_(out);
  p.instance_ = caf::optional<actor>(a);
  return a;
}

template <template <class> class Farm, typename T>
caf::optional<actor> spawn_pattern(actor_system &sys, Farm<T> &p,
                                   const caf::optional<actor> &out) {
  // is Farm
  cout << "[DEBUG] "
       << "inside FARM spawn" << endl;
  auto spawn_fun = [&]() {
    cout << "[DEBUG] "
         << "inside actor_pool spawn_fun" << endl;
    return spawn_pattern(sys, p.stage_, out).value();
  };
  cout << "[DEBUG] "
       << "make actor_pool" << endl;
  auto a = caf::actor_pool::make(sys.dummy_execution_unit(), p.replicas_,
                                 spawn_fun, p.policy_);
  p.instance_ = caf::optional<actor>(a);
  return a;
}

template <template <class> class Pipeline, typename... T>
caf::optional<actor> spawn_pattern(actor_system &sys, Pipeline<T...> &p,
                                   const caf::optional<actor> &out) {
  // is Pipeline
  cout << "[DEBUG] "
       << "inside PIPELINE spawn" << endl;
  auto a = for_each_tuple(sys, p.stages_, out).value();
  p.instance_ = caf::optional<actor>(a);
  return a;
}

template <size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), caf::optional<actor>>::type
for_each_tuple(actor_system &sys, std::tuple<Tp...> &t,
               const caf::optional<actor> &out) {
  return out;
}

template <size_t I = 0, typename... Tp>
    inline typename std::enable_if <
    I<sizeof...(Tp), caf::optional<actor>>::type
    for_each_tuple(actor_system &sys, std::tuple<Tp...> &t,
                   const caf::optional<actor> &out) {
  auto last = spawn_pattern(sys, std::get<(sizeof...(Tp)) - 1 - I>(t), out);
  return for_each_tuple<I + 1, Tp...>(sys, t, caf::optional<actor>(last));
}

// TODO: deduct KEY parameter from GetKey function
template <class Key, class GetKey, class Router> struct by_key_policy {
  by_key_policy(GetKey gf, Router rf) : gf_(gf), rf_(rf) {
    // nop
  }
  void operator()(actor_system &, actor_pool::uplock &guard,
                  const actor_pool::actor_vec &vec, mailbox_element_ptr &ptr,
                  execution_unit *host) {
    CAF_ASSERT(!vec.empty());
    type_erased_tuple &tp = ptr->content();
    Key key = gf_(tp);
    size_t hashcode = hf_(key);
    size_t idx = rf_(hashcode, vec.size());
    CAF_ASSERT(idx > 0 && idx < vec.size());
    cout << "[DEBUG] hashcode=" << hashcode << " select=" << idx << endl;
    actor selected = vec[idx];
    guard.unlock();
    selected->enqueue(std::move(ptr), host);
  }
  GetKey gf_;
  hash<Key> hf_;
  Router rf_;
};

template <class Key, class GetKey, class Router>
actor_pool::policy by_key(GetKey gf, Router rf) {
  return by_key_policy<Key, GetKey, Router>{gf, rf};
}

template <class Key, class GetKey> actor_pool::policy by_key(GetKey gf) {
  return by_key<Key>(gf, [](size_t i, size_t n) { return i % n; });
}

struct config : actor_system_config {
  config() {
    opt_group{custom_options_, "global"}
        .add(silent, "silent", "")
        .add(stream, "stream", "")
        .add(worker, "worker", "");
  }

  bool silent = false;
  uint64_t stream = 9;
  uint64_t worker = 2;
};

class my_general_worker : public event_based_actor {
public:
  my_general_worker(actor_config &cfg, caf::optional<actor> next, int _,
                    string __)
      : event_based_actor(cfg), next_(next) {
    // nop
  }

  behavior make_behavior() override {
    return {[=](string value) {
      aout(this) << "\tworker_" << id() << "   value=" << value << endl;
      if (next_)
        send(next_.value(), value + "[" + to_string(id()) + "]");
    }};
  }

private:
  caf::optional<actor> next_;
};

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

void caf_main(actor_system &sys, const config &cfg) {
  cout << "CAF_VERSION=" << CAF_VERSION << endl;

  Node security_node(
      [&](caf::optional<actor> next) { return sys.spawn<security>(next); });
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
