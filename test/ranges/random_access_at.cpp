// Configuration-independent random access through ranges::element_at for ranges whose iterator
// is not subscriptable (UE TDeque) or non-contiguous (std::deque), and through a
// pass-through view (ref_view). Both build configurations.
#include "CoreMinimal.h"
#include "Containers/Deque.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/all.h"
#include "zr/unreal/ue_range_traits.h"

#include <deque>

#include <gtest/gtest.h>

using namespace zr::ranges;

static_assert(random_access_range<TDeque<int>> && !contiguous_range<TDeque<int>>);
static_assert(random_access_range<ref_view<TDeque<int>>> && !contiguous_range<ref_view<TDeque<int>>>);

TEST(RandomAccessAt, UnrealDequeIndexedNotViaIterator) {
    // TDeque's ranged-for iterator is forward-only; element_at must use operator[].
    TDeque<int> d{10, 20, 30};
    EXPECT_EQ(zr::ranges::element_at(d, 0), 10);
    EXPECT_EQ(zr::ranges::element_at(d, 2), 30);
}

TEST(RandomAccessAt, StdDequeViaRandomAccessIterator) {
    std::deque<int> sd{1, 2, 3};
    EXPECT_EQ(zr::ranges::element_at(sd, 1), 2);
}

TEST(RandomAccessAt, ContiguousViaData) {
    TArray<int> a{5, 6, 7};
    EXPECT_EQ(zr::ranges::element_at(a, 1), 6);
}

TEST(RandomAccessAt, ThroughRefViewForwardsAt) {
    TDeque<int> d{10, 20, 30};
    auto v = views::all(d);
    EXPECT_EQ(v[1], 20);
}
