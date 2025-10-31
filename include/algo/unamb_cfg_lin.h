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
concept cfg_type = requires(const std::optional<typename T::non_terminal>& o1,
                            const std::optional<typename T::non_terminal>& o2,
                            std::optional<typename T::non_terminal>& o3,
                            operation_t<value_type>* optr) {
  typename T::non_terminal;
  { T::START_SYMBOL } -> std::convertible_to<typename T::non_terminal>;
  { T::init_entry(optr) } -> std::convertible_to<typename T::non_terminal>;
  { T::entry_mul(o1, o2, o3) } -> std::convertible_to<bool>;
};

template <typename value_type, cfg_type<value_type> cfg>
struct impl {
  using non_terminal = cfg::non_terminal;
  using nonterm_entry = std::optional<non_terminal>;
  template <typename entry_t>
  using matrix_t = std::vector<std::vector<entry_t>>;
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

    matrix_t<nonterm_entry> dp_table;
    matrix_t<entry_index_t> adj_list;
    std::unordered_map<node, entry_index_t, node_hash> indices;
    init_mats(dp_table, adj_list, indices);

    // time complexity bottleneck O(n^3)
    for (auto [dist, entry_pos] : entry_order(adj_list))
      calc_entry(entry_pos.first, entry_pos.second, dp_table);

    node dest = fgraph.first_same_node({static_cast<int>(events.size()), 0U});
    nonterm_entry& target = dp_table[indices[{0, 0}]][indices[dest]];
    return target && *target == START_SYMBOL;
  }

 private:
  void init_mats(matrix_t<nonterm_entry>& dp_table,
                 matrix_t<entry_index_t>& adj_list,
                 std::unordered_map<node, entry_index_t, node_hash>& indices) {
    for (auto& [a, v] : fgraph.adj_list()) {
      indices.emplace(a, indices.size());
      for (auto [b, optr] : v) indices.emplace(b, indices.size());
    }
    std::size_t n = indices.size();
    dp_table.resize(n, std::vector<nonterm_entry>(n));
    adj_list.resize(n);

    for (auto& [a, v] : fgraph.adj_list()) {
      entry_index_t a_i = indices[a];
      for (auto [b, optr] : v) {
        entry_index_t b_i = indices[b];
        adj_list[a_i].push_back(b_i);
        dp_table[a_i][b_i].emplace(cfg::init_entry(optr));
      }
    }
  }

  // BFS from each node O(V * (V + E)), where E = O(kV)
  entry_order_t entry_order(matrix_t<entry_index_t>& adj_list) {
    std::size_t n = adj_list.size();
    entry_order_t ret;

    for (entry_index_t src = 0; src < adj_list.size(); ++src) {
      std::vector<int> dist(n, -1);
      std::queue<entry_index_t> q;

      dist[src] = 0;
      q.push(src);

      while (!q.empty()) {
        entry_index_t u = q.front();
        q.pop();

        // Explore neighbors from adj_list[u]
        for (entry_index_t v : adj_list[u])
          if (dist[v] == -1) {
            dist[v] = dist[u] + 1;
            q.push(v);
          }
      }

      // Store computed distances
      for (entry_index_t i = 0; i < n; ++i)
        if (dist[i] != -1) ret.push_back({dist[i], {src, i}});
    }
    std::sort(ret.begin(), ret.end());
    return ret;
  }

  void calc_entry(entry_index_t a, entry_index_t b,
                  matrix_t<nonterm_entry>& dp_table) {
    for (entry_index_t c = 0; c < dp_table[a].size(); ++c)
      if (cfg::entry_mul(dp_table[a][c], dp_table[c][b], dp_table[a][b]))
        return;
  }

  frontier_graph<value_type> fgraph;
};

}  // namespace unamb_cfg

}  // namespace fptlin
