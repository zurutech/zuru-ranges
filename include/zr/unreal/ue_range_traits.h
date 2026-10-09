// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/views/subrange.h"

#include <concepts>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include <utility>

/**
 * @file ue_range_traits.h
 *
 * @brief range_traits of the Unreal Engine containers. No Epic source: template names and
 *        heads only.
 *
 * Include after "CoreMinimal.h" (or the test fakes); includes no Unreal header. Containers
 * CoreMinimal.h may not declare are forward-declared with UE 5.7's template heads and must be
 * complete where their trait is used. TList and TCircularBuffer have no begin/end: not ranges
 * ([range.range]).
 */

// Not necessarily declared by CoreMinimal.h.
template <typename InElementType, typename InAllocatorType> class TDeque;
template <typename InElementType, uint32 NumElements, uint32 Alignment> class TStaticArray;
template <typename InElementType, uint32 TargetBytesPerChunk, typename AllocatorType> class TChunkedArray;
template <typename T, typename Allocator> class TIndirectArray;
template <typename T, typename AllocatorT> class TRingBuffer;
template <typename InElementType, typename InSizeType> class TStridedView;
template <typename InElementType, typename KeyFuncs, typename Allocator> class TCompactSet;
template <typename InElementType, typename KeyFuncs, typename Allocator> class TSparseSet;
template <typename InKeyType, typename InValueType, typename SetAllocator, typename KeyFuncs> class TCompactMap;
template <typename InKeyType, typename InValueType, typename SetAllocator, typename KeyFuncs> class TSparseMap;
template <typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs> class TCompactMultiMap;
template <typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs> class TSparseMultiMap;
template <typename InElementType, typename ArrayAllocator, typename SortPredicate> class TSortedSet;
template <class ElementType> class TLinkedList;
template <class ElementType> class TIntrusiveLinkedList;
template <class ElementType> class TDoubleLinkedList;
class FAnsiString;
class FUtf8String;
template <typename BaseType> class TIteratorAdapter;
class FBitReference;
class FConstBitReference;

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// TIteratorAdapter (TDeque's, TPagedArray's iterator) declares `--` for every base; it compiles
// only over a base with Decrement, which is protected: probed from a derived class.
template <class Base> struct ue_adapter_probe : Base {
    template <class P> static auto decrements(int) -> decltype(std::declval<P&>().Decrement(), std::true_type{});
    template <class P> static std::false_type decrements(...);
    static constexpr bool value = decltype(decrements<ue_adapter_probe>(0))::value;
};

template <class Base> inline constexpr bool decrement_disabled<TIteratorAdapter<Base>> = !ue_adapter_probe<Base>::value;

// TBitArray's proxies: value type bool, as vector<bool>'s. A proxy value type would alias the
// container (FBitReference's assignment writes the bit; to<vector>() of an rvalue would dangle).
template <> struct proxy_value<FBitReference> {
    using type = bool;
};
template <> struct proxy_value<FConstBitReference> {
    using type = bool;
};

template <class Container, range_category Category> struct ue_sized_traits {
    static constexpr range_category category = Category;

    static auto size(const Container& container) { return container.Num(); }
};

// Random access through operator[], never the iterator (forward-only, configuration-dependent).
template <class Container> struct ue_subscript_traits : ue_sized_traits<Container, range_category::random_access> {
    template <std::integral Index> static decltype(auto) element_at(Container& c, Index index) { return c[static_cast<decltype(c.Num())>(index)]; }
    template <std::integral Index> static decltype(auto) element_at(const Container& c, Index index) { return c[static_cast<decltype(c.Num())>(index)]; }
};

// Contiguous over GetCharArray(); Len() excludes the null terminator.
template <class String> struct ue_string_traits : ue_sized_traits<String, range_category::contiguous> {
    static auto data(String& string) { return string.GetCharArray().GetData(); }
    static auto data(const String& string) { return string.GetCharArray().GetData(); }
    static auto size(const String& string) { return string.Len(); }
};

template <class It, class E>
concept ue_array_ranged_for_iterator =
    std::same_as<It, typename TArray<E>::RangedForIteratorType> || std::same_as<It, typename TArray<E>::RangedForConstIteratorType> ||
    std::same_as<It, typename TArray<E, FDefaultAllocator64>::RangedForIteratorType> ||
    std::same_as<It, typename TArray<E, FDefaultAllocator64>::RangedForConstIteratorType>;

// A type without ElementType iterating TArray's ranged-for iterator: a standard_range in
// Shipping (T*), not in debug. Contiguous through the checked iterator's operator-> (UE 5.7+),
// bidirectional before (the checked iterator decrements, for FString).
template <class R>
concept ue_array_checked_iterator_range = !has_element_type<R> && !std::ranges::range<R> && range<R> &&
                                          ue_array_ranged_for_iterator<iterator_t<R>, std::remove_cvref_t<range_reference_t<R>>>;

