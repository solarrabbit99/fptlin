#include <queue>

#include "frontier_graph.h"

namespace fptlin {

namespace queue {

template <typename value_type>
struct impl {
  using non_terminal = std::make_signed_t<value_type>;
  using matrix_entry = std::unordered_set<non_terminal>;
  using dym_matrix =
      std::unordered_map<node, std::unordered_map<node, matrix_entry>>;

 public:
  impl(value_type empty_val) : empty_val(empty_val) {}

  bool is_linearizable(history_t<value_type>& hist) {
    if (hist.empty()) return true;

    events_t<value_type> events = get_events(hist);
    std::sort(events.begin(), events.end());
    enq_graph.build(events);
    front_graph.build(events);
    dest = front_graph.first_same_node({static_cast<int>(events.size()), 0});
    while (!bfs.empty()) {
      node v = bfs.front();
      if (global_vis.insert(v).second && extend_node(v)) return true;
      bfs.pop();
    }
    return false;
  }

 private:
  // Returns `true` if `dest` is found/reached
  bool extend_node(node a) { return extend_front(a) || extend_enq(a); }

  bool extend_front(node a) {
    std::queue<node> next_b;
    for (auto& [b, entry] : matrix[a]) next_b.push(b);

    node_set local_vis;
    while (!next_b.empty()) {
      node b = next_b.front();
      next_b.pop();
      if (!local_vis.insert(b).second) continue;

      const matrix_entry& entry = matrix[a][b];
      for (auto [c, optr] : front_graph.next(b)) {
        if (optr->method != Method::ENQ && optr->value != empty_val &&
            entry.count(static_cast<non_terminal>(optr->value))) {
          if (c == dest) return true;
          next_b.push(c);
          matrix[a][c].insert(optr->method == Method::PEEK ? optr->value
                                                           : -optr->value);
        }
      }
    }
    return false;
  }

  bool extend_enq(node a) { return true; }

  node dest;
  value_type empty_val;
  std::queue<node> bfs;
  node_set global_vis;
  frontier_graph<value_type, Method::ENQ> enq_graph;
  frontier_graph<value_type, Method::PEEK, Method::DEQ> front_graph;
  dym_matrix matrix;
};

template <typename value_type>
bool is_linearizable(history_t<value_type>& hist, value_type empty_val) {
  return impl<value_type>(empty_val).is_linearizable(hist);
}

}  // namespace queue

}  // namespace fptlin
