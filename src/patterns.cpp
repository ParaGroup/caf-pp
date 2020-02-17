#include "patterns.hpp"

namespace caf_pp {
ostream &operator<<(ostream &o, Runtime e) {
  switch (e) {
  case Runtime::threads:
    return o << "threads";
  case Runtime::actors:
    return o << "actors";
  default:
    return o << "(invalid value)";
  }
}
} // namespace caf_pp
