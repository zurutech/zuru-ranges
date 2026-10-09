// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/iterator/indirect_requirements.h"

#include <compare>
#include <concepts>
#include <iterator>
#include <type_traits>
#include <utility>

/**
 * @file counted_iterator.h
 *
 * @brief [counted.iterator]: detail::counted_iterator_t<I>, std::counted_iterator<I> for a
 *        std::input_or_output_iterator I.
 *
 * DEVIATION: otherwise detail::relaxed_counted_iterator<I> (Unreal iterators are not
 * std::input_or_output_iterator): see its note.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

template <class I> struct counted_iterator_category {};
template <class I>
    requires requires { typename I::iterator_category; }
struct counted_iterator_category<I> {
    using iterator_category = typename I::iterator_category;
};

// copy_constructible and relaxed equality: the multi-pass notion of subrange_category.
template <class I>
concept relaxed_forward_iterator = std::copy_constructible<I> && relaxed_equality_comparable<I>;

// [counted.iterator] over an iterator that is not a std::input_or_output_iterator.
// DEVIATION: iterator_concept is derived (Unreal iterators declare none): bidirectional for a
// relaxed_forward_iterator with prefix --, forward for a relaxed_forward_iterator, else input.
// The members the draft gates on forward_iterator<I> / bidirectional_iterator<I> require
// relaxed_forward_iterator<I> / prefix_decrementable<I>; the random-access members, operator->
// (contiguous_iterator<I>) and value_type's indirectly_readable<I> condition do not apply.
template <class I> class relaxed_counted_iterator : public counted_iterator_category<I> {
private:
    template <class I2> friend class relaxed_counted_iterator;

    I current_ = I();
    iter_difference_t<I> length_ = 0;

    static constexpr auto concept_tag() {
        if constexpr (relaxed_forward_iterator<I> && prefix_decrementable<I>) {
            return std::bidirectional_iterator_tag{};
        } else if constexpr (relaxed_forward_iterator<I>) {
            return std::forward_iterator_tag{};
        } else {
            return std::input_iterator_tag{};
        }
    }

public:
    using iterator_type = I;
    using value_type = iter_value_t<I>;
    using difference_type = iter_difference_t<I>;
    using iterator_concept = decltype(concept_tag());

    relaxed_counted_iterator()
        requires std::default_initializable<I>
    = default;

    constexpr relaxed_counted_iterator(I x, iter_difference_t<I> n)
        : current_(std::move(x))
        , length_(n) {}

    template <class I2>
        requires std::convertible_to<const I2&, I>
    constexpr relaxed_counted_iterator(const relaxed_counted_iterator<I2>& x)
        : current_(x.current_)
        , length_(x.length_) {}

    template <class I2>
        requires std::assignable_from<I&, const I2&>
    constexpr relaxed_counted_iterator& operator=(const relaxed_counted_iterator<I2>& x) {
        current_ = x.current_;
        length_ = x.length_;
        return *this;
    }

    constexpr const I& base() const& noexcept { return current_; }
    constexpr I base() && { return std::move(current_); }
    constexpr iter_difference_t<I> count() const noexcept { return length_; }

    constexpr decltype(auto) operator*() { return *current_; }
    constexpr decltype(auto) operator*() const
        requires dereferenceable<const I>
    {
        return *current_;
    }

    constexpr relaxed_counted_iterator& operator++() {
        ++current_;
        --length_;
        return *this;
    }
    constexpr decltype(auto) operator++(int)
        requires(!relaxed_forward_iterator<I>)
    {
        --length_;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
        try {
            return current_++;
        } catch (...) {
            ++length_;
            throw;
        }
#else
        return current_++;
#endif
    }
    constexpr relaxed_counted_iterator operator++(int)
        requires relaxed_forward_iterator<I>
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    constexpr relaxed_counted_iterator& operator--()
        requires prefix_decrementable<I>
    {
        --current_;
        ++length_;
        return *this;
    }
    constexpr relaxed_counted_iterator operator--(int)
        requires prefix_decrementable<I> && relaxed_forward_iterator<I>
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    template <std::common_with<I> I2>
    friend constexpr iter_difference_t<I2> operator-(const relaxed_counted_iterator& x, const relaxed_counted_iterator<I2>& y) {
        return y.length_ - x.length_;
    }
    friend constexpr iter_difference_t<I> operator-(const relaxed_counted_iterator& x, std::default_sentinel_t) noexcept { return -x.length_; }
    friend constexpr iter_difference_t<I> operator-(std::default_sentinel_t, const relaxed_counted_iterator& y) noexcept { return y.length_; }

    template <std::common_with<I> I2>
    friend constexpr bool operator==(const relaxed_counted_iterator& x, const relaxed_counted_iterator<I2>& y) {
        return x.length_ == y.length_;
    }
    friend constexpr bool operator==(const relaxed_counted_iterator& x, std::default_sentinel_t) noexcept { return x.length_ == 0; }

    template <std::common_with<I> I2>
    friend constexpr std::strong_ordering operator<=>(const relaxed_counted_iterator& x, const relaxed_counted_iterator<I2>& y) {
        return y.length_ <=> x.length_;
    }

    friend constexpr decltype(auto) iter_move(const relaxed_counted_iterator& i) noexcept(noexcept(detail::relaxed_iter_move(i.current_)))
        requires relaxed_input_iterator<I>
    {
        return detail::relaxed_iter_move(i.current_);
    }

    template <class I2>
        requires indirectly_swappable<I2, I>
    friend constexpr void iter_swap(const relaxed_counted_iterator& x, const relaxed_counted_iterator<I2>& y)
        noexcept(noexcept(detail::relaxed_iter_swap(x.current_, y.current_))) {
        detail::relaxed_iter_swap(x.current_, y.current_);
    }
};

template <class I> struct counted_iterator_for {
    using type = relaxed_counted_iterator<I>;
};
template <std::input_or_output_iterator I> struct counted_iterator_for<I> {
    using type = std::counted_iterator<I>;
};

// counted_iterator<I> (see file note).
template <class I> using counted_iterator_t = typename counted_iterator_for<I>::type;

} // namespace detail

ZR_END_NAMESPACE_RANGES
