// Interop with std::ranges: a zr view over std ranges only models the std concepts and composes
// with std adaptors in both directions:
//     std::views::all(v)      | zr::views::transform(f)
//     zr::views::concat(v, a) | std::views::transform(f)
// Over an Unreal base it is a zr range only. Built in both iterator configurations.
#include "CoreMinimal.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/cartesian_product_view.h"
#include "zr/ranges/views/chunk_by_view.h"
#include "zr/ranges/views/concat_view.h"
#include "zr/ranges/views/drop_view.h"
#include "zr/ranges/views/elements_view.h"
#include "zr/ranges/views/empty_view.h"
#include "zr/ranges/views/enumerate_view.h"
#include "zr/ranges/views/filter_view.h"
#include "zr/ranges/views/iota_view.h"
#include "zr/ranges/views/join_view.h"
#include "zr/ranges/views/reverse_view.h"
#include "zr/ranges/views/subrange.h"
#include "zr/ranges/views/take_view.h"
#include "zr/ranges/views/transform_view.h"
#include "zr/ranges/views/zip_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <map>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

// The MSVC STL after 14.29 and before 14.37 specializes basic_common_reference for tuple
// (P2321R2) without the tuple(tuple<U...>&) constructor, so no tuple-of-references iterator is
// std::indirectly_readable: zip, enumerate and cartesian_product cannot reach std there.
#if defined(_MSVC_STL_UPDATE) && _MSVC_STL_UPDATE > 202105L && _MSVC_STL_UPDATE < 202305L
#define ZR_TEST_STD_TUPLE_PROXIES 0
#else
#define ZR_TEST_STD_TUPLE_PROXIES 1
#endif

// A program-defined view and a program-defined borrowed range, each opted in through the
// std trait only ([range.view], [range.range]). zr's traits are the std ones, so zr agrees.
struct UserView {
    int* begin() const;
    int* end() const;
};
struct UserBorrowed {
    int* begin() const;
    int* end() const;
};
template <> inline constexpr bool std::ranges::enable_view<UserView> = true;
template <> inline constexpr bool std::ranges::enable_borrowed_range<UserBorrowed> = true;

