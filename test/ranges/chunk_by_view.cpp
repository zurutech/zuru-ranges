// chunk_by_view over std and Unreal bases, both build configurations.
#include "CoreMinimal.h"
#include "Containers/List.h"

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/chunk_by_view.h"
#include "zr/ranges/views/iota_view.h"
#include "zr/ranges/views/take_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <forward_list>
#include <functional>
#include <iterator>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {

using chunks_t = std::vector<std::vector<int>>;

template <class R> chunks_t to_chunks(R&& r) {
    chunks_t out;
    for (auto chunk : r) {
        out.emplace_back();
        for (int x : chunk) {
            out.back().push_back(x);
        }
    }
    return out;
}

using vec_chunks = decltype(std::declval<std::vector<int>&>() | views::chunk_by(std::ranges::less_equal{}));
using fwd_chunks = decltype(std::declval<std::forward_list<int>&>() | views::chunk_by(std::ranges::less_equal{}));
using arr_chunks = decltype(std::declval<TArray<int>&>() | views::chunk_by(std::ranges::less_equal{}));
using list_chunks = decltype(std::declval<TDoubleLinkedList<int>&>() | views::chunk_by(std::ranges::less_equal{}));

// [range.chunk.by.iter]: bidirectional iff V is; never sized; common iff V is; not const-iterable.
static_assert(bidirectional_range<vec_chunks> && !random_access_range<vec_chunks> && common_range<vec_chunks> && !sized_range<vec_chunks>);
static_assert(forward_range<fwd_chunks> && !bidirectional_range<fwd_chunks>);
static_assert(bidirectional_range<arr_chunks> && !range<const arr_chunks> && view<arr_chunks>);
static_assert(bidirectional_range<list_chunks>);
static_assert(std::same_as<range_value_t<vec_chunks>, subrange<std::vector<int>::iterator>>);

} // namespace

TEST(ChunkByView, DraftExample) {
    std::vector<int> v{1, 2, 2, 3, 0, 4, 5, 2};
    EXPECT_EQ(to_chunks(v | views::chunk_by(std::ranges::less_equal{})), (chunks_t{{1, 2, 2, 3}, {0, 4, 5}, {2}}));
}

TEST(ChunkByView, EmptyAndSingle) {
    std::vector<int> none;
    EXPECT_TRUE(to_chunks(none | views::chunk_by(std::equal_to<>{})).empty());
    std::vector<int> one{7};
    EXPECT_EQ(to_chunks(one | views::chunk_by(std::equal_to<>{})), (chunks_t{{7}}));
}

TEST(ChunkByView, Decrement) {
    std::vector<int> v{1, 2, 2, 3, 0, 4, 5, 2};
    auto c = v | views::chunk_by(std::ranges::less_equal{});
    auto it = c.end();
    --it;
    EXPECT_EQ((*it).front(), 2);
    --it;
    EXPECT_EQ((*it).front(), 0);
    --it;
    EXPECT_EQ((*it).front(), 1);
    EXPECT_TRUE(it == c.begin());
}

TEST(ChunkByView, CachesBegin) {
    std::vector<int> v{1, 1, 2};
    int calls = 0;
    auto c = v | views::chunk_by([&calls](int x, int y) {
                 ++calls;
                 return x == y;
             });
    EXPECT_EQ((*c.begin()).size(), 2U);
    const int after_first = calls;
    EXPECT_EQ((*c.begin()).size(), 2U);
    EXPECT_EQ(calls, after_first);
}

TEST(ChunkByView, NonCommonBase) {
    auto c = views::iota(0) | views::take(5) | views::chunk_by([](int x, int y) { return y == x + 1 && y != 3; });
    static_assert(!common_range<decltype(c)>);
    EXPECT_EQ(to_chunks(c), (chunks_t{{0, 1, 2}, {3, 4}}));
}

TEST(ChunkByView, UnrealBases) {
    TArray<int> a;
    for (int x : {1, 2, 2, 3, 0, 4, 5, 2}) {
        a.Add(x);
    }
    EXPECT_EQ(to_chunks(a | views::chunk_by(std::ranges::less_equal{})), (chunks_t{{1, 2, 2, 3}, {0, 4, 5}, {2}}));

    TDoubleLinkedList<int> dl;
    for (int x : {1, 1, 2, 3, 3}) {
        dl.AddTail(x);
    }
    auto c = dl | views::chunk_by(std::equal_to<>{});
    EXPECT_EQ(to_chunks(c), (chunks_t{{1, 1}, {2}, {3, 3}}));
    auto it = c.end();
    --it;
    EXPECT_EQ((*it).front(), 3);
    --it;
    --it;
    EXPECT_TRUE(it == c.begin());

    TMap<int, int> m;
    m.Add(1, 1);
    m.Add(2, 1);
    m.Add(3, 2);
    auto by_value = m | views::chunk_by([](const auto& x, const auto& y) { return x.Value == y.Value; });
    EXPECT_EQ(distance(by_value), 2);
}
