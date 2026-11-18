// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/iterator/move_iterator.h"
#include "zr/ranges/adaptor_helpers.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"
#include "zr/ranges/views/all.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

/**
 * @file as_rvalue_view.h
 *
 * @brief [range.as.rvalue] as_rvalue_view, views::as_rvalue.
 *
 * DEVIATION: move_iterator and move_sentinel are detail::move_iterator_t and
 * detail::move_sentinel_t (zr/iterator/move_iterator.h).
 *
 * range_traits: the base's category capped at random_access; element_at(v, n) is
 * ranges::iter_move(ranges::begin(base) + n), computed from element_at(base, n) where
 * iterator_t<V> has no arithmetic (detail::rvalue_at).
 */

ZR_BEGIN_NAMESPACE_RANGES

template <view V>
    requires input_range<V>
class as_rvalue_view : public view_interface<as_rvalue_view<V>> {
private:
    template <class T> friend struct range_traits_impl;

    V base_ = V();

    template <class Base> static constexpr auto end_(Base& base) {
        if constexpr (common_range<Base>) {
            return detail::move_iterator_t<iterator_t<Base>>(zr::ranges::end(base));
        } else {
            return detail::move_sentinel_t<sentinel_t<Base>, iterator_t<Base>>(zr::ranges::end(base));
        }
    }

public:
    as_rvalue_view()
        requires std::default_initializable<V>
    = default;
    constexpr explicit as_rvalue_view(V base)
        : base_(std::move(base)) {}

    constexpr V base() const&
        requires std::copy_constructible<V>
    {
        return base_;
    }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin()
        requires(!detail::simple_view<V>)
    {
        return detail::move_iterator_t<iterator_t<V>>(zr::ranges::begin(base_));
    }
    constexpr auto begin() const
        requires range<const V>
    {
        return detail::move_iterator_t<iterator_t<const V>>(zr::ranges::begin(base_));
    }

    constexpr auto end()
        requires(!detail::simple_view<V>)
    {
        return end_(base_);
    }
    constexpr auto end() const
        requires range<const V>
    {
        return end_(base_);
    }

    constexpr auto size()
        requires sized_range<V>
    {
        return zr::ranges::size(base_);
    }
    constexpr auto size() const
        requires sized_range<const V>
    {
        return zr::ranges::size(base_);
    }

    constexpr auto reserve_hint()
        requires approximately_sized_range<V>
    {
        return zr::ranges::reserve_hint(base_);
    }
    constexpr auto reserve_hint() const
        requires approximately_sized_range<const V>
    {
        return zr::ranges::reserve_hint(base_);
    }
};

template <class R> as_rvalue_view(R&&) -> as_rvalue_view<views::all_t<R>>;

namespace detail {

// The RR that ranges::iter_move gives for an element whose reference is R, from the reference
// alone: R itself, T&& for a T&, and memberwise for a tuple (zip, enumerate and
// cartesian_product, whose iter_move maps the tuple's elements).
template <class RR, class R> struct rvalue_of {
    static constexpr bool valid = std::same_as<RR, R> || (std::is_lvalue_reference_v<R> && std::same_as<RR, std::remove_reference_t<R>&&>);

    static constexpr RR cast(R&& r) { return static_cast<RR>(std::forward<R>(r)); }
};

template <class... RRs, class... Rs>
    requires(sizeof...(RRs) == sizeof...(Rs)) && (!std::same_as<std::tuple<RRs...>, std::tuple<Rs...>>)
struct rvalue_of<std::tuple<RRs...>, std::tuple<Rs...>> {
    static constexpr bool valid = (rvalue_of<RRs, Rs>::valid && ...);

    static constexpr std::tuple<RRs...> cast(std::tuple<Rs...>&& r) {
        return std::apply([](Rs&&... rs) { return std::tuple<RRs...>(rvalue_of<RRs, Rs>::cast(std::forward<Rs>(rs))...); }, std::move(r));
    }
};

template <class R>
concept rvalue_by_position = requires(R& r) { zr::ranges::element_at(r, 0); } &&
                             rvalue_of<range_rvalue_reference_t<R>, decltype(zr::ranges::element_at(std::declval<R&>(), 0))>::valid;

template <class R>
concept rvalue_indexable = base_random_access<R> || rvalue_by_position<R>;

// ranges::iter_move(ranges::begin(r) + n), never through the arithmetic of an Unreal iterator.
template <class R, class Index> constexpr range_rvalue_reference_t<R> rvalue_at(R& r, Index n) {
    using RR = range_rvalue_reference_t<R>;
    if constexpr (base_random_access<R>) {
        return std::ranges::iter_move(zr::ranges::begin(r) + static_cast<range_difference_t<R>>(n));
    } else if constexpr (std::same_as<RR, decltype(zr::ranges::element_at(r, n))>) {
        return zr::ranges::element_at(r, n);
    } else {
        return rvalue_of<RR, decltype(zr::ranges::element_at(r, n))>::cast(zr::ranges::element_at(r, n));
    }
}

} // namespace detail

template <class V> struct range_traits_impl<as_rvalue_view<V>> {
    static constexpr range_category category = std::min(detail::category_of<V>(), range_category::random_access);

    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && detail::rvalue_indexable<V>
    static constexpr decltype(auto) element_at(as_rvalue_view<V>& v, Index n) {
        return detail::rvalue_at(v.base_, n);
    }
    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && range<const V> && detail::rvalue_indexable<const V>
    static constexpr decltype(auto) element_at(const as_rvalue_view<V>& v, Index n) {
        return detail::rvalue_at(v.base_, n);
    }
};

template <class V> struct range_traits_impl<const as_rvalue_view<V>> : range_traits_impl<as_rvalue_view<V>> {
    static constexpr range_category category = std::min(detail::category_of<const V>(), range_category::random_access);
};

ZR_END_NAMESPACE_RANGES

namespace std::ranges {

template <class V> inline constexpr bool enable_borrowed_range<zr::ranges::as_rvalue_view<V>> = enable_borrowed_range<V>;

} // namespace std::ranges

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

template <class E>
concept rvalue_range = input_range<E> && std::same_as<range_rvalue_reference_t<E>, range_reference_t<E>>;

// [range.as.rvalue.overview]. as_rvalue_view(E) is spelled as_rvalue_view<views::all_t<E>>(E):
// see reverse_fn.
struct as_rvalue_fn : range_adaptor_closure<as_rvalue_fn> {
private:
    template <class E> static constexpr bool nothrow() {
        if constexpr (rvalue_range<E>) {
            return noexcept(views::all(std::declval<E>()));
        } else {
            return noexcept(as_rvalue_view<views::all_t<E>>(std::declval<E>()));
        }
    }

public:
    template <class E>
        requires(rvalue_range<E> && requires { views::all(std::declval<E>()); }) ||
                requires { as_rvalue_view<views::all_t<E>>(std::declval<E>()); }
    constexpr auto operator()(E&& e) const noexcept(nothrow<E>()) {
        if constexpr (rvalue_range<E>) {
            return views::all(std::forward<E>(e));
        } else {
            return as_rvalue_view<views::all_t<E>>(std::forward<E>(e));
        }
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::as_rvalue_fn as_rvalue{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
