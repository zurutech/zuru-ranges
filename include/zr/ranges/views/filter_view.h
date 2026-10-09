// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/ranges/adaptor_helpers.h"
#include "zr/ranges/movable_box.h"
#include "zr/ranges/non_propagating_cache.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"
#include "zr/ranges/views/all.h"

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

/**
 * @file filter_view.h
 *
 * @brief [range.filter] filter_view, views::filter.
 *
 * The draft's defaulted postfix operators ([over.inc.default], C++26) are written out.
 */

ZR_BEGIN_NAMESPACE_RANGES

template <input_range V, detail::indirect_unary_predicate<iterator_t<V>> Pred>
    requires view<V> && std::is_object_v<Pred>
class filter_view : public view_interface<filter_view<V, Pred>> {
private:
    V base_ = V();
    ZR_NO_UNIQUE_ADDRESS movable_box<Pred> pred_;

    // [range.filter.view] begin(): "caches the result ... when filter_view models forward_range".
    ZR_NO_UNIQUE_ADDRESS detail::cache_if<forward_range<V>, iterator_t<V>> begin_;

    template <bool Const> class iterator;
    template <bool Const> class sentinel;

    // `it = ranges::find_if(std::move(it), ranges::end(base), ref(pred))`, advancing in place:
    // UE's debug array iterator is not assignable.
    template <class Base, class I, class P> static constexpr void satisfy_(Base& base, I& it, P& pred) {
        const auto last = zr::ranges::end(base);
        while (!detail::iter_equal(it, last) && !std::invoke(pred, *it)) {
            ++it;
        }
    }

    template <class Base, class P> static constexpr iterator_t<Base> find_if_(Base& base, P& pred) {
        auto it = zr::ranges::begin(base);
        satisfy_(base, it, pred);
        return it;
    }

public:
    filter_view()
        requires std::default_initializable<V> && std::default_initializable<Pred>
    = default;

    // Parameters named apart from base() and pred(): GCC < 14 -Wshadow.
    constexpr explicit filter_view(V base_view, Pred predicate)
        : base_(std::move(base_view))
        , pred_(std::move(predicate)) {}

    constexpr V base() const&
        requires std::copy_constructible<V>
    {
        return base_;
    }
    constexpr V base() && { return std::move(base_); }

    constexpr const Pred& pred() const { return *pred_; }

    constexpr iterator<false> begin() {
        if constexpr (forward_range<V>) {
            if (!begin_) {
                begin_.emplace(find_if_(base_, *pred_));
            }
            return {*this, *begin_};
        } else {
            return {*this, find_if_(base_, *pred_)};
        }
    }

    constexpr iterator<true> begin() const
        requires(input_range<const V> && !forward_range<const V> && detail::indirect_unary_predicate<const Pred, iterator_t<const V>>)
    {
        return {*this, find_if_(base_, *pred_)};
    }

    constexpr auto end() {
        if constexpr (common_range<V>) {
            return iterator<false>{*this, zr::ranges::end(base_)};
        } else {
            return sentinel<false>{*this};
        }
    }

    constexpr sentinel<true> end() const
        requires(input_range<const V> && !forward_range<const V> && detail::indirect_unary_predicate<const Pred, iterator_t<const V>>)
    {
        return sentinel<true>{*this};
    }
};

template <class R, class Pred> filter_view(R&&, Pred) -> filter_view<views::all_t<R>, Pred>;

namespace detail {

template <bool Const, class V>
inline constexpr bool filter_has_category = forward_range<maybe_const<Const, V>> && has_cpp17_category<iterator_t<maybe_const<Const, V>>>;

template <bool Const, class V> constexpr auto filter_category() {
    if constexpr (filter_has_category<Const, V>) {
        using C = cpp17_category_t<iterator_t<maybe_const<Const, V>>>;
        if constexpr (std::derived_from<C, std::bidirectional_iterator_tag>) {
            return std::bidirectional_iterator_tag{};
        } else if constexpr (std::derived_from<C, std::forward_iterator_tag>) {
            return std::forward_iterator_tag{};
        } else {
            return C{};
        }
    }
}

} // namespace detail

