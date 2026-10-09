// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/iterator/operations.h"
#include "zr/ranges/movable_box.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"

#include <array>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

/**
 * @file subrange.h
 *
 * @brief [range.subrange] subrange.
 */

ZR_BEGIN_NAMESPACE_RANGES

// [ranges.syn] subrange_kind.
enum class subrange_kind : bool { unsized, sized };

namespace detail {

// [tuple.like] pair-like.
template <class T> inline constexpr bool tuple_like_v = false;
template <class... Ts> inline constexpr bool tuple_like_v<std::tuple<Ts...>> = true;
template <class A, class B> inline constexpr bool tuple_like_v<std::pair<A, B>> = true;
template <class T, std::size_t N> inline constexpr bool tuple_like_v<std::array<T, N>> = true;

template <class T>
concept pair_like_std = tuple_like_v<std::remove_cvref_t<T>> && std::tuple_size_v<std::remove_cvref_t<T>> == 2;

// [range.subrange.general]
template <class From, class To>
concept uses_nonqualification_pointer_conversion =
    std::is_pointer_v<From> && std::is_pointer_v<To> &&
    !std::convertible_to<std::remove_pointer_t<From> (*)[], std::remove_pointer_t<To> (*)[]>;

template <class From, class To>
concept convertible_to_non_slicing =
    std::convertible_to<From, To> && !uses_nonqualification_pointer_conversion<std::decay_t<From>, std::decay_t<To>>;

template <class T, class U, class V>
concept pair_like_convertible_from = !range<T> && !std::is_reference_v<T> && pair_like_std<T> && std::constructible_from<T, U, V> &&
                                     convertible_to_non_slicing<U, std::tuple_element_t<0, T>> &&
                                     std::convertible_to<V, std::tuple_element_t<1, T>>;

// begin_, end_ and, iff StoreSize, size_. No empty size member: MSVC 14.29 value-initializes an
// empty [[msvc::no_unique_address]] member by storing a zero byte over the member at its address
// (a compiler bug: [intro.object]/9; fixed by 14.44).
// DEVIATION: movable-box, so subrange stays a view over UE's debug array iterator, which is not
// assignable ([range.subrange.general] stores a bare I and S).
template <class I, class S, class Size, bool StoreSize> struct subrange_storage {
    ZR_NO_UNIQUE_ADDRESS movable_box<I> begin_;
    ZR_NO_UNIQUE_ADDRESS movable_box<S> end_;
};

template <class I, class S, class Size> struct subrange_storage<I, S, Size, true> {
    ZR_NO_UNIQUE_ADDRESS movable_box<I> begin_;
    ZR_NO_UNIQUE_ADDRESS movable_box<S> end_;
    Size size_ = 0;
};

template <class I, class S> constexpr subrange_kind default_subrange_kind() {
    return std::sized_sentinel_for<S, I> ? subrange_kind::sized : subrange_kind::unsized;
}

} // namespace detail

// DEVIATION: input_or_output_iterator and sentinel_for are the relaxed zr concepts
// (iterator/concepts.h).
template <input_or_output_iterator I, sentinel_for<I> S = I, subrange_kind K = detail::default_subrange_kind<I, S>()>
    requires(K == subrange_kind::sized || !std::sized_sentinel_for<S, I>)
class subrange : public view_interface<subrange<I, S, K>> {
private:
    using size_type = detail::make_unsigned_like_t<iter_difference_t<I>>;

    static constexpr bool StoreSize = K == subrange_kind::sized && !std::sized_sentinel_for<S, I>;

    detail::subrange_storage<I, S, size_type, StoreSize> m_;

public:
    subrange()
        requires std::default_initializable<I>
    = default;

    template <detail::convertible_to_non_slicing<I> I2>
    constexpr subrange(I2 i, S s)
        requires(!StoreSize)
        : m_{movable_box<I>(I(std::move(i))), movable_box<S>(std::move(s))} {}

    template <detail::convertible_to_non_slicing<I> I2>
    constexpr subrange(I2 i, S s, [[maybe_unused]] size_type n)
        requires(K == subrange_kind::sized)
        : m_{movable_box<I>(I(std::move(i))), movable_box<S>(std::move(s))} {
        if constexpr (StoreSize) {
            m_.size_ = n;
        }
    }

    template <detail::different_from<subrange> R>
        requires borrowed_range<R> && detail::convertible_to_non_slicing<iterator_t<R>, I> && std::convertible_to<sentinel_t<R>, S>
    constexpr subrange(R&& r)
        requires(!StoreSize || sized_range<R>)
        : subrange(construct_from(r)) {}

    template <borrowed_range R>
        requires detail::convertible_to_non_slicing<iterator_t<R>, I> && std::convertible_to<sentinel_t<R>, S>
    constexpr subrange(R&& r, size_type n)
        requires(K == subrange_kind::sized)
        : subrange{zr::ranges::begin(r), zr::ranges::end(r), n} {}

    template <detail::different_from<subrange> PairLike>
        requires detail::pair_like_convertible_from<PairLike, const I&, const S&>
    constexpr operator PairLike() const {
        return PairLike(*m_.begin_, *m_.end_);
    }

    // DEVIATION: copy_constructible<I>, not copyable<I>: UE's debug array iterator is copy-
    // constructible but not assignable, and begin() must not move from it.
    constexpr I begin() const
        requires std::copy_constructible<I>
    {
        return *m_.begin_;
    }
    constexpr I begin()
        requires(!std::copy_constructible<I>)
    {
        return std::move(*m_.begin_);
    }
    constexpr S end() const { return *m_.end_; }

    constexpr bool empty() const { return detail::iter_equal(*m_.begin_, *m_.end_); }

