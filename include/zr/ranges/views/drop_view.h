// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/iterator/operations.h"
#include "zr/ranges/adaptor_helpers.h"
#include "zr/ranges/non_propagating_cache.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"
#include "zr/ranges/views/all.h"
#include "zr/ranges/views/iota_view.h"
#include "zr/ranges/views/subrange.h"
#include "zr/ranges/views/take_view.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>

/**
 * @file drop_view.h
 *
 * @brief [range.drop] drop_view, views::drop.
 *
 * DEVIATION: ranges::next(ranges::begin(base_), count_, ranges::end(base_)) increments at most
 * count_ times when the base iterator is not a std::input_or_output_iterator (Unreal); the const
 * begin() of a random-access sized V without iterator arithmetic is then O(count_), not O(1).
 *
 * DEVIATION: views::drop slices span, basic_string_view, iota_view and subrange only under
 * detail::sliceable (take_view.h), as views::take.
 *
 * range_traits: the base's category (see drop_category); element_at(v, n) is
 * element_at(base, min(count_, size) + n), data(v) is data(base) + min(count_, size).
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

template <class T> inline constexpr bool subrange_stores_size = false;
template <class I, class S, subrange_kind K>
inline constexpr bool subrange_stores_size<subrange<I, S, K>> = K == subrange_kind::sized && !std::sized_sentinel_for<S, I>;

// Named: Clang < 16 substitutes a member's requires-clause eagerly.
template <class V>
concept drop_const_begin = random_access_range<const V> && sized_range<const V>;

// [range.drop.view] begin() "caches the result" when drop_view models forward_range, for the
// amortized constant time [range.range] requires; not when ranges::next is already O(1) (a base
// iterator with arithmetic and a sized sentinel), as libstdc++ and the MSVC STL.
template <class V>
concept drop_caches_begin = forward_range<V> && !(base_random_access<V> && std::sized_sentinel_for<sentinel_t<V>, iterator_t<V>>);

} // namespace detail

template <view V> class drop_view : public view_interface<drop_view<V>> {
private:
    template <class T> friend struct range_traits_impl;

    V base_ = V();
    range_difference_t<V> count_ = 0;

    ZR_NO_UNIQUE_ADDRESS detail::cache_if<detail::drop_caches_begin<V>, iterator_t<V>> begin_;

    template <class Base> static constexpr iterator_t<Base> next_(Base& base, range_difference_t<Base> n) {
        auto it = zr::ranges::begin(base);
        if constexpr (std::input_or_output_iterator<iterator_t<Base>> && std::sentinel_for<sentinel_t<Base>, iterator_t<Base>>) {
            detail::advance(it, n, zr::ranges::end(base));
        } else {
            for (const auto last = zr::ranges::end(base); n > 0 && !detail::iter_equal(it, last); --n) {
                ++it;
            }
        }
        return it;
    }

    template <class Base> static constexpr range_difference_t<Base> offset_(Base& base, range_difference_t<V> n) {
        return std::min(static_cast<range_difference_t<Base>>(n), static_cast<range_difference_t<Base>>(zr::ranges::size(base)));
    }

public:
    drop_view()
        requires std::default_initializable<V>
    = default;

    // Parameter named apart from ranges::count: GCC -Wshadow.
    constexpr explicit drop_view(V base, range_difference_t<V> n)
        : base_(std::move(base))
        , count_(n) {}

    constexpr V base() const&
        requires std::copy_constructible<V>
    {
        return base_;
    }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin()
        requires(!(detail::simple_view<V> && detail::drop_const_begin<V>))
    {
        if constexpr (detail::drop_caches_begin<V>) {
            if (!begin_) {
                begin_.emplace(next_(base_, count_));
            }
            return *begin_;
        } else {
            return next_(base_, count_);
        }
    }

    constexpr auto begin() const
        requires detail::drop_const_begin<V>
    {
        return next_(base_, static_cast<range_difference_t<const V>>(count_));
    }

    constexpr auto end()
        requires(!detail::simple_view<V>)
    {
        return zr::ranges::end(base_);
    }
    constexpr auto end() const
        requires range<const V>
    {
        return zr::ranges::end(base_);
    }

    constexpr auto size()
        requires sized_range<V>
    {
        const auto s = zr::ranges::size(base_);
        const auto c = static_cast<decltype(s)>(count_);
        return s < c ? 0 : s - c;
    }
    constexpr auto size() const
        requires sized_range<const V>
    {
        const auto s = zr::ranges::size(base_);
        const auto c = static_cast<decltype(s)>(count_);
        return s < c ? 0 : s - c;
    }

    constexpr auto reserve_hint()
        requires approximately_sized_range<V>
    {
        const auto s = static_cast<range_difference_t<V>>(zr::ranges::reserve_hint(base_));
        return detail::to_unsigned_like(s < count_ ? 0 : s - count_);
    }
    constexpr auto reserve_hint() const
        requires approximately_sized_range<const V>
    {
        const auto s = static_cast<range_difference_t<const V>>(zr::ranges::reserve_hint(base_));
        const auto c = static_cast<range_difference_t<const V>>(count_);
        return detail::to_unsigned_like(s < c ? 0 : s - c);
    }
};

template <class R> drop_view(R&&, range_difference_t<R>) -> drop_view<views::all_t<R>>;

namespace detail {

// The base's category; bidirectional at most for a random-access V that is neither sized nor
// iterator-arithmetic, whose element_at needs the size.
template <class V> constexpr range_category drop_category() {
    if constexpr (sized_range<V> || base_random_access<V>) {
        return category_of<V>();
    } else {
        return std::min(category_of<V>(), range_category::bidirectional);
    }
}

} // namespace detail

template <class V> struct range_traits_impl<drop_view<V>> {
    static constexpr range_category category = detail::drop_category<V>();

    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && sized_range<V>
    static constexpr decltype(auto) element_at(drop_view<V>& v, Index n) {
        return zr::ranges::element_at(v.base_, drop_view<V>::offset_(v.base_, v.count_) + static_cast<range_difference_t<V>>(n));
    }
    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && sized_range<const V> && detail::const_element_at<V>
    static constexpr decltype(auto) element_at(const drop_view<V>& v, Index n) {
        return zr::ranges::element_at(v.base_, drop_view<V>::offset_(v.base_, v.count_) + static_cast<range_difference_t<const V>>(n));
    }

    static constexpr auto data(drop_view<V>& v)
        requires contiguous_range<V> && sized_range<V>
    {
        return zr::ranges::data(v.base_) + drop_view<V>::offset_(v.base_, v.count_);
    }
    static constexpr auto data(const drop_view<V>& v)
        requires contiguous_range<const V> && sized_range<const V>
    {
        return zr::ranges::data(v.base_) + drop_view<V>::offset_(v.base_, v.count_);
    }
};

template <class V> struct range_traits_impl<const drop_view<V>> : range_traits_impl<drop_view<V>> {
    static constexpr range_category category = detail::drop_category<const V>();
};

namespace detail {

// [range.drop.overview]
template <class T>
concept drop_sliceable =
    (is_span<T> || is_basic_string_view<T> || is_iota_view<T> || (is_subrange<T> && !subrange_stores_size<T>)) && sliceable<T>;

// U: span<element_type> for a span, else T.
template <class T> struct drop_slice {
    using type = T;
};
template <class T, std::size_t Extent> struct drop_slice<std::span<T, Extent>> {
    using type = std::span<T>;
};

struct drop_fn {
private:
    // noexcept of the expression the call is equivalent to.
    template <class E, class F> static constexpr bool nothrow() {
        using T = std::remove_cvref_t<E>;
        using D = range_difference_t<E&>;
        if constexpr (is_empty_view<T>) {
            return noexcept(decay_copy(std::declval<E>()));
        } else if constexpr (is_optional<T> && view<T>) {
            return noexcept(static_cast<D>(std::declval<F>()) == D() ? decay_copy(std::declval<E>()) : T());
        } else if constexpr (drop_sliceable<T>) {
            return noexcept(typename drop_slice<T>::type(
                zr::ranges::begin(std::declval<E&>()) + std::min<D>(zr::ranges::distance(std::declval<E&>()), std::declval<F>()),
                zr::ranges::end(std::declval<E&>())));
        } else if constexpr (is_subrange<T> && sliceable<T>) {
            return noexcept(T(zr::ranges::begin(std::declval<E&>()) + std::min<D>(zr::ranges::distance(std::declval<E&>()), std::declval<F>()),
                              zr::ranges::end(std::declval<E&>()),
                              to_unsigned_like(zr::ranges::distance(std::declval<E&>()) -
                                               std::min<D>(zr::ranges::distance(std::declval<E&>()), std::declval<F>()))));
        } else {
            return noexcept(drop_view(std::declval<E>(), std::declval<F>()));
        }
    }

public:
    // Each branch's T is a view, so drop_view(E, F) is valid whenever a branch applies.
    template <class E, class F>
        requires range<E&> && std::convertible_to<F, range_difference_t<E&>> && requires { drop_view(std::declval<E>(), std::declval<F>()); }
    constexpr auto operator()(E&& e, F&& f) const noexcept(nothrow<E, F>()) {
        using T = std::remove_cvref_t<E>;
        using D = range_difference_t<E&>;
        if constexpr (is_empty_view<T>) {
            (void)f;
            return decay_copy(std::forward<E>(e));
        } else if constexpr (is_optional<T> && view<T>) {
            return static_cast<D>(std::forward<F>(f)) == D() ? decay_copy(std::forward<E>(e)) : ((void)e, T());
        } else if constexpr (drop_sliceable<T>) {
            const auto n = std::min<D>(zr::ranges::distance(e), std::forward<F>(f));
            return typename drop_slice<T>::type(zr::ranges::begin(e) + n, zr::ranges::end(e));
        } else if constexpr (is_subrange<T> && sliceable<T>) {
            const auto d = zr::ranges::distance(e);
            const auto n = std::min<D>(d, std::forward<F>(f));
            return T(zr::ranges::begin(e) + n, zr::ranges::end(e), to_unsigned_like(d - n));
        } else {
            return drop_view(std::forward<E>(e), std::forward<F>(f));
        }
    }

    template <class F>
        requires std::constructible_from<std::decay_t<F>, F>
    constexpr auto operator()(F&& f) const noexcept(std::is_nothrow_constructible_v<std::decay_t<F>, F>) {
        return make_bound_closure(*this, std::forward<F>(f));
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

namespace std::ranges {

// [ranges.syn]: enable_borrowed_range<drop_view<T>> = enable_borrowed_range<T>.
template <class T> inline constexpr bool enable_borrowed_range<zr::ranges::drop_view<T>> = enable_borrowed_range<T>;

} // namespace std::ranges

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::drop_fn drop{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
