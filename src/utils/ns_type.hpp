#pragma once

#include <caf/all.hpp>

namespace caf_pp {
namespace utils {

template <class T> struct ns_type {
  ns_type(T &&obj) { ptr = new T(move(obj)); };
  ns_type(const ns_type &other) : ptr{other.ptr} {};
  ns_type(ns_type &&other) : ptr{other.ptr} {}

  ns_type &operator=(const ns_type &other) = delete;

  T &operator*() { return *ptr; }
  T *operator->() { return ptr; }

  T release() {
    // must be called to avoid memory leak
    T res(move(*ptr));
    delete ptr;
    return res;
  }

private:
  T *ptr;
};

template <class Inspector, class T>
typename Inspector::result_type inspect(Inspector &f, ns_type<T> &x) {
  return f(caf::meta::type_name("ns_type"));
}

} // namespace utils
} // namespace caf_pp
