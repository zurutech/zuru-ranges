// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"

#include <compare>
#include <concepts>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

/**
 * @file const_iterator.h
 *
 * @brief [const.iterators] basic_const_iterator, const_iterator, const_sentinel,
 *        iter_const_reference_t, make_const_iterator, make_const_sentinel.
 *
 * DEVIATION: Iterator need only model detail::relaxed_input_iterator (Unreal); the
 * operations the draft gates on forward/bidirectional/random-access iterators keep the std
 * concepts, and `current_ == s` is detail::iter_equal. iter_const_reference_t reads the
 * relaxed iter_value_t. const_sentinel<S> drops semiregular<S>.
 */

ZR_BEGIN_NAMESPACE_RANGES

// [const.iterators.alias]
template <dereferenceable It> using iter_const_reference_t = std::common_reference_t<const iter_value_t<It>&&, iter_reference_t<It>>;

namespace detail {

// [const.iterators.alias] constant-iterator
template <class It>
concept constant_iterator = relaxed_input_iterator<It> && std::same_as<iter_const_reference_t<It>, iter_reference_t<It>>;

} // namespace detail

template <class Iterator>
    requires detail::relaxed_input_iterator<Iterator>
class basic_const_iterator;

namespace detail {

template <class I> inline constexpr bool is_basic_const_iterator = false;
template <class I> inline constexpr bool is_basic_const_iterator<basic_const_iterator<I>> = true;

// [const.iterators.iterator] not-a-const-iterator
template <class I>
concept not_a_const_iterator = !is_basic_const_iterator<std::remove_cv_t<I>>;

// [const.iterators.iterator] iter-const-rvalue-reference-t
template <dereferenceable I> using iter_const_rvalue_reference_t = std::common_reference_t<const iter_value_t<I>&&, iter_rvalue_reference_t<I>>;

template <class I> struct const_iterator_impl {
    using type = basic_const_iterator<I>;
};
template <class I>
    requires constant_iterator<I>
struct const_iterator_impl<I> {
    using type = I;
};

template <class S> struct const_sentinel_impl {
    using type = S;
};
template <class S>
    requires relaxed_input_iterator<S>
struct const_sentinel_impl<S> {
    using type = typename const_iterator_impl<S>::type;
};

template <bool Present, class I> struct const_iterator_category {};
template <class I> struct const_iterator_category<true, I> {
    using iterator_category = typename std::iterator_traits<I>::iterator_category;
};

} // namespace detail

template <detail::relaxed_input_iterator I> using const_iterator = typename detail::const_iterator_impl<I>::type;
template <class S> using const_sentinel = typename detail::const_sentinel_impl<S>::type;

// [const.iterators.iterator]
template <class Iterator>
    requires detail::relaxed_input_iterator<Iterator>
class basic_const_iterator : public detail::const_iterator_category<std::forward_iterator<Iterator>, Iterator> {
    template <class U>
        requires detail::relaxed_input_iterator<U>
    friend class basic_const_iterator;

    Iterator current_ = Iterator();
    using reference = iter_const_reference_t<Iterator>;
    using rvalue_reference = detail::iter_const_rvalue_reference_t<Iterator>;

    static constexpr auto concept_tag() {
        if constexpr (std::contiguous_iterator<Iterator>) {
            return std::contiguous_iterator_tag{};
        } else if constexpr (std::random_access_iterator<Iterator>) {
            return std::random_access_iterator_tag{};
        } else if constexpr (std::bidirectional_iterator<Iterator>) {
            return std::bidirectional_iterator_tag{};
        } else if constexpr (std::forward_iterator<Iterator>) {
            return std::forward_iterator_tag{};
        } else {
            return std::input_iterator_tag{};
        }
    }

public:
    using iterator_type = Iterator;
    using iterator_concept = decltype(concept_tag());
    using value_type = iter_value_t<Iterator>;
    using difference_type = iter_difference_t<Iterator>;

    basic_const_iterator()
        requires std::default_initializable<Iterator>
    = default;
    constexpr basic_const_iterator(Iterator current)
        : current_(std::move(current)) {}
    template <std::convertible_to<Iterator> U>
    constexpr basic_const_iterator(basic_const_iterator<U> current)
        : current_(std::move(current.current_)) {}
    template <detail::different_from<basic_const_iterator> T>
        requires std::convertible_to<T, Iterator>
    constexpr basic_const_iterator(T&& current)
        : current_(std::forward<T>(current)) {}

    constexpr const Iterator& base() const& noexcept { return current_; }
    constexpr Iterator base() && { return std::move(current_); }

    constexpr reference operator*() const { return static_cast<reference>(detail::deref(current_)); }

    constexpr const auto* operator->() const
        requires std::is_lvalue_reference_v<iter_reference_t<Iterator>> && std::same_as<std::remove_cvref_t<iter_reference_t<Iterator>>, value_type>
    {
        if constexpr (std::contiguous_iterator<Iterator>) {
            return std::to_address(current_);
        } else {
            return std::addressof(*current_);
        }
    }

    constexpr basic_const_iterator& operator++() {
        ++current_;
        return *this;
    }
    constexpr void operator++(int) { ++current_; }
    // [over.inc.default] (C++26) written out.
    constexpr basic_const_iterator operator++(int)
        requires std::forward_iterator<Iterator>
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr basic_const_iterator& operator--()
        requires std::bidirectional_iterator<Iterator>
    {
        --current_;
        return *this;
    }
    constexpr basic_const_iterator operator--(int)
        requires std::bidirectional_iterator<Iterator>
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    constexpr basic_const_iterator& operator+=(difference_type n)
        requires std::random_access_iterator<Iterator>
    {
        current_ += n;
        return *this;
    }
    constexpr basic_const_iterator& operator-=(difference_type n)
        requires std::random_access_iterator<Iterator>
    {
        current_ -= n;
        return *this;
    }

