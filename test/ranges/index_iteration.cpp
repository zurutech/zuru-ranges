// take_view, zip_view and reverse_view over a base that range_traits classify random access and
// sized while its iterator only increments (TArray in debug, TDeque): iterated by
// detail::index_iterator, so random access, common and O(1) at both ends in every build
// configuration.
#include "CoreMinimal.h"
#include "Containers/Deque.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/concat_view.h"
#include "zr/ranges/views/filter_view.h"
#include "zr/ranges/views/reverse_view.h"
#include "zr/ranges/views/take_view.h"
#include "zr/ranges/views/zip_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {

// A UE-like container counting the increments of its forward-only, `!=`-only iterator.
struct FCounted {
    using ElementType = int;

    struct FIterator {
        const int* ptr;

        FIterator& operator++() {
            ++increments;
            ++ptr;
            return *this;
        }
        int operator*() const { return *ptr; }
        bool operator!=(const FIterator& other) const { return ptr != other.ptr; }
    };

    static inline long increments = 0;
    std::vector<int> values;

    int Num() const { return static_cast<int>(values.size()); }
    int operator[](int i) const { return values[static_cast<std::size_t>(i)]; }
    FIterator begin() const { return {values.data()}; }
    FIterator end() const { return {values.data() + values.size()}; }
};

FCounted counted(int n) {
    FCounted c;
    for (int i = 0; i < n; ++i) {
        c.values.push_back(i);
    }
    return c;
}

template <class R> long count(R&& r) {
    long n = 0;
    for (auto&& x : r) {
        (void)x;
        ++n;
    }
    return n;
}

template <class R> std::vector<int> collect(R&& r) {
    std::vector<int> out;
    for (auto&& x : r) {
        out.push_back(x);
    }
    return out;
}

} // namespace

namespace zr::ranges {
template <> struct range_traits_impl<FCounted> {
    static constexpr range_category category = range_category::random_access;
    static int size(const FCounted& c) { return c.Num(); }
    static int element_at(const FCounted& c, std::ptrdiff_t i) { return c[static_cast<int>(i)]; }
};
} // namespace zr::ranges

namespace {

using arr_take = decltype(std::declval<TArray<int>&>() | views::take(1));
using deque_take = decltype(std::declval<TDeque<int>&>() | views::take(1));
using zip_arrays = decltype(views::zip(std::declval<TArray<int>&>(), std::declval<TArray<int>&>()));
using deque_rev = decltype(std::declval<TDeque<int>&>() | views::reverse);

// index_iterable: random access and sized by trait, without iterator arithmetic.
static_assert(detail::index_iterable<TDeque<int>> && detail::index_iterable<const TDeque<int>> && detail::index_iterable<FCounted>);
static_assert(detail::index_iterable<ref_view<TArray<int>>> == !std::is_pointer_v<iterator_t<TArray<int>>>);
static_assert(!detail::index_iterable<std::vector<int>> && !detail::index_iterable<TSet<int>>);

// The views' iterators are std random-access iterators in both configurations (T*, or the index
// iterator), and the views random access, sized and common; take over TArray stays contiguous.
static_assert(std::random_access_iterator<iterator_t<arr_take>> && std::random_access_iterator<iterator_t<deque_take>>);
static_assert(std::random_access_iterator<iterator_t<zip_arrays>> && std::random_access_iterator<iterator_t<deque_rev>>);
static_assert(contiguous_range<arr_take> && sized_range<arr_take> && common_range<arr_take>);
static_assert(contiguous_range<const arr_take> && common_range<const arr_take>);
static_assert(random_access_range<deque_take> && sized_range<deque_take> && common_range<deque_take> && common_range<const deque_take>);
static_assert(random_access_range<zip_arrays> && sized_range<zip_arrays> && common_range<zip_arrays> && common_range<const zip_arrays>);
static_assert(random_access_range<deque_rev> && common_range<deque_rev> && common_range<const deque_rev>);

// Borrowed exactly when the base is: the index iterator copies a borrowed base.
static_assert(borrowed_range<arr_take> && borrowed_range<deque_take> && borrowed_range<zip_arrays> && borrowed_range<deque_rev>);
static_assert(!borrowed_range<take_view<owning_view<TDeque<int>>>>);

} // namespace

