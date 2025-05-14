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
  using matrix_entry = std::unordered_set<non_terminal, non_terminal_hash>;
  using matrix_t = std::unordered_map<
      node, std::unordered_map<node, matrix_entry, node_hash>, node_hash>;

 public:
  impl(value_type empty_val) : empty_val(empty_val) {}

  // TODO support unmatched histories with empty values
  bool is_linearizable(history_t<value_type>& hist) {
    int n = hist.size();
    if (n == 0) return true;

    events_t<value_type> events = get_events(hist);
    std::sort(events.begin(), events.end());
    fgraph.build(events);
    matrix_t m;
    node_set nodes;
    init_mat(m, nodes);

    matrix_t m_prime;
    smat_pow(m, n, m_prime, nodes);

    node dest = fgraph.first_same_node({static_cast<int>(events.size()), 0U});
    return m_prime[{0, 0}].count(dest);
  }

 private:
  void init_mat(matrix_t& m, node_set& nodes) {
    for (auto& [a, v] : fgraph.adj_list()) {
      nodes.insert(a);
      for (auto [b, optr] : v) {
        nodes.insert(b);
        switch (optr->method) {
          {
            case Method::PUSH:
              m[a][b].insert({NonTerminalSymbol::PUSH, optr->value});
              break;
            case Method::PEEK:
              m[a][b].insert({NonTerminalSymbol::PEEK, optr->value});
              break;
            case Method::POP:
              m[a][b].insert({NonTerminalSymbol::S, optr->value});
              break;
            default:
              std::unreachable();
          }
        }
      }
    }
  }

  matrix_entry entry_mul(const matrix_entry& a, const matrix_entry& b) {
    matrix_entry ret;
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

  void entry_accum(matrix_entry& a, const matrix_entry& b) {
    a.insert(b.begin(), b.end());
  }

  void smat_mul(matrix_t& a, matrix_t& b, matrix_t& c, const node_set& nodes) {
    for (auto& i : nodes)
      for (auto& k : nodes)
        for (auto& j : nodes) entry_accum(c[i][j], entry_mul(a[i][k], b[k][j]));
  }

  void smat_pow(matrix_t& m, int pow, matrix_t& ret, const node_set& nodes) {
    assert(pow > 0);

    if (pow == 1) {
      ret = m;
      return;
    }

    matrix_t tmp;
    smat_pow(m, pow >> 1, tmp, nodes);
    smat_mul(tmp, tmp, ret, nodes);
    if (pow & 1) {
      matrix_t tmp2;
      smat_mul(ret, m, tmp2, nodes);
      ret = tmp2;
    }
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
