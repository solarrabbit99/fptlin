#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "definitions.h"
#include "fptlinutils.h"

namespace fptlin {

namespace aadt {

template <typename aadt_impl_t, typename value_type>
concept aadt_impl = std::is_default_constructible_v<aadt_impl_t> &&
                    requires(aadt_impl_t x, operation_t<value_type>* o) {
                      { x.apply(o) } -> std::same_as<bool>;
                      { x.undo(o) } -> std::same_as<void>;
                    };

template <typename value_type, aadt_impl<value_type> aadt_impl_t>
struct impl {
 public:
  bool is_linearizable(history_t<value_type>& hist) {
    events = get_events(hist);
    std::sort(events.begin(), events.end());
    pattern = get_bit_pattern(events);
    return dfs({0, 0});
  }

 private:
  // tries to move time forward without scheduling operations
  bool inter_layer(node v, uint32_t res_bit, uint32_t inv_bit) {
    // `v.bits` doesn't satisfy `res_bit`
    if (res_bit & ~v.bits) return false;

    if (inv_bit)  // add `inv_bit` optr
      ongoing[std::countr_zero(inv_bit)] = std::get<2>(events[v.layer]);

    bool good = dfs({v.layer + 1, v.bits ^ res_bit});

    if (res_bit)  // add back potentially lost responding optr
      ongoing[std::countr_zero(res_bit)] = std::get<2>(events[v.layer]);

    return good;
  }

  // tries to schedule operations
  bool intra_layer(node v, uint32_t max_bit) {
    for (uint32_t x = max_bit; x; x &= (x - 1)) {
      uint32_t curr_bit = x & -x;

      if ((curr_bit & v.bits) | (curr_bit & ~max_bit)) continue;

      operation_t<value_type>* to_add = ongoing[std::countr_zero(x)];
      node next{v.layer, v.bits | curr_bit};

      if (obj_impl.apply(to_add)) {
        if (dfs(next)) return true;
        obj_impl.undo(to_add);
        continue;
      }
    }

    return false;
  }

  bool dfs(node v) {
    if (std::cmp_equal(v.layer, events.size())) return true;
    if (!visited.insert(v).second) return false;
    auto [max_bit, res_bit, inv_bit] = pattern[v.layer];
    return intra_layer(v, max_bit) || inter_layer(v, res_bit, inv_bit);
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