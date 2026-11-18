// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/concepts.h"
#include "zr/iterator/counted_iterator.h"
#include "zr/iterator/indirect_requirements.h"

#include <concepts>
#include <iterator>
#include <type_traits>
#include <utility>

/**
 * @file move_iterator.h
 *
 * @brief [move.iterators], [move.sentinel]: detail::move_iterator_t<I>, std::move_iterator<I> for
 *        a std::input_iterator I, and detail::move_sentinel_t<S, I>, std::move_sentinel<S> for a
 *        std::sentinel_for<I> S of such an I.
 *
 * DEVIATION: otherwise detail::relaxed_move_iterator<I> and detail::relaxed_move_sentinel<S>
 * (Unreal iterators are not std::input_iterator, and need not be semiregular): see their notes.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

template <class I> struct move_iterator_category {};
template <class I>
    requires requires { typename I::iterator_category; }
struct move_iterator_category<I> {
    using iterator_category = typename I::iterator_category;
};

// [move.iterators] over an iterator that is not a std::input_iterator.
// DEVIATION: iterator_concept is derived as relaxed_counted_iterator's (Unreal iterators declare
// none). The members the draft gates on forward_iterator<I> / bidirectional_iterator<I> require
// relaxed_forward_iterator<I> / prefix_decrementable<I>; the random-access members and the
// ordering operators do not apply. `==` is detail::iter_equal, operator* and iter_move
// detail::relaxed_iter_move.
template <class I> class relaxed_move_iterator : public move_iterator_category<I> {
private:
    I current_ = I();

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
    using iterator_concept = decltype(concept_tag());
    using value_type = iter_value_t<I>;
    using difference_type = iter_difference_t<I>;
    using pointer = I;
    using reference = iter_rvalue_reference_t<I>;

    relaxed_move_iterator()
        requires std::default_initializable<I>
    = default;

    constexpr explicit relaxed_move_iterator(I i)
        : current_(std::move(i)) {}

    template <class U>
        requires(!std::same_as<U, I>) && std::convertible_to<const U&, I>
    constexpr relaxed_move_iterator(const relaxed_move_iterator<U>& u)
        : current_(u.base()) {}

    template <class U>
        requires(!std::same_as<U, I>) && std::convertible_to<const U&, I> && std::assignable_from<I&, const U&>
    constexpr relaxed_move_iterator& operator=(const relaxed_move_iterator<U>& u) {
        current_ = u.base();
        return *this;
    }

    constexpr const I& base() const& noexcept { return current_; }
    constexpr I base() && { return std::move(current_); }

    constexpr reference operator*() const { return detail::relaxed_iter_move(current_); }

    constexpr relaxed_move_iterator& operator++() {
        ++current_;
        return *this;
    }
    constexpr void operator++(int)
        requires(!relaxed_forward_iterator<I>)
    {
        ++current_;
    }
    constexpr relaxed_move_iterator operator++(int)
        requires relaxed_forward_iterator<I>
    {
        auto tmp = *this;
        ++current_;
        return tmp;
    }

    constexpr relaxed_move_iterator& operator--()
        requires prefix_decrementable<I>
    {
        --current_;
        return *this;
    }
    constexpr relaxed_move_iterator operator--(int)
        requires prefix_decrementable<I> && relaxed_forward_iterator<I>
    {
        auto tmp = *this;
        --current_;
        return tmp;
    }

    template <class I2>
        requires relaxed_equality_comparable_with<I, I2>
    friend constexpr bool operator==(const relaxed_move_iterator& x, const relaxed_move_iterator<I2>& y) {
        return detail::iter_equal(x.current_, y.base());
    }

    // The return type names I and I2, not x.base(): the class is incomplete here, and Clang
    // before 19 rejects the member access when it instantiates the class.
    template <class I2>
        requires std::sized_sentinel_for<I, I2>
    friend constexpr auto operator-(const relaxed_move_iterator& x, const relaxed_move_iterator<I2>& y)
        -> decltype(std::declval<const I&>() - std::declval<const I2&>()) {
        return x.current_ - y.base();
    }

    friend constexpr reference iter_move(const relaxed_move_iterator& i) noexcept(noexcept(detail::relaxed_iter_move(i.current_))) {
        return detail::relaxed_iter_move(i.current_);
    }

    template <class I2>
        requires indirectly_swappable<I2, I>
    friend constexpr void iter_swap(const relaxed_move_iterator& x, const relaxed_move_iterator<I2>& y)
        noexcept(noexcept(detail::relaxed_iter_swap(x.current_, y.base()))) {
        detail::relaxed_iter_swap(x.current_, y.base());
    }
};

template <class T> inline constexpr bool is_move_iterator = false;
template <class I> inline constexpr bool is_move_iterator<std::move_iterator<I>> = true;
template <class I> inline constexpr bool is_move_iterator<relaxed_move_iterator<I>> = true;

// [move.sentinel] for a sentinel that is not a std::sentinel_for of a std::input_iterator.
// DEVIATION: no semiregular<S> (the sentinel of an Unreal range is an Unreal iterator); its
// operators, hidden friends of move_iterator in the draft, are declared here, over
// std::move_iterator and relaxed_move_iterator, and require zr's sentinel_for.
template <class S> class relaxed_move_sentinel {
private:
    S last_ = S();

public:
    relaxed_move_sentinel()
        requires std::default_initializable<S>
    = default;

    constexpr explicit relaxed_move_sentinel(S s)
        : last_(std::move(s)) {}

    template <class S2>
        requires std::convertible_to<const S2&, S>
    constexpr relaxed_move_sentinel(const relaxed_move_sentinel<S2>& s)
        : last_(s.base()) {}

    template <class S2>
        requires std::assignable_from<S&, const S2&>
    constexpr relaxed_move_sentinel& operator=(const relaxed_move_sentinel<S2>& s) {
        last_ = s.base();
        return *this;
    }

    constexpr S base() const { return last_; }

    template <class MI>
        requires is_move_iterator<MI> && sentinel_for<S, typename MI::iterator_type>
    friend constexpr bool operator==(const MI& x, const relaxed_move_sentinel& y) {
        return detail::iter_equal(x.base(), y.last_);
    }

    template <class MI>
        requires is_move_iterator<MI> && std::sized_sentinel_for<S, typename MI::iterator_type>
    friend constexpr iter_difference_t<typename MI::iterator_type> operator-(const relaxed_move_sentinel& x, const MI& y) {
        return x.last_ - y.base();
    }
    template <class MI>
        requires is_move_iterator<MI> && std::sized_sentinel_for<S, typename MI::iterator_type>
    friend constexpr iter_difference_t<typename MI::iterator_type> operator-(const MI& x, const relaxed_move_sentinel& y) {
        return x.base() - y.last_;
    }
};

template <class I> struct move_iterator_for {
    using type = relaxed_move_iterator<I>;
};
template <std::input_iterator I> struct move_iterator_for<I> {
    using type = std::move_iterator<I>;
};

template <class S, class I> struct move_sentinel_for {
    using type = relaxed_move_sentinel<S>;
};
template <class S, std::input_iterator I>
    requires std::sentinel_for<S, I>
struct move_sentinel_for<S, I> {
    using type = std::move_sentinel<S>;
};

// move_iterator<I>, and move_sentinel<S> for the sentinel S of an I (see file note).
template <class I> using move_iterator_t = typename move_iterator_for<I>::type;
template <class S, class I> using move_sentinel_t = typename move_sentinel_for<S, I>::type;

} // namespace detail

ZR_END_NAMESPACE_RANGES
