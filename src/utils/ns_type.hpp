#pragma once

#include <caf/all.hpp>

namespace caf_pp {
namespace utils {

template <class T> struct ns_type {
  ns_type(T &&obj) {
    ptr = new T(std::move(obj));
    // if (__verbose__)
    //   cout << "  [DEBUG] constructor" << endl;
  };
  ns_type(const ns_type &other)
      : ptr{other.ptr} {
            // if (__verbose__)
            //   std::cout << "  [DEBUG] copy constructor" << std::endl;
        };
  ns_type(ns_type &&other) : ptr{other.ptr} {
    // if (__verbose__)
    //   std::cout << "  [DEBUG] std::move constructor" < std::endl;
  }
  ns_type &operator=(const ns_type &other) = delete;

  T &operator*() { return *ptr; }
  T *operator->() { return ptr; }

  T release() {
    // must be called to avoid memory leak
    T res(std::move(*ptr));
    delete ptr;
    return res;
  }

private:
  T *ptr;
};

template <class Inspector, class T>
typename Inspector::result_type inspect(Inspector &f, ns_type<T> &x) {
  //   if (__verbose__)
  //     std::cout << "  [DEBUG] ns_type inspector" << std::endl;
  return f(caf::meta::type_name("ns_type"), *x);
}
} // namespace utils
} // namespace caf_pp
