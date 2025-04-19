#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <set>
#include <unordered_set>
#include <utility>

#include "definitions.h"
#include "fptlinutils.h"

namespace fptlin {

namespace priorityqueue {

struct node {
  int layer;
  uint32_t bit;
};

struct node_hash {
  std::size_t operator()(const node& n) const noexcept {
    return std::hash<int64_t>{}(std::bit_cast<int64_t>(n));
  }
};

struct node_equal {
  bool operator()(const node& a, const node& b) const noexcept {
    return std::bit_cast<int64_t>(a) == std::bit_cast<int64_t>(b);
  }
};

typedef std::unordered_set<node, node_hash, node_equal> node_set;

template <typename value_type>
struct lin_impl {
 public:
  lin_impl(value_type empty_val) : empty_val(empty_val) {}

  bool inter_layer(node v, uint32_t crit_bit, uint32_t in_bit) {
    // `v.bit` satisfies `crit_bit`
    if (crit_bit & ~v.bit) return false;

    if (in_bit)  // add `in_bit`
      ongoing[std::countr_zero(in_bit)] = std::get<2>(events[v.layer]);

    bool good = dfs({v.layer + 1, v.bit ^ crit_bit});

    if (crit_bit)  // add back potentially lost `crit_bit` optr
      ongoing[std::countr_zero(crit_bit)] = std::get<2>(events[v.layer]);

    return good;
  }

  bool intra_layer(node v, uint32_t max_bit) {
    for (int i = 0; i < MAX_PROC_NUM; ++i) {
      // includes ongoing op in thread `i`
      if (!((v.bit ^ max_bit) & (1 << i))) continue;

      operation_t<value_type>* to_add = ongoing[i];
      node next{v.layer, v.bit | (1 << i)};

      if (to_add->method == Method::INSERT) {
        auto iter = values.insert(to_add->value);
        if (dfs(next)) return true;
        values.erase(iter);
        continue;
      }

      // empty `PEEK`/`DEQ`
      if (values.empty() && to_add->value == empty_val)
        if (dfs(next)) return true;

      auto highest_iter = values.rbegin();
      if (!values.empty() && *highest_iter == to_add->value) {
        if (to_add->method == Method::POLL)
          values.erase(std::prev(highest_iter.base()));
        if (dfs(next)) return true;
        if (to_add->method == Method::POLL) values.insert(to_add->value);
      }
    }

    return false;
  }

  // practically O(k log n) time for each loop
  bool dfs(node v) {
    if (std::cmp_equal(v.layer, events.size())) return true;
    if (!visited.insert(v).second) return false;
    auto [max_bit, crit_bit, in_bit] = pattern[v.layer];
    return intra_layer(v, max_bit) || inter_layer(v, crit_bit, in_bit);
  }

  bool is_linearizable(history_t<value_type>& hist) {
    events = get_events(hist);
    std::sort(events.begin(), events.end());
    pattern = get_bit_pattern(events);
    return dfs({0, 0});
  }

 private:
  // global states
  events_t<value_type> events;
  std::vector<bit_pattern> pattern;
  node_set visited;
  value_type empty_val;

  // local states
  std::multiset<value_type> values;
  operation_t<value_type>* ongoing[MAX_PROC_NUM];
};

template <typename value_type>
bool is_linearizable(history_t<value_type>& hist, value_type empty_val) {
  return lin_impl<value_type>(empty_val).is_linearizable(hist);
}

}  // namespace priorityqueue

}  // namespace fptlin