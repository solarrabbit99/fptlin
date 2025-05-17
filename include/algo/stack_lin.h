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
  using matrix_t =
      std::unordered_map<node, std::unordered_map<node, entry_t, node_hash>,
                         node_hash>;

 public:
  impl(value_type empty_val) : empty_val(empty_val) {}

  // TODO support unmatched histories with empty values
  bool is_linearizable(history_t<value_type>& hist) {
    int n = hist.size();
    if (n == 0) return true;

    events_t<value_type> events = get_events(hist);
    std::sort(events.begin(), events.end());
    fgraph.build(events);
    matrix_t<nonterm_entry> dp_table;
    matrix_t<int> dist_table;
    node_set nodes;
    init_mats(dp_table, dist_table, nodes);

    dist_closure(dist_table, nodes);
    for (auto [dist, entry_pos] : entry_order(dist_table))
      calc_entry(entry_pos.first, entry_pos.second, dp_table);

    node dest = fgraph.first_same_node({static_cast<int>(events.size()), 0U});
    return dp_table[{0, 0}][dest].count({NonTerminalSymbol::S, empty_val});
  }

 private:
  void init_mats(matrix_t<nonterm_entry>& dp_table, matrix_t<int>& dist_table,
                 node_set& nodes) {
    for (auto& [a, v] : fgraph.adj_list()) {
      nodes.insert(a);
      for (auto [b, optr] : v) {
        nodes.insert(b);
        dist_table[a][b] = 1;
        switch (optr->method) {
          {
            case Method::PUSH:
              dp_table[a][b].insert({NonTerminalSymbol::PUSH, optr->value});
              break;
            case Method::PEEK:
              dp_table[a][b].insert({NonTerminalSymbol::PEEK, optr->value});
              break;
            case Method::POP:
              dp_table[a][b].insert({NonTerminalSymbol::S, optr->value});
              break;
            default:
              std::unreachable();
          }
        }
      }
    }
  }

  // Floyd-Warshall algorithm
  void dist_closure(matrix_t<int>& m, const node_set& nodes) {
    for (auto& i : nodes)
      for (auto& j : nodes)
        for (auto& k : nodes) {
          auto it = m[i].find(j);
          if (it == m[i].end()) continue;
          auto it_2 = m[j].find(k);
          if (it_2 == m[j].end()) continue;
          auto it_3 = m[i].find(k);
          if (it_3 == m[i].end()) m[i][k] = it->second + it_2->second;
        }
  }

  std::vector<std::pair<int, std::pair<node, node>>> entry_order(
      const matrix_t<int>& dist_table) {
    std::vector<std::pair<int, std::pair<node, node>>> ret;
    for (auto& [a, v] : dist_table)
      for (auto& [b, d] : v) ret.push_back({d, {a, b}});
    std::sort(ret.begin(), ret.end());
    return ret;
  }

  void calc_entry(const node& a, const node& b,
                  matrix_t<nonterm_entry>& dp_table) {
    for (const auto& [c, e] : dp_table[a]) {
      auto it = dp_table[c].find(b);
      if (it == dp_table[c].end()) continue;
      entry_accum(dp_table[a][b], entry_mul(e, it->second));
    }
  }

  nonterm_entry entry_mul(const nonterm_entry& a, const nonterm_entry& b) {
    nonterm_entry ret;
    for (non_terminal x : b) {
      if (x.first != NonTerminalSymbol::S || x.second == empty_val) continue;

      if (a.count({NonTerminalSymbol::PUSH, x.second}))
        ret.insert({NonTerminalSymbol::S, empty_val});
      else if (a.count({NonTerminalSymbol::PEEK, x.second}))
        ret.insert({NonTerminalSymbol::S, x.second});
      else if (a.count({NonTerminalSymbol::S, empty_val}))
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
