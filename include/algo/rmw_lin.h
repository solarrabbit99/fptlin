#pragma once

#include "aadt_lin.h"

namespace fptlin {

namespace rmw {

template <typename pair_value_t>
struct rmw_impl {
  using value_type = std::tuple_element<0, pair_value_t>::type;

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

template <typename pair_value_t>
bool is_linearizable(history_t<pair_value_t>& hist) {
  return aadt::impl<pair_value_t, rmw_impl<pair_value_t>>().is_linearizable(
      hist);
}

}  // namespace rmw

}  // namespace fptlin