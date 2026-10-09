// Compile-only checks for the fake-unreal quirks. Built in both iterator configurations: every
// unconditional assertion is also a configuration-independence check.
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

#include "zr/ranges/range_concepts.h"
#include "zr/ranges/views/enumerate_view.h"
#include "zr/ranges/views/filter_view.h"
#include "zr/ranges/views/maybe_view.h"
#include "zr/ranges/views/reverse_view.h"
#include "zr/ranges/views/subrange.h"
#include "zr/ranges/views/transform_view.h"
#include "zr/ranges/views/zip_view.h"
#include "zr/unreal/ue_range_traits.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
using namespace zr::ranges;

struct FNode : TIntrusiveLinkedList<FNode> {};

// Not a standard_range (ElementType) and no trait.
struct FTraitless {
    using ElementType = int;
    TArray<int> Items;
    auto begin() { return Items.begin(); }
    auto end() { return Items.end(); }
};

template <class I> constexpr bool const_dereferenceable = requires(const I& it) { *it; };

template <class R, range_category Category, bool Sized>
constexpr bool classified_as = range<R> && detail::category_of<R>() == Category && sized_range<R> == Sized;

// Config-dependent iterator types and shapes...
#if ZR_FAKE_UE_RANGED_FOR_CHECKS
static_assert(!std::is_pointer_v<iterator_t<TSortedSet<int>>>);
static_assert(!std::is_pointer_v<iterator_t<TCompactSet<int>>>);
static_assert(!std::is_pointer_v<iterator_t<FAnsiString>>);
static_assert(!std::is_copy_assignable_v<iterator_t<TIndirectArray<int>>>);
#else
static_assert(std::is_pointer_v<iterator_t<TSortedSet<int>>>);
static_assert(std::is_pointer_v<iterator_t<TCompactSet<int>>>);
static_assert(std::is_pointer_v<iterator_t<FAnsiString>>);
static_assert(std::is_copy_assignable_v<iterator_t<TIndirectArray<int>>>);
#endif

// ...the classification does not.
static_assert(classified_as<TArray<int>, range_category::contiguous, true>);
static_assert(classified_as<TArrayView<int>, range_category::contiguous, true>);
static_assert(classified_as<TStaticArray<int, 4>, range_category::contiguous, true>);
static_assert(classified_as<TStaticArray<int, 4, 64>, range_category::random_access, true>); // padded elements
static_assert(classified_as<FString, range_category::contiguous, true>);
static_assert(classified_as<FAnsiString, range_category::contiguous, true>);
static_assert(classified_as<FUtf8String, range_category::contiguous, true>);
static_assert(classified_as<FStringView, range_category::contiguous, true>);
static_assert(classified_as<TChunkedArray<int>, range_category::random_access, true>);
static_assert(classified_as<TIndirectArray<int>, range_category::random_access, true>);
static_assert(classified_as<TRingBuffer<int>, range_category::random_access, true>);
static_assert(classified_as<TStridedView<int>, range_category::random_access, true>);
#if ZR_FAKE_UE_VERSION >= 504 // TBitArray is a range from UE 5.4
static_assert(classified_as<TBitArray<>, range_category::random_access, true>);
#endif
static_assert(classified_as<TDoubleLinkedList<int>, range_category::bidirectional, true>);
static_assert(classified_as<TSparseArray<int>, range_category::forward, true>);
static_assert(classified_as<TSet<int>, range_category::forward, true>);
static_assert(classified_as<TSparseSet<int>, range_category::forward, true>);
static_assert(classified_as<TCompactSet<int>, range_category::forward, true>);
static_assert(classified_as<TSparseMap<int, int>, range_category::forward, true>);
static_assert(classified_as<TCompactMap<int, int>, range_category::forward, true>);
static_assert(classified_as<TSparseMultiMap<int, int>, range_category::forward, true>);
static_assert(classified_as<TCompactMultiMap<int, int>, range_category::forward, true>);
static_assert(classified_as<TSortedSet<int>, range_category::forward, true>);
static_assert(classified_as<TMap<int, int>, range_category::forward, true>);
static_assert(classified_as<TMultiMap<int, int>, range_category::forward, true>);
static_assert(classified_as<TSortedMap<int, int>, range_category::forward, true>);
static_assert(classified_as<TLinkedList<int>, range_category::forward, false>);
static_assert(classified_as<FNode, range_category::forward, false>);

