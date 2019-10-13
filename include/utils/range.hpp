#pragma once

#include "caf/all.hpp"

using namespace caf;

namespace caf_pp {
namespace utils {

template <typename T> struct Range {
  Range(T start, T end) : start_(start), end_(end) {
    // nop
  }
  T begin() { return start_; }
  T end() { return end_; }

private:
  T start_;
  T end_;
};

template <class Inspector, class T>
typename Inspector::result_type inspect(Inspector &f, Range<T> &x) {
  return f(meta::type_name("Range"));
}

} // namespace utils
} // namespace caf_pp
