// Views over every fake-unreal container, including those whose ranged-for iterator switches
// with the build configuration, is non-assignable, yields a proxy, has a non-const operator*, or is
// reached through hidden friends. Results must be identical in both configurations.
#include "CoreMinimal.h"
#include "Containers/AnsiString.h"
#include "Containers/ChunkedArray.h"
#include "Containers/CompactSet.h"
#include "Containers/Deque.h"
#include "Containers/IndirectArray.h"
#include "Containers/List.h"
#include "Containers/RingBuffer.h"
#include "Containers/SortedMap.h"
#include "Containers/SortedSet.h"
#include "Containers/StaticArray.h"
#include "Containers/StridedView.h"
#include "Containers/Utf8String.h"

#include "zr/algorithm/nonmodifying.h"
#include "zr/ranges/to.h"
#include "zr/ranges/views/concat_view.h"
#include "zr/ranges/views/elements_view.h"
#include "zr/ranges/views/enumerate_view.h"
#include "zr/ranges/views/filter_view.h"
#include "zr/ranges/views/reverse_view.h"
#include "zr/ranges/views/subrange.h"
#include "zr/ranges/views/transform_view.h"
#include "zr/ranges/views/zip_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

using namespace zr::ranges;

namespace {
constexpr bool is_odd(int x) { return x % 2 != 0; }

template <class R> std::vector<int> collect(R&& r) {
    std::vector<int> out;
    for (auto&& x : r) {
        out.push_back(static_cast<int>(x));
    }
    return out;
}

struct FNode : TIntrusiveLinkedList<FNode> {
    int Value = 0;
};
} // namespace

TEST(UnrealQuirks, SortedMapIteratesInKeyOrder) {
    TSortedMap<int, int> map;
    map.Add(3, 30);
    map.Add(1, 10);
    map.Add(2, 20);
    map.Add(1, 11);
    std::vector<int> keys = map | views::transform([](const TPair<int, int>& p) { return p.Key; }) | zr::ranges::to<std::vector<int>>();
    EXPECT_EQ(keys, (std::vector<int>{1, 2, 3}));
}

TEST(UnrealQuirks, SortedAndCompactSets) {
    TSortedSet<int> sorted;
    TCompactSet<int> compact;
    for (int v : {5, 1, 3, 1, 4}) {
        sorted.Add(v);
        compact.Add(v);
    }
    EXPECT_EQ(collect(sorted | views::filter(is_odd)), (std::vector<int>{1, 3, 5}));
    EXPECT_EQ(zr::ranges::size(sorted), 4);
    EXPECT_EQ(zr::ranges::size(compact), 4);
    int sum = 0;
    for (auto [i, v] : compact | views::enumerate) {
        sum += v;
        EXPECT_LT(i, 4);
    }
    EXPECT_EQ(sum, 5 + 1 + 3 + 4);
}

TEST(UnrealQuirks, StaticArrayPlainAndOverAligned) {
    TStaticArray<int, 3> plain;
    TStaticArray<int, 3, 64> aligned;
    for (uint32 i = 0; i < 3; ++i) {
        plain[i] = static_cast<int>(i) + 1;
        aligned[i] = static_cast<int>(i) + 1;
    }
    EXPECT_EQ(collect(plain | views::reverse), (std::vector<int>{3, 2, 1}));
    EXPECT_EQ(collect(aligned | views::reverse), (std::vector<int>{3, 2, 1}));
    EXPECT_EQ(zr::ranges::element_at(aligned, 2), 3);
    EXPECT_EQ(zr::ranges::data(plain), plain.GetData());
}

TEST(UnrealQuirks, IndirectArrayDereferencesTwice) {
    TIndirectArray<int> arr;
    arr.Add(new int(1)); // NOLINT(cppcoreguidelines-owning-memory): Add takes ownership
    arr.Add(new int(2)); // NOLINT(cppcoreguidelines-owning-memory): Add takes ownership
    arr.Add(new int(3)); // NOLINT(cppcoreguidelines-owning-memory): Add takes ownership
    for (int& v : arr | views::filter(is_odd)) {
        v *= 10;
    }
    EXPECT_EQ(collect(arr | views::reverse), (std::vector<int>{30, 2, 10}));
}

