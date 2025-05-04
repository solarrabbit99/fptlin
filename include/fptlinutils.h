#pragma once

#include <cstdint>
#include <unordered_set>

#include "commons/ufds.h"
#include "definitions.h"

namespace fptlin {

struct bit_pattern {
  uint32_t max_bit;
  uint32_t critical_bit;
  uint32_t pending_bit;
};

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
typedef fptlin::ufds<node, node_hash, node_equal> node_ufds;

/**
 * retrieves events, O(n)
 */
template <typename value_type>
events_t<value_type> get_events(history_t<value_type>& hist) {
  events_t<value_type> events;
  events.reserve(hist.size() << 1);
  for (operation_t<value_type>& o : hist) {
    events.emplace_back(o.startTime, true, &o);
    events.emplace_back(o.endTime, false, &o);
  }
  return events;
}

/**
 * Assume `events` is sorted, O(n)
 */
template <typename value_type, fptlin::Method... methods>
std::vector<bit_pattern> get_bit_pattern(events_t<value_type>& events) {
  std::vector<bit_pattern> ret;
  ret.reserve(events.size());
  uint32_t max_bit = 0;
  for (auto [time, is_inv, optr] : events) {
    uint32_t opbit = 1 << optr->proc;
    if constexpr (sizeof...(methods) > 0) {
      if (((optr->method != methods) && ...)) {
        ret.push_back({max_bit, 0, 0});
        continue;
      }
    }

    if (is_inv) {
      ret.push_back({max_bit, 0, opbit});
      max_bit |= opbit;
    } else {
      ret.push_back({max_bit, opbit, 0});
      max_bit ^= opbit;
    }
  }
  return ret;
}

void join_nodes(std::vector<bit_pattern>& pattern, node_ufds& ufds) {
  for (int layer = 0; std::cmp_less(layer, pattern.size()); ++layer) {
    auto [max_bit, crit_bit, pend_bit] = pattern[layer];
    for (uint32_t sub = max_bit;; sub = (sub - 1) & max_bit) {
      if (crit_bit && !(crit_bit & sub)) continue;
      ufds.join({layer, sub}, {layer + 1, sub ^ crit_bit});
      if (sub == 0) break;
    }
  }
}

}  // namespace fptlin