// Iterator shapes accepted: `++i` not yielding I& (TSparseArray: base class; TChunkedArray:
// void), hidden-friend begin/end ([range.access.begin] ADL branch), non-const operator*.
static_assert(!std::is_same_v<decltype(++std::declval<iterator_t<TSparseArray<int>>&>()), iterator_t<TSparseArray<int>>&>);
static_assert(std::is_void_v<decltype(++std::declval<iterator_t<TChunkedArray<int>>&>())>);
static_assert(!const_dereferenceable<iterator_t<TStridedView<int>>>);
static_assert(range<filter_view<views::all_t<TStridedView<int>&>, bool (*)(int)>>);

// No ElementType, forwards TArray's ranged-for iterator: a standard_range in Shipping only;
// classified identically in both configurations.
struct FPolygon {
    TArray<int> Points;
    auto begin() { return Points.begin(); }
    auto end() { return Points.end(); }
    auto begin() const { return Points.begin(); }
    auto end() const { return Points.end(); }
};
#if ZR_FAKE_UE_VERSION >= 507 || !ZR_FAKE_UE_RANGED_FOR_CHECKS // the checked iterator's operator-> (UE 5.7)
static_assert(classified_as<FPolygon, range_category::contiguous, true>);
static_assert(contiguous_range<const FPolygon>);
static_assert(std::is_same_v<range_size_t<FPolygon>, std::size_t>);
#else
static_assert(classified_as<FPolygon, range_category::bidirectional, false>);
#endif

// subrange over non-assignable iterators is still a view in both configurations.
static_assert(view<subrange<iterator_t<TArray<int>>>>);
static_assert(view<subrange<iterator_t<TIndirectArray<int>>>>);
static_assert(view<subrange<iterator_t<TSet<int>>>>);

// Views over a trait-less base are input ranges ([range.refinements]).
using traitless_ref = views::all_t<FTraitless&>;
static_assert(range<FTraitless> && !forward_range<FTraitless>);
static_assert(!forward_range<filter_view<traitless_ref, bool (*)(int)>>);
static_assert(!forward_range<transform_view<traitless_ref, int (*)(int)>>);
static_assert(!forward_range<enumerate_view<traitless_ref>>);
static_assert(!forward_range<zip_view<traitless_ref, views::all_t<TArray<int>&>>>);

// Set/map iterators are never assignable.
static_assert(!std::is_copy_assignable_v<iterator_t<TSet<int>>>);
static_assert(!std::is_copy_assignable_v<iterator_t<TMap<int, int>>>);

// P1255R14 [range.nullable.view]/3: contextual bool, operator*.
static_assert(can_maybe_view<TArray<int*>>);
static_assert(can_maybe_view<TArray<TOptional<int>>>);
static_assert(can_maybe_view<TArray<TSharedPtr<int>>>);
static_assert(can_maybe_view<std::vector<std::optional<int>>>);
static_assert(can_maybe_view<std::vector<std::unique_ptr<int>>>);
static_assert(!can_maybe_view<TArray<int>>);
static_assert(!can_maybe_view<std::vector<bool>>);
static_assert(!can_maybe_view<TArray<TSharedRef<int>>>); // never null: no operator bool

// Bit arrays yield proxy classes, FConstBitReference when const; the value type is bool.
#if ZR_FAKE_UE_VERSION >= 504 // TBitArray is a range from UE 5.4
static_assert(std::is_same_v<range_reference_t<TBitArray<>>, FBitReference>);
static_assert(std::is_same_v<range_reference_t<const TBitArray<>>, FConstBitReference>);
static_assert(std::is_same_v<range_value_t<TBitArray<>>, bool> && std::is_same_v<range_value_t<const TBitArray<>>, bool>);
#else
static_assert(!range<TBitArray<>>);
#endif

// TDeque's iterator declares `--` over a base without Decrement: not decrementable, so views over
// a TDeque decrement only by position.
template <class R>
concept reversible = requires(R& r) { r | views::reverse; };
static_assert(!prefix_decrementable<iterator_t<TDeque<int>>> && !prefix_decrementable<iterator_t<const TDeque<int>>>);
static_assert(prefix_decrementable<iterator_t<TRingBuffer<int>>>);
static_assert(reversible<TDeque<int>> && reversible<decltype(std::declval<TDeque<int>&>() | views::transform(std::negate<>{}))>);
static_assert(!reversible<decltype(std::declval<TDeque<int>&>() | views::filter(std::identity{}))>);

// zip over Unreal arrays is common and random access in both configurations (index iteration in
// debug), as take over them; the base iterator decides nothing beyond the iterator tags.
using zip_arrays = zip_view<views::all_t<TArray<int>&>, views::all_t<TArray<int>&>>;
static_assert(common_range<zip_arrays> && random_access_range<zip_arrays> && sized_range<zip_arrays>);
static_assert(detail::index_iterable<TDeque<int>> && detail::index_iterable<TRingBuffer<int>> && detail::index_iterable<TChunkedArray<int>>);
} // namespace
