#pragma once

#include <cassert>
#include <utility>

#include "frontier_graph.h"

namespace fptlin {

namespace stack {

template <typename value_type>
struct impl {
  enum NonTerminalSymbol {
    S,
    PUSH,
    PEEK,
  };
  using non_terminal = std::pair<NonTerminalSymbol, value_type>;
  struct non_terminal_hash {
    std::size_t operator()(const non_terminal& n) const noexcept {
      return std::hash<uint64_t>{}(static_cast<uint64_t>(n.first) << 32U |
                                   static_cast<uint64_t>(n.second));
    }
  };
  using nonterm_entry = std::unordered_set<non_terminal, non_terminal_hash>;
  template <typename entry_t>
  using matrix_t = std::vector<std::vector<entry_t>>;
  using entry_index_t = std::size_t;
  using entry_order_t =
      std::vector<std::pair<int, std::pair<entry_index_t, entry_index_t>>>;

  // some unused value
  constexpr static value_type VAL_EPSILON =
      std::numeric_limits<value_type>::max();

 public:
  impl(value_type empty_val) : empty_val(empty_val) {}

  bool is_linearizable(history_t<value_type>& hist) {
    int n = hist.size();
    if (n == 0) return true;

    handle_empty(hist, empty_val);
    make_match(hist);

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
    std::size_t start_i = indices[{0, 0}];
    std::size_t end_i = indices[dest];
    return dp_table[start_i][end_i].count({NonTerminalSymbol::S, VAL_EPSILON});
  }

 private:
  // prepend an operation that pushes the empty value
  void handle_empty(history_t<value_type>& hist, value_type empty_val) {
    id_type id = hist.back().id + 1;
    for (auto& op : hist) {
      op.startTime += 2;
      op.endTime += 2;
    }
    hist.push_back({id, hist.back().proc, Method::PUSH, empty_val, 0, 1});
  }

  void make_match(history_t<value_type>& hist) {
    time_type last_time = 0;
    id_type id = hist.back().id + 1;
    for (auto& op : hist) last_time = std::max(op.endTime + 1, last_time);
    last_time <<= 1;

    int n = hist.size();
    for (int i = 0; i < n; ++i) {
      auto& op = hist[i];
      if (op.method != Method::PEEK)
        hist.push_back(
            {id++, op.proc,
             (op.method == Method::PUSH) ? Method::POP : Method::PUSH, op.value,
             last_time - op.endTime, last_time - op.startTime});
    }
  }

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
        switch (optr->method) {
          {
            case Method::PUSH:
              dp_table[a_i][b_i].insert({NonTerminalSymbol::PUSH, optr->value});
              break;
            case Method::PEEK:
              dp_table[a_i][b_i].insert({NonTerminalSymbol::PEEK, optr->value});
              break;
            case Method::POP:
              dp_table[a_i][b_i].insert({NonTerminalSymbol::S, optr->value});
              break;
            default:
              std::unreachable();
          }
        }
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
      entry_accum(dp_table[a][b], entry_mul(dp_table[a][c], dp_table[c][b]));
  }

  nonterm_entry entry_mul(const nonterm_entry& a, const nonterm_entry& b) {
    nonterm_entry ret;
    for (non_terminal x : b) {
      if (x.first != NonTerminalSymbol::S || x.second == VAL_EPSILON) continue;

      if (a.count({NonTerminalSymbol::PUSH, x.second}))
        ret.insert({NonTerminalSymbol::S, VAL_EPSILON});
      else if (a.count({NonTerminalSymbol::PEEK, x.second}))
        ret.insert({NonTerminalSymbol::S, x.second});
      else if (a.count({NonTerminalSymbol::S, VAL_EPSILON}))
        ret.insert(x);
    }

    return ret;
  }

  void entry_accum(nonterm_entry& a, const nonterm_entry& b) {
    a.insert(b.begin(), b.end());
  }

  value_type empty_val;
  frontier_graph<value_type, Method::PUSH, Method::PEEK, Method::POP> fgraph;
};

template <typename value_type>
bool is_linearizable(history_t<value_type>& hist, value_type empty_val) {
  return impl<value_type>(empty_val).is_linearizable(hist);
}

}  // namespace stack

}  // namespace fptlin
