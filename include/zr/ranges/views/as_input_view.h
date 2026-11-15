// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/iterator/indirect_requirements.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"
#include "zr/ranges/views/all.h"

#include <concepts>
#include <iterator>
#include <utility>

/**
 * @file as_input_view.h
 *
 * @brief [range.as.input] as_input_view, views::as_input.
 *
 * range_traits: input, whatever the base's category.
 */

ZR_BEGIN_NAMESPACE_RANGES

template <input_range V>
    requires view<V>
class as_input_view : public view_interface<as_input_view<V>> {
private:
    V base_ = V();

    template <bool Const> class iterator;

public:
    as_input_view()
        requires std::default_initializable<V>
    = default;
    constexpr explicit as_input_view(V base)
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

template <class R> as_input_view(R&&) -> as_input_view<views::all_t<R>>;

// [range.as.input.iterator]
template <input_range V>
    requires view<V>
template <bool Const>
class as_input_view<V>::iterator {
private:
    using Base = detail::maybe_const<Const, V>;

    friend as_input_view;
    friend iterator<!Const>;

    iterator_t<Base> current_ = iterator_t<Base>();

    constexpr explicit iterator(iterator_t<Base> current)
        : current_(std::move(current)) {}

public:
    using difference_type = range_difference_t<Base>;
    using value_type = range_value_t<Base>;
    using iterator_concept = std::input_iterator_tag;

    iterator()
        requires std::default_initializable<iterator_t<Base>>
    = default;

    // Move-only. The assignment is deleted over a base iterator that is not assignable (UE's
    // debug array iterator).
    iterator(iterator&&) = default;
    iterator& operator=(iterator&&) = default;

    constexpr iterator(iterator<!Const> i)
        requires Const && std::convertible_to<iterator_t<V>, iterator_t<Base>>
        : current_(std::move(i.current_)) {}

    constexpr iterator_t<Base> base() && { return std::move(current_); }
    constexpr const iterator_t<Base>& base() const& noexcept { return current_; }

    constexpr decltype(auto) operator*() const { return detail::deref(current_); }

    constexpr iterator& operator++() {
        ++current_;
        return *this;
    }
    constexpr void operator++(int) { ++*this; }

    friend constexpr bool operator==(const iterator& x, const sentinel_t<Base>& y) { return detail::iter_equal(x.current_, y); }

    friend constexpr difference_type operator-(const sentinel_t<Base>& y, const iterator& x)
        requires std::sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>>
    {
        return y - x.current_;
    }
    friend constexpr difference_type operator-(const iterator& x, const sentinel_t<Base>& y)
        requires std::sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>>
    {
        return x.current_ - y;
    }

    friend constexpr range_rvalue_reference_t<Base> iter_move(const iterator& i) noexcept(noexcept(detail::relaxed_iter_move(i.current_))) {
        return detail::relaxed_iter_move(i.current_);
    }

    friend constexpr void iter_swap(const iterator& x, const iterator& y)
        noexcept(noexcept(detail::relaxed_iter_swap(x.current_, y.current_)))
        requires detail::indirectly_swappable<iterator_t<Base>>
    {
        detail::relaxed_iter_swap(x.current_, y.current_);
    }
};

template <class V> struct range_traits_impl<as_input_view<V>> {
    static constexpr range_category category = range_category::input;
};

ZR_END_NAMESPACE_RANGES

namespace std::ranges {

template <class V> inline constexpr bool enable_borrowed_range<zr::ranges::as_input_view<V>> = enable_borrowed_range<V>;

} // namespace std::ranges

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

template <class E>
concept input_only_range = input_range<E> && !common_range<E> && !forward_range<E>;

// [range.as.input.overview]. as_input_view(E) is spelled as_input_view<views::all_t<E>>(E): see
// reverse_fn.
struct as_input_fn : range_adaptor_closure<as_input_fn> {
private:
    template <class E> static constexpr bool nothrow() {
        if constexpr (input_only_range<E>) {
            return noexcept(views::all(std::declval<E>()));
        } else {
            return noexcept(as_input_view<views::all_t<E>>(std::declval<E>()));
        }
    }

public:
    template <class E>
        requires(input_only_range<E> && requires { views::all(std::declval<E>()); }) ||
                requires { as_input_view<views::all_t<E>>(std::declval<E>()); }
    constexpr auto operator()(E&& e) const noexcept(nothrow<E>()) {
        if constexpr (input_only_range<E>) {
            return views::all(std::forward<E>(e));
        } else {
            return as_input_view<views::all_t<E>>(std::forward<E>(e));
        }
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::as_input_fn as_input{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
