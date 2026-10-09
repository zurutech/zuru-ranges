// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/ranges/range_access.h"
#include "zr/ranges/range_adaptor.h"
#include "zr/ranges/range_concepts.h"
#include "zr/ranges/view_interface.h"
#include "zr/ranges/views/all.h"
#include "zr/ranges/views/transform_view.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>

/**
 * @file to.h
 *
 * @brief [range.utility.conv] ranges::to.
 */

ZR_BEGIN_NAMESPACE_RANGES

namespace detail {

// Named concepts, not requires-expressions in `if constexpr`: MSVC < 19.44 evaluates those to
// false.

// Tuple-like of size 2: TPair/TTuple, std::pair, a zip element.
template <class T>
concept pair_like = requires { std::tuple_size<std::remove_cvref_t<T>>::value; } && std::tuple_size<std::remove_cvref_t<T>>::value == 2;

// Element I of a pair-like `ref`, forwarded as std::get<I>(std::forward<Ref>(ref)) would be:
// a reference element stays an lvalue, an object element of an rvalue is moved.
template <std::size_t I, class Ref> constexpr decltype(auto) pair_element(Ref&& ref) {
    using E = std::tuple_element_t<I, std::remove_cvref_t<Ref>>;
    auto& [first, second] = ref;
    if constexpr (I == 0) {
        if constexpr (std::is_lvalue_reference_v<Ref> || std::is_reference_v<E>) {
            return (first);
        } else {
            return std::move(first);
        }
    } else {
        if constexpr (std::is_lvalue_reference_v<Ref> || std::is_reference_v<E>) {
            return (second);
        } else {
            return std::move(second);
        }
    }
}

// [range.utility.conv.general] container-appendable, one concept per container-append branch.
// DEVIATION: UE's AddElement, Add(Key, Value), Add, Emplace, PushLast, AppendChar first.
// AddElement before Add: TChunkedArray::Add(int32 Count) appends Count default elements, which
// Add(ref) would select for an integral element type. Add(Key, Value) before Add: TSortedMap has
// no Add(TPair) and an unconstrained Emplace; TMap's Add(TTuple) hard-errors on std::pair.
template <class C, class Ref>
concept appendable_by_add_element = requires(C& c, Ref&& ref) { c.AddElement(std::forward<Ref>(ref)); };
template <class C, class Ref>
concept appendable_by_add_key_value = pair_like<Ref> && requires(C& c, Ref&& ref) {
    c.Add(pair_element<0>(std::forward<Ref>(ref)), pair_element<1>(std::forward<Ref>(ref)));
};
template <class C, class Ref>
concept appendable_by_add = requires(C& c, Ref&& ref) { c.Add(std::forward<Ref>(ref)); };
template <class C, class Ref>
concept appendable_by_emplace = requires(C& c, Ref&& ref) { c.Emplace(std::forward<Ref>(ref)); };
template <class C, class Ref>
concept appendable_by_push_last = requires(C& c, Ref&& ref) { c.PushLast(std::forward<Ref>(ref)); };
template <class C, class Ref>
concept appendable_by_append_char = requires(C& c, Ref&& ref) { c.AppendChar(std::forward<Ref>(ref)); };
template <class C, class Ref>
concept appendable_by_emplace_back = requires(C& c, Ref&& ref) { c.emplace_back(std::forward<Ref>(ref)); };
template <class C, class Ref>
concept appendable_by_push_back = requires(C& c, Ref&& ref) { c.push_back(std::forward<Ref>(ref)); };
template <class C, class Ref>
concept appendable_by_emplace_hint = requires(C& c, Ref&& ref) { c.emplace_hint(c.end(), std::forward<Ref>(ref)); };
template <class C, class Ref>
concept appendable_by_insert_end = requires(C& c, Ref&& ref) { c.insert(c.end(), std::forward<Ref>(ref)); };

template <class C, class Ref>
concept container_appendable = appendable_by_add_element<C, Ref> || appendable_by_add_key_value<C, Ref> || appendable_by_add<C, Ref> || appendable_by_emplace<C, Ref> ||
                               appendable_by_push_last<C, Ref> || appendable_by_append_char<C, Ref> || appendable_by_emplace_back<C, Ref> || appendable_by_push_back<C, Ref> ||
                               appendable_by_emplace_hint<C, Ref> || appendable_by_insert_end<C, Ref>;

// [range.utility.conv.general] container-append
template <class C, class Ref> constexpr void container_append(C& c, Ref&& ref) {
    if constexpr (appendable_by_add_element<C, Ref>) {
        c.AddElement(std::forward<Ref>(ref));
    } else if constexpr (appendable_by_add_key_value<C, Ref>) {
        c.Add(pair_element<0>(std::forward<Ref>(ref)), pair_element<1>(std::forward<Ref>(ref)));
    } else if constexpr (appendable_by_add<C, Ref>) {
        c.Add(std::forward<Ref>(ref));
    } else if constexpr (appendable_by_emplace<C, Ref>) {
        c.Emplace(std::forward<Ref>(ref));
    } else if constexpr (appendable_by_push_last<C, Ref>) {
        c.PushLast(std::forward<Ref>(ref));
    } else if constexpr (appendable_by_append_char<C, Ref>) {
        c.AppendChar(std::forward<Ref>(ref));
    } else if constexpr (appendable_by_emplace_back<C, Ref>) {
        c.emplace_back(std::forward<Ref>(ref));
    } else if constexpr (appendable_by_push_back<C, Ref>) {
        c.push_back(std::forward<Ref>(ref));
    } else if constexpr (appendable_by_emplace_hint<C, Ref>) {
        c.emplace_hint(c.end(), std::forward<Ref>(ref));
    } else {
        c.insert(c.end(), std::forward<Ref>(ref));
    }
}

// [range.utility.conv.general] reservable-container
template <class C>
concept reservable_container = sized_range<C> && requires(C& c, range_size_t<C> n) {
    c.reserve(n);
    { c.capacity() } -> std::same_as<decltype(n)>;
    { c.max_size() } -> std::same_as<decltype(n)>;
};

// DEVIATION: UE containers reserve through Reserve, sized by Num; UE strings by Len.
template <class C>
concept ue_reservable_container = requires(C& c) { c.Reserve(c.Num()); };
template <class C>
concept ue_reservable_string = requires(C& c) { c.Reserve(c.Len()); };

// A hint converted to Reserve's own parameter type, saturated: a range longer than that type
// (int32 for most UE containers, unsigned for TRingBuffer) must not reserve a wrapped count.
struct saturating_size {
    std::uintmax_t value;

