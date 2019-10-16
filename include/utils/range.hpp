#pragma once

#include "caf/all.hpp"

namespace caf_pp {
namespace utils {

template <typename T> struct MyRange {
  MyRange(T start, T end) : start_(start), end_(end) {
    // nop
  }
  T begin() { return start_; }
  T end() { return end_; }

private:
  T start_;
  T end_;
};

template <class Inspector, class T>
typename Inspector::result_type inspect(Inspector &f, MyRange<T> &x) {
  return f(caf::meta::type_name("MyRange"));
}

} // namespace utils
} // namespace caf_pp
