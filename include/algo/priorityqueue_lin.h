#pragma once

#include <queue>
#include <unordered_map>

#include "aadt_lin.h"

namespace fptlin {

namespace priorityqueue {

template <typename value_type>
struct priority_queue_impl {
  priority_queue_impl() {}

  bool apply(operation_t<value_type>* o) {
    cleanup();

    switch (o->method) {
      case INSERT:
        heap.push(o->value);
        return true;
      case POLL:
        if (heap.empty() && o->value == EMPTY_VALUE) return true;
        if (!heap.empty() && heap.top() == o->value) {
          heap.pop();
          return true;
        }
        return false;
      case PEEK:
        if (heap.empty() && o->value == EMPTY_VALUE) return true;
        if (!heap.empty() && heap.top() == o->value) return true;
        return false;
      default:
        std::unreachable();
    }
  }

  void undo(operation_t<value_type>* o) {
    if (o->value == EMPTY_VALUE) return;

    switch (o->method) {
      case INSERT:
        removed_count[o->value]++;
        return;
      case POLL:
        heap.push(o->value);
        return;
      default:
        return;
    }
  }

 private:
  std::priority_queue<value_type> heap;
  std::unordered_map<value_type, std::size_t> removed_count;

  void cleanup() {
    while (!heap.empty()) {
      auto top = heap.top();
      auto it = removed_count.find(top);
      if (it == removed_count.end() || it->second == 0) break;
      it->second--;
      heap.pop();
    }
  }
};

template <typename value_type>
bool is_linearizable(history_t<value_type>& hist) {
  return aadt::impl<value_type, priority_queue_impl<value_type>>()
      .is_linearizable(hist);
}

}  // namespace priorityqueue

}  // namespace fptlin