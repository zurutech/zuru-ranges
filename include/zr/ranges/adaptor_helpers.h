// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/iterator/indirect_requirements.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/view_interface.h"

#include <compare>
#include <concepts>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

/**
 * @file adaptor_helpers.h
 *
 * @brief [range.adaptor.helpers], and the base-iterator gates shared by the view iterators.
 *
 * DEVIATION (all view iterators): an operation the draft gates on bidirectional_range<Base> or
 * random_access_range<Base> additionally requires iterator_t<Base> to provide it
 * (base_decrementable, base_random_access), as do the iterator_concept tags above forward.
 * range_traits classify Unreal containers whose iterators only increment; random access to them
 * goes through ranges::element_at.
 *
 * DEVIATION (all view iterators): iterator_category is declared only if
 * iterator_traits<iterator_t<Base>>::iterator_category is valid (Unreal iterators have none).
 *
 * DEVIATION (take_view, zip_view, reverse_view): over a base that range_traits classify random
 * access and sized while its iterator only increments (index_iterable), the view iterates with
 * detail::index_iterator, random access by position through ranges::element_at, so it is random
 * access and common in every build configuration and end() is O(1).
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// [range.adaptor.helpers]
template <class F, class Tuple> constexpr auto tuple_transform(F&& f, Tuple&& t) {
    return std::apply(
        [&]<class... Ts>(Ts&&... elements) { return std::tuple<std::invoke_result_t<F&, Ts>...>(std::invoke(f, std::forward<Ts>(elements))...); },
        std::forward<Tuple>(t));
}

template <class F, class Tuple> constexpr void tuple_for_each(F&& f, Tuple&& t) {
    std::apply([&]<class... Ts>(Ts&&... elements) { (static_cast<void>(std::invoke(f, std::forward<Ts>(elements))), ...); },
               std::forward<Tuple>(t));
}

template <class T> constexpr T& as_lvalue(T&& t) { return static_cast<T&>(t); }

// A specialization of optional (a view since P3168R2, C++26): views::reverse, take, drop.
template <class T> inline constexpr bool is_optional = false;
template <class T> inline constexpr bool is_optional<std::optional<T>> = true;

// Gates (see file note).
template <class R>
concept base_decrementable = bidirectional_range<R> && prefix_decrementable<iterator_t<R>>;

template <class R>
concept base_random_access = random_access_range<R> && std::random_access_iterator<iterator_t<R>>;

template <bool Const, class... Views>
concept all_random_access = (base_random_access<maybe_const<Const, Views>> && ...);
template <bool Const, class... Views>
concept all_bidirectional = (base_decrementable<maybe_const<Const, Views>> && ...);
template <bool Const, class... Views>
concept all_forward = (forward_range<maybe_const<Const, Views>> && ...);

// (convertible_to<iterator_t<Vs>, iterator_t<const Vs>> && ...), named: Clang < 16 substitutes a
// member's requires-clause eagerly, and const Vs need not be ranges.
template <class... Vs>
concept const_convertible_iterators = (range<const Vs> && ...) && (std::convertible_to<iterator_t<Vs>, iterator_t<const Vs>> && ...);

// Gates of the index iterator (see file note).
template <class R>
concept index_iterable = random_access_range<R> && sized_range<R> && !base_random_access<R> && requires(R& r) { zr::ranges::element_at(r, 0); };

// The index iterator owns a copy of a borrowed copyable Base, so the view stays borrowed exactly
// when Base is; otherwise it points to the view.
template <class Base>
inline constexpr bool index_iterator_holds_copy = enable_borrowed_range<std::remove_cv_t<Base>> && std::copyable<std::remove_cv_t<Base>>;

// A std random-access iterator over an index_iterable Base: *it is element_at(base, pos).
template <class Base> class index_iterator {
private:
    // optional: Base need not be default-initializable (ref_view), the iterator must be.
    using holder_t = std::conditional_t<index_iterator_holds_copy<Base>, std::optional<std::remove_cv_t<Base>>, Base*>;

    mutable holder_t base_ = holder_t();
    range_difference_t<Base> pos_ = 0;

    constexpr Base& base_ref() const { return *base_; }

    template <class U> friend class index_iterator;

public:
    using iterator_concept = std::random_access_iterator_tag;
    // [iterator.traits]: a Cpp17RandomAccessIterator only when *it is a reference.
    using iterator_category =
        std::conditional_t<std::is_reference_v<range_reference_t<Base>>, std::random_access_iterator_tag, std::input_iterator_tag>;
    using value_type = range_value_t<Base>;
    using difference_type = range_difference_t<Base>;

    index_iterator() = default;

    constexpr index_iterator(Base& base, difference_type pos)
        : base_([&]() -> holder_t {
            if constexpr (index_iterator_holds_copy<Base>) {
                return holder_t(std::in_place, base);
            } else {
                return std::addressof(base);
            }
        }())
        , pos_(pos) {}

    template <class Other>
        requires std::same_as<const Other, Base> && (!std::same_as<Other, Base>)
    constexpr index_iterator(index_iterator<Other> i)
        : base_(std::move(i.base_))
        , pos_(i.pos_) {}

    constexpr difference_type index() const noexcept { return pos_; }

    constexpr decltype(auto) operator*() const { return zr::ranges::element_at(base_ref(), pos_); }
    constexpr decltype(auto) operator[](difference_type n) const { return zr::ranges::element_at(base_ref(), pos_ + n); }

    constexpr index_iterator& operator++() {
        ++pos_;
        return *this;
    }
    constexpr index_iterator operator++(int) {
        auto tmp = *this;
        ++pos_;
        return tmp;
    }
    constexpr index_iterator& operator--() {
        --pos_;
        return *this;
    }
    constexpr index_iterator operator--(int) {
        auto tmp = *this;
        --pos_;
        return tmp;
    }
    constexpr index_iterator& operator+=(difference_type n) {
        pos_ += n;
        return *this;
    }
    constexpr index_iterator& operator-=(difference_type n) {
        pos_ -= n;
        return *this;
    }

    friend constexpr bool operator==(const index_iterator& x, const index_iterator& y) noexcept { return x.pos_ == y.pos_; }
    friend constexpr std::strong_ordering operator<=>(const index_iterator& x, const index_iterator& y) noexcept { return x.pos_ <=> y.pos_; }

    friend constexpr index_iterator operator+(index_iterator i, difference_type n) { return i += n; }
    friend constexpr index_iterator operator+(difference_type n, index_iterator i) { return i += n; }
    friend constexpr index_iterator operator-(index_iterator i, difference_type n) { return i -= n; }
    friend constexpr difference_type operator-(const index_iterator& x, const index_iterator& y) noexcept { return x.pos_ - y.pos_; }
};

// The iterator and sentinel a view iterates R with: index_iterator over an index_iterable R,
// else R's own; indexed_random_access / indexed_decrementable are the gates of that iterator.
template <class R> using indexed_iterator_t = std::conditional_t<index_iterable<R>, index_iterator<R>, iterator_t<R>>;
template <class R> using indexed_sentinel_t = std::conditional_t<index_iterable<R>, index_iterator<R>, sentinel_t<R>>;

template <class R>
concept indexed_random_access = base_random_access<R> || index_iterable<R>;
template <class R>
concept indexed_decrementable = base_decrementable<R> || index_iterable<R>;

struct indexed_begin_fn {
    template <class R> constexpr indexed_iterator_t<R> operator()(R& r) const {
        if constexpr (index_iterable<R>) {
            return index_iterator<R>(r, 0);
        } else {
            return zr::ranges::begin(r);
        }
    }
};

struct indexed_end_fn {
    template <class R> constexpr indexed_sentinel_t<R> operator()(R& r) const {
        if constexpr (index_iterable<R>) {
            return index_iterator<R>(r, static_cast<range_difference_t<R>>(zr::ranges::size(r)));
        } else {
            return zr::ranges::end(r);
        }
    }
};

inline constexpr indexed_begin_fn indexed_begin{};
inline constexpr indexed_end_fn indexed_end{};

template <class I>
concept has_cpp17_category = requires { typename std::iterator_traits<I>::iterator_category; };

template <class I> using cpp17_category_t = typename std::iterator_traits<I>::iterator_category;

// Holder of the member typedef-name iterator_category, declared iff Present.
template <bool Present, class Category = void> struct iterator_category_base {};
template <class Category> struct iterator_category_base<true, Category> {
    using iterator_category = Category;
};

} // namespace detail

ZR_END_NAMESPACE_RANGES
