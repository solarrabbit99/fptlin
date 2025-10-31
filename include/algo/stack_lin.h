#pragma once

#include "unamb_cfg_lin.h"

namespace fptlin {

namespace stack {

template <typename value_type>
struct stack_grammar {
  enum NonTerminalSymbol {
    T,
    PUSH,
    PEEK,
  };
  using non_terminal = std::pair<NonTerminalSymbol, value_type>;
  using nonterm_entry = std::optional<non_terminal>;

  // some unused value
  constexpr static value_type VAL_EPSILON =
      std::numeric_limits<value_type>::max();
  constexpr static non_terminal START_SYMBOL{NonTerminalSymbol::T, VAL_EPSILON};

  static non_terminal init_entry(operation_t<value_type>* optr) {
    switch (optr->method) {
      case Method::PUSH:
        return {NonTerminalSymbol::PUSH, optr->value};
      case Method::PEEK:
        return {NonTerminalSymbol::PEEK, optr->value};
      case Method::POP:
        return {NonTerminalSymbol::T, optr->value};
      default:
        std::unreachable();
    }
  }

  static bool entry_mul(const nonterm_entry& a, const nonterm_entry& b,
                        nonterm_entry& c) {
    if (!a || !b) return false;

    auto [symbol, value] = *b;
    if (symbol != NonTerminalSymbol::T || value == VAL_EPSILON) return false;

    if (*a == non_terminal{NonTerminalSymbol::PUSH, value})
      c.emplace(NonTerminalSymbol::T, VAL_EPSILON);
    else if (*a == non_terminal{NonTerminalSymbol::PEEK, value})
      c.emplace(NonTerminalSymbol::T, value);
    else if (*a == non_terminal{NonTerminalSymbol::T, VAL_EPSILON})
      c = b;

    return true;
  }
};

// prepend an operation that pushes the empty value
template <typename value_type>
void handle_empty(history_t<value_type>& hist) {
  id_type id = hist.back().id + 1;
  for (auto& op : hist) {
    op.startTime += 2;
    op.endTime += 2;

    if (op.value == EMPTY_VALUE) op.method = Method::PEEK;
  }
  hist.push_back({id, hist.back().proc, Method::PUSH, EMPTY_VALUE, 0, 1});
}

template <typename value_type>
void make_match(history_t<value_type>& hist) {
  time_type last_time = 0;
  id_type id = hist.back().id + 1;
  for (auto& op : hist) last_time = std::max(op.endTime + 1, last_time);
  last_time <<= 1;

  int n = hist.size();
  for (int i = 0; i < n; ++i) {
    auto& op = hist[i];
    if (op.method != Method::PEEK)
      hist.push_back({id++, op.proc,
                      (op.method == Method::PUSH) ? Method::POP : Method::PUSH,
                      op.value, last_time - op.endTime,
                      last_time - op.startTime});
  }
}

template <typename value_type>
bool is_linearizable(history_t<value_type>& hist) {
  handle_empty(hist);
  make_match(hist);
  return unamb_cfg::impl<value_type, stack_grammar<value_type>>()
      .is_linearizable(hist);
}

}  // namespace stack

}  // namespace fptlin