TEST(UnrealQuirks, ChunkedArrayVoidIncrementAsymmetricInequality) {
    TChunkedArray<int> arr;
    for (int v : {1, 2, 3, 4}) {
        arr.AddElement(v);
    }
    EXPECT_EQ(collect(arr | views::filter(is_odd)), (std::vector<int>{1, 3}));
    EXPECT_EQ(collect(views::concat(arr, arr | views::reverse)), (std::vector<int>{1, 2, 3, 4, 4, 3, 2, 1}));
    EXPECT_EQ(zr::ranges::element_at(arr, 3), 4);
}

TEST(UnrealQuirks, SparseArraySkipsHoles) {
    TSparseArray<int> arr;
    for (int v : {1, 9, 2, 3}) {
        arr.Add(v);
    }
    arr.RemoveAt(1);
    EXPECT_EQ(collect(arr | views::transform([](int v) { return v * 2; })), (std::vector<int>{2, 4, 6}));
    EXPECT_EQ(zr::ranges::size(arr), 3);
}

TEST(UnrealQuirks, RingBufferRandomAccess) {
    TRingBuffer<int> ring;
    ring.Add(2);
    ring.Add(3);
    ring.AddFront(1);
    EXPECT_EQ(collect(ring | views::reverse), (std::vector<int>{3, 2, 1}));
    EXPECT_EQ(zr::ranges::element_at(ring, 0), 1);
}

TEST(UnrealQuirks, StridedViewNonConstDereference) {
    int raw[6]{1, 0, 2, 0, 3, 0};
    TStridedView<int> strided(static_cast<int32>(2 * sizeof(int)), raw, 3);
    EXPECT_EQ(collect(strided | views::filter(is_odd)), (std::vector<int>{1, 3}));
    EXPECT_EQ(collect(views::zip(strided, strided) | views::transform([](auto t) { return std::get<0>(t) + std::get<1>(t); })),
              (std::vector<int>{2, 4, 6}));
    EXPECT_EQ(collect(strided | views::reverse), (std::vector<int>{3, 2, 1}));
}

#if ZR_FAKE_UE_VERSION >= 504 // TBitArray is a range from UE 5.4
TEST(UnrealQuirks, BitArrayProxyReference) {
    TBitArray<> bits(false, 4);
    zr::ranges::element_at(bits, 1) = true;
    zr::ranges::element_at(bits, 3) = true;
    int set = 0;
    for (auto [i, bit] : bits | views::enumerate) {
        set += bit ? static_cast<int>(i) : 0;
    }
    EXPECT_EQ(set, 1 + 3);
}
#endif

TEST(UnrealQuirks, LinkedListsThroughHiddenFriends) {
    TDoubleLinkedList<int> doubly;
    for (int v : {1, 2, 3}) {
        doubly.AddTail(v);
    }
    EXPECT_EQ(collect(doubly | views::filter(is_odd)), (std::vector<int>{1, 3}));
    EXPECT_EQ(zr::ranges::size(doubly), 3);

    TLinkedList<int> a(1), b(2), c(3);
    TLinkedList<int>* head = nullptr;
    c.LinkHead(head);
    b.LinkHead(head);
    a.LinkHead(head);
    EXPECT_EQ(collect(*head | views::transform([](int v) { return v * 10; })), (std::vector<int>{10, 20, 30}));

    FNode x, y;
    x.Value = 4;
    y.Value = 5;
    FNode* intrusive = nullptr;
    y.LinkHead(intrusive);
    x.LinkHead(intrusive);
    EXPECT_EQ(collect(*intrusive | views::transform([](const FNode& n) { return n.Value; })), (std::vector<int>{4, 5}));
}

TEST(UnrealQuirks, StringsAndStringView) {
    FAnsiString ansi("abc");
    FUtf8String utf8(u8"ab");
    FStringView sv(TEXT("xyz"));
    EXPECT_EQ(zr::ranges::size(ansi), 3);
    EXPECT_EQ(zr::ranges::size(utf8), 2);
    EXPECT_EQ(zr::ranges::size(sv), 3);
    EXPECT_EQ(collect(ansi | views::reverse), (std::vector<int>{'c', 'b', 'a'}));
    EXPECT_EQ(*zr::ranges::begin(sv | views::reverse), TEXT('z'));
}

