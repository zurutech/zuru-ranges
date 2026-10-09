// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

// UE's TTuple; TPair is an alias of its two-element case, which exposes .Key / .Value. The tuple
// protocol is UE's: a member Get<Index>() (capital G) and a global get<int N>(tuple) found by
// argument-dependent lookup, which is what structured bindings (`auto [key, value] : map`) use.
// Only the two-element case is defined.
template <class... Types> struct TTuple;

template <class KeyType, class ValueType> struct TTuple<KeyType, ValueType> {
    KeyType Key{};
    ValueType Value{};

    TTuple() = default;
    TTuple(KeyType key, ValueType value)
        : Key(std::move(key))
        , Value(std::move(value)) {}

    template <unsigned Index> decltype(auto) Get() & {
        if constexpr (Index == 0) {
            return (Key);
        } else {
            return (Value);
        }
    }
    template <unsigned Index> decltype(auto) Get() const& {
        if constexpr (Index == 0) {
            return (Key);
        } else {
            return (Value);
        }
    }
    template <unsigned Index> decltype(auto) Get() && {
        if constexpr (Index == 0) {
            return std::move(Key);
        } else {
            return std::move(Value);
        }
    }
    template <unsigned Index> decltype(auto) Get() const&& {
        if constexpr (Index == 0) {
            return std::move(Key);
        } else {
            return std::move(Value);
        }
    }
};

template <class KeyType, class ValueType> using TPair = TTuple<KeyType, ValueType>;

template <class T> inline constexpr bool TIsTuple_V = false;
template <class... Types> inline constexpr bool TIsTuple_V<TTuple<Types...>> = true;

// --- UE's structured-binding support: global get<int N>, by ADL ---
template <int N, class TupleType>
    requires(TIsTuple_V<std::decay_t<TupleType>>)
decltype(auto) get(TupleType&& tuple) {
    return static_cast<TupleType&&>(tuple).template Get<N>();
}

namespace std {
template <class... Types> struct tuple_size<TTuple<Types...>> : integral_constant<size_t, sizeof...(Types)> {};
template <size_t N, class... Types> struct tuple_element<N, TTuple<Types...>> : tuple_element<N, tuple<Types...>> {};
} // namespace std