namespace {

constexpr int add_one(int x) { return x + 1; }
constexpr bool is_even(int x) { return x % 2 == 0; }

// ---- Compile-time: std concept modelling when every base is a std range --------------
using StdConcat = decltype(zr::views::concat(std::declval<std::vector<int>&>(), std::declval<std::array<int, 2>&>()));
static_assert(std::ranges::range<StdConcat>);
static_assert(std::ranges::bidirectional_range<StdConcat>);
static_assert(std::ranges::common_range<StdConcat>);
static_assert(std::ranges::sized_range<StdConcat>);
static_assert(std::input_iterator<std::ranges::iterator_t<StdConcat>>);
static_assert(std::sentinel_for<std::ranges::sentinel_t<StdConcat>, std::ranges::iterator_t<StdConcat>>);

using StdTransform = decltype(zr::views::transform(std::declval<std::vector<int>&>(), add_one));
static_assert(std::ranges::forward_range<StdTransform>);
static_assert(std::input_iterator<std::ranges::iterator_t<StdTransform>>);

#if ZR_TEST_STD_TUPLE_PROXIES
// zip and enumerate: proxy-reference views, so iterator_category stays input_iterator_tag
// while iterator_concept (what the concepts read) reaches forward.
using StdZip = decltype(zr::views::zip(std::declval<std::vector<int>&>(), std::declval<std::array<int, 2>&>()));
static_assert(std::ranges::forward_range<StdZip>);
static_assert(std::ranges::common_range<StdZip>);
static_assert(std::ranges::sized_range<StdZip>);
static_assert(std::is_same_v<std::iterator_traits<std::ranges::iterator_t<StdZip>>::iterator_category, std::input_iterator_tag>);

using StdCartesian = decltype(zr::views::cartesian_product(std::declval<std::vector<int>&>(), std::declval<std::array<int, 2>&>()));
static_assert(std::ranges::forward_range<StdCartesian>);
static_assert(std::ranges::common_range<StdCartesian>);
static_assert(std::ranges::sized_range<StdCartesian>);
static_assert(std::is_same_v<std::iterator_traits<std::ranges::iterator_t<StdCartesian>>::iterator_category, std::input_iterator_tag>);

using StdEnumerate = decltype(zr::views::enumerate(std::declval<std::vector<int>&>()));
static_assert(std::ranges::forward_range<StdEnumerate>);
static_assert(std::ranges::common_range<StdEnumerate>);
static_assert(std::ranges::sized_range<StdEnumerate>);
static_assert(std::is_same_v<std::ranges::range_difference_t<StdEnumerate>, std::ptrdiff_t>);
#endif

// filter and join: reference-forwarding, iterator_category from the base's
// ([range.filter.iterator], [range.join.iterator]).
using StdFilter = decltype(zr::views::filter(std::declval<std::vector<int>&>(), is_even));
static_assert(std::ranges::bidirectional_range<StdFilter>);
static_assert(std::is_same_v<std::iterator_traits<std::ranges::iterator_t<StdFilter>>::iterator_category, std::bidirectional_iterator_tag>);

using StdJoin = decltype(zr::views::join(std::declval<std::vector<std::vector<int>>&>()));
static_assert(std::ranges::forward_range<StdJoin>);
static_assert(std::ranges::bidirectional_range<StdJoin> && std::ranges::common_range<StdJoin>);
static_assert(std::sentinel_for<std::ranges::sentinel_t<StdJoin>, std::ranges::iterator_t<StdJoin>>);

// reverse: reverse_iterator over a std base; over an Unreal base the index iterator
// (reverse_view.h), a std random-access iterator in every configuration.
using StdReverse = decltype(zr::views::reverse(std::declval<std::vector<int>&>()));
using UeReverse = decltype(zr::views::reverse(std::declval<TArray<int>&>()));
static_assert(std::ranges::forward_range<StdReverse>);
static_assert(std::ranges::random_access_range<StdReverse> && std::ranges::random_access_range<UeReverse>);
static_assert(std::ranges::sized_range<UeReverse>);

// A gated zr view is a std::ranges::view ([range.view]/7 opt-in in view_interface.h), so
// std adaptors take it as is, also on libraries without P2415R2's owning_view (libstdc++ 11).
static_assert(std::ranges::view<StdTransform>);
static_assert(std::ranges::enable_view<StdTransform>);

// The converse: whatever std::ranges::enable_view accepts is a zr view too, so views::all
// decay-copies it rather than wrapping it in a ref_view or a move-only owning_view.
using StdIota = decltype(std::views::iota(0, 3));
using StdFilterView = decltype(std::views::filter(std::declval<std::vector<int>&>(), is_even));
static_assert(zr::ranges::view<std::span<int>>);
static_assert(zr::ranges::view<std::string_view>);
static_assert(zr::ranges::view<StdIota>);
static_assert(zr::ranges::view<StdFilterView>);
static_assert(std::same_as<decltype(zr::views::all(std::declval<std::span<int>&>())), std::span<int>>);
static_assert(std::same_as<decltype(zr::views::all(std::declval<StdIota>())), StdIota>);
static_assert(!zr::ranges::view<std::vector<int>>);

// A zr adaptor over an rvalue std view holds the view itself, so it stays copyable and an
// lvalue of it feeds std adaptors (an owning_view would make it move-only).
using ZrOverStdView = decltype(zr::views::filter(std::views::iota(0, 3), is_even));
static_assert(std::copyable<ZrOverStdView>);
static_assert(std::ranges::viewable_range<ZrOverStdView&>);

static_assert(zr::ranges::view<UserView>);
static_assert(zr::ranges::borrowed_range<UserBorrowed>);
static_assert(!zr::ranges::borrowed_range<UserView>);

// zr's own borrowed views are borrowed to std as well, so std algorithms on an rvalue of
// them return an iterator, not std::ranges::dangling.
static_assert(std::ranges::borrowed_range<zr::ranges::iota_view<int, int>>);
static_assert(std::ranges::enable_borrowed_range<zr::ranges::empty_view<int>>);
static_assert(std::ranges::borrowed_range<decltype(zr::views::all(std::declval<std::vector<int>&>()))>);
static_assert(std::ranges::borrowed_range<zr::ranges::subrange<int*, int*>>);
static_assert(std::ranges::borrowed_range<decltype(zr::views::reverse(std::declval<std::vector<int>&>()))>);
static_assert(!std::ranges::borrowed_range<decltype(zr::views::reverse(std::declval<std::vector<int>>()))>);
static_assert(!std::ranges::borrowed_range<decltype(zr::views::filter(std::declval<std::vector<int>&>(), is_even))>);

// ---- Compile-time: the gate degrades for Unreal (relaxed) bases ----------------------
// TSet's iterator is neither semiregular nor equality-comparable in either configuration: a
// concat including it is a zr range but not a std range.
using UeConcat = decltype(zr::views::concat(std::declval<std::vector<int>&>(), std::declval<TSet<int>&>()));
static_assert(!std::ranges::range<UeConcat>);
static_assert(zr::ranges::range<UeConcat>);
static_assert(zr::ranges::forward_range<UeConcat>);

// Same for cartesian_product: a TArray leg degrades the gate away in the debug configuration
// (checked iterator) and TSet does so in both, while the zr range is untouched.
using UeCartesian = decltype(zr::views::cartesian_product(std::declval<std::vector<int>&>(), std::declval<TSet<int>&>()));
static_assert(!std::ranges::range<UeCartesian>);
static_assert(zr::ranges::range<UeCartesian>);
static_assert(zr::ranges::forward_range<UeCartesian>);

} // namespace