TEST(UnrealQuirks, MultiMapRepeatsKeys) {
    TMultiMap<int, int> map;
    map.Add(1, 10);
    map.Add(1, 11);
    EXPECT_EQ(collect(map | views::transform([](const TPair<int, int>& p) { return p.Value; })), (std::vector<int>{10, 11}));
    EXPECT_EQ(zr::ranges::size(map), 2);
}

TEST(UnrealQuirks, NonAssignableSetMapIterators) {
    TSet<int> set{1, 2, 3};
    TMap<int, int> map{{1, 10}, {2, 20}};
    int sum = 0;
    for (int v : views::concat(set, set | views::filter(is_odd))) {
        sum += v;
    }
    EXPECT_EQ(sum, 6 + 4);
    sum = 0;
    for (auto [s, p] : views::zip(set, map)) {
        sum += s * p.Value;
    }
    EXPECT_EQ(sum, (1 * 10) + (2 * 20));
}

namespace {
struct FPolygon {
    TArray<int> Points;
    auto begin() const { return Points.begin(); }
    auto end() const { return Points.end(); }
};
} // namespace

TEST(UnrealQuirks, ArrayForwardingWrapperSameInBothConfigs) {
    FPolygon polygon{{1, 2, 3}};
    EXPECT_EQ(collect(polygon | views::reverse), (std::vector<int>{3, 2, 1}));
#if ZR_FAKE_UE_VERSION >= 507 || !ZR_FAKE_UE_RANGED_FOR_CHECKS // the checked iterator's operator-> (UE 5.7)
    EXPECT_EQ(zr::ranges::size(polygon), 3u);
    EXPECT_EQ(zr::ranges::data(polygon), polygon.Points.GetData());
    EXPECT_EQ(zr::ranges::element_at(polygon, 1), 2);
    EXPECT_EQ(zr::ranges::size(FPolygon{}), 0u);
#endif
}

TEST(UnrealQuirks, SubrangeOverNonAssignableIterators) {
    TArray<int> array{1, 2, 3};
    TSet<int> set{4, 5};
    EXPECT_EQ(collect(views::filter(subrange(array.begin(), array.end()), is_odd)), (std::vector<int>{1, 3}));
    EXPECT_EQ(collect(subrange(set.begin(), set.end()) | views::transform([](int v) { return v * 10; })), (std::vector<int>{40, 50}));
    subrange<decltype(array.begin())> a(array.begin(), array.end());
    subrange<decltype(array.begin())> b(array.begin(), array.begin());
    b = a; // reassigns through movable_box
    EXPECT_EQ(collect(b), (std::vector<int>{1, 2, 3}));
}

TEST(UnrealQuirks, AsymmetricInequalityComparesEqualSymmetrically) {
    // TChunkedArray's `!=` is `Count < Rhs.Count`: `end != it` is always false. iter_equal checks
    // both orders, so `end == it` is false until they are equal.
    TChunkedArray<int> arr;
    for (int v : {1, 2, 3}) {
        arr.AddElement(v);
    }
    auto f = arr | views::filter([](int) { return true; });
    auto it = f.begin();
    auto last = f.end();
    EXPECT_FALSE(it == last);
    EXPECT_FALSE(last == it);
    EXPECT_TRUE(it != last);
    EXPECT_TRUE(last != it);
    ++it;
    ++it;
    ++it;
    EXPECT_TRUE(it == last);
    EXPECT_TRUE(last == it);

    auto t = arr | views::transform([](int v) { return v * 10; });
    auto found = zr::ranges::find(t, 20);
    EXPECT_FALSE(found == t.end());
    EXPECT_FALSE(t.end() == found);
    EXPECT_EQ(*found, 20);
}