    constexpr reference operator[](difference_type n) const
        requires std::random_access_iterator<Iterator>
    {
        return static_cast<reference>(current_[n]);
    }

    template <sentinel_for<Iterator> S> constexpr bool operator==(const S& s) const { return detail::iter_equal(current_, s); }

    template <detail::not_a_const_iterator CI>
        requires detail::constant_iterator<CI> && std::convertible_to<const Iterator&, CI>
    constexpr operator CI() const& {
        return current_;
    }
    template <detail::not_a_const_iterator CI>
        requires detail::constant_iterator<CI> && std::convertible_to<Iterator, CI>
    constexpr operator CI() && {
        return std::move(current_);
    }

    constexpr bool operator<(const basic_const_iterator& y) const
        requires std::random_access_iterator<Iterator>
    {
        return current_ < y.current_;
    }
    constexpr bool operator>(const basic_const_iterator& y) const
        requires std::random_access_iterator<Iterator>
    {
        return current_ > y.current_;
    }
    constexpr bool operator<=(const basic_const_iterator& y) const
        requires std::random_access_iterator<Iterator>
    {
        return current_ <= y.current_;
    }
    constexpr bool operator>=(const basic_const_iterator& y) const
        requires std::random_access_iterator<Iterator>
    {
        return current_ >= y.current_;
    }
    constexpr auto operator<=>(const basic_const_iterator& y) const
        requires std::random_access_iterator<Iterator> && std::three_way_comparable<Iterator>
    {
        return current_ <=> y.current_;
    }

    template <detail::different_from<basic_const_iterator> I>
    constexpr bool operator<(const I& y) const
        requires std::random_access_iterator<Iterator> && std::totally_ordered_with<Iterator, I>
    {
        return current_ < y;
    }
    template <detail::different_from<basic_const_iterator> I>
    constexpr bool operator>(const I& y) const
        requires std::random_access_iterator<Iterator> && std::totally_ordered_with<Iterator, I>
    {
        return current_ > y;
    }
    template <detail::different_from<basic_const_iterator> I>
    constexpr bool operator<=(const I& y) const
        requires std::random_access_iterator<Iterator> && std::totally_ordered_with<Iterator, I>
    {
        return current_ <= y;
    }
    template <detail::different_from<basic_const_iterator> I>
    constexpr bool operator>=(const I& y) const
        requires std::random_access_iterator<Iterator> && std::totally_ordered_with<Iterator, I>
    {
        return current_ >= y;
    }
    template <detail::different_from<basic_const_iterator> I>
    constexpr auto operator<=>(const I& y) const
        requires std::random_access_iterator<Iterator> && std::totally_ordered_with<Iterator, I> && std::three_way_comparable_with<Iterator, I>
    {
        return current_ <=> y;
    }

    template <detail::not_a_const_iterator I, std::same_as<Iterator> J>
    friend constexpr bool operator<(const I& x, const basic_const_iterator<J>& y)
        requires std::random_access_iterator<Iterator> && std::totally_ordered_with<Iterator, I>
    {
        return x < y.current_;
    }
    template <detail::not_a_const_iterator I, std::same_as<Iterator> J>
    friend constexpr bool operator>(const I& x, const basic_const_iterator<J>& y)
        requires std::random_access_iterator<Iterator> && std::totally_ordered_with<Iterator, I>
    {
        return x > y.current_;
    }
    template <detail::not_a_const_iterator I, std::same_as<Iterator> J>
    friend constexpr bool operator<=(const I& x, const basic_const_iterator<J>& y)
        requires std::random_access_iterator<Iterator> && std::totally_ordered_with<Iterator, I>
    {
        return x <= y.current_;
    }
    template <detail::not_a_const_iterator I, std::same_as<Iterator> J>
    friend constexpr bool operator>=(const I& x, const basic_const_iterator<J>& y)
        requires std::random_access_iterator<Iterator> && std::totally_ordered_with<Iterator, I>
    {
        return x >= y.current_;
    }

    friend constexpr basic_const_iterator operator+(const basic_const_iterator& i, difference_type n)
        requires std::random_access_iterator<Iterator>
    {
        return basic_const_iterator(i.current_ + n);
    }
    friend constexpr basic_const_iterator operator+(difference_type n, const basic_const_iterator& i)
        requires std::random_access_iterator<Iterator>
    {
        return basic_const_iterator(i.current_ + n);
    }
    friend constexpr basic_const_iterator operator-(const basic_const_iterator& i, difference_type n)
        requires std::random_access_iterator<Iterator>
    {
        return basic_const_iterator(i.current_ - n);
    }
    template <std::sized_sentinel_for<Iterator> S> constexpr difference_type operator-(const S& y) const { return current_ - y; }
    template <detail::not_a_const_iterator S>
        requires std::sized_sentinel_for<S, Iterator>
    friend constexpr difference_type operator-(const S& x, const basic_const_iterator& y) {
        return x - y.current_;
    }

    friend constexpr rvalue_reference iter_move(const basic_const_iterator& i) noexcept(
        noexcept(static_cast<rvalue_reference>(detail::relaxed_iter_move(i.current_)))) {
        return static_cast<rvalue_reference>(detail::relaxed_iter_move(i.current_));
    }
};

// [iterator.synopsis] make_const_iterator, make_const_sentinel
template <detail::relaxed_input_iterator I> constexpr const_iterator<I> make_const_iterator(I it) { return it; }
template <class S> constexpr const_sentinel<S> make_const_sentinel(S s) { return s; }

ZR_END_NAMESPACE_RANGES
