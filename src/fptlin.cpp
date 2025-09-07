#include <getopt.h>
#include <unistd.h>

#include <chrono>
#include <iostream>

#include "algo/algos.h"
#include "history_reader.h"

using namespace fptlin;

typedef std::chrono::steady_clock hr_clock;
typedef int default_value_type;

hr_clock::time_point start, end;
bool result;
std::string hist_type;
size_t hist_size;

#define FPTLIN_ADT_EXPAND(VARIADIC_MACRO)           \
  VARIADIC_MACRO(stack, default_value_type)         \
  VARIADIC_MACRO(queue, default_value_type)         \
  VARIADIC_MACRO(priorityqueue, default_value_type) \
  VARIADIC_MACRO(rmw, default_value_type, default_value_type)

void monitor(const std::string& input_file) {
  history_reader reader(input_file);
  hist_type = reader.get_type_s();

#define FPTLIN_ADT_SWITCH(ADT, ...)             \
  if (hist_type == #ADT) {                      \
    auto hist = reader.get_hist<__VA_ARGS__>(); \
    hist_size = hist.size();                    \
    start = hr_clock::now();                    \
    result = ADT::is_linearizable(hist);        \
    end = hr_clock::now();                      \
    return;                                     \
  }
  FPTLIN_ADT_EXPAND(FPTLIN_ADT_SWITCH)
#undef FPTLIN_ADT_SWITCH

  throw std::invalid_argument("Unknown data type '" + hist_type + "'");
}

#undef FPTLIN_ADT_EXPAND

void print_usage() {
  std::cout << "Usage: ./fptlin [-tvh] <history_file>\n"
            << "Options:\n"
            << "  -t\treport time taken in seconds\n"
            << "  -v\tprint verbose information\n"
            << "  -h\tinclude headers\n";
}

int main(int argc, char* argv[]) {
  const char* titles[]{"result", "time_taken", "size", "exclude_peeks"};
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

  monitor(input_file);

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
  if (print_size) std::cout << hist_size << " ";
  std::cout << std::endl;

  return 0;
}