template <class R>
concept ue_array_iterator_range = ue_array_checked_iterator_range<R> && requires(const iterator_t<R>& it) { it.operator->(); };

// TArray's debug ranged-for iterator; with operator-> from UE 5.7.
template <class It>
concept ue_array_checked_iterator = std::is_class_v<It> && ue_array_ranged_for_iterator<It, std::remove_cvref_t<iter_reference_t<It>>>;

template <class It>
concept ue_array_debug_iterator = ue_array_checked_iterator<It> && requires(const It& it) { it.operator->(); };

} // namespace detail

// subrange of TArray's debug ranged-for iterator before UE 5.7: no operator->, no data.
template <detail::ue_array_checked_iterator I, subrange_kind K>
    requires(!detail::ue_array_debug_iterator<I>)
struct range_traits_impl<subrange<I, I, K>> {
    static constexpr range_category category = range_category::bidirectional;
};

// subrange of TArray's debug ranged-for iterator: as subrange<T*> in Shipping.
template <detail::ue_array_debug_iterator I, subrange_kind K> struct range_traits_impl<subrange<I, I, K>> {
    static constexpr range_category category = range_category::contiguous;

    template <class Self> static auto data(Self& s) { return s.begin().operator->(); }
    template <class Self> static auto size(Self& s) { return static_cast<std::size_t>(s.end().operator->() - s.begin().operator->()); }
};

// UE 5.3 to 5.6: the checked iterator has no operator->, so no data or size.
template <detail::ue_array_checked_iterator_range R>
    requires(!detail::ue_array_iterator_range<R>)
struct range_traits_impl<R> {
    static constexpr range_category category = range_category::bidirectional;
};

// Same category, data and size as the standard_range lifting gives it in Shipping.
template <detail::ue_array_iterator_range R> struct range_traits_impl<R> {
    static constexpr range_category category = range_category::contiguous;

    template <class Self> static auto data(Self& r) { return zr::ranges::begin(r).operator->(); }
    template <class Self> static auto size(Self& r) {
        return static_cast<std::size_t>(zr::ranges::end(r).operator->() - zr::ranges::begin(r).operator->());
    }
};

template <class T, class Alloc> struct range_traits_impl<TArray<T, Alloc>> : detail::ue_sized_traits<TArray<T, Alloc>, range_category::contiguous> {
    static auto data(TArray<T, Alloc>& array) { return array.GetData(); }
    static auto data(const TArray<T, Alloc>& array) { return array.GetData(); }
};

template <class T, class S> struct range_traits_impl<TArrayView<T, S>> : detail::ue_sized_traits<TArrayView<T, S>, range_category::contiguous> {
    static auto data(const TArrayView<T, S>& v) { return v.GetData(); }
};

namespace detail {

// Contiguous over T unless elements are padded. 5.7: Alignment defaults to uint32(-1) and any
// other value stores TAlignedElement (GetData() is not a T*). 5.3-5.6: Alignment defaults to
// alignof(T) and GetData() static_asserts that alignof(T) is a multiple of it.
template <class T, uint32 N, uint32 Alignment>
inline constexpr bool ue_static_array_contiguous =
    Alignment == uint32(-1) ||
    (alignof(T) % Alignment == 0 && std::same_as<decltype(std::declval<TStaticArray<T, N, Alignment>&>().GetData()), T*>);

} // namespace detail

template <class T, uint32 N, uint32 Alignment>
struct range_traits_impl<TStaticArray<T, N, Alignment>>
    : detail::ue_sized_traits<TStaticArray<T, N, Alignment>, detail::ue_static_array_contiguous<T, N, Alignment> ? range_category::contiguous
                                                                                                              : range_category::random_access> {
    static auto data(TStaticArray<T, N, Alignment>& array)
        requires detail::ue_static_array_contiguous<T, N, Alignment>
    {
        return array.GetData();
    }
    static auto data(const TStaticArray<T, N, Alignment>& array)
        requires detail::ue_static_array_contiguous<T, N, Alignment>
    {
        return array.GetData();
    }

    template <std::integral Index>
        requires(!detail::ue_static_array_contiguous<T, N, Alignment>)
    static decltype(auto) element_at(TStaticArray<T, N, Alignment>& array, Index index) {
        return array[static_cast<uint32>(index)];
    }
    template <std::integral Index>
        requires(!detail::ue_static_array_contiguous<T, N, Alignment>)
    static decltype(auto) element_at(const TStaticArray<T, N, Alignment>& array, Index index) {
        return array[static_cast<uint32>(index)];
    }
};

