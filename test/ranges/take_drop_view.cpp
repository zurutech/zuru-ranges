// take_view, drop_view and their adaptor objects over std and Unreal bases, both build configurations.
#include "CoreMinimal.h"
#include "Containers/List.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/drop_view.h"
#include "zr/ranges/views/empty_view.h"
#include "zr/ranges/views/filter_view.h"
#include "zr/ranges/views/iota_view.h"
#include "zr/ranges/views/reverse_view.h"
#include "zr/ranges/views/take_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <compare>
#include <cstddef>
#include <forward_list>
#include <iterator>
#include <list>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {

template <class R> std::vector<int> to_vector(R&& r) {
    std::vector<int> out;
    for (auto&& x : r) {
        out.push_back(x);
    }
    return out;
}

TArray<int> make_array() {
    TArray<int> a;
    for (int x : {1, 2, 3, 4, 5}) {
        a.Add(x);
    }
    return a;
}

constexpr bool is_odd(int x) { return x % 2 != 0; }

using vec_take = decltype(std::declval<std::vector<int>&>() | views::take(1));
using vec_drop = decltype(std::declval<std::vector<int>&>() | views::drop(1));
using arr_take = decltype(std::declval<TArray<int>&>() | views::take(1));
using arr_drop = decltype(std::declval<TArray<int>&>() | views::drop(1));
using map_take = decltype(std::declval<TMap<int, int>&>() | views::take(1));
using list_drop = decltype(std::declval<TDoubleLinkedList<int>&>() | views::drop(1));

// [range.take.view], [range.drop.view]: the base's category, size and constness; TArray the same
// in both configurations (the trait forwards element_at and data).
static_assert(contiguous_range<vec_take> && sized_range<vec_take> && common_range<vec_take> && view<vec_take>);
static_assert(contiguous_range<vec_drop> && sized_range<vec_drop> && common_range<vec_drop> && view<vec_drop>);
static_assert(contiguous_range<arr_take> && sized_range<arr_take> && common_range<arr_take> && range<const arr_take>);
static_assert(contiguous_range<arr_drop> && sized_range<arr_drop> && range<const arr_drop>);
static_assert(forward_range<map_take> && !bidirectional_range<map_take> && sized_range<map_take>);
static_assert(bidirectional_range<list_drop> && !random_access_range<list_drop> && sized_range<list_drop>);

// [ranges.syn] enable_borrowed_range.
static_assert(borrowed_range<vec_take> && borrowed_range<arr_drop>);
static_assert(!borrowed_range<take_view<owning_view<std::vector<int>>>>);

// [range.take.view]: neither sized nor sized-sentinel: counted_iterator and sentinel.
using filter_take = decltype(std::declval<std::vector<int>&>() | views::filter(is_odd) | views::take(1));
static_assert(forward_range<filter_take> && !common_range<filter_take> && !sized_range<filter_take>);

// Adaptor objects are expression-equivalent, noexcept included.
static_assert(noexcept(views::take(views::empty<int>, 1)) && noexcept(views::drop(views::empty<int>, 1)));
static_assert(noexcept(std::declval<std::vector<int>&>() | views::take(1)) ==
              noexcept(take_view(std::declval<std::vector<int>&>(), 1)));

// [counted.iterator] over an Unreal iterator (TMap's in both configurations).
using map_counted = iterator_t<map_take>;
static_assert(std::is_same_v<map_counted, detail::relaxed_counted_iterator<iterator_t<TMap<int, int>>>>);
static_assert(std::is_same_v<map_counted::iterator_type, iterator_t<TMap<int, int>>>);
static_assert(std::is_same_v<decltype(std::declval<map_counted>() <=> std::declval<map_counted>()), std::strong_ordering>);

// A difference type whose conversion counts its evaluations.
struct counting_count {
    static inline int conversions = 0;
    operator std::ptrdiff_t() const {
        ++conversions;
        return 1;
    }
};

} // namespace

