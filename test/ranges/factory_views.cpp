// empty_view, single_view, iota_view (factories), both build configurations.
#include "CoreMinimal.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/to.h"
#include "zr/ranges/views/empty_view.h"
#include "zr/ranges/views/iota_view.h"
#include "zr/ranges/views/single_view.h"
#include "zr/ranges/views/transform_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <climits>
#include <forward_list>
#include <list>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
constexpr int doubler(int x) { return x * 2; }
} // namespace

static_assert(contiguous_range<empty_view<int>> && sized_range<empty_view<int>>);
static_assert(contiguous_range<single_view<int>>);
static_assert(random_access_range<iota_view<int, int>> && sized_range<iota_view<int, int>> && !contiguous_range<iota_view<int, int>>);

// [range.iota.iterator]/1: the category follows W. A pointer is advanceable (random access);
// a list iterator only decrementable (bidirectional); a forward_list iterator incrementable.
static_assert(random_access_range<iota_view<int*, int*>>);
static_assert(bidirectional_range<iota_view<std::list<int>::iterator, std::list<int>::iterator>> &&
              !random_access_range<iota_view<std::list<int>::iterator, std::list<int>::iterator>>);
static_assert(forward_range<iota_view<std::forward_list<int>::iterator, std::forward_list<int>::iterator>> &&
              !bidirectional_range<iota_view<std::forward_list<int>::iterator, std::forward_list<int>::iterator>>);

// A Bound of another type gets a sentinel: not common, and sized through size() because both
// are integer-like (the sentinel itself has no operator-: int is not an iterator).
static_assert(sized_range<iota_view<int, long>> && !common_range<iota_view<int, long>>);
static_assert(!sized_range<iota_view<int>>);

// [range.iota.overview]: views::iota and views::indices are expression-equivalent, noexcept included.
static_assert(noexcept(views::iota(1)) == noexcept(iota_view<int>(1)));
static_assert(noexcept(views::iota(1, 3)) == noexcept(iota_view(1, 3)));
static_assert(noexcept(views::indices(3)) == noexcept(views::iota(0, 3)));

// [range.iota.view]/1 IOTA-DIFF-T: a signed type wider than W where one exists.
static_assert(sizeof(std::iter_difference_t<iterator_t<iota_view<int, int>>>) > sizeof(int));
static_assert(std::signed_integral<std::iter_difference_t<iterator_t<iota_view<unsigned short, unsigned short>>>>);

TEST(EmptyView, NoElements) {
    auto e = views::empty<int>;
    EXPECT_EQ(static_cast<int>(e.size()), 0);
    EXPECT_TRUE(e.empty());
    int iterations = 0;
    for (int v : e) {
        (void)v;
        ++iterations;
    }
    EXPECT_EQ(iterations, 0);
}

TEST(SingleView, OneElement) {
    auto one = views::single(42);
    EXPECT_EQ(static_cast<int>(one.size()), 1);
    EXPECT_EQ(one[0], 42);
    int sum = 0;
    for (int v : one) {
        sum += v;
    }
    EXPECT_EQ(sum, 42);
}

TEST(IotaView, BoundedIsSizedAndIndexable) {
    auto io = views::iota(0, 5); // 0,1,2,3,4
    EXPECT_EQ(static_cast<int>(io.size()), 5);
    EXPECT_EQ(io[4], 4);
    int sum = 0;
    for (int v : io) {
        sum += v;
    }
    EXPECT_EQ(sum, 10);
}

TEST(IotaView, ComposesWithTransformAndTo) {
    TArray<int> doubled = views::iota(1, 4) | views::transform(doubler) | zr::ranges::to<TArray<int>>();
    EXPECT_EQ(doubled.Num(), 3);
    EXPECT_EQ(doubled[0] + doubled[1] + doubled[2], 12); // (1+2+3)*2
}

TEST(IotaView, UnboundedNeverReachesEnd) {
    auto inf = views::iota(100);
    auto it = zr::ranges::begin(inf);
    EXPECT_EQ(*it, 100);
    ++it;
    EXPECT_EQ(*it, 101);
    EXPECT_TRUE(it != zr::ranges::end(inf)); // unreachable sentinel
}

TEST(IotaView, NegativeAndMixedSignSizes) {
    EXPECT_EQ(views::iota(-5, -2).size(), 3u);
    EXPECT_EQ(views::iota(-2, 3).size(), 5u);
    EXPECT_EQ(views::iota(2u, 9u).size(), 7u);
}

TEST(IotaView, IteratorArithmetic) {
    auto r = views::iota(10, 20);
    auto it = r.begin();
    it += 5;
    EXPECT_EQ(*it, 15);
    EXPECT_EQ(it[-2], 13);
    EXPECT_EQ(it - r.begin(), 5);
    EXPECT_EQ(r.end() - it, 5);
    it -= 3;
    EXPECT_EQ(*it, 12);
    EXPECT_TRUE(r.begin() < it);

    auto u = views::iota(2u, 9u); // unsigned W: differences are still signed
    EXPECT_EQ(u.begin() - u.end(), -7);
    auto ub = u.end();
    ub += -7;
    EXPECT_EQ(*ub, 2u);
}

TEST(IotaView, DistinctBoundUsesSentinel) {
    auto r = views::iota(3, 8L);
    int sum = 0;
    for (int v : r) {
        sum += v;
    }
    EXPECT_EQ(sum, 25); // 3+4+5+6+7
    EXPECT_EQ(r.size(), 5u);
}

TEST(IotaView, ConstructFromIteratorAndEnd) {
    auto r = views::iota(0, 10);
    iota_view<int, int> tail(r.begin() + 4, r.end());
    EXPECT_EQ(tail.size(), 6u);
    EXPECT_EQ(*tail.begin(), 4);
    EXPECT_FALSE(tail.empty());
    EXPECT_TRUE((iota_view<int, int>(5, 5).empty()));
}

TEST(IotaView, IndicesCountsFromZero) {
    auto ix = views::indices(4u);
    static_assert(std::same_as<decltype(ix), iota_view<unsigned, unsigned>>);
    unsigned sum = 0;
    for (unsigned v : ix) {
        sum += v;
    }
    EXPECT_EQ(sum, 6u); // 0+1+2+3
}

TEST(IotaView, FullWidthSignedRange) {
    auto r = views::iota(LLONG_MIN, LLONG_MAX);
    EXPECT_EQ(r.size(), ULLONG_MAX); // no signed negation of LLONG_MIN on the way
    using D = std::iter_difference_t<iterator_t<decltype(r)>>;
    if constexpr (sizeof(D) > sizeof(long long)) { // __int128 where the library accepts it
        EXPECT_TRUE(r.end() - r.begin() > D(0));
    }
}
