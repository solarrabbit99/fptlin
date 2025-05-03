#pragma once

#include <cstdint>

#include "definitions.h"

namespace fptlin {

struct bit_pattern {
  uint32_t max_bit;
  uint32_t critical_bit;
  uint32_t pending_bit;
};

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
      if (((optr->method != methods) && ...)) continue;
    }

    if (is_inv) {
      ret.push_back({max_bit, 0, opbit});
      max_bit |= opbit;
    } else {
      ret.push_back({max_bit, opbit, 0});
      max_bit ^= max_bit;
    }
  }
  return ret;
}

}  // namespace fptlin
