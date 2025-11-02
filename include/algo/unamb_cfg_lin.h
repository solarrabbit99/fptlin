#pragma once

#include <cassert>
#include <optional>
#include <queue>
#include <utility>
#include <variant>

#include "frontier_graph.h"

namespace fptlin {

namespace unamb_cfg {

template <typename T, typename value_type>
concept cfg_type =
    requires(const T::non_terminal& o1, const T::non_terminal& o2,
             operation_t<value_type>* optr) {
      typename T::non_terminal;
      { T::START_SYMBOL } -> std::convertible_to<typename T::non_terminal>;
      { T::init_entry(optr) } -> std::convertible_to<typename T::non_terminal>;
      {
        T::entry_mul(o1, o2)
      } -> std::convertible_to<std::optional<typename T::non_terminal>>;
    };

template <typename value_type, cfg_type<value_type> cfg>
struct impl {
  using non_terminal = typename cfg::non_terminal;

  // Sparse, memory-efficient matrix row
  using dp_row_t = std::unordered_map<std::size_t, non_terminal>;
  using dp_table_t = std::vector<dp_row_t>;

  using entry_index_t = std::size_t;
  using entry_order_t =
      std::vector<std::pair<int, std::pair<entry_index_t, entry_index_t>>>;

  static constexpr non_terminal START_SYMBOL = cfg::START_SYMBOL;

 public:
  bool is_linearizable(history_t<value_type>& hist) {
    // in the context of linearizability,
    // empty histories can be assumed to be linearizable
    if (hist.empty()) return true;

    events_t<value_type> events = get_events(hist);
    std::sort(events.begin(), events.end());
    fgraph.build(events);
    std::size_t graph_size = fgraph.size();

    dp_table_t dp_table;
    std::unordered_map<node, entry_index_t, node_hash> indices;
    std::vector<node> index_to_node;  // new vector
    indices.reserve(graph_size);
    index_to_node.reserve(graph_size);
    dp_table.resize(graph_size);

    init_mats(dp_table, indices, index_to_node);

    // Precompute traversal order efficiently
    for (auto [dist, entry_pos] : entry_order(indices, index_to_node))
      calc_entry(entry_pos.first, entry_pos.second, dp_table);

    node dest = fgraph.first_same_node({static_cast<int>(events.size()), 0U});
    auto it_src = indices.find({0, 0});
    auto it_dst = indices.find(dest);
    if (it_src == indices.end() || it_dst == indices.end()) return false;

    auto it = dp_table[it_src->second].find(it_dst->second);
    return (it != dp_table[it_src->second].end()) && it->second == START_SYMBOL;
  }

 private:
  void init_mats(dp_table_t& dp_table,
                 std::unordered_map<node, entry_index_t, node_hash>& indices,
                 std::vector<node>& index_to_node) {
    const auto& adj = fgraph.adj_list();

    for (auto& [a, v] : adj) {
      if (!indices.contains(a)) {
        indices.emplace(a, indices.size());
        index_to_node.push_back(a);
      }
      for (auto& [b, optr] : v) {
        if (!indices.contains(b)) {
          indices.emplace(b, indices.size());
          index_to_node.push_back(b);
        }
      }
    }

    for (auto& [a, v] : adj) {
      entry_index_t a_i = indices[a];
      dp_row_t& row = dp_table[a_i];
      row.reserve(v.size());
      for (auto& [b, optr] : v) {
        entry_index_t b_i = indices[b];
        row.emplace(b_i, cfg::init_entry(optr));
      }
    }
  }

  void calc_entry(entry_index_t a, entry_index_t b, dp_table_t& dp_table) {
    auto& row_a = dp_table[a];
    for (const auto& [c, entry_ac] : row_a) {
      const auto it_cb = dp_table[c].find(b);
      if (it_cb == dp_table[c].end()) continue;
      std::optional<non_terminal> res = cfg::entry_mul(entry_ac, it_cb->second);
      if (res) row_a[b] = *res;
    }
  }

  entry_order_t entry_order(
      const std::unordered_map<node, entry_index_t, node_hash>& indices,
      const std::vector<node>& index_to_node) {
    const std::size_t n = indices.size();
    entry_order_t ret;

    std::vector<int> dist(n, -1);
    std::queue<entry_index_t> q;
    const auto& adj = fgraph.adj_list();

    for (entry_index_t src = 0; src < n; ++src) {
      std::fill(dist.begin(), dist.end(), -1);
      dist[src] = 0;
      q.push(src);

      while (!q.empty()) {
        const entry_index_t u = q.front();
        q.pop();

        const node& u_node = index_to_node[u];
        auto it = adj.find(u_node);
        if (it == adj.end()) continue;

        for (const auto& [vnode, _] : it->second) {
          auto v_it = indices.find(vnode);
          if (v_it == indices.end()) continue;
          const entry_index_t v = v_it->second;
          if (dist[v] == -1) {
            dist[v] = dist[u] + 1;
            q.push(v);
          }
        }
      }

      for (entry_index_t i = 0; i < n; ++i)
        if (dist[i] != -1) ret.push_back({dist[i], {src, i}});
    }

    std::sort(ret.begin(), ret.end());
    return ret;
  }

  frontier_graph<value_type> fgraph;
};

}  // namespace unamb_cfg

}  // namespace fptlin
