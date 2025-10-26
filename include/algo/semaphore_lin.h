#pragma once

#include <utility>

#include "aadt_lin.h"

namespace fptlin {

namespace semaphore {

struct semaphore_impl {
  bool apply(operation_t<bool>* o) {
    if (!o->value) return cnt == 0U;

    switch (o->method) {
      case INCR:
        ++cnt;
        return true;
      case DECR:
        if (!cnt) return false;
        --cnt;
        return true;
      default:
        std::unreachable();
    }
  }

  void undo(operation_t<bool>* o) {
    if (!o->value) return;

    if (o->method == INCR)
      --cnt;
    else
      ++cnt;
  }

 private:
  uint32_t cnt;
};

// `value_t` is expected to be bool
bool is_linearizable(history_t<bool>& hist) {
  return aadt::impl<bool, semaphore_impl>().is_linearizable(hist);
}

}  // namespace semaphore

}  // namespace fptlin