template <input_range V, detail::indirect_unary_predicate<iterator_t<V>> Pred>
    requires view<V> && std::is_object_v<Pred>
template <bool Const>
class filter_view<V, Pred>::iterator
    : public detail::iterator_category_base<detail::filter_has_category<Const, V>, decltype(detail::filter_category<Const, V>())> {
private:
    using Parent = detail::maybe_const<Const, filter_view>;
    using Base = detail::maybe_const<Const, V>;

    friend filter_view;
    friend iterator<!Const>;

    iterator_t<Base> current_ = iterator_t<Base>();
    Parent* parent_ = nullptr;

    constexpr iterator(Parent& parent, iterator_t<Base> current)
        : current_(std::move(current))
        , parent_(std::addressof(parent)) {}

    static constexpr auto concept_tag() {
        if constexpr (Const) {
            return std::input_iterator_tag{};
        } else if constexpr (detail::base_decrementable<V>) {
            return std::bidirectional_iterator_tag{};
        } else if constexpr (forward_range<V>) {
            return std::forward_iterator_tag{};
        } else {
            return std::input_iterator_tag{};
        }
    }

public:
    using iterator_concept = decltype(concept_tag());
    using value_type = range_value_t<Base>;
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

    constexpr range_reference_t<Base> operator*() const { return detail::deref(current_); }

    constexpr iterator_t<Base> operator->() const
        requires detail::has_arrow<iterator_t<Base>> && std::copyable<iterator_t<Base>>
    {
        return current_;
    }

    constexpr iterator& operator++() {
        ++current_;
        satisfy_(parent_->base_, current_, *parent_->pred_);
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
        do {
            --current_;
        } while (!std::invoke(*parent_->pred_, *current_));
        return *this;
    }
    constexpr iterator operator--(int)
        requires detail::base_decrementable<Base>
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y)
        requires detail::relaxed_equality_comparable<iterator_t<Base>>
    {
        return detail::iter_equal(x.current_, y.current_);
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

template <input_range V, detail::indirect_unary_predicate<iterator_t<V>> Pred>
    requires view<V> && std::is_object_v<Pred>
template <bool Const>
class filter_view<V, Pred>::sentinel {
private:
    using Parent = detail::maybe_const<Const, filter_view>;
    using Base = detail::maybe_const<Const, V>;

    friend filter_view;
    friend sentinel<!Const>;

    sentinel_t<Base> end_ = sentinel_t<Base>();

    constexpr explicit sentinel(Parent& parent)
        : end_(zr::ranges::end(parent.base_)) {}

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
};

template <class V, class Pred> struct range_traits_impl<filter_view<V, Pred>> {
    static constexpr range_category category = std::min(detail::category_of<V>(), range_category::bidirectional);
};

template <class V, class Pred> struct range_traits_impl<const filter_view<V, Pred>> {
    static constexpr range_category category = std::min(detail::category_of<const V>(), range_category::bidirectional);
};

namespace detail {

// [range.filter.overview]
struct filter_fn {
    template <class E, class P>
        requires requires { filter_view(std::declval<E>(), std::declval<P>()); }
    constexpr auto operator()(E&& e, P&& p) const noexcept(noexcept(filter_view(std::forward<E>(e), std::forward<P>(p)))) {
        return filter_view(std::forward<E>(e), std::forward<P>(p));
    }

    template <class P>
        requires std::constructible_from<std::decay_t<P>, P>
    constexpr auto operator()(P&& p) const noexcept(std::is_nothrow_constructible_v<std::decay_t<P>, P>) {
        return make_bound_closure(*this, std::forward<P>(p));
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::filter_fn filter{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