// zr -> std, one pipeline per view, each ending in a std adaptor.
TEST(StdInterop, GatedViewsFeedStdAdaptors) {
    std::vector<int> a{1, 2, 3, 4};
    std::vector<int> b{10, 20, 30, 40};

    int sum = 0;
    for (int v : zr::views::filter(a, is_even) | std::views::transform(add_one)) {
        sum += v;
    }
    EXPECT_EQ(sum, 8); // (2+1) + (4+1)

#if ZR_TEST_STD_TUPLE_PROXIES
    // zip yields a tuple; take the second component through a std adaptor.
    sum = 0;
    for (int v : zr::views::zip(a, b) | std::views::transform([](const auto& t) { return std::get<1>(t); })) {
        sum += v;
    }
    EXPECT_EQ(sum, 100);

    // enumerate's index half is signed, so this comparison needs no cast.
    sum = 0;
    for (auto t : zr::views::enumerate(b) | std::views::filter([](const auto& e) { return std::get<0>(e) >= 2; })) {
        sum += std::get<1>(t);
    }
    EXPECT_EQ(sum, 70); // indices 2 and 3 -> 30 + 40
#endif

    std::vector<std::vector<int>> nested{{1, 2}, {}, {3}};
    sum = 0;
    for (int v : zr::views::join(nested) | std::views::transform(add_one)) {
        sum += v;
    }
    EXPECT_EQ(sum, 9); // (1+1)+(2+1)+(3+1)

#if ZR_TEST_STD_TUPLE_PROXIES
    // cartesian_product yields a tuple, like zip; multiply the two components.
    sum = 0;
    for (int v : zr::views::cartesian_product(a, b) | std::views::transform([](const auto& t) { return std::get<0>(t) * std::get<1>(t); })) {
        sum += v;
    }
    EXPECT_EQ(sum, 1000); // (1+2+3+4) * (10+20+30+40)
#endif
}

// take, drop, chunk_by, elements over std bases are std views.
TEST(StdInterop, SlicingAndGroupingViewsFeedStdAdaptors) {
    std::vector<int> a{1, 2, 3, 4, 5};
    std::vector<int> got;
    for (int v : a | zr::views::drop(1) | zr::views::take(3) | std::views::transform(add_one)) {
        got.push_back(v);
    }
    EXPECT_EQ(got, (std::vector<int>{3, 4, 5}));
    EXPECT_EQ(std::ranges::distance(zr::views::iota(0) | zr::views::take(4) | std::views::filter(is_even)), 2);

    // take over a non-sized base: std::counted_iterator and the take_view sentinel.
    static_assert(std::ranges::forward_range<decltype(zr::views::filter(a, is_even) | zr::views::take(1))>);

    std::vector<int> runs{1, 1, 2, 3, 3, 3};
    got.clear();
    for (auto n : runs | zr::views::chunk_by(std::ranges::equal_to{}) | std::views::transform(std::ranges::size)) {
        got.push_back(static_cast<int>(n));
    }
    EXPECT_EQ(got, (std::vector<int>{2, 1, 3}));

    std::map<int, int> m{{1, 10}, {2, 20}};
    got.clear();
    for (int v : m | zr::views::values | std::views::reverse) {
        got.push_back(v);
    }
    EXPECT_EQ(got, (std::vector<int>{20, 10}));
    EXPECT_TRUE(std::ranges::equal(m | zr::views::keys, m | std::views::keys));
}

// reverse over a random-access Unreal base is a std range: its iterator is an index, not a UE
// iterator.
TEST(StdInterop, UnrealBackedReverseFeedsStdAdaptor) {
    TArray<int> arr{1, 2, 3, 4};

    std::vector<int> got;
    for (int v : zr::views::reverse(arr) | std::views::filter(is_even)) {
        got.push_back(v);
    }
    EXPECT_EQ(got, (std::vector<int>{4, 2}));
}

// std -> zr.
TEST(StdInterop, StdRangeIntoZrAdaptor) {
    std::vector<int> a{1, 2, 3};
    auto piped = std::views::all(a) | zr::views::transform([](int x) { return x * 2; });
    int s = 0;
    for (int v : piped) {
        s += v;
    }
    EXPECT_EQ(s, 12); // (1+2+3)*2
}

