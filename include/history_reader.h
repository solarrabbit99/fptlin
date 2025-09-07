#pragma once

#include <fstream>
#include <sstream>

#include "definitions.h"

namespace fptlin {

struct history_reader {
 public:
  history_reader(const std::string& path) : path(path) {}

  template <typename... Args>
  history_t<std::conditional_t<
      (sizeof...(Args) == 1),
      typename std::tuple_element<0, std::tuple<Args...>>::type,
      std::tuple<Args...>>>
  get_hist() {
    using value_type = std::conditional_t<
        (sizeof...(Args) == 1),
        typename std::tuple_element<0, std::tuple<Args...>>::type,
        std::tuple<Args...>>;

    std::ifstream f(path);
    std::string line;
    history_t<value_type> hist;
    id_type id = 0;
    while (std::getline(f, line)) {
      if (line.empty() || line[0] == '#') continue;

      std::stringstream ss{line};
      proc_type proc;
      std::string methodStr;
      time_type startTime, endTime;
      ss >> proc >> startTime >> endTime >> methodStr;

      value_type value;
      if constexpr (sizeof...(Args) == 1)
        ss >> value;
      else
        parse<value_type, Args...>(ss, value);

      hist.emplace_back(++id, proc, stomethod(methodStr), value, startTime,
                        endTime);
    }
    return hist;
  }

  std::string get_type_s() {
    std::ifstream f(path);
    std::string line;
    if (!std::getline(f, line) || line.empty() || line[0] != '#') return "";

    std::string_view sv{line};
    sv.remove_prefix(1);

    auto start = sv.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return "";
    auto end = sv.find_last_not_of(" \t\r\n");

    return std::string{sv.substr(start, end - start + 1)};
  }

 private:
  template <typename Tuple, typename T, typename... Args>
  void parse(std::istream& ss, Tuple& value) {
    ss >> std::get<std::tuple_size_v<Tuple> - sizeof...(Args) - 1>(value);
    if constexpr (sizeof...(Args) > 0) parse<Tuple, Args...>(ss, value);
  }

  const std::string path;
};

}  // namespace fptlin