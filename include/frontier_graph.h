#pragma once

#include "fptlinutils.h"

namespace fptlin {

template <typename value_type, Method... methods>
struct frontier_graph {
  using frontier_list_t =
      std::vector<std::pair<node, operation_t<value_type>*>>;
  using frontier_adj_list =
      std::unordered_map<node, frontier_list_t, node_hash>;
  using node_map = std::unordered_map<node, node, node_hash>;
  using node_ufds = fptlin::ufds<node, node_hash>;

  const frontier_list_t& next(const node& node) { return madj_list[node]; }

  const frontier_adj_list& adj_list() const { return madj_list; }

  node first_same_node(const node& node) { return ufds.find(node); }

  node last_same_node(const node& node) {
    auto iter = last_same_nodes.find(node);
    return iter == last_same_nodes.end() ? node : iter->second;
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

      uint32_t opbit = 1 << optr->proc;
      uint32_t crit_bit =
          !is_inv && ((optr->method == methods) || ...) ? opbit : 0;

      // iterate through all sub-masks in shrinking order
      for (uint32_t sub = max_bit;; sub = (sub - 1) & max_bit) {
        // populate `ufds` and `last_same_nodes`
        node first = ufds.find({layer, sub});
        if (!crit_bit || (crit_bit & sub)) {
          node last = {layer + 1, sub ^ crit_bit};
          ufds.join(first, last);
          last_same_nodes[first] = last;
        }

        // populate `adj_list`
        for (uint32_t x = (max_bit & ~sub); x; x &= (x - 1)) {
          uint32_t curr_bit = x & -x;
          operation_t<value_type>* to_add = ongoing[std::countr_zero(x)];
          node next{layer, sub | curr_bit};
          madj_list[first].emplace_back(ufds.find(next), to_add);
        }

        if (sub == 0) break;
      }

      if (((optr->method == methods) || ...)) {
        if (is_inv) {
          max_bit |= opbit;
          ongoing[optr->proc] = optr;
        } else {
          max_bit ^= opbit;
        }
      }
    }
  }

 private:
  node_map last_same_nodes;
  node_ufds ufds;
  frontier_adj_list madj_list;
};

}  // namespace fptlin
