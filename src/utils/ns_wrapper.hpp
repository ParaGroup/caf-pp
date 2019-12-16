#pragma once

#include <caf/all.hpp>

namespace caf_pp {
namespace utils {

template <class T> struct ns_wrapper {
  ns_wrapper(T &&obj) : ref(obj) {};
  ns_wrapper(const ns_wrapper &other) : ref(other.ref) {};
  ns_wrapper(ns_wrapper &&other) : ref(std::move(other.ref)) {}

  ns_wrapper &operator=(const ns_wrapper &) = delete;

  T &operator*() { return ref; }
  // T operator->() { return ref; }

private:
  T ref;
};

template <class Inspector, class T>
typename Inspector::result_type inspect(Inspector &f, ns_wrapper<T> &x) {
  return f(caf::meta::type_name("ns_wrapper"));
}

} // namespace utils
} // namespace caf_pp
