// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"
#include "Containers/ContainersFwd.h"
#include "Templates/Tuple.h"

#include <initializer_list>
#include <utility>

// TSortedMap stores a TArray and inherits its RangedForIteratorType: TCheckedPointerIterator in
// debug, TPair* in shipping. Semantically forward, so the category comes from range_traits,
// never from the iterator.
//
// No Add(TPair). Emplace is an unconstrained template, as in UE: `requires { m.Emplace(pair); }`
// holds and the instantiation then fails, so requires-based detection must not trust it.
template <class InKeyType, class InValueType, class ArrayAllocator, class SortPredicate> class TSortedMap {
public:
    using KeyType = InKeyType;
    using ValueType = InValueType;
    using ElementType = TPair<KeyType, ValueType>;
    using ElementArrayType = TArray<ElementType, ArrayAllocator>;
    using SizeType = int32;

    // Inherited verbatim from the backing array; configuration-dependent, just like UE.
    using RangedForIteratorType = typename ElementArrayType::RangedForIteratorType;
    using RangedForConstIteratorType = typename ElementArrayType::RangedForConstIteratorType;

    TSortedMap() = default;
    TSortedMap(std::initializer_list<ElementType> init) {
        for (const ElementType& pair : init) {
            Emplace(pair.Key, pair.Value);
        }
    }

    int32 Num() const { return pairs_.Num(); }

    void Reserve(int32 n) { pairs_.Reserve(n); }

    ValueType& Add(const KeyType& key, const ValueType& value) { return Emplace(key, value); }
    ValueType& Add(const KeyType& key) { return Emplace(key); }

    template <class InitKeyType> ValueType& Emplace(InitKeyType&& key) { return Emplace(std::forward<InitKeyType>(key), ValueType{}); }

    template <class InitKeyType, class InitValueType> ValueType& Emplace(InitKeyType&& key, InitValueType&& value) {
        KeyType k(std::forward<InitKeyType>(key));
        int32 index = 0;
        while (index < pairs_.Num() && pairs_[index].Key < k) {
            ++index;
        }
        if (index < pairs_.Num() && !(k < pairs_[index].Key)) {
            pairs_[index].Value = ValueType(std::forward<InitValueType>(value));
        } else {
            pairs_.Insert(ElementType{std::move(k), ValueType(std::forward<InitValueType>(value))}, index);
        }
        return pairs_[index].Value;
    }

    RangedForIteratorType begin() { return pairs_.begin(); }
    RangedForIteratorType end() { return pairs_.end(); }
    RangedForConstIteratorType begin() const { return pairs_.begin(); }
    RangedForConstIteratorType end() const { return pairs_.end(); }

private:
    ElementArrayType pairs_;
};
