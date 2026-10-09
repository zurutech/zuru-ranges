// ranges::to: materialize into std and Unreal containers, including pipelines and a
// nested (range-of-ranges) conversion. Both build configurations.
#include "CoreMinimal.h"
#include "Containers/ChunkedArray.h"
#include "Containers/Deque.h"
#include "Containers/SortedMap.h"

#include "zr/ranges/to.h"
#include "zr/ranges/views/filter_view.h"
#include "zr/ranges/views/transform_view.h"
#include "zr/ranges/views/zip_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
constexpr int doubler(int x) { return x * 2; }
constexpr bool is_even(int x) { return x % 2 == 0; }
} // namespace

TEST(RangesTo, StdToUnrealElementByElement) {
    std::vector<int> v{1, 2, 3};
    TArray<int> a = zr::ranges::to<TArray<int>>(v);
    EXPECT_EQ(a.Num(), 3);
    EXPECT_EQ(a[0] + a[1] + a[2], 6);
}

TEST(RangesTo, UnrealToStd) {
    TArray<int> ua{4, 5};
    std::vector<int> sv = zr::ranges::to<std::vector<int>>(ua);
    EXPECT_EQ(sv.size(), 2u);
    EXPECT_EQ(sv[0] + sv[1], 9);
}

TEST(RangesTo, UnrealToSet) {
    TArray<int> ua{4, 5};
    TSet<int> set = zr::ranges::to<TSet<int>>(ua);
    EXPECT_EQ(set.Num(), 2);
}

TEST(RangesTo, PipelineClosureOverUnreal) {
    TArray<int> src{1, 2, 3, 4};
    TArray<int> evens_doubled = src | views::filter(is_even) | views::transform(doubler) | zr::ranges::to<TArray<int>>();
    EXPECT_EQ(evens_doubled.Num(), 2);          // 2, 4
    EXPECT_EQ(evens_doubled[0], 4);             // doubled
    EXPECT_EQ(evens_doubled[1], 8);
}

TEST(RangesTo, NestedRangeOfRangesRecurses) {
    std::vector<std::vector<int>> nested{{1, 2}, {3}};
    TArray<TArray<int>> out = zr::ranges::to<TArray<TArray<int>>>(nested);
    EXPECT_EQ(out.Num(), 2);
    EXPECT_EQ(out[0].Num(), 2);
    EXPECT_EQ(out[1].Num(), 1);
    EXPECT_EQ(out[0][0] + out[0][1] + out[1][0], 6);
}

namespace {
// One insertion API each, so every container-append branch is taken; Reserve/reserve record
// the size hint. A branch wrongly evaluated false yields an empty container. User-declared
// constructors: as aggregates, C++20 `C(range)` could take the constructible_from branch.
struct FAddOnly {
    FAddOnly() = default;
    std::vector<int> Items;
    int32 Reserved = -1;
    void Add(int v) { Items.push_back(v); }
    void Reserve(int32 n) { Reserved = n; }
    int32 Num() const { return static_cast<int32>(Items.size()); }
};

struct FEmplaceOnly {
    FEmplaceOnly() = default;
    std::vector<int> Items;
    void Emplace(int v) { Items.push_back(v); }
};

// A sized std range, so reservable-container applies.
struct push_back_only {
    push_back_only() = default;
    std::vector<int> items;
    std::size_t reserved = 0;
    void push_back(int v) { items.push_back(v); }
    void reserve(std::size_t n) { reserved = n; }
    std::size_t capacity() const { return items.capacity(); }
    std::size_t max_size() const { return items.max_size(); }
    std::size_t size() const { return items.size(); }
    auto begin() const { return items.begin(); }
    auto end() const { return items.end(); }
};

struct emplace_back_only {
    emplace_back_only() = default;
    std::vector<int> items;
    void emplace_back(int v) { items.push_back(v); }
};
} // namespace

