#pragma once

#include "caf/all.hpp"

using namespace caf;

namespace caf_pp {

template <class T> struct ns_type {
  ns_type(T &&obj) {
    ptr = new T(move(obj));
    // if (__verbose__)
    //   cout << "  [DEBUG] constructor" << endl;
  };
  ns_type(const ns_type &other)
      : ptr{other.ptr} {
            // if (__verbose__)
            //   cout << "  [DEBUG] copy constructor" << endl;
        };
  ns_type(ns_type &&other) : ptr{other.ptr} {
    // if (__verbose__)
    //   cout << "  [DEBUG] move constructor" << endl;
  }
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
  //   if (__verbose__)
  //     cout << "  [DEBUG] ns_type inspector" << endl;
  return f(meta::type_name("ns_type"), *x);
}
} // namespace caf_pp
