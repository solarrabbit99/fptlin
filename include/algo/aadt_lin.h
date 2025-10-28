#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <stack>
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
  struct frame_t {
    node v;

    // pattern for this layer (cached when frame is first entered)
    uint32_t max_bit = 0;
    uint32_t res_bit = 0;
    uint32_t inv_bit = 0;

    // intra-layer iterator state: bits remaining to try
    uint32_t intra_remaining = 0;

    // bookkeeping to emulate recursive apply/undo and inter-layer side-effects
    operation_t<value_type>* applied_op =
        nullptr;  // non-null when we've applied and are waiting for child

    bool entered = false;  // whether we've executed "entering" logic
    bool inter_pushed_restore =
        false;  // whether inter child was pushed and we must restore afterward
  };

  bool dfs(node start) {
    std::stack<frame_t> st;
    st.push(frame_t{start});

    while (!st.empty()) {
      frame_t& f = st.top();

      // --- entering logic (once per frame) ---
      if (!f.entered) {
        // base case
        if (std::cmp_equal(f.v.layer, events.size())) return true;

        // attempt to mark visited
        if (!visited.insert(f.v).second) {
          st.pop();
          continue;
        }

        // populate layer-specific pattern
        auto [mb, rb, ib] = pattern[f.v.layer];
        f.max_bit = mb;
        f.res_bit = rb;
        f.inv_bit = ib;
        f.intra_remaining = f.max_bit;
        f.applied_op = nullptr;
        f.entered = true;
        f.inter_pushed_restore = false;

        // continue into main loop to try intra-layer
      }

      // --- if we returned from a child that we entered by applying an op, undo
      // now and continue ---
      if (f.applied_op != nullptr) {
        obj_impl.undo(f.applied_op);
        f.applied_op = nullptr;
        continue;  // re-evaluate this frame (try next intra bit / inter)
      }

      // --- try intra_layer successors ---
      bool pushed_child = false;
      while (f.intra_remaining) {
        uint32_t curr_bit = f.intra_remaining & -f.intra_remaining;
        f.intra_remaining &= (f.intra_remaining - 1);

        // already scheduled
        if (curr_bit & f.v.bits) continue;

        operation_t<value_type>* to_add = ongoing[std::countr_zero(curr_bit)];

        if (obj_impl.apply(to_add)) {
          f.applied_op = to_add;
          st.push(frame_t{node{f.v.layer, f.v.bits | curr_bit}});
          pushed_child = true;
          break;  // break intra loop; child will be processed next iteration
        }
      }

      // process child
      if (pushed_child) continue;

      // --- try inter_layer successor ---
      if (!f.inter_pushed_restore) {
        // cannot advance; `v.bits` doesn't satisfy `res_bit` -> pop and
        // backtrack
        if (f.res_bit & ~f.v.bits) {
          st.pop();
          continue;
        }

        // set invoked operation
        if (f.inv_bit) {
          ongoing[std::countr_zero(f.inv_bit)] = std::get<2>(events[f.v.layer]);
        }

        // push the next-layer child and continue
        node next{f.v.layer + 1, f.v.bits ^ f.res_bit};
        f.inter_pushed_restore = true;
        st.push(frame_t{next});
        continue;
      }

      // both intra-layer and inter-layer have been tried and failed
      // restore possibly lost operation
      if (f.inter_pushed_restore && f.res_bit) {
        ongoing[std::countr_zero(f.res_bit)] = std::get<2>(events[f.v.layer]);
      }

      // finished exploring frame -> pop and backtrack
      st.pop();
    }

    return false;  // exhausted all reachable states
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