TEST(IndexIteration, TakeNeverTouchesTheBaseIterator) {
    FCounted c = counted(1000);
    FCounted::increments = 0;
    EXPECT_EQ(count(c | views::take(500)), 500);
    EXPECT_EQ(count(c | views::take(500) | views::filter([](int) { return true; })), 500);
    EXPECT_EQ(count(views::concat(c | views::take(500), c | views::take(1))), 501);
    EXPECT_EQ(FCounted::increments, 0); // element_at only; end() is O(1), filter's per-step end() too

    auto t = c | views::take(500);
    EXPECT_EQ(t.end() - t.begin(), 500);
    EXPECT_EQ(t[499], 499);
    EXPECT_EQ(t.back(), 499);
    EXPECT_EQ((c | views::take(2000)).end() - t.begin(), 1000);
}

TEST(IndexIteration, TakeOverUnrealArraysAndDeques) {
    TArray<int> a{1, 2, 3, 4, 5};
    auto t = a | views::take(3);
    EXPECT_EQ(t.end() - t.begin(), 3);
    EXPECT_EQ(*(t.begin() + 2), 3);
    EXPECT_EQ(t.back(), 3);
    EXPECT_EQ(data(t), a.GetData());
    EXPECT_TRUE(t.begin() < t.end());
    auto it = t.end();
    --it;
    EXPECT_EQ(*it, 3);

    TDeque<int> d{10, 20, 30, 40};
    auto dt = d | views::take(9);
    EXPECT_EQ(dt.end() - dt.begin(), 4);
    EXPECT_EQ(dt[3], 40);
    EXPECT_EQ(dt.back(), 40);
    const auto& cdt = dt;
    EXPECT_EQ(cdt.back(), 40);
    EXPECT_EQ(collect(cdt), (std::vector<int>{10, 20, 30, 40}));
}

TEST(IndexIteration, ZipOverUnrealArraysIsCommonAndRandomAccess) {
    TArray<int> a{1, 2, 3};
    TArray<int> b{10, 20, 30, 40};
    auto z = views::zip(a, b);
    EXPECT_EQ(z.end() - z.begin(), 3);
    auto [x, y] = z.back();
    EXPECT_EQ(x + y, 33);
    auto [p, q] = z[1];
    EXPECT_EQ(p + q, 22);
    auto it = z.end();
    --it;
    EXPECT_EQ(std::get<0>(*it), 3);
    for (auto&& [i, j] : z) {
        i += j;
    }
    EXPECT_EQ(a[0] + a[1] + a[2], 66);
    const auto& cz = z;
    EXPECT_EQ(std::get<1>(cz.back()), 30);

    std::vector<int> firsts;
    for (auto&& [i, j] : z | views::reverse) {
        firsts.push_back(i);
    }
    EXPECT_EQ(firsts, (std::vector<int>{33, 22, 11}));
}

TEST(IndexIteration, ReverseByIndex) {
    TDeque<int> d{1, 2, 3};
    auto r = d | views::reverse;
    EXPECT_EQ(r.end() - r.begin(), 3);
    EXPECT_EQ(r[0], 3);
    EXPECT_EQ(r.back(), 1);
    EXPECT_EQ(collect(r), (std::vector<int>{3, 2, 1}));
    const auto& cr = r;
    EXPECT_EQ(cr.front(), 3);
    EXPECT_EQ(collect(d | views::reverse | views::take(2)), (std::vector<int>{3, 2}));
    EXPECT_EQ(collect(d | views::take(2) | views::reverse), (std::vector<int>{2, 1}));
}