    template <std::integral T> constexpr operator T() const noexcept {
        constexpr auto limit = static_cast<std::uintmax_t>(std::numeric_limits<T>::max());
        return value > limit ? std::numeric_limits<T>::max() : static_cast<T>(value);
    }
};

template <class R> constexpr saturating_size hint_of(R& r) {
    const auto hint = zr::ranges::reserve_hint(r);
    if constexpr (std::signed_integral<decltype(hint)>) {
        return {hint < 0 ? std::uintmax_t{0} : static_cast<std::uintmax_t>(hint)};
    } else {
        return {static_cast<std::uintmax_t>(hint)};
    }
}

template <class C, class R> constexpr void reserve_for([[maybe_unused]] C& c, [[maybe_unused]] R& r) {
    if constexpr (approximately_sized_range<R>) {
        if constexpr (ue_reservable_container<C> || ue_reservable_string<C>) {
            c.Reserve(hint_of(r));
        } else if constexpr (reservable_container<C>) {
            c.reserve(static_cast<range_size_t<C>>(zr::ranges::reserve_hint(r)));
        }
    }
}

// [range.utility.conv.to]: "iterator_traits<iterator_t<R>>::iterator_category is valid and
// denotes a type that models derived_from<input_iterator_tag>".
template <class I>
concept cpp17_input_category = requires { typename std::iterator_traits<I>::iterator_category; } &&
                               std::derived_from<typename std::iterator_traits<I>::iterator_category, std::input_iterator_tag>;

#if defined(__cpp_lib_containers_ranges)
template <class C, class R, class... Args>
concept from_range_constructible = std::constructible_from<C, std::from_range_t, R, Args...>;
#else
template <class C, class R, class... Args>
concept from_range_constructible = false;
#endif

// [range.utility.conv.to] no-recursion condition.
// DEVIATION: also when C appends a pair-like element through Add(Key, Value) (TMap's value
// type TPair is not constructible from std::pair).
template <class C, class R>
concept to_direct = !input_range<C> || std::convertible_to<range_reference_t<R>, range_value_t<C>> ||
                    appendable_by_add_key_value<C, range_reference_t<R>>;

} // namespace detail

// [range.utility.conv.to]
template <class C, input_range R, class... Args>
    requires(!view<C>)
