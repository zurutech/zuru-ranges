// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/iterator/counted_iterator.h"
#include "zr/iterator/operations.h"
#include "zr/ranges/adaptor_helpers.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"
#include "zr/ranges/views/all.h"
#include "zr/ranges/views/empty_view.h"
#include "zr/ranges/views/iota_view.h"
#include "zr/ranges/views/subrange.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

/**
 * @file take_view.h
 *
 * @brief [range.take] take_view, views::take.
 *
 * DEVIATION: counted_iterator<I> is detail::counted_iterator_t<I> (zr/iterator/counted_iterator.h).
 *
 * DEVIATION: the branches the draft takes for random_access_range<V> (begin() returning the base
 * iterator, end() as begin + size; views::take slicing span, basic_string_view, subrange and
 * iota_view) also require detail::base_random_access (adaptor_helpers.h). A random-access sized
 * V without it (TArray's debug iterator, TDeque) is iterated by detail::index_iterator from
 * position 0 to size(): random access and common in every Unreal build configuration, end() in
 * O(1). A random-access sized V that is not index_iterable (no element_at, a const view without
 * a const element_at) keeps the counted_iterator with end() at begin + size, reached by size()
 * increments, so that it stays common too.
 *
 * range_traits: the base's category; element_at(v, n) is element_at(base, n), data(v) is
 * data(base), so the classification does not depend on which branch the iterator takes.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

template <class T> inline constexpr bool is_empty_view = false;
template <class T> inline constexpr bool is_empty_view<empty_view<T>> = true;

template <class T> inline constexpr bool is_span = false;
template <class T, std::size_t Extent> inline constexpr bool is_span<std::span<T, Extent>> = true;

template <class T> inline constexpr bool is_basic_string_view = false;
template <class C, class Traits> inline constexpr bool is_basic_string_view<std::basic_string_view<C, Traits>> = true;

template <class T> inline constexpr bool is_subrange = false;
template <class I, class S, subrange_kind K> inline constexpr bool is_subrange<subrange<I, S, K>> = true;

template <class T> inline constexpr bool is_iota_view = false;
template <class W, class Bound> inline constexpr bool is_iota_view<iota_view<W, Bound>> = true;

// random_access_range and sized_range, by iterator arithmetic (see file note).
template <class T>
concept sliceable = base_random_access<T> && sized_range<T>;

} // namespace detail

template <view V> class take_view : public view_interface<take_view<V>> {
private:
    template <class T> friend struct range_traits_impl;

    V base_ = V();
    range_difference_t<V> count_ = 0;

    template <bool Const> class sentinel;

    template <class Self> static constexpr auto begin_(Self& self) {
        using Base = detail::maybe_const<std::is_const_v<Self>, V>;
        using D = range_difference_t<Base>;
        using CI = detail::counted_iterator_t<iterator_t<Base>>;
        if constexpr (sized_range<Base>) {
            if constexpr (detail::base_random_access<Base>) {
                return zr::ranges::begin(self.base_);
            } else if constexpr (detail::index_iterable<Base>) {
                return detail::index_iterator<Base>(self.base_, 0);
            } else {
                return CI(zr::ranges::begin(self.base_), static_cast<D>(self.size()));
            }
        } else if constexpr (std::sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>>) {
            auto it = zr::ranges::begin(self.base_);
            const auto n = std::min(static_cast<D>(self.count_), zr::ranges::end(self.base_) - it);
            return CI(std::move(it), n);
        } else {
            return CI(zr::ranges::begin(self.base_), static_cast<D>(self.count_));
        }
    }

    template <class Self> static constexpr auto end_(Self& self) {
        using Base = detail::maybe_const<std::is_const_v<Self>, V>;
        if constexpr (sized_range<Base>) {
            if constexpr (detail::base_random_access<Base>) {
                return zr::ranges::begin(self.base_) + static_cast<range_difference_t<Base>>(self.size());
            } else if constexpr (detail::index_iterable<Base>) {
                return detail::index_iterator<Base>(self.base_, static_cast<range_difference_t<Base>>(self.size()));
            } else if constexpr (random_access_range<Base>) {
                using CI = detail::counted_iterator_t<iterator_t<Base>>;
                auto it = zr::ranges::begin(self.base_);
                for (auto n = static_cast<range_difference_t<Base>>(self.size()); n > 0; --n) {
                    ++it;
                }
                return CI(std::move(it), 0);
            } else {
                return std::default_sentinel;
            }
        } else if constexpr (std::sized_sentinel_for<sentinel_t<Base>, iterator_t<Base>>) {
            return std::default_sentinel;
        } else {
            return sentinel<std::is_const_v<Self>>{zr::ranges::end(self.base_)};
        }
    }

public:
    take_view()
        requires std::default_initializable<V>
    = default;

    // Parameter named apart from ranges::count: GCC -Wshadow.
    constexpr explicit take_view(V base, range_difference_t<V> n)
        : base_(std::move(base))
        , count_(n) {}

    constexpr V base() const&
        requires std::copy_constructible<V>
    {
        return base_;
    }
    constexpr V base() && { return std::move(base_); }

    constexpr auto begin()
        requires(!detail::simple_view<V>)
    {
        return begin_(*this);
    }
    constexpr auto begin() const
        requires range<const V>
    {
        return begin_(*this);
    }

    constexpr auto end()
        requires(!detail::simple_view<V>)
    {
        return end_(*this);
    }
    constexpr auto end() const
        requires range<const V>
    {
        return end_(*this);
    }

    constexpr auto size()
        requires sized_range<V>
    {
        auto n = zr::ranges::size(base_);
        return std::min(n, static_cast<decltype(n)>(count_));
    }
    constexpr auto size() const
        requires sized_range<const V>
    {
        auto n = zr::ranges::size(base_);
        return std::min(n, static_cast<decltype(n)>(count_));
    }

    constexpr auto reserve_hint() {
        if constexpr (approximately_sized_range<V>) {
            auto n = static_cast<range_difference_t<V>>(zr::ranges::reserve_hint(base_));
            return detail::to_unsigned_like(std::min(n, count_));
        } else {
            return detail::to_unsigned_like(count_);
        }
    }
    constexpr auto reserve_hint() const {
        if constexpr (approximately_sized_range<const V>) {
            auto n = static_cast<range_difference_t<const V>>(zr::ranges::reserve_hint(base_));
            return detail::to_unsigned_like(std::min(n, static_cast<range_difference_t<const V>>(count_)));
        } else {
            return detail::to_unsigned_like(count_);
        }
    }
};

template <class R> take_view(R&&, range_difference_t<R>) -> take_view<views::all_t<R>>;

template <view V>
template <bool Const>
class take_view<V>::sentinel {
private:
    using Base = detail::maybe_const<Const, V>;
    template <bool OtherConst> using CI = detail::counted_iterator_t<iterator_t<detail::maybe_const<OtherConst, V>>>;

    friend take_view;
    friend sentinel<!Const>;

    sentinel_t<Base> end_ = sentinel_t<Base>();

    constexpr explicit sentinel(sentinel_t<Base> end)
        : end_(std::move(end)) {}

public:
    sentinel() = default;

    constexpr sentinel(sentinel<!Const> s)
        requires Const && std::convertible_to<sentinel_t<V>, sentinel_t<Base>>
        : end_(std::move(s.end_)) {}

    constexpr sentinel_t<Base> base() const { return end_; }

    friend constexpr bool operator==(const CI<Const>& y, const sentinel& x) { return y.count() == 0 || detail::iter_equal(y.base(), x.end_); }

    template <bool OtherConst = !Const>
        requires sentinel_for<sentinel_t<Base>, iterator_t<detail::maybe_const<OtherConst, V>>>
    friend constexpr bool operator==(const CI<OtherConst>& y, const sentinel& x) {
        return y.count() == 0 || detail::iter_equal(y.base(), x.end_);
    }
};

template <class V> struct range_traits_impl<take_view<V>> {
    static constexpr range_category category = detail::category_of<V>();

    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access)
    static constexpr decltype(auto) element_at(take_view<V>& v, Index n) {
        return zr::ranges::element_at(v.base_, n);
    }
    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && range<const V> && detail::const_element_at<V>
    static constexpr decltype(auto) element_at(const take_view<V>& v, Index n) {
        return zr::ranges::element_at(v.base_, n);
    }

    static constexpr auto data(take_view<V>& v)
        requires contiguous_range<V>
    {
        return zr::ranges::data(v.base_);
    }
    static constexpr auto data(const take_view<V>& v)
        requires contiguous_range<const V>
    {
        return zr::ranges::data(v.base_);
    }
};

template <class V> struct range_traits_impl<const take_view<V>> : range_traits_impl<take_view<V>> {
    static constexpr range_category category = detail::category_of<const V>();
};

namespace detail {

// [range.take.overview]
// The type the draft slices T into: span<element_type>, T, or subrange<iterator_t<T>>.
template <class T> struct take_slice {
    using type = T;
};
template <class T, std::size_t Extent> struct take_slice<std::span<T, Extent>> {
    using type = std::span<T>;
};
template <class I, class S, subrange_kind K> struct take_slice<subrange<I, S, K>> {
    using type = subrange<I>;
};

template <class T>
concept take_sliceable = (is_span<T> || is_basic_string_view<T> || is_subrange<T>) && sliceable<T>;

struct take_fn {
private:
    // noexcept of the expression the call is equivalent to.
    template <class E, class F> static constexpr bool nothrow() {
        using T = std::remove_cvref_t<E>;
        using D = range_difference_t<E&>;
        if constexpr (is_empty_view<T>) {
            return noexcept(decay_copy(std::declval<E>()));
        } else if constexpr (is_optional<T> && view<T>) {
            return noexcept(static_cast<D>(std::declval<F>()) == D() ? T() : decay_copy(std::declval<E>()));
        } else if constexpr (take_sliceable<T>) {
            return noexcept(typename take_slice<T>::type(
                zr::ranges::begin(std::declval<E&>()),
                zr::ranges::begin(std::declval<E&>()) + std::min<D>(zr::ranges::distance(std::declval<E&>()), std::declval<F>())));
        } else if constexpr (is_iota_view<T> && sliceable<T>) {
            return noexcept(iota_view(*zr::ranges::begin(std::declval<E&>()),
                                      *(zr::ranges::begin(std::declval<E&>()) + std::min<D>(zr::ranges::distance(std::declval<E&>()), std::declval<F>()))));
        } else {
            return noexcept(take_view(std::declval<E>(), std::declval<F>()));
        }
    }

public:
    // Each branch's T is a view, so take_view(E, F) is valid whenever a branch applies.
    template <class E, class F>
        requires range<E&> && std::convertible_to<F, range_difference_t<E&>> && requires { take_view(std::declval<E>(), std::declval<F>()); }
    constexpr auto operator()(E&& e, F&& f) const noexcept(nothrow<E, F>()) {
        using T = std::remove_cvref_t<E>;
        using D = range_difference_t<E&>;
        if constexpr (is_empty_view<T>) {
            (void)f;
            return decay_copy(std::forward<E>(e));
        } else if constexpr (is_optional<T> && view<T>) {
            return static_cast<D>(std::forward<F>(f)) == D() ? ((void)e, T()) : decay_copy(std::forward<E>(e));
        } else if constexpr (take_sliceable<T>) {
            auto first = zr::ranges::begin(e);
            const auto n = std::min<D>(zr::ranges::distance(e), std::forward<F>(f));
            return typename take_slice<T>::type(first, first + n);
        } else if constexpr (is_iota_view<T> && sliceable<T>) {
            auto first = zr::ranges::begin(e);
            const auto n = std::min<D>(zr::ranges::distance(e), std::forward<F>(f));
            return iota_view(*first, *(first + n));
        } else {
            return take_view(std::forward<E>(e), std::forward<F>(f));
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

// [ranges.syn]: enable_borrowed_range<take_view<T>> = enable_borrowed_range<T>.
template <class T> inline constexpr bool enable_borrowed_range<zr::ranges::take_view<T>> = enable_borrowed_range<T>;

} // namespace std::ranges

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::take_fn take{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
