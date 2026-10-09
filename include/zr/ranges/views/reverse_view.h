// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/operations.h"
#include "zr/ranges/adaptor_helpers.h"
#include "zr/ranges/non_propagating_cache.h"
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
#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

/**
 * @file reverse_view.h
 *
 * @brief [range.reverse] reverse_view, views::reverse.
 *
 * DEVIATION: when iterator_t<V> is not a std::bidirectional_iterator (Unreal), the iterator is
 * not reverse_iterator<iterator_t<V>> but
 *   - reverse_iterator<detail::index_iterator<V>> for an index_iterable V (adaptor_helpers.h):
 *     element_at(base, n - 1) for a position n counted from the front;
 *   - else detail::reverse_decrement_iterator for a common V whose iterator decrements:
 *     reverse_iterator's operations without iterator_traits.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

template <class V>
concept reverse_by_iterator = std::bidirectional_iterator<iterator_t<V>>;

template <class V>
concept reverse_by_index = !reverse_by_iterator<V> && index_iterable<V>;

template <class V>
concept reverse_by_decrement = !reverse_by_iterator<V> && !reverse_by_index<V> && base_decrementable<V> && common_range<V> &&
                               std::copy_constructible<iterator_t<V>>;

// [reverse.iterators] over a decrementing non-std iterator: *it is *prev(current).
template <class I> class reverse_decrement_iterator {
private:
    I current_;

    template <class U> friend class reverse_decrement_iterator;

public:
    using iterator_concept = std::bidirectional_iterator_tag;
    using value_type = iter_value_t<I>;
    using difference_type = iter_difference_t<I>;

    reverse_decrement_iterator()
        requires std::default_initializable<I>
        : current_() {}

    constexpr explicit reverse_decrement_iterator(I current)
        : current_(std::move(current)) {}

    template <class U>
        requires(!std::same_as<U, I>) && std::convertible_to<const U&, I>
    constexpr reverse_decrement_iterator(const reverse_decrement_iterator<U>& other)
        : current_(other.current_) {}

    constexpr I base() const { return current_; }

    constexpr decltype(auto) operator*() const {
        I tmp(current_);
        --tmp;
        return detail::deref(tmp);
    }

    constexpr reverse_decrement_iterator& operator++() {
        --current_;
        return *this;
    }
    constexpr reverse_decrement_iterator operator++(int) {
        auto tmp = *this;
        --current_;
        return tmp;
    }
    constexpr reverse_decrement_iterator& operator--() {
        ++current_;
        return *this;
    }
    constexpr reverse_decrement_iterator operator--(int) {
        auto tmp = *this;
        ++current_;
        return tmp;
    }

    friend constexpr bool operator==(const reverse_decrement_iterator& x, const reverse_decrement_iterator& y)
        requires relaxed_equality_comparable<I>
    {
        return detail::iter_equal(x.current_, y.current_);
    }
};

} // namespace detail

template <view V>
    requires bidirectional_range<V> && (detail::reverse_by_iterator<V> || detail::reverse_by_index<V> || detail::reverse_by_decrement<V>)
class reverse_view : public view_interface<reverse_view<V>> {
private:
    V base_ = V();

    ZR_NO_UNIQUE_ADDRESS detail::cache_if<detail::reverse_by_iterator<V> && !common_range<V>, std::reverse_iterator<iterator_t<V>>> begin_;

    template <class Base> static constexpr auto index_iterator_at(Base& base, range_difference_t<Base> pos) {
        return std::reverse_iterator<detail::index_iterator<Base>>(detail::index_iterator<Base>(base, pos));
    }

public:
    reverse_view()
        requires std::default_initializable<V>
    = default;

    constexpr explicit reverse_view(V r)
        : base_(std::move(r)) {}

    constexpr V base() const&
        requires std::copy_constructible<V>
    {
        return base_;
    }
    constexpr V base() && { return std::move(base_); }

    constexpr std::reverse_iterator<iterator_t<V>> begin()
        requires detail::reverse_by_iterator<V>
    {
        if (!begin_) {
            auto it = zr::ranges::begin(base_);
            if constexpr (std::sentinel_for<sentinel_t<V>, iterator_t<V>>) {
                it = detail::next(std::move(it), zr::ranges::end(base_));
            } else {
                for (const auto last = zr::ranges::end(base_); !detail::iter_equal(it, last);) {
                    ++it;
                }
            }
            begin_.emplace(std::make_reverse_iterator(std::move(it)));
        }
        return *begin_;
    }

    constexpr std::reverse_iterator<iterator_t<V>> begin()
        requires detail::reverse_by_iterator<V> && common_range<V>
    {
        return std::make_reverse_iterator(zr::ranges::end(base_));
    }

    constexpr auto begin() const
        requires detail::reverse_by_iterator<V> && common_range<const V>
    {
        return std::make_reverse_iterator(zr::ranges::end(base_));
    }

    constexpr std::reverse_iterator<iterator_t<V>> end()
        requires detail::reverse_by_iterator<V>
    {
        return std::make_reverse_iterator(zr::ranges::begin(base_));
    }

    constexpr auto end() const
        requires detail::reverse_by_iterator<V> && common_range<const V>
    {
        return std::make_reverse_iterator(zr::ranges::begin(base_));
    }

    constexpr auto begin()
        requires detail::reverse_by_index<V>
    {
        return index_iterator_at(base_, static_cast<range_difference_t<V>>(zr::ranges::size(base_)));
    }
    constexpr auto begin() const
        requires detail::reverse_by_index<V> && detail::reverse_by_index<const V>
    {
        return index_iterator_at(base_, static_cast<range_difference_t<const V>>(zr::ranges::size(base_)));
    }
    constexpr auto end()
        requires detail::reverse_by_index<V>
    {
        return index_iterator_at(base_, range_difference_t<V>(0));
    }
    constexpr auto end() const
        requires detail::reverse_by_index<V> && detail::reverse_by_index<const V>
    {
        return index_iterator_at(base_, range_difference_t<const V>(0));
    }

    constexpr auto begin()
        requires detail::reverse_by_decrement<V>
    {
        return detail::reverse_decrement_iterator<iterator_t<V>>(zr::ranges::end(base_));
    }
    constexpr auto begin() const
        requires detail::reverse_by_decrement<V> && detail::reverse_by_decrement<const V>
    {
        return detail::reverse_decrement_iterator<iterator_t<const V>>(zr::ranges::end(base_));
    }
    constexpr auto end()
        requires detail::reverse_by_decrement<V>
    {
        return detail::reverse_decrement_iterator<iterator_t<V>>(zr::ranges::begin(base_));
    }
    constexpr auto end() const
        requires detail::reverse_by_decrement<V> && detail::reverse_by_decrement<const V>
    {
        return detail::reverse_decrement_iterator<iterator_t<const V>>(zr::ranges::begin(base_));
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

template <class R> reverse_view(R&&) -> reverse_view<views::all_t<R>>;

template <class V> struct range_traits_impl<reverse_view<V>> {
    static constexpr range_category category = std::min(detail::category_of<V>(), range_category::random_access);

    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access)
    static constexpr decltype(auto) element_at(reverse_view<V>& v, Index n) {
        return v.begin()[static_cast<range_difference_t<V>>(n)];
    }
    template <detail::is_integer_like Index>
        requires(category >= range_category::random_access) && range<const reverse_view<V>>
    static constexpr decltype(auto) element_at(const reverse_view<V>& v, Index n) {
        return v.begin()[static_cast<range_difference_t<const V>>(n)];
    }
};

template <class V> struct range_traits_impl<const reverse_view<V>> : range_traits_impl<reverse_view<V>> {
    static constexpr range_category category = std::min(detail::category_of<const V>(), range_category::random_access);
};

ZR_END_NAMESPACE_RANGES

namespace std::ranges {

// [range.reverse.view]: enable_borrowed_range<V>.
// DEVIATION (index path): also requires copyable<V>, see detail::index_iterator_holds_copy.
template <class V>
inline constexpr bool enable_borrowed_range<zr::ranges::reverse_view<V>> =
    enable_borrowed_range<V> && (zr::ranges::detail::reverse_by_iterator<V> || zr::ranges::detail::reverse_by_decrement<V> || copyable<V>);

} // namespace std::ranges

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

template <class T> inline constexpr bool is_reverse_view = false;
template <class V> inline constexpr bool is_reverse_view<reverse_view<V>> = true;

template <class T> struct reversed_subrange {
    static constexpr bool value = false;
};
template <class I, subrange_kind K> struct reversed_subrange<subrange<std::reverse_iterator<I>, std::reverse_iterator<I>, K>> {
    static constexpr bool value = true;
    static constexpr subrange_kind kind = K;
    using type = subrange<I, I, K>;
};

// [range.reverse.overview]. reverse_view{E} is spelled reverse_view<views::all_t<E>>(E), the type
// its deduction guide produces: MSVC 14.29 evaluates the class template argument deduction to
// false inside std::invocable for an rvalue E (the pipe `view | views::reverse`).
struct reverse_fn : range_adaptor_closure<reverse_fn> {
    template <class E>
        requires(is_reverse_view<std::remove_cvref_t<E>> && requires { std::declval<E>().base(); }) ||
                (is_optional<std::remove_cvref_t<E>> && view<std::remove_cvref_t<E>>) ||
                 reversed_subrange<std::remove_cvref_t<E>>::value || requires { reverse_view<views::all_t<E>>(std::declval<E>()); }
    constexpr auto operator()(E&& e) const {
        using U = std::remove_cvref_t<E>;
        if constexpr (is_reverse_view<U>) {
            return std::forward<E>(e).base();
        } else if constexpr (is_optional<U> && view<U>) {
            return decay_copy(std::forward<E>(e));
        } else if constexpr (reversed_subrange<U>::value) {
            using S = typename reversed_subrange<U>::type;
            if constexpr (reversed_subrange<U>::kind == subrange_kind::sized) {
                return S(e.end().base(), e.begin().base(), e.size());
            } else {
                return S(e.end().base(), e.begin().base());
            }
        } else {
            return reverse_view<views::all_t<E>>(std::forward<E>(e));
        }
    }
};

} // namespace detail

ZR_END_NAMESPACE_RANGES

ZR_BEGIN_NAMESPACE_VIEWS

inline constexpr detail::reverse_fn reverse{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
