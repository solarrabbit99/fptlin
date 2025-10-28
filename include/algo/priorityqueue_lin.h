#pragma once

#include <set>

#include "aadt_lin.h"

namespace fptlin {

namespace priorityqueue {

template <typename value_type>
struct priority_queue_impl {
  priority_queue_impl() {}

  bool apply(operation_t<value_type>* o) {
    switch (o->method) {
      case INSERT:
        values.insert(o->value);
        return true;
      case POLL: {
        if (values.empty() && o->value == EMPTY_VALUE) return true;
        auto highest_iter = values.rbegin();
        if (!values.empty() && *highest_iter == o->value) {
          values.erase(std::prev(highest_iter.base()));
          return true;
        }
        return false;
      }
      case PEEK:
        if (values.empty() && o->value == EMPTY_VALUE) return true;
        if (!values.empty() && *values.rbegin() == o->value) return true;
        return false;
      default:
        std::unreachable();
    }
  }

  void undo(operation_t<value_type>* o) {
    if (o->value == EMPTY_VALUE) return;

    switch (o->method) {
      case INSERT:
        values.erase(values.find(o->value));
        return;
      case POLL:
        values.insert(o->value);
        return;
      default:
        return;
    }
  }

 private:
  std::multiset<value_type> values;
};

template <typename value_type>
bool is_linearizable(history_t<value_type>& hist) {
  return aadt::impl<value_type, priority_queue_impl<value_type>>()
      .is_linearizable(hist);
}

}  // namespace priorityqueue

}  // namespace fptlin