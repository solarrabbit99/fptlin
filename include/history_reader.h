#pragma once

#include <fstream>
#include <sstream>

#include "commons/utility.h"
#include "definitions.h"

namespace fptlin {

struct history_reader {
 public:
  history_reader(const std::string& path) : path(path) {}

  template <typename value_type>
  history_t<value_type> get_hist() {
    std::ifstream f(path);
    std::string line;
    history_t<value_type> hist;
    id_type id = 0;
    while (std::getline(f, line)) {
      std::stringstream ss{line};
      line = trim(line);
      if (line.empty() || line[0] == '#') continue;

      proc_type proc;
      std::string methodStr;
      value_type value;
      time_type startTime, endTime;

      ss >> proc >> startTime >> endTime >> methodStr >> value;

      hist.emplace_back(++id, proc, stomethod(methodStr), value, startTime,
                        endTime);
    }
    return hist;
  }

  template <pair_type value_type>
  history_t<value_type> get_hist() {
    std::ifstream f(path);
    std::string line;
    history_t<value_type> hist;
    id_type id = 0;
    while (std::getline(f, line)) {
      std::stringstream ss{line};
      line = trim(line);
      if (line.empty() || line[0] == '#') continue;

      proc_type proc;
      std::string methodStr;
      value_type value;
      time_type startTime, endTime;

      ss >> proc >> startTime >> endTime >> methodStr >> value.first >>
          value.second;

      hist.emplace_back(++id, proc, stomethod(methodStr), value, startTime,
                        endTime);
    }
    return hist;
  }

  std::string get_type_s() {
    std::ifstream f(path);
    std::string line;
    if (!std::getline(f, line) || line[0] != '#') return "";

    return trim(line.substr(1));
  }

 private:
  std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(' ');
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(' ');
    if (end == std::string::npos) return "";
    return str.substr(start, end - start + 1);
  }

  const std::string path;
};

}  // namespace fptlin