TEST(RangesTo, EveryAppendBranchPopulates) {
    TArray<int> src{1, 2, 3};
    FAddOnly add = zr::ranges::to<FAddOnly>(src);
    EXPECT_EQ(add.Items, (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(add.Reserved, 3);

    EXPECT_EQ(zr::ranges::to<FEmplaceOnly>(src).Items, (std::vector<int>{1, 2, 3}));

    push_back_only pb = src | views::filter(is_even) | zr::ranges::to<push_back_only>();
    EXPECT_EQ(pb.items, (std::vector<int>{2}));
    EXPECT_EQ(pb.reserved, 0u); // filter is unsized: no hint

    push_back_only sized = zr::ranges::to<push_back_only>(src);
    EXPECT_EQ(sized.reserved, 3u);

    EXPECT_EQ(zr::ranges::to<emplace_back_only>(src).items, (std::vector<int>{1, 2, 3}));

    std::set<int> inserted = src | views::transform(doubler) | zr::ranges::to<std::set<int>>();
    EXPECT_EQ(inserted, (std::set<int>{2, 4, 6}));
}

TEST(RangesTo, UnrealContainersByAdd) {
    std::vector<int> src{3, 1, 2, 3};
    TArray<int> array = zr::ranges::to<TArray<int>>(src);
    ASSERT_EQ(array.Num(), 4);
    EXPECT_EQ(array[3], 3);
    TSet<int> set = zr::ranges::to<TSet<int>>(src);
    EXPECT_EQ(set.Num(), 3);
    TMap<int, int> map{{1, 10}, {2, 20}};
    TMap<int, int> copy = map | views::filter([](const TPair<int, int>&) { return true; }) | zr::ranges::to<TMap<int, int>>();
    EXPECT_EQ(copy.Num(), 2);
}

TEST(RangesTo, DequeByPushLast) {
    TDeque<int> deque = std::vector<int>{1, 2, 3} | views::filter(is_even) | zr::ranges::to<TDeque<int>>();
    ASSERT_EQ(deque.Num(), 1);
    EXPECT_EQ(deque[0], 2);
    TDeque<int> all = zr::ranges::to<TDeque<int>>(TArray<int>{4, 5});
    ASSERT_EQ(all.Num(), 2);
    EXPECT_EQ(all[1], 5);
}

TEST(RangesTo, MapsFromPairLikeElements) {
    TMap<int, int> source{{3, 30}, {1, 10}, {2, 20}};
    TSortedMap<int, int> sorted = zr::ranges::to<TSortedMap<int, int>>(source);
    std::vector<int> keys = sorted | views::transform([](const TPair<int, int>& p) { return p.Key; }) | zr::ranges::to<std::vector<int>>();
    EXPECT_EQ(keys, (std::vector<int>{1, 2, 3}));

    std::vector<std::pair<int, int>> pairs{{1, 10}, {2, 20}, {1, 11}};
    TMap<int, int> map = zr::ranges::to<TMap<int, int>>(pairs);
    ASSERT_EQ(map.Num(), 2);
    TMultiMap<int, int> multi = zr::ranges::to<TMultiMap<int, int>>(pairs);
    EXPECT_EQ(multi.Num(), 3);
    TMap<int, int> from_std = zr::ranges::to<TMap<int, int>>(std::map<int, int>{{7, 70}});
    EXPECT_EQ(from_std.Num(), 1);

    TArray<int> ks{1, 2};
    std::vector<int> vs{100, 200};
    TMap<int, int> zipped = views::zip(ks, vs) | zr::ranges::to<TMap<int, int>>();
    ASSERT_EQ(zipped.Num(), 2);
    int sum = 0;
    for (const auto& [k, v] : zipped) {
        sum += k * v;
    }
    EXPECT_EQ(sum, (1 * 100) + (2 * 200));

    // Pair-like elements into a sequence stay whole: TArray has no Add(Key, Value).
    TArray<TPair<int, int>> flat = zr::ranges::to<TArray<TPair<int, int>>>(source);
    EXPECT_EQ(flat.Num(), 3);
}

TEST(RangesTo, StringsByAppendChar) {
    FString lower(TEXT("abc"));
    FString upper = lower | views::transform([](TCHAR c) { return static_cast<TCHAR>(c - TEXT('a') + TEXT('A')); }) | zr::ranges::to<FString>();
    ASSERT_EQ(upper.Len(), 3);
    EXPECT_EQ(upper.GetCharArray()[0], TEXT('A'));
    EXPECT_EQ(upper.GetCharArray()[2], TEXT('C'));
    EXPECT_EQ(zr::ranges::to<FString>(FString()).Len(), 0);
}

// Add(Key, Value) forwards the members of a prvalue pair.
TEST(RangesTo, PrvaluePairsMoveIntoUnrealMap) {
    std::vector<int> keys{1, 2};
    auto pairs = views::transform(keys, [](int k) { return std::pair<int, std::unique_ptr<int>>(k, std::make_unique<int>(k * 10)); });
    auto map = zr::ranges::to<TMap<int, std::unique_ptr<int>>>(pairs);
    EXPECT_EQ(map.Num(), 2);
    int sum = 0;
    for (const auto& [key, value] : map) {
        sum += key + *value;
    }
    EXPECT_EQ(sum, 33);
}

TEST(RangesTo, ChunkedArrayAppendsThroughAddElement) {
    // TChunkedArray::Add(int32 Count) appends Count default elements: AddElement comes first.
    std::vector<int> v{3, 4};
    TChunkedArray<int> c = zr::ranges::to<TChunkedArray<int>>(v);
    EXPECT_EQ(c.Num(), 2);
    EXPECT_EQ(c[0], 3);
    EXPECT_EQ(c[1], 4);
}

TEST(RangesTo, ReserveSaturatesToTheContainersSizeType) {
    EXPECT_EQ(static_cast<int32>(detail::saturating_size{std::uintmax_t{1} << 40}), std::numeric_limits<int32>::max());
    EXPECT_EQ(static_cast<uint32>(detail::saturating_size{7}), 7u);
    EXPECT_EQ(static_cast<int32>(detail::saturating_size{7}), 7);
    EXPECT_EQ(static_cast<std::size_t>(detail::saturating_size{std::uintmax_t{1} << 40}), std::size_t{1} << 40);
}
