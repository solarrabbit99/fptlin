#pragma once

#include <unordered_set>

#include "aadt_lin.h"

namespace fptlin {

namespace set {

template <typename pair_value_t>
struct set_impl {
  using value_type = std::tuple_element<0, pair_value_t>::type;

  set_impl() : reg{0} {}

  bool apply(operation_t<pair_value_t>* o) {
    auto [a, b] = o->value;
    switch (o->method) {
      case INSERT:
        if (b) return reg.insert(a).second;
        return reg.count(a);
      case CONTAINS:
        return reg.count(a) == b;
      case REMOVE:
        if (b) return reg.erase(a);
        return !reg.count(a);
      default:
        std::unreachable();
    }
  }

  void undo(operation_t<pair_value_t>* o) {
    auto [a, b] = o->value;
    if (!b) return;

    switch (o->method) {
      case INSERT:
        reg.erase(a);
        return;
      case REMOVE:
        reg.insert(a);
        return;
      default:
        return;
    }
  }

 private:
  std::unordered_set<value_type> reg;
};

template <typename pair_value_t>
bool is_linearizable(history_t<pair_value_t>& hist) {
  return aadt::impl<pair_value_t, set_impl<pair_value_t>>().is_linearizable(
      hist);
}

}  // namespace set

}  // namespace fptlin