    constexpr size_type size() const
        requires(K == subrange_kind::sized)
    {
        if constexpr (StoreSize) {
            return m_.size_;
        } else {
            return detail::to_unsigned_like(*m_.end_ - *m_.begin_);
        }
    }

    [[nodiscard]] constexpr subrange next(iter_difference_t<I> n = 1) const&
        requires std::forward_iterator<I>
    {
        auto tmp = *this;
        tmp.advance(n);
        return tmp;
    }

    [[nodiscard]] constexpr subrange next(iter_difference_t<I> n = 1) && {
        advance(n);
        return std::move(*this);
    }

    [[nodiscard]] constexpr subrange prev(iter_difference_t<I> n = 1) const
        requires std::bidirectional_iterator<I>
    {
        auto tmp = *this;
        tmp.advance(-n);
        return tmp;
    }

    constexpr subrange& advance(iter_difference_t<I> n) {
        if constexpr (std::bidirectional_iterator<I>) {
            if (n < 0) {
                detail::advance(*m_.begin_, n);
                if constexpr (StoreSize) {
                    m_.size_ += detail::to_unsigned_like(-n);
                }
                return *this;
            }
        }
        iter_difference_t<I> d = n;
        if constexpr (std::input_or_output_iterator<I> && std::sentinel_for<S, I>) {
            d = n - detail::advance(*m_.begin_, n, *m_.end_);
        } else {
            // DEVIATION: Unreal iterators are not std::input_or_output_iterator.
            for (; d > 0 && !detail::iter_equal(*m_.begin_, *m_.end_); --d) {
                ++*m_.begin_;
            }
            d = n - d;
        }
        if constexpr (StoreSize) {
            m_.size_ -= detail::to_unsigned_like(d);
        }
        return *this;
    }

private:
    template <class R> static constexpr subrange construct_from(R& r) {
        if constexpr (StoreSize) {
            return subrange(r, static_cast<size_type>(zr::ranges::size(r)));
        } else {
            return subrange(zr::ranges::begin(r), zr::ranges::end(r));
        }
    }
};

template <input_or_output_iterator I, sentinel_for<I> S> subrange(I, S) -> subrange<I, S>;

template <input_or_output_iterator I, sentinel_for<I> S>
subrange(I, S, detail::make_unsigned_like_t<iter_difference_t<I>>) -> subrange<I, S, subrange_kind::sized>;

template <borrowed_range R>
subrange(R&&) -> subrange<iterator_t<R>, sentinel_t<R>,
                          (sized_range<R> || std::sized_sentinel_for<sentinel_t<R>, iterator_t<R>>) ? subrange_kind::sized
                                                                                                    : subrange_kind::unsized>;

template <borrowed_range R>
subrange(R&&, detail::make_unsigned_like_t<range_difference_t<R>>) -> subrange<iterator_t<R>, sentinel_t<R>, subrange_kind::sized>;

// [range.subrange.access] get
// DEVIATION: copy_constructible<I>, as subrange::begin() const.
template <std::size_t N, class I, class S, subrange_kind K>
    requires((N == 0 && std::copy_constructible<I>) || N == 1)
constexpr auto get(const subrange<I, S, K>& r) {
    if constexpr (N == 0) {
        return r.begin();
    } else {
        return r.end();
    }
}

template <std::size_t N, class I, class S, subrange_kind K>
    requires(N < 2)
constexpr auto get(subrange<I, S, K>&& r) {
    if constexpr (N == 0) {
        return r.begin();
    } else {
        return r.end();
    }
}

namespace detail {

// Category of an iterator pair.
// DEVIATION: a non-std iterator carries no category: forward if copy-constructible and
// (relaxed) equality-comparable, as every Unreal container iterator is multi-pass; else input.
// ue_range_traits.h classifies TArray's ranged-for iterator as in Shipping.
template <class I> constexpr range_category subrange_category() {
    if constexpr (std::contiguous_iterator<I>) {
        return range_category::contiguous;
    } else if constexpr (std::random_access_iterator<I>) {
        return range_category::random_access;
    } else if constexpr (std::bidirectional_iterator<I>) {
        return range_category::bidirectional;
    } else if constexpr (std::forward_iterator<I> ||
                         (!std::input_or_output_iterator<I> && std::copy_constructible<I> && relaxed_equality_comparable<I>)) {
        return range_category::forward;
    } else {
        return range_category::input;
    }
}

} // namespace detail

template <class I, class S, subrange_kind K> struct range_traits_impl<subrange<I, S, K>> {
    static constexpr range_category category = detail::subrange_category<I>();
};

ZR_END_NAMESPACE_RANGES

namespace std {

// [range.subrange.general] tuple protocol.
template <class I, class S, zr::ranges::subrange_kind K>
struct tuple_size<zr::ranges::subrange<I, S, K>> : integral_constant<size_t, 2> {};
template <class I, class S, zr::ranges::subrange_kind K> struct tuple_element<0, zr::ranges::subrange<I, S, K>> {
    using type = I;
};
template <class I, class S, zr::ranges::subrange_kind K> struct tuple_element<1, zr::ranges::subrange<I, S, K>> {
    using type = S;
};
template <class I, class S, zr::ranges::subrange_kind K> struct tuple_element<0, const zr::ranges::subrange<I, S, K>> {
    using type = I;
};
template <class I, class S, zr::ranges::subrange_kind K> struct tuple_element<1, const zr::ranges::subrange<I, S, K>> {
    using type = S;
};

namespace ranges {

template <class I, class S, zr::ranges::subrange_kind K> inline constexpr bool enable_borrowed_range<zr::ranges::subrange<I, S, K>> = true;

} // namespace ranges

} // namespace std