template <class C> struct range_traits_impl<TStringView<C>> : detail::ue_sized_traits<TStringView<C>, range_category::contiguous> {
    static auto data(const TStringView<C>& v) { return v.GetData(); }
    static auto size(const TStringView<C>& v) { return v.Len(); }
};

template <> struct range_traits_impl<FString> : detail::ue_string_traits<FString> {};
template <> struct range_traits_impl<FAnsiString> : detail::ue_string_traits<FAnsiString> {};
template <> struct range_traits_impl<FUtf8String> : detail::ue_string_traits<FUtf8String> {};

template <class T, class A> struct range_traits_impl<TDeque<T, A>> : detail::ue_subscript_traits<TDeque<T, A>> {};
template <class T, uint32 B, class A> struct range_traits_impl<TChunkedArray<T, B, A>> : detail::ue_subscript_traits<TChunkedArray<T, B, A>> {};
template <class T, class A> struct range_traits_impl<TIndirectArray<T, A>> : detail::ue_subscript_traits<TIndirectArray<T, A>> {};
template <class T, class A> struct range_traits_impl<TRingBuffer<T, A>> : detail::ue_subscript_traits<TRingBuffer<T, A>> {};
template <class T, class S> struct range_traits_impl<TStridedView<T, S>> : detail::ue_subscript_traits<TStridedView<T, S>> {};
template <class A> struct range_traits_impl<TBitArray<A>> : detail::ue_subscript_traits<TBitArray<A>> {};

// Holes left by removal: indices are not positions.
template <class T, class A> struct range_traits_impl<TSparseArray<T, A>> : detail::ue_sized_traits<TSparseArray<T, A>, range_category::forward> {};

// TSet/TMap are stamped from the sparse or the compact storage (UE_USE_COMPACT_SET_AS_DEFAULT,
// 5.7); both storages are also named: TSparseSet/TSparseMap and TCompactSet/TCompactMap. The
// compact ranged-for iterator switches with the configuration, as TArray's: forward in both.
template <class T, class K, class A> struct range_traits_impl<TSet<T, K, A>> : detail::ue_sized_traits<TSet<T, K, A>, range_category::forward> {};
template <class T, class K, class A> struct range_traits_impl<TSparseSet<T, K, A>> : detail::ue_sized_traits<TSparseSet<T, K, A>, range_category::forward> {};
template <class T, class K, class A> struct range_traits_impl<TCompactSet<T, K, A>> : detail::ue_sized_traits<TCompactSet<T, K, A>, range_category::forward> {};
template <class K, class V, class A, class F> struct range_traits_impl<TMap<K, V, A, F>> : detail::ue_sized_traits<TMap<K, V, A, F>, range_category::forward> {};
template <class K, class V, class A, class F> struct range_traits_impl<TSparseMap<K, V, A, F>> : detail::ue_sized_traits<TSparseMap<K, V, A, F>, range_category::forward> {};
template <class K, class V, class A, class F> struct range_traits_impl<TCompactMap<K, V, A, F>> : detail::ue_sized_traits<TCompactMap<K, V, A, F>, range_category::forward> {};
template <class K, class V, class A, class F> struct range_traits_impl<TMultiMap<K, V, A, F>> : detail::ue_sized_traits<TMultiMap<K, V, A, F>, range_category::forward> {};
template <class K, class V, class A, class F> struct range_traits_impl<TSparseMultiMap<K, V, A, F>> : detail::ue_sized_traits<TSparseMultiMap<K, V, A, F>, range_category::forward> {};
template <class K, class V, class A, class F> struct range_traits_impl<TCompactMultiMap<K, V, A, F>> : detail::ue_sized_traits<TCompactMultiMap<K, V, A, F>, range_category::forward> {};

// TSortedMap's iterator is TArray's (T* in Shipping): forward in both configurations.
template <class K, class V, class A, class S> struct range_traits_impl<TSortedMap<K, V, A, S>> : detail::ue_sized_traits<TSortedMap<K, V, A, S>, range_category::forward> {};

// TSortedSet's iterator is TArray's, as TSortedMap's: forward in both configurations.
template <class T, class A, class S> struct range_traits_impl<TSortedSet<T, A, S>> : detail::ue_sized_traits<TSortedSet<T, A, S>, range_category::forward> {};

template <class T> struct range_traits_impl<TDoubleLinkedList<T>> : detail::ue_sized_traits<TDoubleLinkedList<T>, range_category::bidirectional> {};

// A node is its own list head; no Num(), so not sized.
template <class T> struct range_traits_impl<TLinkedList<T>> {
    static constexpr range_category category = range_category::forward;
};
// The list type is the node class deriving from TIntrusiveLinkedList<Node>.
template <class Node>
    requires std::derived_from<Node, TIntrusiveLinkedList<Node>>
struct range_traits_impl<Node> {
    static constexpr range_category category = range_category::forward;
};

ZR_END_NAMESPACE_RANGES
