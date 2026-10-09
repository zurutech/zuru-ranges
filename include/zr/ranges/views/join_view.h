// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/ranges/adaptor_helpers.h"
#include "zr/ranges/non_propagating_cache.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"
#include "zr/ranges/views/all.h"

#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

/**
 * @file join_view.h
 *
 * @brief [range.join] join_view, views::join.
 *
 * DEVIATION: the iterator assigns no base iterator: `inner_ = ranges::begin(inner)` and
 * `outer_ = ranges::begin(base_)` are emplacements (UE's debug array iterator is not
 * assignable). The draft's defaulted postfix operators ([over.inc.default], C++26) are
 * written out.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// A named concept: Clang < 16 substitutes a member's requires-clause eagerly.
template <class V>
concept join_const_iterable =
    forward_range<const V> && std::is_reference_v<range_reference_t<const V>> && input_range<range_reference_t<const V>>;

} // namespace detail

template <input_range V>
    requires view<V> && input_range<range_reference_t<V>>
class join_view : public view_interface<join_view<V>> {
private:
    using InnerRng = range_reference_t<V>;

    template <bool Const> class iterator;
    template <bool Const> class sentinel;

    struct absent {};

    V base_ = V();

    ZR_NO_UNIQUE_ADDRESS detail::cache_if<!forward_range<V>, iterator_t<V>> outer_;
    ZR_NO_UNIQUE_ADDRESS detail::cache_if<!std::is_reference_v<InnerRng>, std::remove_cv_t<InnerRng>> inner_;

public:
    join_view()
        requires std::default_initializable<V>
    = default;
    constexpr explicit join_view(V base)
        : base_(std::move(base)) {}

    constexpr V base() const&
        requires std::copy_constructible<V>
    {
        return base_;
    }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin() {
        if constexpr (forward_range<V>) {
            constexpr bool use_const = detail::simple_view<V> && std::is_reference_v<InnerRng>;
            return iterator<use_const>{*this, zr::ranges::begin(base_)};
        } else {
            outer_.emplace(zr::ranges::begin(base_));
            return iterator<false>{*this};
        }
    }

    constexpr auto begin() const
        requires detail::join_const_iterable<V>
    {
        return iterator<true>{*this, zr::ranges::begin(base_)};
    }

    constexpr auto end() {
        if constexpr (forward_range<V> && std::is_reference_v<InnerRng> && forward_range<InnerRng> && common_range<V> &&
                      common_range<InnerRng>) {
            return iterator<detail::simple_view<V>>{*this, zr::ranges::end(base_)};
        } else {
            return sentinel<detail::simple_view<V>>{*this};
        }
    }

    constexpr auto end() const
        requires detail::join_const_iterable<V>
    {
        if constexpr (forward_range<range_reference_t<const V>> && common_range<const V> && common_range<range_reference_t<const V>>) {
            return iterator<true>{*this, zr::ranges::end(base_)};
        } else {
            return sentinel<true>{*this};
        }
    }
};

template <class R> explicit join_view(R&&) -> join_view<views::all_t<R>>;

namespace detail {

template <bool Const, class V>
inline constexpr bool join_has_category = std::is_reference_v<range_reference_t<maybe_const<Const, V>>> &&
                                          forward_range<maybe_const<Const, V>> && forward_range<range_reference_t<maybe_const<Const, V>>> &&
                                          has_cpp17_category<iterator_t<maybe_const<Const, V>>> &&
                                          has_cpp17_category<iterator_t<range_reference_t<maybe_const<Const, V>>>>;

template <bool Const, class V> constexpr auto join_category() {
    if constexpr (join_has_category<Const, V>) {
        using Base = maybe_const<Const, V>;
        using OUTERC = cpp17_category_t<iterator_t<Base>>;
        using INNERC = cpp17_category_t<iterator_t<range_reference_t<Base>>>;
        if constexpr (std::derived_from<OUTERC, std::bidirectional_iterator_tag> && std::derived_from<INNERC, std::bidirectional_iterator_tag> &&
                      common_range<range_reference_t<Base>>) {
            return std::bidirectional_iterator_tag{};
        } else if constexpr (std::derived_from<OUTERC, std::forward_iterator_tag> && std::derived_from<INNERC, std::forward_iterator_tag>) {
            return std::forward_iterator_tag{};
        } else {
            return std::input_iterator_tag{};
        }
    }
}

} // namespace detail

template <input_range V>
    requires view<V> && input_range<range_reference_t<V>>
template <bool Const>
class join_view<V>::iterator : public detail::iterator_category_base<detail::join_has_category<Const, V>, decltype(detail::join_category<Const, V>())> {
private:
    using Parent = detail::maybe_const<Const, join_view>;
    using Base = detail::maybe_const<Const, V>;
    using OuterIter = iterator_t<Base>;
    using InnerIter = iterator_t<range_reference_t<Base>>;

    friend join_view;
    friend iterator<!Const>;
    template <bool> friend class sentinel;

    static constexpr bool ref_is_glvalue = std::is_reference_v<range_reference_t<Base>>;

    static constexpr bool bidi_ = ref_is_glvalue && detail::base_decrementable<Base> && detail::base_decrementable<range_reference_t<Base>> &&
                                  common_range<range_reference_t<Base>>;
    static constexpr bool fwd_ = ref_is_glvalue && forward_range<Base> && forward_range<range_reference_t<Base>>;

    ZR_NO_UNIQUE_ADDRESS std::conditional_t<forward_range<Base>, OuterIter, absent> outer_;
    std::optional<InnerIter> inner_;
    Parent* parent_ = nullptr;

    constexpr OuterIter& outer() {
        if constexpr (forward_range<Base>) {
            return outer_;
        } else {
            return *parent_->outer_;
        }
    }
    constexpr const OuterIter& outer() const {
        if constexpr (forward_range<Base>) {
            return outer_;
        } else {
            return *parent_->outer_;
        }
    }

    constexpr void satisfy() {
        auto update_inner = [&](const iterator_t<Base>& x) -> auto&& {
            if constexpr (ref_is_glvalue) {
                return *x;
            } else {
                return parent_->inner_.emplace_deref(x);
            }
        };

        for (; !detail::iter_equal(outer(), zr::ranges::end(parent_->base_)); ++outer()) {
            auto&& inner = update_inner(outer());
            inner_.emplace(zr::ranges::begin(inner));
            if (!detail::iter_equal(*inner_, zr::ranges::end(inner))) {
                return;
            }
        }
        if constexpr (ref_is_glvalue) {
            inner_.reset();
        }
    }

    constexpr iterator(Parent& parent, OuterIter outer)
        requires forward_range<Base>
        : outer_(std::move(outer))
        , parent_(std::addressof(parent)) {
        satisfy();
    }

    constexpr explicit iterator(Parent& parent)
        requires(!forward_range<Base>)
        : parent_(std::addressof(parent)) {
        satisfy();
    }

    constexpr decltype(auto) inner_range() {
        if constexpr (ref_is_glvalue) {
            return *outer();
        } else {
            return *parent_->inner_;
        }
    }

    static constexpr auto concept_tag() {
        if constexpr (bidi_) {
            return std::bidirectional_iterator_tag{};
        } else if constexpr (fwd_) {
            return std::forward_iterator_tag{};
        } else {
            return std::input_iterator_tag{};
        }
    }

public:
    using iterator_concept = decltype(concept_tag());
    using value_type = range_value_t<range_reference_t<Base>>;
    using difference_type = std::common_type_t<range_difference_t<Base>, range_difference_t<range_reference_t<Base>>>;

    // As the sentinel's: no default member initializer for outer_.
    constexpr iterator()
        requires std::default_initializable<decltype(outer_)>
        : outer_() {}

    constexpr iterator(iterator<!Const> i)
        requires Const && std::convertible_to<iterator_t<V>, OuterIter> && std::convertible_to<iterator_t<InnerRng>, InnerIter>
        : outer_(std::move(i.outer_))
        , inner_(std::move(i.inner_))
        , parent_(i.parent_) {}

    constexpr decltype(auto) operator*() const { return detail::deref(*inner_); }

    constexpr InnerIter operator->() const
        requires detail::has_arrow<InnerIter> && std::copyable<InnerIter>
    {
        return *inner_;
    }

    constexpr iterator& operator++() {
        ++*inner_;
        if (detail::iter_equal(*inner_, zr::ranges::end(detail::as_lvalue(inner_range())))) {
            ++outer();
            satisfy();
        }
        return *this;
    }
    constexpr void operator++(int) { ++*this; }
    constexpr iterator operator++(int)
        requires ref_is_glvalue && forward_range<Base> && forward_range<range_reference_t<Base>>
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr iterator& operator--()
        requires bidi_
    {
        if (detail::iter_equal(outer_, zr::ranges::end(parent_->base_))) {
            inner_.emplace(zr::ranges::end(detail::as_lvalue(*--outer_)));
        }
        while (detail::iter_equal(*inner_, zr::ranges::begin(detail::as_lvalue(*outer_)))) {
            inner_.emplace(zr::ranges::end(detail::as_lvalue(*--outer_)));
        }
        --*inner_;
        return *this;
    }
    constexpr iterator operator--(int)
        requires bidi_
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    friend constexpr bool operator==(const iterator& x, const iterator& y)
        requires ref_is_glvalue && forward_range<Base> && detail::relaxed_equality_comparable<InnerIter>
    {
        return detail::iter_equal(x.outer_, y.outer_) && x.inner_.has_value() == y.inner_.has_value() &&
               (!x.inner_ || detail::iter_equal(*x.inner_, *y.inner_));
    }

    friend constexpr decltype(auto) iter_move(const iterator& i) noexcept(noexcept(detail::relaxed_iter_move(*i.inner_))) {
        return detail::relaxed_iter_move(*i.inner_);
    }

    friend constexpr void iter_swap(const iterator& x, const iterator& y) noexcept(noexcept(detail::relaxed_iter_swap(*x.inner_, *y.inner_)))
        requires detail::indirectly_swappable<InnerIter>
    {
        detail::relaxed_iter_swap(*x.inner_, *y.inner_);
    }
};

template <input_range V>
    requires view<V> && input_range<range_reference_t<V>>
template <bool Const>
class join_view<V>::sentinel {
private:
    using Parent = detail::maybe_const<Const, join_view>;
    using Base = detail::maybe_const<Const, V>;

    friend join_view;
    friend sentinel<!Const>;

    sentinel_t<Base> end_;

    constexpr explicit sentinel(Parent& parent)
        : end_(zr::ranges::end(parent.base_)) {}

    // MSVC: a hidden friend does not inherit its class's friendship with iterator (CWG1699).
    template <bool OtherConst> static constexpr decltype(auto) outer_of(const iterator<OtherConst>& x) { return x.outer(); }

public:
    // `end_ = sentinel_t<Base>()` as a constrained constructor: a default member initializer
    // would be instantiated for a non-default-initializable (Unreal) sentinel.
    constexpr sentinel()
        requires std::default_initializable<sentinel_t<Base>>
        : end_() {}

    constexpr sentinel(sentinel<!Const> s)
        requires Const && std::convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(s.end_)) {}

    template <bool OtherConst>
        requires sentinel_for<sentinel_t<Base>, iterator_t<detail::maybe_const<OtherConst, V>>>
    friend constexpr bool operator==(const iterator<OtherConst>& x, const sentinel& y) {
        return detail::iter_equal(outer_of(x), y.end_);
    }
};

namespace detail {

template <class V> constexpr range_category join_traits_category() {
    using Inner = range_reference_t<V>;
    if constexpr (std::is_reference_v<Inner> && bidirectional_range<V> && bidirectional_range<Inner> && common_range<Inner>) {
        return range_category::bidirectional;
    } else if constexpr (std::is_reference_v<Inner> && forward_range<V> && forward_range<Inner>) {
        return range_category::forward;
    } else {
        return range_category::input;
    }
}

} // namespace detail

template <class V> struct range_traits_impl<join_view<V>> {
    static constexpr range_category category = detail::join_traits_category<V>();
};

template <class V> struct range_traits_impl<const join_view<V>> {
    static constexpr range_category category = detail::join_traits_category<const V>();
};

namespace detail {

// [range.join.overview]
struct join_fn : range_adaptor_closure<join_fn> {
    template <class E>
        requires requires { join_view<views::all_t<E>>{std::declval<E>()}; }
    constexpr auto operator()(E&& e) const noexcept(noexcept(join_view<views::all_t<E>>{std::forward<E>(e)})) {
        return join_view<views::all_t<E>>{std::forward<E>(e)};
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::join_fn join{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
