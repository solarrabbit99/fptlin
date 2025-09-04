#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <utility>

#include "definitions.h"
#include "fptlinutils.h"

namespace fptlin {

namespace aadt {

template <typename aadt_impl_t, typename value_type>
concept aadt_impl =
    requires(aadt_impl_t x, operation_t<value_type>* o, value_type v) {
      { aadt_impl_t(v) } -> std::same_as<aadt_impl_t>;
      { x.apply(o) } -> std::same_as<bool>;
      { x.undo(o) } -> std::same_as<void>;
    };

template <typename value_type, aadt_impl<value_type> aadt_impl_t>
struct impl {
 public:
  impl(value_type empty_val) : obj_impl(empty_val) {}

  bool is_linearizable(history_t<value_type>& hist) {
    events = get_events(hist);
    std::sort(events.begin(), events.end());
    pattern = get_bit_pattern(events);
    return dfs({0, 0});
  }

 private:
  bool inter_layer(node v, uint32_t crit_bit, uint32_t in_bit) {
    // `v.bit` doesn't satisfy `crit_bit`
    if (crit_bit & ~v.bit) return false;

    if (in_bit)  // add `in_bit`
      ongoing[std::countr_zero(in_bit)] = std::get<2>(events[v.layer]);

    bool good = dfs({v.layer + 1, v.bit ^ crit_bit});

    if (crit_bit)  // add back potentially lost `crit_bit` optr
      ongoing[std::countr_zero(crit_bit)] = std::get<2>(events[v.layer]);

    return good;
  }

  bool intra_layer(node v, uint32_t max_bit) {
    for (uint32_t x = max_bit; x; x &= (x - 1)) {
      uint32_t curr_bit = x & -x;

      if ((curr_bit & v.bit) | (curr_bit & ~max_bit)) continue;

      operation_t<value_type>* to_add = ongoing[std::countr_zero(x)];
      node next{v.layer, v.bit | curr_bit};

      if (obj_impl.apply(to_add)) {
        if (dfs(next)) return true;
        obj_impl.undo(to_add);
        continue;
      }
    }

    return false;
  }

  // practically O(k log n) time for each loop
  bool dfs(node v) {
    if (std::cmp_equal(v.layer, events.size())) return true;
    if (!visited.insert(v).second) return false;
    auto [max_bit, crit_bit, in_bit] = pattern[v.layer];
    return intra_layer(v, max_bit) || inter_layer(v, crit_bit, in_bit);
  }

  // global states
  events_t<value_type> events;
  std::vector<bit_pattern> pattern;
  node_set visited;

  // local states
  aadt_impl_t obj_impl;
  operation_t<value_type>* ongoing[MAX_PROC_NUM];
};

}  // namespace aadt

}  // namespace fptlin