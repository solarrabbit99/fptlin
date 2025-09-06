#pragma once

#include <tuple>

namespace fptlin {

template <typename T>
struct is_std_pair : std::false_type {};

template <typename A, typename B>
struct is_std_pair<std::pair<A, B>> : std::true_type {};

template <typename T>
concept pair_type = is_std_pair<T>::value;

}  // namespace fptlin