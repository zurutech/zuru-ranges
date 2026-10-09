// Compile-only checks: the views, ranges::to and subrange in constant expressions ([range.filter]
// and the others are constexpr throughout). Built as an OBJECT library, never run.
#include "zr/algorithm.h"
#include "zr/ranges.h"

#include <array>
#include <functional>
#include <utility>
#include <vector>
#include <version>

// MSVC 14.29's constant evaluator loses the active member of non-propagating-cache's union: the
// views that cache begin() (filter, drop, reverse, join, chunk_by) are checked from 14.30.
#if defined(_MSC_VER) && !defined(__clang__) && _MSC_VER < 1930
#define ZR_CONSTEXPR_CACHES 0
#else
#define ZR_CONSTEXPR_CACHES 1
#endif

namespace {
namespace v = zr::views;

constexpr int sum(auto&& r) {
    int s = 0;
    for (auto&& x : r) {
        s += x;
    }
    return s;
}

#if ZR_CONSTEXPR_CACHES
// filter: the begin() cache (non-propagating-cache's union storage) at compile time.
constexpr bool filter_cx() {
    std::array<int, 6> a{1, 2, 3, 4, 5, 6};
    auto f = a | v::filter([](int x) { return x % 2 == 0; });
    return sum(f) == 12 && sum(f) == 12;
}
static_assert(filter_cx());

constexpr bool drop_reverse_cx() {
    std::array<int, 6> a{1, 2, 3, 4, 5, 6};
    auto d = a | v::drop(2);
    auto r = a | v::filter([](int x) { return x > 1; }) | v::reverse; // reverse's begin() cache
    return sum(d) == 18 && sum(r) == 20 && sum(r) == 20;
}
static_assert(drop_reverse_cx());
#endif

// A capturing lambda: movable-box's value-only storage, reconstructed on assignment.
constexpr bool transform_capture_cx() {
    std::array<int, 3> a{1, 2, 3};
    int k = 2;
    auto t = a | v::transform([k](int x) { return x * k; });
    auto t2 = t;
    t2 = t;
    auto t3 = std::move(t2);
    t3 = std::move(t);
    return sum(t3) == 12;
}
static_assert(transform_capture_cx());

#if ZR_CONSTEXPR_CACHES && defined(__cpp_lib_optional) && __cpp_lib_optional >= 202106L
// join: inner_ (optional) and, over prvalue inner ranges, non-propagating-cache's emplace-deref.
constexpr bool join_cx() {
    std::array<std::array<int, 2>, 2> aa{{{1, 2}, {3, 4}}};
    auto j = aa | v::join;
    auto j2 = aa | v::transform([](const std::array<int, 2>& x) { return x; }) | v::join;
    return sum(j) == 10 && sum(j2) == 10;
}
static_assert(join_cx());
#endif

#if ZR_CONSTEXPR_CACHES
constexpr bool chunk_by_cx() {
    std::array<int, 6> a{1, 2, 4, 5, 7, 8};
    int n = 0;
    for (auto chunk : a | v::chunk_by([](int x, int y) { return y == x + 1; })) {
        n += sum(chunk);
    }
    return n == 27;
}
static_assert(chunk_by_cx());
#endif

constexpr bool zip_enumerate_cartesian_cx() {
    std::array<int, 3> a{1, 2, 3};
    std::array<int, 2> b{10, 20};
    int s = 0;
    for (auto [x, y] : v::zip(a, b)) {
        s += x * y;
    }
    for (auto [i, x] : a | v::enumerate) {
        s += static_cast<int>(i) * x;
    }
    for (auto [x, y] : v::cartesian_product(a, b)) {
        s += x + y;
    }
    return s == 50 + 8 + 102;
}
static_assert(zip_enumerate_cartesian_cx());

#if defined(__cpp_lib_variant) && __cpp_lib_variant >= 202106L
// concat: variant::emplace is constexpr from P2231R1 (libstdc++ 12).
constexpr bool concat_cx() {
    std::array<int, 3> a{1, 2, 3};
    std::array<int, 2> b{10, 20};
    return sum(v::concat(a, b)) == 36;
}
static_assert(concat_cx());
#endif

constexpr bool take_cx() {
    std::array<int, 5> a{1, 2, 3, 4, 5};
    auto t = v::iota(0, 10) | v::take(3);
    return sum(t) == 3 && sum(a | v::take(2)) == 3;
}
static_assert(take_cx());

#if defined(__cpp_lib_constexpr_vector) && __cpp_lib_constexpr_vector >= 201907L
constexpr bool to_cx() {
    std::array<int, 5> a{1, 2, 3, 4, 5};
    auto vec = a | v::take(2) | zr::ranges::to<std::vector<int>>();
    return vec.size() == 2 && vec[1] == 2;
}
static_assert(to_cx());
#endif

constexpr bool elements_cx() {
    std::array<std::pair<int, int>, 2> p{{{1, 10}, {2, 20}}};
    return sum(p | v::keys) == 3 && sum(p | v::values) == 30;
}
static_assert(elements_cx());

// [alg.fold]
constexpr bool fold_cx() {
    std::array<int, 4> a{1, 2, 3, 4};
    auto r = zr::ranges::fold_left_with_iter(a, 0, std::plus<>{});
    auto f = zr::ranges::fold_left_first_with_iter(a | v::take(0), std::plus<>{});
    return zr::ranges::fold_left(a, 0, std::plus<>{}) == 10 && zr::ranges::fold_right(a, 0, std::minus<>{}) == 1 - (2 - (3 - (4 - 0))) &&
           *zr::ranges::fold_left_first(a, std::plus<>{}) == 10 && *zr::ranges::fold_right_last(a, std::minus<>{}) == 1 - (2 - (3 - 4)) &&
           r.in == a.end() && r.value == 10 && !f.value.has_value();
}
static_assert(fold_cx());

constexpr bool subrange_cx() {
    std::array<int, 4> a{1, 2, 3, 4};
    zr::ranges::subrange s(a.begin(), a.end());
    auto s2 = s;
    s2 = s;
    return sum(s2.next(1)) == 9;
}
static_assert(subrange_cx());
} // namespace
