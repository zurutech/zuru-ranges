// Header consumer of zr (find_package(zuru-ranges) or FetchContent, zuru-ranges::zuru-ranges).
#include "zr/ranges.h"

#include <vector>

int main() {
    const std::vector<int> v = zr::views::iota(1, 5)
                             | zr::views::transform([](int n) { return n * n; })
                             | zr::ranges::to<std::vector<int>>();
    return v == std::vector<int>{1, 4, 9, 16} ? 0 : 1;
}
