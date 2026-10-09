// elements_view, keys, values over std and Unreal bases, both build configurations.
#include "CoreMinimal.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/elements_view.h"
#include "zr/ranges/views/enumerate_view.h"
#include "zr/ranges/views/zip_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <array>
#include <cstddef>
#include <map>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {

template <class R> std::vector<int> to_vector(R&& r) {
    std::vector<int> out;
    for (auto&& x : r) {
        out.push_back(static_cast<int>(x));
    }
    return out;
}

using pairs = std::vector<std::pair<int, long>>;
using vec_values = decltype(std::declval<pairs&>() | views::values);
using map_keys = decltype(std::declval<std::map<int, int>&>() | views::keys);
using tmap_values = decltype(std::declval<TMap<int, int>&>() | views::values);
using tmap_ckeys = decltype(std::declval<const TMap<int, int>&>() | views::keys);
using tarray_keys = decltype(std::declval<TArray<TPair<int, int>>&>() | views::keys);

// [range.elements.iterator]: the base's category capped at random_access.
static_assert(random_access_range<vec_values> && !contiguous_range<vec_values> && sized_range<vec_values>);
static_assert(std::is_same_v<range_reference_t<vec_values>, long&> && std::is_same_v<range_value_t<vec_values>, long>);
static_assert(bidirectional_range<map_keys> && std::is_same_v<range_reference_t<map_keys>, const int&>);
static_assert(std::is_same_v<map_keys, keys_view<ref_view<std::map<int, int>>>>);
static_assert(forward_range<tmap_values> && sized_range<tmap_values> && std::is_same_v<range_reference_t<tmap_values>, int&>);
static_assert(std::is_same_v<range_reference_t<tmap_ckeys>, const int&>);
static_assert(random_access_range<tarray_keys> && !contiguous_range<tarray_keys> && sized_range<tarray_keys>);

// has-tuple-element: no tuple protocol, or N out of range.
template <class R, std::size_t N>
concept elementable = requires(R& r) { r | views::elements<N>; };
static_assert(elementable<pairs, 1> && !elementable<pairs, 2> && !elementable<std::vector<int>, 0>);

// [ranges.syn] enable_borrowed_range.
static_assert(borrowed_range<vec_values> && !borrowed_range<elements_view<owning_view<pairs>, 0>>);

} // namespace

TEST(ElementsView, StdMap) {
    std::map<int, int> m{{1, 10}, {2, 20}};
    EXPECT_EQ(to_vector(m | views::keys), (std::vector<int>{1, 2}));
    EXPECT_EQ(to_vector(m | views::values), (std::vector<int>{10, 20}));
    EXPECT_EQ(to_vector(m | views::elements<1>), (std::vector<int>{10, 20}));
}

TEST(ElementsView, RandomAccessAndWritable) {
    pairs v{{1, 10}, {2, 20}};
    auto vals = v | views::values;
    vals[1] = 21;
    EXPECT_EQ(v[1].second, 21);
    EXPECT_EQ(vals.end() - vals.begin(), 2);
    EXPECT_EQ(vals.size(), 2U);
    std::array<std::array<int, 3>, 2> rows{{{1, 2, 3}, {4, 5, 6}}};
    EXPECT_EQ(to_vector(rows | views::elements<2>), (std::vector<int>{3, 6}));
}

TEST(ElementsView, ProxyReferences) {
    std::vector<int> a{1, 2};
    std::vector<int> b{3, 4};
    auto second = views::zip(a, b) | views::elements<1>;
    static_assert(std::is_same_v<range_reference_t<decltype(second)>, int&>);
    for (int& x : second) {
        x *= 10;
    }
    EXPECT_EQ(b, (std::vector<int>{30, 40}));

    // enumerate's reference is a tuple<D, T&> prvalue: element 0 by value.
    auto indices = a | views::enumerate | views::keys;
    static_assert(!std::is_reference_v<range_reference_t<decltype(indices)>>);
    EXPECT_EQ(to_vector(indices), (std::vector<int>{0, 1}));
}

TEST(ElementsView, UnrealMap) {
    TMap<int, int> m;
    m.Add(1, 10);
    m.Add(2, 20);
    EXPECT_EQ(to_vector(m | views::keys), (std::vector<int>{1, 2}));
    for (int& x : m | views::values) {
        x += 1;
    }
    EXPECT_EQ(to_vector(m | views::values), (std::vector<int>{11, 21}));
    EXPECT_EQ(size(m | views::values), 2);
}

TEST(ElementsView, UnrealArrayOfPairs) {
    TArray<TPair<int, int>> a;
    a.Add(TPair<int, int>(1, 5));
    a.Add(TPair<int, int>(2, 6));
    auto k = a | views::keys;
    EXPECT_EQ(k[1], 2);
    EXPECT_EQ(element_at(k, 0), 1);
    EXPECT_EQ(to_vector(a | views::values), (std::vector<int>{5, 6}));
}
