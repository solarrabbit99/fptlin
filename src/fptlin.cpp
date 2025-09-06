#include <getopt.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <memory>

#include "algo/priorityqueue_lin.h"
#include "algo/queue_lin.h"
#include "algo/rmw_lin.h"
#include "algo/stack_lin.h"
#include "history_reader.h"

using namespace fptlin;

typedef std::chrono::steady_clock hr_clock;
typedef int default_value_type;

#define SUPPORT_DS(TYPE) \
  if (type == #TYPE) return TYPE::is_linearizable<value_type>;

template <typename value_type>
auto get_monitor(const std::string& type) {
  SUPPORT_DS(stack);
  SUPPORT_DS(queue);
  SUPPORT_DS(priorityqueue);
  throw std::invalid_argument("Unknown data type");
}

template <pair_type value_type>
auto get_monitor(const std::string& type) {
  SUPPORT_DS(rmw);
  throw std::invalid_argument("Unknown data type");
}

#undef SUPPORT_DS

void print_usage() {
  std::cout << "Usage: ./fptlin [-tvh] <history_file>\n"
            << "Options:\n"
            << "  -t\treport time taken in seconds\n"
            << "  -v\tprint verbose information\n"
            << "  -h\tinclude headers\n";
}

int main(int argc, char* argv[]) {
  const char* titles[]{"result", "time_taken", "operations", "exclude_peeks"};
  bool to_print[]{true, false, false};
  auto& [_, print_time, print_size] = to_print;
  bool print_header = false;
  std::string input_file;

  if (argc <= 1) {
    print_usage();
    exit(EXIT_SUCCESS);
  }

  int flag;
  int long_optind;
  static struct option long_options[] = {{"help", no_argument, 0, 0},
                                         {0, 0, 0, 0}};
  while ((flag = getopt_long(argc, argv, "txvh", long_options, &long_optind)) !=
         -1)
    switch (flag) {
      case 0:
        print_usage();
        exit(EXIT_SUCCESS);
      case 't':
        print_time = true;
        break;
      case 'v':
        std::fill(to_print, to_print + sizeof(to_print), true);
        break;
      case 'h':
        print_header = true;
        break;
      case '?':
        std::cerr << "Unknown option `" << optopt << "'.\n";
        exit(EXIT_FAILURE);
      default:
        abort();
    }
  if (optind < argc)
    input_file = argv[optind];
  else {
    std::cout << "Please provide a file path\n";
    exit(EXIT_FAILURE);
  }

  history_reader reader(input_file);
  std::string histType = reader.get_type_s();

  hr_clock::time_point start, end;
  bool result;
  size_t operations;
  if (histType == "rmw") {
    using pair_default_value_t =
        std::pair<default_value_type, default_value_type>;
    auto hist = reader.get_hist<pair_default_value_t>();
    operations = hist.size();
    auto monitor = get_monitor<pair_default_value_t>(histType);
    start = hr_clock::now();
    result = monitor(hist);
  } else {
    auto hist = reader.get_hist<default_value_type>();
    operations = hist.size();
    auto monitor = get_monitor<default_value_type>(histType);
    start = hr_clock::now();
    result = monitor(hist);
  }

  end = hr_clock::now();
  int64_t time_micros =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();

  if (print_header) {
    for (size_t i = 0; i < sizeof(to_print); ++i)
      if (to_print[i]) std::cout << titles[i] << " ";
    std::cout << "\n";
  }

  std::cout << result << " ";
  if (print_time) std::cout << (time_micros / 1e6) << " ";
  if (print_size) std::cout << operations << " ";
  std::cout << std::endl;

  return 0;
}