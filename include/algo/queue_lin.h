#include "fptlinutils.h"

namespace fptlin {

namespace queue {

template <typename value_type>
bool is_linearizable(history_t<value_type>& hist, value_type empty_val) {
  // TODO implement
  return hist.empty() && empty_val;
}

}  // namespace queue

}  // namespace fptlin
