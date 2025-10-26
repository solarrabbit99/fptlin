#pragma once

#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace fptlin {

#define MIN_TIME std::numeric_limits<time_type>::lowest()
#define MAX_TIME std::numeric_limits<time_type>::max()
#define MAX_PROC_NUM std::numeric_limits<proc_type>::digits
#define EMPTY_VALUE -1

#define FPTLIN_METHOD_EXPAND(MACRO) \
  MACRO(PUSH)                       \
  MACRO(POP)                        \
  MACRO(PEEK)                       \
  MACRO(ENQ)                        \
  MACRO(DEQ)                        \
  MACRO(INSERT)                     \
  MACRO(POLL)                       \
  MACRO(REMOVE)                     \
  MACRO(CONTAINS)                   \
  MACRO(INCR)                       \
  MACRO(DECR)                       \
  MACRO(READ_MODIFY_WRITE)

enum Method {
#define FPTLIN_METHOD_LIST(ENUM) ENUM,
  FPTLIN_METHOD_EXPAND(FPTLIN_METHOD_LIST)
#undef FPTLIN_METHOD_LIST
};

inline Method stomethod(const std::string& str) {
#define FPTLIN_METHOD_TRANSLATE(ENUM) \
  if (str == #ENUM) return Method::ENUM;
  FPTLIN_METHOD_EXPAND(FPTLIN_METHOD_TRANSLATE)
#undef FPTLIN_METHOD_TRANSLATE
  throw std::invalid_argument("Unknown method: " + str);
}

inline std::string methodtos(const Method& method) {
#define FPTLIN_METHODSTR_TRANSLATE(ENUM) \
  case ENUM:                             \
    return #ENUM;
  switch (method) {
    FPTLIN_METHOD_EXPAND(FPTLIN_METHODSTR_TRANSLATE)
    default:
      throw std::invalid_argument("Unknown method: " + std::to_string(method));
  }
#undef FPTLIN_METHODSTR_TRANSLATE
}

typedef unsigned long long time_type;
typedef unsigned int id_type;
typedef unsigned int proc_type;

// As operations are assumed to be complete, return values are known and can be
// embedded within value_type if desired
template <typename value_type>
struct operation_t {
  id_type id;
  proc_type proc;
  Method method;
  value_type value;
  time_type startTime;
  time_type endTime;
};

template <typename value_type>
using history_t = std::vector<operation_t<value_type>>;

template <typename value_type>
using events_t =
    std::vector<std::tuple<time_type, bool, operation_t<value_type>*>>;

}  // namespace fptlin