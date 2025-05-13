#include <queue>

#include "frontier_graph.h"

namespace fptlin {

namespace stack {

template <typename value_type>
struct impl {
  using non_terminal = value_type;
  using matrix_entry = std::unordered_set<non_terminal>;
  using dym_matrix = std::unordered_map<
      node, std::unordered_map<node, matrix_entry, node_hash>, node_hash>;

 public:
  impl(value_type empty_val) : empty_val(empty_val) {}

  bool is_linearizable(history_t<value_type>& hist) {
    if (hist.empty()) return true;

    events_t<value_type> events = get_events(hist);
    std::sort(events.begin(), events.end());
    enq_graph.build(events);
    front_graph.build(events);

    node_set vis;
    node source{0, 0U};
    dest = front_graph.first_same_node({static_cast<int>(events.size()), 0U});
    bfs.push(source);
    matrix[source][source].insert(empty_val);
    while (!bfs.empty()) {
      node v = bfs.front();
      bfs.pop();
      if (vis.insert(v).second && extend_node(v)) return true;
    }
    return false;
  }

 private:
  // Returns `true` if `dest` is found/reached
  bool extend_node(node a) {
    return extend_front(a) || extend_empty(a) || extend_enq(a);
  }

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
        if (optr->value != empty_val && entry.count(optr->value)) {
          if (c == dest) return true;
          next_b.push(c);
          matrix[a][c].insert(optr->method == Method::PEEK ? optr->value
                                                           : empty_val);
        }
      }
    }
    return false;
  }

  bool extend_empty(node a) {
    std::queue<node> next_b;
    for (auto& [b, entry] : matrix[a]) next_b.push(b);

    node_set local_vis;
    while (!next_b.empty()) {
      node b = next_b.front();
      next_b.pop();
      if (!local_vis.insert(b).second) continue;

      const matrix_entry& entry = matrix[a][b];
      if (!entry.count(empty_val)) continue;

      for (auto [c, optr] : front_graph.next(b)) {
        if (optr->value == empty_val && overlaps(a, c)) {
          if (c == dest) return true;
          next_b.push(c);
          matrix[a][c].insert(empty_val);
        }
      }
    }
    return false;
  }

  bool extend_enq(node a) {
    for (auto& [b, entry] : matrix[a])
      for (auto [c, optr] : enq_graph.next(a)) {
        if (!precedes(b, c)) {
          bfs.push(c);
          matrix[c][b].insert(optr->value);
        }
      }
    return false;
  }

  // `a` from `enq_graph` and `b` from `front_graph`
  // both are first nodes
  bool overlaps(node a, node b) {
    return enq_graph.last_same_node(a).layer >= b.layer &&
           front_graph.last_same_node(b).layer >= a.layer;
  }

  // `a` from `front_graph` and `b` from `enq_graph`
  // both are first nodes
  bool precedes(node a, node b) {
    return front_graph.last_same_node(a).layer < b.layer;
  }

  node dest;
  value_type empty_val;
  frontier_graph<value_type, Method::ENQ> enq_graph;
  frontier_graph<value_type, Method::PEEK, Method::DEQ> front_graph;
  dym_matrix matrix;
  std::queue<node> bfs;
};

template <typename value_type>
bool is_linearizable(history_t<value_type>& hist, value_type empty_val) {
  return impl<value_type>(empty_val).is_linearizable(hist);
}

}  // namespace stack

}  // namespace fptlin