#if ZR_FAKE_UE_VERSION >= 504 // TBitArray is a range from UE 5.4
TEST(UnrealQuirks, ConstBitArrayYieldsConstProxies) {
    TBitArray<> bits(false, 3);
    zr::ranges::element_at(bits, 1) = true;
    const TBitArray<>& cbits = bits;
    static_assert(std::is_same_v<range_reference_t<const TBitArray<>>, FConstBitReference>);
    EXPECT_EQ(collect(cbits), (std::vector<int>{0, 1, 0}));
    EXPECT_EQ(collect(cbits | views::reverse), (std::vector<int>{0, 1, 0}));
    EXPECT_EQ(zr::ranges::count_if(cbits, [](bool b) { return b; }), 1);
    std::vector<bool> copy = cbits | zr::ranges::to<std::vector<bool>>();
    EXPECT_EQ(copy, (std::vector<bool>{false, true, false}));
}

TEST(UnrealQuirks, BitArrayValueTypeIsBool) {
    // A proxy value type would alias the bits: deduced containers hold bool.
    auto copy = TBitArray<>(true, 3) | zr::ranges::to<std::vector>();
    static_assert(std::is_same_v<decltype(copy), std::vector<bool>>);
    EXPECT_EQ(copy, (std::vector<bool>{true, true, true}));
}
#endif

TEST(UnrealQuirks, BitReferenceAssignmentWritesTheBit) {
    TBitArray<> bits(false, 2);
    TBitArray<> ones(true, 1);
    zr::ranges::element_at(bits, 0) = zr::ranges::element_at(ones, 0);
    EXPECT_TRUE(zr::ranges::element_at(bits, 0));
}

TEST(UnrealQuirks, DequeByPositionRingBufferIteratorsDecrement) {
    // TDeque's iterator declares `--` but cannot decrement: reversed by position.
    TDeque<int> deque{1, 2, 3, 4};
    EXPECT_EQ(collect(deque | views::reverse), (std::vector<int>{4, 3, 2, 1}));
    EXPECT_EQ(collect(deque | views::transform([](int v) { return v * 2; }) | views::reverse), (std::vector<int>{8, 6, 4, 2}));
    EXPECT_EQ(collect(deque | views::filter(is_odd)), (std::vector<int>{1, 3}));

    TRingBuffer<int> ring;
    for (int v : {1, 2, 3, 4}) {
        ring.Add(v);
    }
    auto doubled = ring | views::transform([](int v) { return v * 2; });
    auto jt = doubled.end();
    --jt;
    EXPECT_EQ(*jt, 8);
    EXPECT_EQ(collect(ring | views::filter(is_odd) | views::reverse), (std::vector<int>{3, 1}));
}

TEST(UnrealQuirks, SparseAndCompactSetsAndMaps) {
    TSparseSet<int> sparse{1, 2};
    TCompactSet<int> compact{3};
    TSparseMap<int, int> sparse_map;
    sparse_map.Add(1, 10);
    TCompactMap<int, int> compact_map;
    compact_map.Add(2, 20);
    TSparseMultiMap<int, int> sparse_multi;
    sparse_multi.Add(1, 1);
    sparse_multi.Add(1, 2);
    TCompactMultiMap<int, int> compact_multi;
    compact_multi.Add(3, 3);
    EXPECT_EQ(collect(views::concat(sparse, compact)), (std::vector<int>{1, 2, 3}));
    auto value = [](const TPair<int, int>& p) { return p.Value; };
    EXPECT_EQ(collect(views::concat(sparse_map | views::transform(value), compact_map | views::transform(value))), (std::vector<int>{10, 20}));
    EXPECT_EQ(collect(views::concat(sparse_multi | views::values, compact_multi | views::values)), (std::vector<int>{1, 2, 3}));
    EXPECT_EQ(zr::ranges::size(sparse_multi), 2);
}

TEST(UnrealQuirks, TupleProtocolOfTTuple) {
    // TPair is TTuple<K, V>: member Get<N>, global get<int N> by ADL, std tuple_size/tuple_element.
    TArray<TTuple<int, int>> pairs{{1, 10}, {2, 20}};
    EXPECT_EQ(collect(pairs | views::keys), (std::vector<int>{1, 2}));
    EXPECT_EQ(collect(pairs | views::values), (std::vector<int>{10, 20}));
    auto [k, val] = pairs[1];
    EXPECT_EQ(k + val, 22);
}
