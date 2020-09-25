#include "patterns.hpp"

namespace caf_pp {

std::ostream &operator<<(std::ostream &o, Runtime& e) {
  switch (e) {
  case Runtime::threads:
    return o << "threads";
  case Runtime::actors:
    return o << "actors";
  default:
    return o << "(invalid value)";
  }
}

std::ostream &operator<<(std::ostream &o, PartitionVar &e) {
  if (holds_alternative<PartitionSched::static_>(e)) {
    return o << "static_sched";
  } else {
    return o << "dynamic_sched";
  }
}

} // namespace caf_pp
