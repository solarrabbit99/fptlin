#pragma once

#include <set>

#include "aadt_lin.h"

namespace fptlin {

namespace priorityqueue {

template <typename value_type>
struct priority_queue_impl {
  priority_queue_impl(value_type empty_val) : empty_val{empty_val} {}

  bool apply(operation_t<value_type>* o) {
    switch (o->method) {
      case INSERT:
        values.insert(o->value);
        return true;
      case POLL: {
        if (values.empty() && o->value == empty_val) return true;
        auto highest_iter = values.rbegin();
        if (!values.empty() && *highest_iter == o->value) {
          values.erase(std::prev(highest_iter.base()));
          return true;
        }
        return false;
      }
      case PEEK:
        if (values.empty() && o->value == empty_val) return true;
        if (!values.empty() && *values.rbegin() == o->value) return true;
        return false;
      default:
        std::unreachable();
    }
  }

  void undo(operation_t<value_type>* o) {
    switch (o->method) {
      case INSERT:
        [[fallthrough]];
      case POLL:
        values.erase(o->value);
        return;
      case PEEK:
        return;
      default:
        std::unreachable();
    }
  }

 private:
  value_type empty_val;
  std::multiset<value_type> values;
};

template <typename value_type>
bool is_linearizable(history_t<value_type>& hist, value_type empty_val) {
  return aadt::impl<value_type, priority_queue_impl<value_type>>(empty_val)
      .is_linearizable(hist);
}

}  // namespace priorityqueue

}  // namespace fptlin