// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/ranges/adaptor_helpers.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"
#include "zr/ranges/views/all.h"
#include "zr/ranges/views/subrange.h"

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

/**
 * @file elements_view.h
 *
 * @brief [range.elements] elements_view, keys_view, values_view, views::elements, views::keys,
 *        views::values.
 *
 * DEVIATION: has-tuple-element accepts, besides tuple-like ([tuple.like], a closed set), any type
 * with the tuple protocol of structured bindings ([dcl.struct.bind]/4): tuple_size<T>::value and
 * get<N> as a member or found by argument-dependent lookup (Unreal's TPair/TTuple). std::get<N>
 * is detail::tuple_get<N>, which reduces to std::get<N> for the tuple-like types.
 *
 * range_traits: the base's category capped at random_access; element_at(v, n) is
 * get-element(element_at(base, n)). The draft's defaulted postfix operators ([over.inc.default],
 * C++26) are written out.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

template <class T, std::size_t N>
concept std_tuple_get = requires(T&& t) { std::get<N>(std::forward<T>(t)); };

template <class T, std::size_t N>
concept member_tuple_get = requires(T&& t) { std::forward<T>(t).template get<N>(); };

// std::get<N> where valid (the tuple-like types), else [dcl.struct.bind]/4: member get<N>(), else
// get<N>(t) by argument-dependent lookup. A member get is used when the call is valid, not
// whenever a member template named get exists.
template <std::size_t N, class T> constexpr decltype(auto) tuple_get(T&& t) {
    if constexpr (std_tuple_get<T, N>) {
        return std::get<N>(std::forward<T>(t));
    } else if constexpr (member_tuple_get<T, N>) {
        return std::forward<T>(t).template get<N>();
    } else {
        return get<N>(std::forward<T>(t));
    }
}

template <class T>
concept tuple_protocol = requires { std::tuple_size<T>::value; };

// [range.elements.view] has-tuple-element (see file note).
template <class T, std::size_t N>
concept has_tuple_element = tuple_protocol<T> && N < std::tuple_size_v<T> && requires(T t) {
    { detail::tuple_get<N>(t) } -> std::convertible_to<const std::tuple_element_t<N, T>&>;
};

// [range.elements.view] returnable-element
template <class T, std::size_t N>
concept returnable_element = std::is_reference_v<T> || std::move_constructible<std::tuple_element_t<N, T>>;

template <class V, std::size_t N>
concept elements_viewable = has_tuple_element<range_value_t<V>, N> && has_tuple_element<std::remove_reference_t<range_reference_t<V>>, N> &&
                            returnable_element<range_reference_t<V>, N>;

// [range.elements.iterator] get-element(*i), from the reference `r` of Base.
template <std::size_t N, class Base, class R> constexpr decltype(auto) get_element(R&& r) {
    if constexpr (std::is_reference_v<range_reference_t<Base>>) {
        return detail::tuple_get<N>(std::forward<R>(r));
    } else {
        using E = std::remove_cv_t<std::tuple_element_t<N, range_reference_t<Base>>>;
        return static_cast<E>(detail::tuple_get<N>(std::forward<R>(r)));
    }
}

// Named: Clang < 16 substitutes a member's requires-clause eagerly.
template <class V, std::size_t N>
concept elements_const_iterable = range<const V> && elements_viewable<const V, N>;

} // namespace detail

template <input_range V, std::size_t N>
    requires view<V> && detail::elements_viewable<V, N>
class elements_view : public view_interface<elements_view<V, N>> {
private:
    template <bool Const> class iterator;
    template <bool Const> class sentinel;

    template <class T> friend struct range_traits_impl;

    V base_ = V();

public:
    elements_view()
        requires std::default_initializable<V>
    = default;

    constexpr explicit elements_view(V base)
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
        return iterator<false>(zr::ranges::begin(base_));
    }

    constexpr auto begin() const
        requires range<const V>
    {
        return iterator<true>(zr::ranges::begin(base_));
    }

    constexpr auto end()
        requires(!detail::simple_view<V> && !common_range<V>)
    {
        return sentinel<false>{zr::ranges::end(base_)};
    }

    constexpr auto end()
        requires(!detail::simple_view<V> && common_range<V>)
    {
        return iterator<false>{zr::ranges::end(base_)};
    }

    constexpr auto end() const
        requires range<const V>
    {
        return sentinel<true>{zr::ranges::end(base_)};
    }

    constexpr auto end() const
        requires common_range<const V>
    {
        return iterator<true>{zr::ranges::end(base_)};
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

template <class R> using keys_view = elements_view<R, 0>;
template <class R> using values_view = elements_view<R, 1>;

namespace detail {

template <bool Const, class V>
inline constexpr bool elements_has_category = forward_range<maybe_const<Const, V>> && has_cpp17_category<iterator_t<maybe_const<Const, V>>>;

template <bool Const, class V, std::size_t N> constexpr auto elements_category() {
    if constexpr (elements_has_category<Const, V>) {
        using Base = maybe_const<Const, V>;
        using C = cpp17_category_t<iterator_t<Base>>;
        if constexpr (!std::is_lvalue_reference_v<decltype(detail::tuple_get<N>(*std::declval<iterator_t<Base>&>()))>) {
            return std::input_iterator_tag{};
        } else if constexpr (std::derived_from<C, std::random_access_iterator_tag>) {
            return std::random_access_iterator_tag{};
        } else {
            return C{};
        }
    }
}

} // namespace detail

template <input_range V, std::size_t N>
    requires view<V> && detail::elements_viewable<V, N>
template <bool Const>
class elements_view<V, N>::iterator
    : public detail::iterator_category_base<detail::elements_has_category<Const, V>, decltype(detail::elements_category<Const, V, N>())> {
private:
    using Base = detail::maybe_const<Const, V>;

    friend elements_view;
    friend iterator<!Const>;

    iterator_t<Base> current_ = iterator_t<Base>();

    static constexpr decltype(auto) get_element(const iterator_t<Base>& i) { return detail::get_element<N, Base>(detail::deref(i)); }

    constexpr explicit iterator(iterator_t<Base> current)
        : current_(std::move(current)) {}

    static constexpr auto concept_tag() {
        if constexpr (detail::base_random_access<Base>) {
            return std::random_access_iterator_tag{};
        } else if constexpr (detail::base_decrementable<Base>) {
            return std::bidirectional_iterator_tag{};
        } else if constexpr (forward_range<Base>) {
            return std::forward_iterator_tag{};
        } else {
            return std::input_iterator_tag{};
        }
    }

public:
    using iterator_concept = decltype(concept_tag());
    using value_type = std::remove_cvref_t<std::tuple_element_t<N, range_value_t<Base>>>;
    using difference_type = range_difference_t<Base>;

    iterator()
        requires std::default_initializable<iterator_t<Base>>
    = default;

    constexpr iterator(iterator<!Const> i)
        requires Const && std::convertible_to<iterator_t<V>, iterator_t<Base>>
        : current_(std::move(i.current_)) {}

    constexpr const iterator_t<Base>& base() const& noexcept { return current_; }
    constexpr iterator_t<Base> base() && { return std::move(current_); }

    constexpr decltype(auto) operator*() const { return get_element(current_); }

    constexpr iterator& operator++() {
        ++current_;
        return *this;
    }
    constexpr void operator++(int) { ++current_; }
    constexpr iterator operator++(int)
        requires forward_range<Base>
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr iterator& operator--()
        requires detail::base_decrementable<Base>
    {
        --current_;
        return *this;
    }
    constexpr iterator operator--(int)
        requires detail::base_decrementable<Base>
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    constexpr iterator& operator+=(difference_type n)
        requires detail::base_random_access<Base>
    {
        current_ += n;
        return *this;
    }
    constexpr iterator& operator-=(difference_type n)
        requires detail::base_random_access<Base>
    {
        current_ -= n;
        return *this;
    }

    constexpr decltype(auto) operator[](difference_type n) const
        requires detail::base_random_access<Base>
    {
        return get_element(current_ + n);
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y)
        requires detail::relaxed_equality_comparable<iterator_t<Base>>
    {
        return detail::iter_equal(x.current_, y.current_);
    }

    friend constexpr bool operator<(const iterator& x, const iterator& y)
        requires detail::base_random_access<Base>
    {
        return x.current_ < y.current_;
    }
    friend constexpr bool operator>(const iterator& x, const iterator& y)
        requires detail::base_random_access<Base>
    {
        return y < x;
    }
    friend constexpr bool operator<=(const iterator& x, const iterator& y)
        requires detail::base_random_access<Base>
    {
        return !(y < x);
    }
    friend constexpr bool operator>=(const iterator& x, const iterator& y)
        requires detail::base_random_access<Base>
    {
        return !(x < y);
    }
    friend constexpr auto operator<=>(const iterator& x, const iterator& y)
        requires detail::base_random_access<Base> && std::three_way_comparable<iterator_t<Base>>
    {
        return x.current_ <=> y.current_;
    }

    friend constexpr iterator operator+(const iterator& x, difference_type y)
        requires detail::base_random_access<Base>
    {
        return iterator{x} += y;
    }
    friend constexpr iterator operator+(difference_type x, const iterator& y)
        requires detail::base_random_access<Base>
    {
        return y + x;
    }
    friend constexpr iterator operator-(const iterator& x, difference_type y)
        requires detail::base_random_access<Base>
    {
        return iterator{x} -= y;
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
        requires std::sized_sentinel_for<iterator_t<Base>, iterator_t<Base>>
    {
        return x.current_ - y.current_;
    }
};

template <input_range V, std::size_t N>
    requires view<V> && detail::elements_viewable<V, N>
template <bool Const>
class elements_view<V, N>::sentinel {
private:
    using Base = detail::maybe_const<Const, V>;

    friend elements_view;
    friend sentinel<!Const>;

    sentinel_t<Base> end_ = sentinel_t<Base>();

    constexpr explicit sentinel(sentinel_t<Base> end)
        : end_(std::move(end)) {}

public:
    sentinel() = default;

    constexpr sentinel(sentinel<!Const> other)
        requires Const && std::convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(other.end_)) {}

    constexpr sentinel_t<Base> base() const { return end_; }

    template <bool OtherConst>
        requires sentinel_for<sentinel_t<Base>, iterator_t<detail::maybe_const<OtherConst, V>>>
    friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y) {
        return detail::iter_equal(x.base(), y.end_);
    }

    template <bool OtherConst>
        requires std::sized_sentinel_for<sentinel_t<Base>, iterator_t<detail::maybe_const<OtherConst, V>>>
    friend constexpr range_difference_t<detail::maybe_const<OtherConst, V>> operator-(const iterator<OtherConst>& x, const sentinel& y) {
        return x.base() - y.end_;
    }

    template <bool OtherConst>
        requires std::sized_sentinel_for<sentinel_t<Base>, iterator_t<detail::maybe_const<OtherConst, V>>>
    friend constexpr range_difference_t<detail::maybe_const<OtherConst, V>> operator-(const sentinel& x, const iterator<OtherConst>& y) {
        return x.end_ - y.base();
    }
};

template <class V, std::size_t N> struct range_traits_impl<elements_view<V, N>> {
    static constexpr range_category category = std::min(detail::category_of<V>(), range_category::random_access);

    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access)
    static constexpr decltype(auto) element_at(elements_view<V, N>& v, Index n) {
        return detail::get_element<N, V>(zr::ranges::element_at(v.base_, n));
    }
    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && detail::elements_const_iterable<V, N> &&
                requires(const V& base) { zr::ranges::element_at(base, 0); }
    static constexpr decltype(auto) element_at(const elements_view<V, N>& v, Index n) {
        return detail::get_element<N, const V>(zr::ranges::element_at(v.base_, n));
    }
};

// const elements_view: maybe-const<true, V> ([range.elements.iterator]).
template <class V, std::size_t N> struct range_traits_impl<const elements_view<V, N>> : range_traits_impl<elements_view<V, N>> {
    static constexpr range_category category = std::min(detail::category_of<const V>(), range_category::random_access);
};

namespace detail {

// [range.elements.overview]
template <std::size_t N> struct elements_fn : range_adaptor_closure<elements_fn<N>> {
    template <viewable_range E>
        requires requires { elements_view<views::all_t<E>, N>{std::declval<E>()}; }
    constexpr auto operator()(E&& e) const noexcept(noexcept(elements_view<views::all_t<E>, N>{std::forward<E>(e)})) {
        return elements_view<views::all_t<E>, N>{std::forward<E>(e)};
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

namespace std::ranges {

// [ranges.syn]: enable_borrowed_range<elements_view<T, N>> = enable_borrowed_range<T>.
template <class T, size_t N> inline constexpr bool enable_borrowed_range<zr::ranges::elements_view<T, N>> = enable_borrowed_range<T>;

} // namespace std::ranges

ZR_BEGIN_NAMESPACE_VIEWS

template <std::size_t N> inline constexpr detail::elements_fn<N> elements{};
inline constexpr auto keys = elements<0>;
inline constexpr auto values = elements<1>;

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
