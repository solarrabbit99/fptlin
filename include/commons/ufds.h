#include <unordered_map>
#include <vector>

namespace fptlin {

template <typename value_type, typename hash_type = std::hash<value_type>,
          typename equal_type = std::equal_to<value_type>>
struct ufds {
  value_type find(value_type v) { return values[find(assign(v))]; }

  void join(value_type a, value_type b) {
    std::size_t tmp_a = find(assign(a));
    std::size_t tmp_b = find(assign(b));
    if (subsizes[tmp_a] > subsizes[tmp_b]) std::swap(tmp_a, tmp_b);
    subsizes[tmp_b] += subsizes[tmp_a];
    parents[tmp_a] = tmp_b;
  }

  void reserve(std::size_t max_size) {
    parents.reserve(max_size);
    subsizes.reserve(max_size);
    values.reserve(max_size);
    value_index.reserve(max_size);
  }

 private:
  std::size_t assign(value_type v) {
    auto iter = value_index.find(v);
    if (iter != value_index.end()) return *iter;
    std::size_t new_i = values.size();
    values.push_back(v);
    value_index[v] = new_i;
    parents.push_back(new_i);
    subsizes.push_back(1U);
    return new_i;
  }

  std::size_t find(std::size_t v) {
    while (parents[v] != v) v = parents[v];
    return v;
  }

  std::vector<std::size_t> parents;
  std::vector<std::size_t> subsizes;
  std::vector<value_type> values;
  std::unordered_map<value_type, std::size_t, hash_type, equal_type>
      value_index;
};

}  // namespace fptlin
