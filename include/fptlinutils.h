#pragma once

#include <cstdint>
#include <unordered_set>

#include "definitions.h"

namespace fptlin {

struct bit_pattern {
  uint32_t max_bit;
  uint32_t critical_bit;
  uint32_t pending_bit;
};

struct node {
  int layer;
  uint32_t bits;
};

struct node_hash {
  std::size_t operator()(const node& n) const noexcept {
    return std::hash<int64_t>{}(std::bit_cast<int64_t>(n));
  }
};

bool operator==(const node& a, const node& b) noexcept {
  return std::bit_cast<int64_t>(a) == std::bit_cast<int64_t>(b);
}

// for tie-breaking only, and does not imply actual ordering of the two
bool operator<(const node& a, const node& b) noexcept {
  return std::bit_cast<int64_t>(a) < std::bit_cast<int64_t>(b);
}

typedef std::unordered_set<node, node_hash> node_set;

template <typename... Args>
std::ostream& operator<<(std::ostream& os, const std::tuple<Args...>& tuple) {
  std::apply([&os](Args... valArgs) { ((os << valArgs), ...); }, tuple);
  return os;
}

template <typename value_type>
std::ostream& operator<<(std::ostream& os,
                         const fptlin::operation_t<value_type>& o) {
  os << "[id=" << o.id << ", proc=" << o.proc
     << ", method=" << fptlin::methodtos(o.method) << ", value=" << o.value
     << ", startTime=" << o.startTime << ", endTime=" << o.endTime << "]";
  return os;
}

std::ostream& operator<<(std::ostream& os, const node& v) {
  os << "[layer=" << v.layer << ", bits=" << v.bits << "]";
  return os;
}

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
template <typename value_type>
std::vector<bit_pattern> get_bit_pattern(events_t<value_type>& events) {
  std::vector<bit_pattern> ret;
  ret.reserve(events.size());
  uint32_t max_bit = 0;
  for (auto [time, is_inv, optr] : events) {
    uint32_t opbit = 1 << optr->proc;
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

}  // namespace fptlin
