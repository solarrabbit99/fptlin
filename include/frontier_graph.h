#pragma once

#include <unordered_map>

#include "fptlinutils.h"

namespace fptlin {

template <typename value_type, Method... methods>
struct frontier_graph {
  using frontier_list_t =
      std::vector<std::pair<node, operation_t<value_type>*>>;
  using frontier_adj_list =
      std::unordered_map<node, frontier_list_t, node_hash>;
  using node_map = std::unordered_map<node, node, node_hash>;

  const frontier_list_t& next(const node& node) { return madj_list[node]; }

  const frontier_adj_list& adj_list() const { return madj_list; }

  node first_same_node(const node& node) { return parent_map[node]; }

  node last_same_node(const node& first_node) {
    auto iter = last_added_child_map.find(first_node);
    return iter == last_added_child_map.end() ? first_node : iter->second;
  }

  /**
   * Joining is performed in increments of layer.
   * Resulting ufds will have depth of at most 1.
   * Hence, joining and finding are all O(1).
   */
  void build(const events_t<value_type>& events) {
    uint32_t max_bit = 0;
    operation_t<value_type>* ongoing[MAX_PROC_NUM];
    for (int layer = 0; std::cmp_less(layer, events.size()); ++layer) {
      auto [time, is_inv, optr] = events[layer];

      bool ignore =
          (sizeof...(methods) > 0) && ((optr->method != methods) && ...);
      uint32_t opbit = ignore ? 0 : 1 << optr->proc;
      uint32_t crit_bit = is_inv ? 0 : opbit;

      // iterate through all sub-masks in shrinking order
      for (uint32_t sub = max_bit;; sub = (sub - 1) & max_bit) {
        // union join
        node curr{layer, sub};
        node first = parent_map.try_emplace(curr, curr).first->second;
        if (!crit_bit || (crit_bit & sub)) {
          node last = {layer + 1, sub ^ crit_bit};
          parent_map[last] = first;
          last_added_child_map[first] = last;
        }

        // populate `adj_list`
        for (uint32_t x = (max_bit & ~sub); x; x &= (x - 1)) {
          uint32_t curr_bit = x & -x;
          operation_t<value_type>* to_add = ongoing[std::countr_zero(x)];
          node next{layer, sub | curr_bit};
          madj_list[first].emplace_back(
              parent_map.try_emplace(next, next).first->second, to_add);
        }

        if (sub == 0) break;
      }

      if (ignore) continue;

      if (is_inv) {
        max_bit |= opbit;
        ongoing[optr->proc] = optr;
      } else {
        max_bit ^= opbit;
      }
    }
  }

 private:
  node_map parent_map;
  node_map last_added_child_map;
  frontier_adj_list madj_list;
};

}  // namespace fptlin
