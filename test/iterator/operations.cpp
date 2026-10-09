// ranges::distance over std and Unreal iterators and ranges, both build configurations.
#include "CoreMinimal.h"
#include "Containers/ChunkedArray.h"
#include "Containers/List.h"

#include "zr/iterator/operations.h"
#include "zr/ranges/views/filter_view.h"
#include "zr/ranges/views/iota_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <forward_list>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
constexpr bool is_odd(int x) { return x % 2 != 0; }

static_assert(distance(views::iota(0, 4)) == 4);
static_assert(std::is_same_v<decltype(distance(std::declval<TArray<int>&>())), std::ptrdiff_t>);
} // namespace

TEST(Distance, StdIterators) {
    std::vector<int> v{1, 2, 3};
    EXPECT_EQ(distance(v.begin(), v.end()), 3);
    EXPECT_EQ(distance(v.end(), v.begin()), -3);
    std::forward_list<int> fl{1, 2, 3, 4};
    EXPECT_EQ(distance(fl.begin(), fl.end()), 4);
    int arr[5]{};
    EXPECT_EQ(distance(arr, arr + 5), 5);
}

TEST(Distance, Ranges) {
    std::vector<int> v{1, 2, 3, 4, 5};
    EXPECT_EQ(distance(v), 5);
    EXPECT_EQ(distance(v | views::filter(is_odd)), 3);
    int arr[4]{};
    EXPECT_EQ(distance(arr), 4);
}

TEST(Distance, UnrealContainers) {
    TArray<int> a;
    for (int x : {1, 2, 3}) {
        a.Add(x);
    }
    EXPECT_EQ(distance(a), 3);
    EXPECT_EQ(distance(a.begin(), a.end()), 3);
    EXPECT_EQ(distance(a | views::filter(is_odd)), 2);

    TMap<int, int> m;
    m.Add(1, 1);
    m.Add(2, 2);
    EXPECT_EQ(distance(m), 2);
    EXPECT_EQ(distance(m.begin(), m.end()), 2);

    TChunkedArray<int> chunked;
    chunked.AddElement(1);
    chunked.AddElement(2);
    EXPECT_EQ(distance(chunked.begin(), chunked.end()), 2);

    TDoubleLinkedList<int> dl;
    dl.AddTail(1);
    EXPECT_EQ(distance(dl), 1);
}