constexpr C to(R&& r, Args&&... args) {
    static_assert(std::is_class_v<C> && std::same_as<C, std::remove_cv_t<C>>, "C is a cv-unqualified class type");

    if constexpr (detail::to_direct<C, R>) {
        if constexpr (std::constructible_from<C, R, Args...>) {
            return C(std::forward<R>(r), std::forward<Args>(args)...);
        }
#if defined(__cpp_lib_containers_ranges)
        else if constexpr (detail::from_range_constructible<C, R, Args...>) {
            return C(std::from_range, std::forward<R>(r), std::forward<Args>(args)...);
        }
#endif
        else if constexpr (common_range<R> && detail::cpp17_input_category<iterator_t<R>> &&
                           std::constructible_from<C, iterator_t<R>, sentinel_t<R>, Args...>) {
            return C(zr::ranges::begin(r), zr::ranges::end(r), std::forward<Args>(args)...);
        } else {
            static_assert(std::constructible_from<C, Args...> && detail::container_appendable<C, range_reference_t<R>>,
                          "ranges::to: no constructor from R and C is not container-appendable");
            C c(std::forward<Args>(args)...);
            detail::reserve_for(c, r);
            auto it = zr::ranges::begin(r);
            const auto last = zr::ranges::end(r);
            for (; !detail::iter_equal(it, last); ++it) {
                detail::container_append(c, *it);
            }
            return c;
        }
    } else {
        static_assert(input_range<range_reference_t<R>>, "ranges::to: range_reference_t<R> is not an input range");
        return zr::ranges::to<C>(ref_view(r) | views::transform([](auto&& elem) {
                                      return zr::ranges::to<range_value_t<C>>(std::forward<decltype(elem)>(elem));
                                  }),
                                  std::forward<Args>(args)...);
    }
}

namespace detail {

// [range.utility.conv.to] input-iterator
template <class R> struct to_input_iterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = range_value_t<R>;
    using difference_type = std::ptrdiff_t;
    using pointer = std::add_pointer_t<range_reference_t<R>>;
    using reference = range_reference_t<R>;
    reference operator*() const;
    pointer operator->() const;
    to_input_iterator& operator++();
    to_input_iterator operator++(int);
    bool operator==(const to_input_iterator&) const;
};

template <template <class...> class C, class R, class... Args>
concept deduce_from_range = requires { C(std::declval<R>(), std::declval<Args>()...); };
#if defined(__cpp_lib_containers_ranges)
template <template <class...> class C, class R, class... Args>
concept deduce_from_range_t = requires { C(std::from_range, std::declval<R>(), std::declval<Args>()...); };
#endif
template <template <class...> class C, class R, class... Args>
concept deduce_from_iterators = requires { C(std::declval<to_input_iterator<R>>(), std::declval<to_input_iterator<R>>(), std::declval<Args>()...); };

// [range.utility.conv.to] DEDUCE_EXPR
template <template <class...> class C, class R, class... Args> auto deduce_to() {
    if constexpr (deduce_from_range<C, R, Args...>) {
        return std::type_identity<decltype(C(std::declval<R>(), std::declval<Args>()...))>{};
    }
#if defined(__cpp_lib_containers_ranges)
    else if constexpr (deduce_from_range_t<C, R, Args...>) {
        return std::type_identity<decltype(C(std::from_range, std::declval<R>(), std::declval<Args>()...))>{};
    }
#endif
    else if constexpr (deduce_from_iterators<C, R, Args...>) {
        return std::type_identity<
            decltype(C(std::declval<to_input_iterator<R>>(), std::declval<to_input_iterator<R>>(), std::declval<Args>()...))>{};
    }
}

template <template <class...> class C, class R, class... Args>
concept deducible_to = !std::same_as<decltype(deduce_to<C, R, Args...>()), void>;

} // namespace detail

template <template <class...> class C, input_range R, class... Args> constexpr auto to(R&& r, Args&&... args) {
    static_assert(detail::deducible_to<C, R, Args...>, "ranges::to: DEDUCE_EXPR is ill-formed");
    using container = typename decltype(detail::deduce_to<C, R, Args...>())::type;
    return zr::ranges::to<container>(std::forward<R>(r), std::forward<Args>(args)...);
}

namespace detail {

// Declared return types: the closure's invocable checks must not instantiate to's body.
template <class C> struct to_fn {
    template <input_range R, class... Args> constexpr C operator()(R&& r, Args&&... args) const {
        return zr::ranges::to<C>(std::forward<R>(r), std::forward<Args>(args)...);
    }
};

template <template <class...> class C> struct to_template_fn {
    template <input_range R, class... Args>
        requires detail::deducible_to<C, R, Args...>
    constexpr typename decltype(detail::deduce_to<C, R, Args...>())::type operator()(R&& r, Args&&... args) const {
        return zr::ranges::to<C>(std::forward<R>(r), std::forward<Args>(args)...);
    }
};

} // namespace detail

// [range.utility.conv.adaptors]
template <class C, class... Args>
    requires(!view<C>)
constexpr auto to(Args&&... args) {
    static_assert(std::is_class_v<C> && std::same_as<C, std::remove_cv_t<C>>, "C is a cv-unqualified class type");
    return detail::make_bound_closure(detail::to_fn<C>{}, std::forward<Args>(args)...);
}

template <template <class...> class C, class... Args> constexpr auto to(Args&&... args) {
    return detail::make_bound_closure(detail::to_template_fn<C>{}, std::forward<Args>(args)...);
}

ZR_END_NAMESPACE_RANGES
