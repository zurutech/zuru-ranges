// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/ranges/adaptor_helpers.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"
#include "zr/ranges/views/all.h"

#include <algorithm>
#include <compare>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

/**
 * @file enumerate_view.h
 *
 * @brief [range.enumerate] enumerate_view, views::enumerate.
 *
 * range_traits: the base's category capped at random_access; element_at(v, n) is
 * tuple(n, element_at(base, n)). The draft's defaulted postfix operators ([over.inc.default],
 * C++26) are written out.
 */

ZR_BEGIN_NAMESPACE_RANGES

template <view V>
    requires detail::range_with_movable_references<V>
class enumerate_view : public view_interface<enumerate_view<V>> {
private:
    V base_ = V();

    template <bool Const> class iterator;
    template <bool Const> class sentinel;

    template <class T> friend struct range_traits_impl;

public:
    constexpr enumerate_view()
        requires std::default_initializable<V>
    = default;
    constexpr explicit enumerate_view(V base)
        : base_(std::move(base)) {}

    constexpr auto begin()
        requires(!detail::simple_view<V>)
    {
        return iterator<false>(zr::ranges::begin(base_), 0);
    }
    constexpr auto begin() const
        requires detail::range_with_movable_references<const V>
    {
        return iterator<true>(zr::ranges::begin(base_), 0);
    }

    constexpr auto end()
        requires(!detail::simple_view<V>)
    {
        if constexpr (forward_range<V> && common_range<V> && sized_range<V>) {
            return iterator<false>(zr::ranges::end(base_), static_cast<range_difference_t<V>>(zr::ranges::size(base_)));
        } else {
            return sentinel<false>(zr::ranges::end(base_));
        }
    }
    constexpr auto end() const
        requires detail::range_with_movable_references<const V>
    {
        if constexpr (forward_range<const V> && common_range<const V> && sized_range<const V>) {
            return iterator<true>(zr::ranges::end(base_), static_cast<range_difference_t<const V>>(zr::ranges::size(base_)));
        } else {
            return sentinel<true>(zr::ranges::end(base_));
        }
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

    constexpr V base() const&
        requires std::copy_constructible<V>
    {
        return base_;
    }
    constexpr V base() && { return std::move(base_); }
};

template <class R> enumerate_view(R&&) -> enumerate_view<views::all_t<R>>;

template <view V>
    requires detail::range_with_movable_references<V>
template <bool Const>
class enumerate_view<V>::iterator {
private:
    using Base = detail::maybe_const<Const, V>;

    friend enumerate_view;
    friend iterator<!Const>;

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
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = decltype(concept_tag());
    using difference_type = range_difference_t<Base>;
    using value_type = std::tuple<difference_type, range_value_t<Base>>;

private:
    using reference_type = std::tuple<difference_type, range_reference_t<Base>>;

    iterator_t<Base> current_ = iterator_t<Base>();
    difference_type pos_ = 0;

    constexpr explicit iterator(iterator_t<Base> current, difference_type pos)
        : current_(std::move(current))
        , pos_(pos) {}

public:
    iterator()
        requires std::default_initializable<iterator_t<Base>>
    = default;

    constexpr iterator(iterator<!Const> i)
        requires Const && std::convertible_to<iterator_t<V>, iterator_t<Base>>
        : current_(std::move(i.current_))
        , pos_(i.pos_) {}

    constexpr const iterator_t<Base>& base() const& noexcept { return current_; }
    constexpr iterator_t<Base> base() && { return std::move(current_); }

    constexpr difference_type index() const noexcept { return pos_; }

    constexpr auto operator*() const { return reference_type(pos_, detail::deref(current_)); }

    constexpr iterator& operator++() {
        ++current_;
        ++pos_;
        return *this;
    }
    constexpr void operator++(int) { ++*this; }
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
        --pos_;
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
        pos_ += n;
        return *this;
    }
    constexpr iterator& operator-=(difference_type n)
        requires detail::base_random_access<Base>
    {
        current_ -= n;
        pos_ -= n;
        return *this;
    }

    constexpr auto operator[](difference_type n) const
        requires detail::base_random_access<Base>
    {
        return reference_type(pos_ + n, current_[n]);
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y) noexcept { return x.pos_ == y.pos_; }
    friend constexpr std::strong_ordering operator<=>(const iterator& x, const iterator& y) noexcept { return x.pos_ <=> y.pos_; }

    friend constexpr iterator operator+(const iterator& x, difference_type y)
        requires detail::base_random_access<Base>
    {
        auto temp = x;
        temp += y;
        return temp;
    }
    friend constexpr iterator operator+(difference_type x, const iterator& y)
        requires detail::base_random_access<Base>
    {
        return y + x;
    }
    friend constexpr iterator operator-(const iterator& x, difference_type y)
        requires detail::base_random_access<Base>
    {
        auto temp = x;
        temp -= y;
        return temp;
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y) noexcept { return x.pos_ - y.pos_; }

    friend constexpr auto iter_move(const iterator& i) noexcept(noexcept(detail::relaxed_iter_move(i.current_)) &&
                                                                std::is_nothrow_move_constructible_v<range_rvalue_reference_t<Base>>) {
        return std::tuple<difference_type, range_rvalue_reference_t<Base>>(i.pos_, detail::relaxed_iter_move(i.current_));
    }
};

template <view V>
    requires detail::range_with_movable_references<V>
template <bool Const>
class enumerate_view<V>::sentinel {
private:
    using Base = detail::maybe_const<Const, V>;

    friend enumerate_view;
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

template <class V> struct range_traits_impl<enumerate_view<V>> {
    static constexpr range_category category = std::min(detail::category_of<V>(), range_category::random_access);

    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access)
    static constexpr auto element_at(enumerate_view<V>& v, Index n) {
        using D = range_difference_t<V>;
        return std::tuple<D, range_reference_t<V>>(static_cast<D>(n), zr::ranges::element_at(v.base_, n));
    }
    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && requires(const V& base) { zr::ranges::element_at(base, 0); }
    static constexpr auto element_at(const enumerate_view<V>& v, Index n) {
        using D = range_difference_t<const V>;
        return std::tuple<D, range_reference_t<const V>>(static_cast<D>(n), zr::ranges::element_at(v.base_, n));
    }
};

template <class V> struct range_traits_impl<const enumerate_view<V>> : range_traits_impl<enumerate_view<V>> {
    static constexpr range_category category = std::min(detail::category_of<const V>(), range_category::random_access);
};

ZR_END_NAMESPACE_RANGES

namespace std::ranges {

template <class V> inline constexpr bool enable_borrowed_range<zr::ranges::enumerate_view<V>> = enable_borrowed_range<V>;

} // namespace std::ranges

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// [range.enumerate.overview]
struct enumerate_fn : range_adaptor_closure<enumerate_fn> {
    template <class E>
        requires requires { enumerate_view<views::all_t<E>>(std::declval<E>()); }
    constexpr auto operator()(E&& e) const noexcept(noexcept(enumerate_view<views::all_t<E>>(std::forward<E>(e)))) {
        return enumerate_view<views::all_t<E>>(std::forward<E>(e));
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::enumerate_fn enumerate{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
