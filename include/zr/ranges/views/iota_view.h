// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/iterator/operations.h"
#include "zr/ranges/range_traits.h"
#include "zr/ranges/view_interface.h"

#include <compare>
#include <concepts>
#include <iterator>
#include <type_traits>
#include <utility>

/**
 * @file iota_view.h
 *
 * @brief [range.iota] iota_view, views::iota, views::indices (C++26).
 *
 * The draft's defaulted postfix operators ([over.inc.default], C++26) are written out.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// [concept.equalitycomparable] weakly-equality-comparable-with (values, not Unreal iterators).
template <class T, class U>
concept weakly_equality_comparable_with = requires(const std::remove_reference_t<T>& t, const std::remove_reference_t<U>& u) {
    { t == u } -> boolean_testable;
    { t != u } -> boolean_testable;
    { u == t } -> boolean_testable;
    { u != t } -> boolean_testable;
};

#if defined(__SIZEOF_INT128__)
__extension__ using int128 = __int128;

// Whether the standard library accepts __int128 as signed-integer-like.
struct int128_difference_probe {
    using difference_type = int128;
    int128_difference_probe& operator++();
    int128_difference_probe operator++(int);
};
inline constexpr bool std_accepts_int128 = std::weakly_incrementable<int128_difference_probe>;
#else
inline constexpr bool std_accepts_int128 = false;
#endif

// [range.iota.view]/1 IOTA-DIFF-T(W).
template <class W> constexpr auto iota_diff_type() {
    if constexpr (!std::integral<W> || sizeof(std::iter_difference_t<W>) > sizeof(W)) {
        return std::type_identity<std::iter_difference_t<W>>{};
    } else if constexpr (sizeof(short) > sizeof(W)) {
        return std::type_identity<short>{};
    } else if constexpr (sizeof(int) > sizeof(W)) {
        return std::type_identity<int>{};
    } else if constexpr (sizeof(long) > sizeof(W)) {
        return std::type_identity<long>{};
    } else if constexpr (sizeof(long long) > sizeof(W)) {
        return std::type_identity<long long>{};
    }
#if defined(__SIZEOF_INT128__)
    else if constexpr (std_accepts_int128) {
        // "a signed-integer-like type with width not less than the width of W".
        return std::type_identity<int128>{};
    }
#endif
    else if constexpr (std::signed_integral<W>) {
        // W's own signed type has exactly W's width, which the draft allows.
        return std::type_identity<long long>{};
    } else {
        // Without __int128 (MSVC, strict libstdc++): long long, of W's width, as permitted;
        // distances >= 2^63 overflow.
        return std::type_identity<long long>{};
    }
}

template <class W> using iota_diff_t = typename decltype(iota_diff_type<W>())::type;

// [range.iota.view]/2
template <class I>
concept decrementable = std::incrementable<I> && requires(I i) {
    { --i } -> std::same_as<I&>;
    { i-- } -> std::same_as<I>;
};

// [range.iota.view]/4
template <class I>
concept advanceable = decrementable<I> && std::totally_ordered<I> && requires(I i, const I j, const iota_diff_t<I> n) {
    { i += n } -> std::same_as<I&>;
    { i -= n } -> std::same_as<I&>;
    I(j + n);
    I(n + j);
    I(j - n);
    { j - j } -> std::convertible_to<iota_diff_t<I>>;
};

// [range.iota.iterator]: iterator_category is present only if W models incrementable and
// IOTA-DIFF-T(W) is an integral type.
template <class W> struct iota_iterator_category {};

template <class W>
    requires std::incrementable<W> && std::integral<iota_diff_t<W>>
struct iota_iterator_category<W> {
    using iterator_category = std::input_iterator_tag;
};

template <class W> constexpr auto iota_iterator_concept() {
    if constexpr (advanceable<W>) {
        return std::random_access_iterator_tag{};
    } else if constexpr (decrementable<W>) {
        return std::bidirectional_iterator_tag{};
    } else if constexpr (std::incrementable<W>) {
        return std::forward_iterator_tag{};
    } else {
        return std::input_iterator_tag{};
    }
}

} // namespace detail

// [range.iota.view]
template <std::weakly_incrementable W, std::semiregular Bound = std::unreachable_sentinel_t>
    requires detail::weakly_equality_comparable_with<W, Bound> && std::copyable<W>
class iota_view : public view_interface<iota_view<W, Bound>> {
private:
    class sentinel;

    // [range.iota.iterator]
    class iterator : public detail::iota_iterator_category<W> {
    private:
        friend iota_view;
        friend sentinel;

        W value_ = W();

        constexpr explicit iterator(W value)
            : value_(value) {}

    public:
        using iterator_concept = decltype(detail::iota_iterator_concept<W>());
        using value_type = W;
        using difference_type = detail::iota_diff_t<W>;

        iterator()
            requires std::default_initializable<W>
        = default;

        constexpr W operator*() const noexcept(std::is_nothrow_copy_constructible_v<W>) { return value_; }

        constexpr iterator& operator++() {
            ++value_;
            return *this;
        }
        constexpr void operator++(int) { ++*this; }
        constexpr iterator operator++(int)
            requires std::incrementable<W>
        {
            iterator tmp = *this;
            ++*this;
            return tmp;
        }

        constexpr iterator& operator--()
            requires detail::decrementable<W>
        {
            --value_;
            return *this;
        }
        constexpr iterator operator--(int)
            requires detail::decrementable<W>
        {
            iterator tmp = *this;
            --*this;
            return tmp;
        }

        constexpr iterator& operator+=(difference_type n)
            requires detail::advanceable<W>
        {
            if constexpr (detail::is_integer_like<W> && !detail::is_signed_integer_like<W>) {
                if (n >= difference_type(0)) {
                    value_ += static_cast<W>(n);
                } else {
                    value_ -= static_cast<W>(-n);
                }
            } else if constexpr (detail::is_integer_like<W>) {
                value_ = static_cast<W>(value_ + n); // value_ += n, without the implicit narrowing
            } else {
                value_ += n;
            }
            return *this;
        }

        constexpr iterator& operator-=(difference_type n)
            requires detail::advanceable<W>
        {
            if constexpr (detail::is_integer_like<W> && !detail::is_signed_integer_like<W>) {
                if (n >= difference_type(0)) {
                    value_ -= static_cast<W>(n);
                } else {
                    value_ += static_cast<W>(-n);
                }
            } else if constexpr (detail::is_integer_like<W>) {
                value_ = static_cast<W>(value_ - n); // value_ -= n, without the implicit narrowing
            } else {
                value_ -= n;
            }
            return *this;
        }

        constexpr W operator[](difference_type n) const
            requires detail::advanceable<W>
        {
            return W(value_ + n);
        }

        friend constexpr bool operator==(const iterator& x, const iterator& y)
            requires std::equality_comparable<W>
        {
            return x.value_ == y.value_;
        }

        friend constexpr bool operator<(const iterator& x, const iterator& y)
            requires std::totally_ordered<W>
        {
            return x.value_ < y.value_;
        }
        friend constexpr bool operator>(const iterator& x, const iterator& y)
            requires std::totally_ordered<W>
        {
            return y < x;
        }
        friend constexpr bool operator<=(const iterator& x, const iterator& y)
            requires std::totally_ordered<W>
        {
            return !(y < x);
        }
        friend constexpr bool operator>=(const iterator& x, const iterator& y)
            requires std::totally_ordered<W>
        {
            return !(x < y);
        }
        friend constexpr auto operator<=>(const iterator& x, const iterator& y)
            requires std::totally_ordered<W> && std::three_way_comparable<W>
        {
            return x.value_ <=> y.value_;
        }

        friend constexpr iterator operator+(iterator i, difference_type n)
            requires detail::advanceable<W>
        {
            i += n;
            return i;
        }
        friend constexpr iterator operator+(difference_type n, iterator i)
            requires detail::advanceable<W>
        {
            return i + n;
        }

        friend constexpr iterator operator-(iterator i, difference_type n)
            requires detail::advanceable<W>
        {
            i -= n;
            return i;
        }
        friend constexpr difference_type operator-(const iterator& x, const iterator& y)
            requires detail::advanceable<W>
        {
            using D = difference_type;
            if constexpr (detail::is_integer_like<W>) {
                if constexpr (detail::is_signed_integer_like<W>) {
                    return D(D(x.value_) - D(y.value_));
                } else {
                    return (y.value_ > x.value_) ? D(-D(y.value_ - x.value_)) : D(x.value_ - y.value_);
                }
            } else {
                return x.value_ - y.value_;
            }
        }
    };

    // [range.iota.sentinel]
    class sentinel {
    private:
        friend iota_view;

        Bound bound_ = Bound();

        constexpr explicit sentinel(Bound bound)
            : bound_(bound) {}

        // Members, so they reach iterator::value_ through `friend sentinel`; the hidden friends
        // below are not members of sentinel and do not inherit that friendship.
        constexpr bool equal_(const iterator& x) const { return x.value_ == bound_; }
        constexpr std::iter_difference_t<W> distance_from_(const iterator& x) const { return x.value_ - bound_; }

    public:
        sentinel() = default;

        friend constexpr bool operator==(const iterator& x, const sentinel& y) { return y.equal_(x); }

        friend constexpr std::iter_difference_t<W> operator-(const iterator& x, const sentinel& y)
            requires std::sized_sentinel_for<Bound, W>
        {
            return y.distance_from_(x);
        }
        friend constexpr std::iter_difference_t<W> operator-(const sentinel& x, const iterator& y)
            requires std::sized_sentinel_for<Bound, W>
        {
            return -(y - x);
        }
    };

    // [range.iota.view] constructor from an iterator and the type end() returns.
    using last_type =
        std::conditional_t<std::same_as<W, Bound>, iterator,
                           std::conditional_t<std::same_as<Bound, std::unreachable_sentinel_t>, Bound, sentinel>>;

    W value_ = W();
    Bound bound_ = Bound();

public:
    iota_view()
        requires std::default_initializable<W>
    = default;

    constexpr explicit iota_view(W value)
        : value_(value) {}

    constexpr explicit iota_view(std::type_identity_t<W> value, std::type_identity_t<Bound> bound)
        : value_(value)
        , bound_(bound) {}

    constexpr explicit iota_view(iterator first, last_type last)
        : value_(first.value_) {
        if constexpr (std::same_as<W, Bound>) {
            bound_ = last.value_;
        } else if constexpr (std::same_as<Bound, std::unreachable_sentinel_t>) {
            bound_ = last;
        } else {
            bound_ = last.bound_;
        }
    }

    constexpr iterator begin() const { return iterator{value_}; }

    constexpr auto end() const {
        if constexpr (std::same_as<Bound, std::unreachable_sentinel_t>) {
            return std::unreachable_sentinel;
        } else {
            return sentinel{bound_};
        }
    }

    constexpr iterator end() const
        requires std::same_as<W, Bound>
    {
        return iterator{bound_};
    }

    constexpr bool empty() const { return value_ == bound_; }

    constexpr auto size() const
        requires(std::same_as<W, Bound> && detail::advanceable<W>) || (detail::is_integer_like<W> && detail::is_integer_like<Bound>) ||
                std::sized_sentinel_for<Bound, W>
    {
        if constexpr (detail::is_integer_like<W> && detail::is_integer_like<Bound>) {
            // The draft's expression, split by signedness (no `< 0` on unsigned operands);
            // neg_ negates after to-unsigned-like: no UB for the minimum value.
            using detail::to_unsigned_like;
            // Unary + has the type unary - would have, without negating an unsigned operand.
            using R = std::common_type_t<decltype(to_unsigned_like(+value_) - to_unsigned_like(+bound_)),
                                         decltype(to_unsigned_like(bound_) + to_unsigned_like(+value_)),
                                         decltype(to_unsigned_like(bound_) - to_unsigned_like(value_))>;
            const auto neg_ = [](auto x) { return decltype(to_unsigned_like(+x))(0) - to_unsigned_like(+x); };
            if constexpr (detail::is_signed_integer_like<W>) {
                if (value_ < 0) {
                    if constexpr (detail::is_signed_integer_like<Bound>) {
                        if (bound_ < 0) {
                            return static_cast<R>(neg_(value_) - neg_(bound_));
                        }
                    }
                    return static_cast<R>(to_unsigned_like(bound_) + neg_(value_));
                }
            }
            return static_cast<R>(to_unsigned_like(bound_) - to_unsigned_like(value_));
        } else {
            return detail::to_unsigned_like(bound_ - value_);
        }
    }
};

template <class W, class Bound>
    requires(!detail::is_integer_like<W> || !detail::is_integer_like<Bound> ||
             (detail::is_signed_integer_like<W> == detail::is_signed_integer_like<Bound>))
iota_view(W, Bound) -> iota_view<W, Bound>;

// range_traits: the iterator's category ([range.iota.iterator]/1).
template <class W, class Bound> struct range_traits_impl<iota_view<W, Bound>> {
    static constexpr range_category category = detail::advanceable<W>     ? range_category::random_access
                                               : detail::decrementable<W> ? range_category::bidirectional
                                               : std::incrementable<W>    ? range_category::forward
                                                                          : range_category::input;
};

ZR_END_NAMESPACE_RANGES

namespace std::ranges {

// [ranges.syn] enable_borrowed_range<iota_view<W, Bound>> = true.
template <class W, class Bound> inline constexpr bool enable_borrowed_range<zr::ranges::iota_view<W, Bound>> = true;

} // namespace std::ranges

ZR_BEGIN_NAMESPACE_VIEWS

// [range.iota.overview]: customization point objects.
struct iota_fn {
    template <class W>
        requires requires(W&& value) { iota_view<std::decay_t<W>>(std::forward<W>(value)); }
    constexpr auto operator()(W&& value) const noexcept(noexcept(iota_view<std::decay_t<W>>(std::forward<W>(value)))) {
        return iota_view<std::decay_t<W>>(std::forward<W>(value));
    }

    template <class W, class Bound>
        requires requires(W&& value, Bound&& bound) { iota_view(std::forward<W>(value), std::forward<Bound>(bound)); }
    constexpr auto operator()(W&& value, Bound&& bound) const noexcept(noexcept(iota_view(std::forward<W>(value), std::forward<Bound>(bound)))) {
        return iota_view(std::forward<W>(value), std::forward<Bound>(bound));
    }
};

inline constexpr iota_fn iota{};

// views::indices(E) is views::iota(T(0), E), T = remove_cvref_t<decltype((E))>.
struct indices_fn {
    template <class E>
        requires detail::is_integer_like<std::remove_cvref_t<E>>
    constexpr auto operator()(E&& e) const noexcept(noexcept(iota(std::remove_cvref_t<E>(0), std::forward<E>(e)))) {
        using T = std::remove_cvref_t<E>;
        return iota(T(0), std::forward<E>(e));
    }
};

inline constexpr indices_fn indices{};

ZR_END_NAMESPACE_VIEWS

namespace zr {
namespace views = ranges::views;
}