TEST(TakeView, StdBases) {
    std::vector<int> v{1, 2, 3, 4, 5};
    EXPECT_EQ(to_vector(v | views::take(3)), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(to_vector(v | views::take(9)), v);
    EXPECT_EQ(to_vector(v | views::take(0)), std::vector<int>{});
    EXPECT_EQ((v | views::take(3)).size(), 3U);
    EXPECT_EQ((v | views::take(9)).size(), 5U);

    std::list<int> l{1, 2, 3, 4};
    EXPECT_EQ(to_vector(l | views::take(2)), (std::vector<int>{1, 2}));
    std::forward_list<int> fl{1, 2, 3};
    EXPECT_EQ(to_vector(fl | views::take(2)), (std::vector<int>{1, 2}));
    EXPECT_EQ(to_vector(views::iota(0) | views::take(3)), (std::vector<int>{0, 1, 2}));
}

TEST(TakeView, SentinelOverUnsizedBase) {
    std::vector<int> v{1, 2, 3, 4, 5};
    auto t = v | views::filter(is_odd) | views::take(2);
    EXPECT_EQ(to_vector(t), (std::vector<int>{1, 3}));
    auto all = v | views::filter(is_odd) | views::take(9);
    EXPECT_EQ(to_vector(all), (std::vector<int>{1, 3, 5}));
}

TEST(TakeView, ReserveHint) {
    std::vector<int> v{1, 2, 3};
    EXPECT_EQ((v | views::take(2)).reserve_hint(), 2U);
    EXPECT_EQ((views::iota(0) | views::take(4)).reserve_hint(), 4U);
}

TEST(TakeView, UnrealBases) {
    TArray<int> a = make_array();
    auto t = a | views::take(3);
    EXPECT_EQ(to_vector(t), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(t[2], 3);
    EXPECT_EQ(element_at(t, 1), 2);
    EXPECT_EQ(data(t), a.GetData());
    const auto& ct = t;
    EXPECT_EQ(to_vector(ct), (std::vector<int>{1, 2, 3}));

    TMap<int, int> m;
    m.Add(1, 10);
    m.Add(2, 20);
    m.Add(3, 30);
    EXPECT_EQ(distance(m | views::take(2)), 2);

    TDoubleLinkedList<int> dl;
    for (int x : {1, 2, 3}) {
        dl.AddTail(x);
    }
    EXPECT_EQ(to_vector(dl | views::take(2)), (std::vector<int>{1, 2}));
}

TEST(TakeView, AdaptorSlicesKnownViews) {
    std::vector<int> v{1, 2, 3, 4};
    std::span<int> sp(v);
    static_assert(std::is_same_v<decltype(sp | views::take(2)), std::span<int>>);
    EXPECT_EQ((sp | views::take(2)).size(), 2U);
    std::span<int, 4> fixed(v.data(), 4);
    static_assert(std::is_same_v<decltype(fixed | views::take(2)), std::span<int>>);

    std::string_view sv = "hello";
    static_assert(std::is_same_v<decltype(sv | views::take(2)), std::string_view>);
    EXPECT_EQ(sv | views::take(2), "he");

    subrange sr(v.begin(), v.end());
    static_assert(std::is_same_v<decltype(sr | views::take(2)), subrange<std::vector<int>::iterator>>);
    EXPECT_EQ(to_vector(sr | views::take(9)), v);

    static_assert(std::is_same_v<decltype(views::iota(0, 10) | views::take(3)), iota_view<int, int>>);
    EXPECT_EQ(to_vector(views::iota(0, 10) | views::take(3)), (std::vector<int>{0, 1, 2}));

    static_assert(std::is_same_v<decltype(views::empty<int> | views::take(2)), empty_view<int>>);
    static_assert(std::is_same_v<decltype(views::iota(0) | views::take(2)), take_view<iota_view<int, std::unreachable_sentinel_t>>>);
}

TEST(TakeView, CountedIteratorOverUnreal) {
    TMap<int, int> m;
    m.Add(1, 10);
    m.Add(2, 20);
    m.Add(3, 30);
    auto t = m | views::take(2);
    auto first = t.begin();
    EXPECT_EQ(first.count(), 2);
    EXPECT_EQ(distance(first, std::default_sentinel), 2);
    auto second = first;
    ++second;
    EXPECT_TRUE(first < second);
    EXPECT_EQ(second - first, 1);
    EXPECT_EQ((*second).Key, 2);
}

TEST(TakeView, EmptyViewDoesNotConvertCount) {
    counting_count::conversions = 0;
    (void)views::take(views::empty<int>, counting_count{});
    (void)views::drop(views::empty<int>, counting_count{});
    EXPECT_EQ(counting_count::conversions, 0);
}

TEST(TakeView, DropLast) {
    std::vector<int> v{1, 2, 3, 4, 5};
    EXPECT_EQ(to_vector(v | views::take(ssize(v) - 2)), (std::vector<int>{1, 2, 3}));
    TArray<int> a = make_array();
    EXPECT_EQ(to_vector(a | views::take(ssize(a) - 2)), (std::vector<int>{1, 2, 3}));
}

TEST(DropView, StdBases) {
    std::vector<int> v{1, 2, 3, 4, 5};
    auto d = v | views::drop(3);
    EXPECT_EQ(to_vector(d), (std::vector<int>{4, 5}));
    EXPECT_EQ(d.size(), 2U);
    EXPECT_EQ(d.data(), v.data() + 3);
    EXPECT_TRUE((v | views::drop(9)).empty());
    EXPECT_EQ((v | views::drop(9)).size(), 0U);

    std::list<int> l{1, 2, 3, 4};
    EXPECT_EQ(to_vector(l | views::drop(2)), (std::vector<int>{3, 4}));
    EXPECT_EQ(to_vector(l | views::drop(2) | views::reverse), (std::vector<int>{4, 3}));
    std::forward_list<int> fl{1, 2, 3};
    EXPECT_EQ(to_vector(fl | views::drop(1)), (std::vector<int>{2, 3}));
}

TEST(DropView, CachesBeginOverForward) {
    std::vector<int> v{1, 2, 3, 4, 5};
    int calls = 0;
    auto counting = [&calls](int x) {
        ++calls;
        return x != 0;
    };
    auto d = v | views::filter(counting) | views::drop(2);
    EXPECT_EQ(*d.begin(), 3);
    const int after_first = calls;
    EXPECT_EQ(*d.begin(), 3);
    EXPECT_EQ(calls, after_first);
}

TEST(DropView, ReserveHint) {
    std::vector<int> v{1, 2, 3};
    EXPECT_EQ((v | views::drop(1)).reserve_hint(), 2U);
    EXPECT_EQ((v | views::drop(5)).reserve_hint(), 0U);
}

TEST(DropView, UnrealBases) {
    TArray<int> a = make_array();
    auto d = a | views::drop(3);
    EXPECT_EQ(to_vector(d), (std::vector<int>{4, 5}));
    EXPECT_EQ(d[1], 5);
    EXPECT_EQ(element_at(d, 0), 4);
    EXPECT_EQ(data(d), a.GetData() + 3);
    EXPECT_EQ(size(a | views::drop(9)), 0U);
    const auto& cd = d;
    EXPECT_EQ(to_vector(cd), (std::vector<int>{4, 5}));

    TMap<int, int> m;
    m.Add(1, 10);
    m.Add(2, 20);
    EXPECT_EQ(distance(m | views::drop(1)), 1);

    TDoubleLinkedList<int> dl;
    for (int x : {1, 2, 3}) {
        dl.AddTail(x);
    }
    EXPECT_EQ(to_vector(dl | views::drop(1)), (std::vector<int>{2, 3}));
    EXPECT_EQ(to_vector(dl | views::drop(1) | views::reverse), (std::vector<int>{3, 2}));
}

TEST(DropView, AdaptorSlicesKnownViews) {
    std::vector<int> v{1, 2, 3, 4};
    std::span<int> sp(v);
    static_assert(std::is_same_v<decltype(sp | views::drop(2)), std::span<int>>);
    EXPECT_EQ((sp | views::drop(3)).front(), 4);

    std::string_view sv = "hello";
    static_assert(std::is_same_v<decltype(sv | views::drop(2)), std::string_view>);
    EXPECT_EQ(sv | views::drop(2), "llo");

    static_assert(std::is_same_v<decltype(views::iota(0, 10) | views::drop(7)), iota_view<int, int>>);
    EXPECT_EQ(to_vector(views::iota(0, 10) | views::drop(7)), (std::vector<int>{7, 8, 9}));

    subrange sr(v.begin(), v.end());
    static_assert(std::is_same_v<decltype(sr | views::drop(1)), subrange<std::vector<int>::iterator>>);
    EXPECT_EQ(to_vector(sr | views::drop(3)), (std::vector<int>{4}));

    // [range.drop.overview]: a subrange storing its size keeps it.
    std::list<int> l{1, 2, 3};
    subrange<std::list<int>::iterator, std::list<int>::iterator, subrange_kind::sized> sized(l.begin(), l.end(), 3);
    static_assert(std::is_same_v<decltype(sized | views::drop(1)), drop_view<decltype(sized)>>);
    EXPECT_EQ((sized | views::drop(1)).size(), 2U);

    static_assert(std::is_same_v<decltype(views::empty<int> | views::drop(2)), empty_view<int>>);
}

TEST(TakeDropView, Compose) {
    TArray<int> a = make_array();
    EXPECT_EQ(to_vector(a | views::drop(1) | views::take(3)), (std::vector<int>{2, 3, 4}));
    EXPECT_EQ(to_vector(a | views::drop(1) | views::take(2) | views::reverse), (std::vector<int>{3, 2}));
    EXPECT_EQ(to_vector(views::iota(0) | views::drop(2) | views::take(2)), (std::vector<int>{2, 3}));
}
