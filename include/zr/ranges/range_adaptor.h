// Copyright (c) 2025-2026 Zuru Tech Italy S.r.l.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "zr/config.h"
#include "zr/ranges/range_concepts.h"

#include <concepts>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

/**
 * @file range_adaptor.h
 *
 * @brief [range.adaptor.object] range_adaptor_closure, `R | C`, `C | D`, and the closure
 *        `adaptor(args...)` of a range adaptor object.
 */

ZR_BEGIN_NAMESPACE_RANGES

template <class D>
    requires std::is_class_v<D> && std::same_as<D, std::remove_cv_t<D>>
class range_adaptor_closure {};

namespace detail {

template <class T, class U>
    requires(!std::same_as<T, range_adaptor_closure<U>>)
void closure_marker(const T&, const range_adaptor_closure<U>&); // not defined

// [range.adaptor.object]/2: derived_from<range_adaptor_closure<T>>, no other
// range_adaptor_closure<U> base, does not model range.
template <class T>
concept closure = std::is_class_v<std::remove_cvref_t<T>> && !range<std::remove_cvref_t<T>> &&
                  std::derived_from<std::remove_cvref_t<T>, range_adaptor_closure<std::remove_cvref_t<T>>> &&
                  requires(const std::remove_cvref_t<T>& t) { detail::closure_marker(t, t); };

// [range.adaptor.object]/1 `C | D`: perfect forwarding call wrapper ([func.require]) with
// target d, bound entity c, call pattern d(c(arg)).
template <class C, class D> struct composed_closure : range_adaptor_closure<composed_closure<C, D>> {
    template <class C2, class D2>
    constexpr composed_closure(std::in_place_t, C2&& c, D2&& d)
        : c_(std::forward<C2>(c))
        , d_(std::forward<D2>(d)) {}

#define ZR_COMPOSED_CALL(CV, REF)                                                                                                   \
    template <class R>                                                                                                               \
        requires std::invocable<C CV REF, R> && std::invocable<D CV REF, std::invoke_result_t<C CV REF, R>>                          \
    constexpr decltype(auto) operator()(R&& r) CV REF noexcept(                                                                      \
        std::is_nothrow_invocable_v<C CV REF, R> && std::is_nothrow_invocable_v<D CV REF, std::invoke_result_t<C CV REF, R>>) {      \
        return std::invoke(static_cast<D CV REF>(d_), std::invoke(static_cast<C CV REF>(c_), std::forward<R>(r)));                    \
    }
    ZR_COMPOSED_CALL(, &)
    ZR_COMPOSED_CALL(const, &)
    ZR_COMPOSED_CALL(, &&)
    ZR_COMPOSED_CALL(const, &&)
#undef ZR_COMPOSED_CALL

private:
    ZR_NO_UNIQUE_ADDRESS C c_;
    ZR_NO_UNIQUE_ADDRESS D d_;
};

// [range.adaptor.object]/8 `adaptor(args...)`: perfect forwarding call wrapper with target
// adaptor, bound entities bound_args, call pattern adaptor(r, bound_args...).
template <class Adaptor, class... BoundArgs> struct bound_closure : range_adaptor_closure<bound_closure<Adaptor, BoundArgs...>> {
    template <class... Args>
    constexpr explicit bound_closure(std::in_place_t, Adaptor adaptor, Args&&... args)
        : adaptor_(adaptor)
        , bound_args_(std::forward<Args>(args)...) {}

private:
    template <class Self, class R> static constexpr decltype(auto) call_(Self&& self, R&& r) {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) -> decltype(auto) {
            return std::invoke(std::forward<Self>(self).adaptor_, std::forward<R>(r), std::get<Is>(std::forward<Self>(self).bound_args_)...);
        }(std::index_sequence_for<BoundArgs...>{});
    }

public:
#define ZR_BOUND_CALL(CV, REF)                                                                                                      \
    template <class R>                                                                                                               \
        requires std::invocable<const Adaptor&, R, BoundArgs CV REF...>                                                              \
    constexpr decltype(auto) operator()(R&& r) CV REF noexcept(std::is_nothrow_invocable_v<const Adaptor&, R, BoundArgs CV REF...>) { \
        return call_(static_cast<bound_closure CV REF>(*this), std::forward<R>(r));                                                  \
    }
    ZR_BOUND_CALL(, &)
    ZR_BOUND_CALL(const, &)
    ZR_BOUND_CALL(, &&)
    ZR_BOUND_CALL(const, &&)
#undef ZR_BOUND_CALL

private:
    ZR_NO_UNIQUE_ADDRESS Adaptor adaptor_;
    ZR_NO_UNIQUE_ADDRESS std::tuple<BoundArgs...> bound_args_;
};

// `adaptor(args...)`, well-formed iff each bound argument entity is ([range.adaptor.object]/8).
template <class Adaptor, class... Args>
    requires(std::constructible_from<std::decay_t<Args>, Args> && ...)
constexpr auto make_bound_closure(Adaptor adaptor, Args&&... args) noexcept((std::is_nothrow_constructible_v<std::decay_t<Args>, Args> && ...)) {
    return bound_closure<Adaptor, std::decay_t<Args>...>(std::in_place, adaptor, std::forward<Args>(args)...);
}

} // namespace detail

// `R | C` is `C(R)`.
template <class R, class C>
    requires range<R> && detail::closure<C> && std::invocable<C, R>
constexpr decltype(auto) operator|(R&& r, C&& c) noexcept(std::is_nothrow_invocable_v<C, R>) {
    return std::invoke(std::forward<C>(c), std::forward<R>(r));
}

// `C | D`, well-formed iff the state entities' initializations are.
template <class C, class D>
    requires detail::closure<C> && detail::closure<D> && std::constructible_from<std::decay_t<C>, C> &&
             std::constructible_from<std::decay_t<D>, D>
constexpr auto operator|(C&& c, D&& d) noexcept(std::is_nothrow_constructible_v<std::decay_t<C>, C> &&
                                                std::is_nothrow_constructible_v<std::decay_t<D>, D>) {
    return detail::composed_closure<std::decay_t<C>, std::decay_t<D>>(std::in_place, std::forward<C>(c), std::forward<D>(d));
}

ZR_END_NAMESPACE_RANGES
