#pragma once

#include "aadt_lin.h"
#include "commons/utility.h"

namespace fptlin {

namespace rmw {

template <pair_type pair_value_t>
struct rmw_impl {
  using value_type = decltype(std::declval<pair_value_t>().first);

  rmw_impl() : reg{0} {}

  bool apply(operation_t<pair_value_t>* o) {
    auto [a, b] = o->value;
    if (a != reg) return false;
    reg = b;
    return true;
  }

  void undo(operation_t<pair_value_t>* o) {
    auto [a, b] = o->value;
    reg = a;
  }

 private:
  value_type reg;
};

template <pair_type pair_value_t>
bool is_linearizable(history_t<pair_value_t>& hist) {
  return aadt::impl<pair_value_t, rmw_impl<pair_value_t>>().is_linearizable(
      hist);
}

}  // namespace rmw

}  // namespace fptlin