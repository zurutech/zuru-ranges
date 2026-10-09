// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/ranges/adaptor_helpers.h"
#include "zr/ranges/movable_box.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"
#include "zr/ranges/views/all.h"

#include <algorithm>
#include <compare>
#include <concepts>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

/**
 * @file transform_view.h
 *
 * @brief [range.transform] transform_view, views::transform.
 *
 * range_traits: the base's category capped at random_access; element_at(v, n) is
 * invoke(f, element_at(base, n)). The draft's defaulted postfix operators ([over.inc.default],
 * C++26) are written out.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// Named concepts in member requires-clauses: Clang < 16 substitutes the whole clause eagerly.
template <class V, class F>
concept transform_const_iterable = range<const V> && std::regular_invocable<const F&, range_reference_t<const V>>;

} // namespace detail

template <input_range V, std::move_constructible F>
    requires view<V> && std::is_object_v<F> && std::regular_invocable<F&, range_reference_t<V>> &&
             detail::can_reference<std::invoke_result_t<F&, range_reference_t<V>>>
class transform_view : public view_interface<transform_view<V, F>> {
private:
    template <bool Const> class iterator;
    template <bool Const> class sentinel;

    template <class T> friend struct range_traits_impl;

    V base_ = V();
    ZR_NO_UNIQUE_ADDRESS movable_box<F> fun_;

public:
    transform_view()
        requires std::default_initializable<V> && std::default_initializable<F>
    = default;

    constexpr explicit transform_view(V base, F fun)
        : base_(std::move(base))
        , fun_(std::move(fun)) {}

    constexpr V base() const&
        requires std::copy_constructible<V>
    {
        return base_;
    }
    constexpr V base() && { return std::move(base_); }

    constexpr iterator<false> begin() { return iterator<false>{*this, zr::ranges::begin(base_)}; }

    constexpr iterator<true> begin() const
        requires detail::transform_const_iterable<V, F>
    {
        return iterator<true>{*this, zr::ranges::begin(base_)};
    }

    constexpr sentinel<false> end() { return sentinel<false>{zr::ranges::end(base_)}; }

    constexpr iterator<false> end()
        requires common_range<V>
    {
        return iterator<false>{*this, zr::ranges::end(base_)};
    }

    constexpr sentinel<true> end() const
        requires detail::transform_const_iterable<V, F>
    {
        return sentinel<true>{zr::ranges::end(base_)};
    }

    constexpr iterator<true> end() const
        requires common_range<const V> && detail::transform_const_iterable<V, F>
    {
        return iterator<true>{*this, zr::ranges::end(base_)};
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

template <class R, class F> transform_view(R&&, F) -> transform_view<views::all_t<R>, F>;

namespace detail {

template <bool Const, class V, class F>
inline constexpr bool transform_has_category = forward_range<maybe_const<Const, V>> && has_cpp17_category<iterator_t<maybe_const<Const, V>>>;

template <bool Const, class V, class F> constexpr auto transform_category() {
    if constexpr (transform_has_category<Const, V, F>) {
        using C = cpp17_category_t<iterator_t<maybe_const<Const, V>>>;
        if constexpr (std::is_reference_v<std::invoke_result_t<maybe_const<Const, F>&, range_reference_t<maybe_const<Const, V>>>>) {
            if constexpr (std::derived_from<C, std::contiguous_iterator_tag>) {
                return std::random_access_iterator_tag{};
            } else {
                return C{};
            }
        } else {
            return std::input_iterator_tag{};
        }
    }
}

} // namespace detail

template <input_range V, std::move_constructible F>
    requires view<V> && std::is_object_v<F> && std::regular_invocable<F&, range_reference_t<V>> &&
             detail::can_reference<std::invoke_result_t<F&, range_reference_t<V>>>
template <bool Const>
class transform_view<V, F>::iterator
    : public detail::iterator_category_base<detail::transform_has_category<Const, V, F>, decltype(detail::transform_category<Const, V, F>())> {
private:
    using Parent = detail::maybe_const<Const, transform_view>;
    using Base = detail::maybe_const<Const, V>;

    friend transform_view;
    friend iterator<!Const>;

    iterator_t<Base> current_ = iterator_t<Base>();
    Parent* parent_ = nullptr;

    constexpr iterator(Parent& parent, iterator_t<Base> current)
        : current_(std::move(current))
        , parent_(std::addressof(parent)) {}

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
    using value_type = std::remove_cvref_t<std::invoke_result_t<detail::maybe_const<Const, F>&, range_reference_t<Base>>>;
    using difference_type = range_difference_t<Base>;

    iterator()
        requires std::default_initializable<iterator_t<Base>>
    = default;

    constexpr iterator(iterator<!Const> i)
        requires Const && std::convertible_to<iterator_t<V>, iterator_t<Base>>
        : current_(std::move(i.current_))
        , parent_(i.parent_) {}

    constexpr const iterator_t<Base>& base() const& noexcept { return current_; }
    constexpr iterator_t<Base> base() && { return std::move(current_); }

    constexpr decltype(auto) operator*() const noexcept(noexcept(std::invoke(*parent_->fun_, detail::deref(current_)))) {
        return std::invoke(*parent_->fun_, detail::deref(current_));
    }

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
        return std::invoke(*parent_->fun_, current_[n]);
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

    friend constexpr iterator operator+(iterator i, difference_type n)
        requires detail::base_random_access<Base>
    {
        return iterator{*i.parent_, i.current_ + n};
    }
    friend constexpr iterator operator+(difference_type n, iterator i)
        requires detail::base_random_access<Base>
    {
        return iterator{*i.parent_, i.current_ + n};
    }
    friend constexpr iterator operator-(iterator i, difference_type n)
        requires detail::base_random_access<Base>
    {
        return iterator{*i.parent_, i.current_ - n};
    }
    friend constexpr difference_type operator-(const iterator& x, const iterator& y)
        requires std::sized_sentinel_for<iterator_t<Base>, iterator_t<Base>>
    {
        return x.current_ - y.current_;
    }
};

template <input_range V, std::move_constructible F>
    requires view<V> && std::is_object_v<F> && std::regular_invocable<F&, range_reference_t<V>> &&
             detail::can_reference<std::invoke_result_t<F&, range_reference_t<V>>>
template <bool Const>
class transform_view<V, F>::sentinel {
private:
    using Parent = detail::maybe_const<Const, transform_view>;
    using Base = detail::maybe_const<Const, V>;

    friend transform_view;
    friend sentinel<!Const>;

    sentinel_t<Base> end_ = sentinel_t<Base>();

    constexpr explicit sentinel(sentinel_t<Base> end)
        : end_(std::move(end)) {}

public:
    sentinel() = default;

    constexpr sentinel(sentinel<!Const> i)
        requires Const && std::convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(i.end_)) {}

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
    friend constexpr range_difference_t<detail::maybe_const<OtherConst, V>> operator-(const sentinel& y, const iterator<OtherConst>& x) {
        return y.end_ - x.base();
    }
};

template <class V, class F> struct range_traits_impl<transform_view<V, F>> {
    static constexpr range_category category = std::min(detail::category_of<V>(), range_category::random_access);

    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access)
    static constexpr decltype(auto) element_at(transform_view<V, F>& v, Index n) {
        return std::invoke(*v.fun_, zr::ranges::element_at(v.base_, n));
    }
    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && detail::transform_const_iterable<V, F> &&
                requires(const V& base) { zr::ranges::element_at(base, 0); }
    static constexpr decltype(auto) element_at(const transform_view<V, F>& v, Index n) {
        return std::invoke(*v.fun_, zr::ranges::element_at(v.base_, n));
    }
};

// const transform_view: maybe-const<true, V> ([range.transform.iterator]).
template <class V, class F> struct range_traits_impl<const transform_view<V, F>> : range_traits_impl<transform_view<V, F>> {
    static constexpr range_category category = std::min(detail::category_of<const V>(), range_category::random_access);
};

namespace detail {

// [range.transform.overview]
struct transform_fn {
    template <class E, class F>
        requires requires { transform_view(std::declval<E>(), std::declval<F>()); }
    constexpr auto operator()(E&& e, F&& f) const noexcept(noexcept(transform_view(std::forward<E>(e), std::forward<F>(f)))) {
        return transform_view(std::forward<E>(e), std::forward<F>(f));
    }

    template <class F>
        requires std::constructible_from<std::decay_t<F>, F>
    constexpr auto operator()(F&& f) const noexcept(std::is_nothrow_constructible_v<std::decay_t<F>, F>) {
        return make_bound_closure(*this, std::forward<F>(f));
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::transform_fn transform{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