// zr -> std.
TEST(StdInterop, ZrConcatIntoStdAdaptor) {
    std::vector<int> a{1, 2, 3};
    std::array<int, 2> d{4, 5};

    auto doubled = zr::views::concat(a, d) | std::views::transform([](int x) { return x * 2; });
    int s = 0;
    for (int v : doubled) {
        s += v;
    }
    EXPECT_EQ(s, 30); // (1+2+3+4+5)*2

    // Bidirectionality is genuine: std::views::reverse requires a bidirectional and
    // common range, which an all-std concat models.
    std::vector<int> rev;
    for (int v : zr::views::concat(a, d) | std::views::reverse) {
        rev.push_back(v);
    }
    EXPECT_EQ(rev, (std::vector<int>{5, 4, 3, 2, 1}));
}

// A longer pipeline crossing the zr/std boundary twice.
TEST(StdInterop, MixedChainCrossesBoundaryTwice) {
    std::vector<int> a{1, 2, 3};
    std::array<int, 2> d{4, 5};
    auto chain = zr::views::concat(a, d)                             // 1,2,3,4,5
               | zr::views::transform([](int x) { return x * 2; })   // 2,4,6,8,10
               | std::views::filter([](int x) { return x > 4; })      // 6,8,10
               | std::views::transform([](int x) { return x + 100; });// 106,108,110
    int s = 0;
    for (int v : chain) {
        s += v;
    }
    EXPECT_EQ(s, 324);
}

// An Unreal-backed concat composes in a zr pipeline although it is not a std range.
TEST(StdInterop, ZrOverStdViewLvalueIntoStdAdaptor) {
    auto evens = std::views::iota(0, 7) | zr::views::filter(is_even);
    int sum = 0;
    for (int v : evens | std::views::transform(add_one)) {
        sum += v;
    }
    EXPECT_EQ(sum, 16); // 1 + 3 + 5 + 7
    EXPECT_EQ(std::ranges::distance(evens), 4); // evens itself is untouched
}

TEST(StdInterop, StdAlgorithmOnRvalueZrBorrowedView) {
    std::vector<int> v{1, 2, 3, 4};
    auto it = std::ranges::find(zr::views::all(v), 3); // an rvalue zr::ranges::ref_view
    static_assert(std::same_as<decltype(it), std::vector<int>::iterator>);
    EXPECT_EQ(it, v.begin() + 2);
}

// iota_view's iterator is a std random-access iterator ([range.iota.iterator]), so the
// view is a std view and feeds std adaptors and algorithms directly.
static_assert(std::ranges::random_access_range<zr::ranges::iota_view<int, int>>);
static_assert(std::ranges::sized_range<zr::ranges::iota_view<int, int>>);
static_assert(std::ranges::common_range<zr::ranges::iota_view<int, int>>);
static_assert(std::ranges::view<zr::ranges::iota_view<int, int>>);
static_assert(std::ranges::random_access_range<zr::ranges::iota_view<std::size_t, std::size_t>>);

TEST(StdInterop, IotaIntoStdAlgorithmsAndAdaptors) {
    auto it = std::ranges::find(zr::views::iota(0, 10), 7); // rvalue, borrowed: an iterator
    EXPECT_EQ(*it, 7);

    int sum = 0;
    for (int v : zr::views::iota(1, 5) | std::views::reverse | std::views::transform(add_one)) {
        sum += v;
    }
    EXPECT_EQ(sum, 14); // 5 + 4 + 3 + 2
}

TEST(StdInterop, UnrealBackedConcatUnaffected) {
    std::vector<int> a{1, 2, 3};
    TSet<int> c{4};
    int s = 0;
    for (int v : zr::views::concat(a, c) | zr::views::transform(add_one)) {
        s += v; // (1+1)+(2+1)+(3+1)+(4+1) = 2+3+4+5 = 14
    }
    EXPECT_EQ(s, 14);
}

// [range.transform.sentinel]: semiregular sentinel, so a non-common transform is a std range.
TEST(StdInterop, TransformOverUnboundedIntoStdTake) {
    int sum = 0;
    for (int v : zr::views::transform(zr::views::iota(0), add_one) | std::views::take(3)) {
        sum += v;
    }
    EXPECT_EQ(sum, 6);
}

// Full iterator categories over std bases.
TEST(StdInterop, StdAlgorithmsThroughZrViews) {
    std::vector<int> v{3, 1, 2};
    std::ranges::sort(zr::views::transform(v, [](int& x) -> int& { return x; }));
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3}));
    std::ranges::sort(zr::views::reverse(v));
    EXPECT_EQ(v, (std::vector<int>{3, 2, 1}));
    std::vector<int> w{5, 4};
    std::ranges::sort(zr::views::concat(v, w));
    EXPECT_EQ(v, (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(w, (std::vector<int>{4, 5}));

    std::vector<int> out;
    for (int x : zr::views::filter(v, is_even) | std::views::reverse) {
        out.push_back(x);
    }
    EXPECT_EQ(out, (std::vector<int>{2}));
}
