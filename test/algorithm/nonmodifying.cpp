// ranges::all_of / any_of / none_of / find / find_if / count_if over std and Unreal
// containers, both build configurations.
#include "CoreMinimal.h"

#include "zr/algorithm.h"
#include "zr/ranges/views/all.h"
#include "zr/ranges/views/concat_view.h"
#include "zr/ranges/views/filter_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <cstddef>
#include <functional>
#include <ranges>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
struct FDevice {
    int Id = 0;
    bool bActive = false;
};

constexpr bool is_even(int x) { return x % 2 == 0; }

template <class R> std::vector<int> collect(R&& r) {
    std::vector<int> out;
    for (auto&& x : r) {
        out.push_back(static_cast<int>(x));
    }
    return out;
}

} // namespace

// [alg.any.of]
TEST(Algorithm, AnyOf) {
    TArray<int> array{1, 3, 5};
    TSet<int> set{7, 8};
    EXPECT_FALSE(any_of(array, is_even));
    EXPECT_TRUE(any_of(set, is_even));
    EXPECT_TRUE(any_of(views::concat(array, set), is_even)); // several ranges: one concat
    EXPECT_TRUE(any_of(array.begin(), array.end(), [](int x) { return x == 3; }));
    EXPECT_FALSE(any_of(TArray<int>{}, is_even));
    TArray<FDevice> devices{{1, false}, {2, true}};
    EXPECT_TRUE(any_of(devices, std::identity{}, &FDevice::bActive)); // projection is the predicate input
    EXPECT_TRUE(any_of(devices, is_even, &FDevice::Id));
    TMap<int, int> map{{1, 10}, {2, 21}};
    EXPECT_TRUE(any_of(map, [](int v) { return v % 2 != 0; }, [](const TPair<int, int>& p) { return p.Value; }));
}

// [alg.all.of], [alg.none.of]
TEST(Algorithm, AllOfNoneOf) {
    TArray<int> evens{2, 4};
    TSet<int> odds{1, 3};
    EXPECT_TRUE(all_of(evens, is_even));
    EXPECT_FALSE(all_of(views::concat(evens, odds), is_even));
    EXPECT_TRUE(all_of(TArray<int>{}, is_even)); // vacuous
    EXPECT_TRUE(none_of(odds, is_even));
    EXPECT_FALSE(none_of(evens.begin(), evens.end(), is_even));
    TArray<FDevice> devices{{1, true}, {2, true}};
    EXPECT_TRUE(all_of(devices, std::identity{}, &FDevice::bActive));
    EXPECT_TRUE(none_of(devices, [](int id) { return id > 2; }, &FDevice::Id));
}

// [alg.find]
TEST(Algorithm, Find) {
    TArray<int> array{1, 4, 6};
    EXPECT_EQ(*find(array, 4), 4);
    EXPECT_FALSE(find(array, 5) != array.end());
    TSet<FString> names{TEXT("a"), TEXT("b")};
    EXPECT_TRUE(find(names, FString(TEXT("b"))) != names.end());
    TArray<FDevice> devices{{1, false}, {2, true}};
    EXPECT_TRUE((*find(devices, 2, &FDevice::Id)).bActive);
    TArray<TPair<int, int>> pairs{{1, 10}, {2, 20}};
    EXPECT_EQ((*find(pairs, {2, 20}, [](const TPair<int, int>& p) { return std::pair(p.Key, p.Value); })).Key, 2); // T defaults to the projected value
    static_assert(std::is_same_v<decltype(find(TArray<int>{}, 1)), std::ranges::dangling>);
}

TEST(Algorithm, FindIf) {
    TArray<int> array{1, 4, 6};
    auto it = find_if(array, is_even);
    ASSERT_TRUE(it != array.end());
    EXPECT_EQ(*it, 4);
    *it = 5;
    EXPECT_EQ(array[1], 5);
    EXPECT_FALSE(find_if(array, [](int x) { return x > 9; }) != array.end());

    TSet<int> set{1, 3, 8};
    EXPECT_EQ(*find_if(set, is_even), 8);
    TArray<FDevice> devices{{1, false}, {2, true}};
    EXPECT_EQ((*find_if(devices, std::identity{}, &FDevice::bActive)).Id, 2);

    auto even = array | views::filter(is_even);
    EXPECT_EQ(*find_if(even, [](int x) { return x == 6; }), 6);
}

TEST(Algorithm, FindIfDanglingOnTemporaries) {
    // A temporary owning range yields std::ranges::dangling, not an iterator into it.
    static_assert(std::is_same_v<decltype(find_if(TArray<int>{}, is_even)), std::ranges::dangling>);
    static_assert(std::is_same_v<decltype(find_if(std::vector<int>{}, is_even)), std::ranges::dangling>);
    TArray<int> array{1, 2};
    static_assert(std::is_same_v<decltype(find_if(views::all(array), is_even)), iterator_t<ref_view<TArray<int>>>>);
    EXPECT_EQ(*find_if(views::all(array), is_even), 2);
}

// [alg.count]
TEST(Algorithm, CountIf) {
    TArray<int> array{1, 2, 3, 4};
    TSet<int> set{6};
    EXPECT_EQ(count_if(array, is_even), 2);
    EXPECT_EQ(count_if(views::concat(array, set), is_even), 3);
    EXPECT_EQ(count_if(array.begin(), array.end(), [](int x) { return x > 2; }), 2);
    static_assert(std::is_same_v<decltype(count_if(array, is_even)), std::ptrdiff_t>);
    TArray<FDevice> devices{{1, true}, {2, false}, {3, true}};
    EXPECT_EQ(count_if(devices, std::identity{}, &FDevice::bActive), 2);
}
