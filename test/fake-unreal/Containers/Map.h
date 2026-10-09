// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"
#include "Containers/ContainersFwd.h"
#include "Detail/FakeContainerIterator.h"
#include "Templates/Tuple.h"

#include <initializer_list>
#include <utility>
#include <vector>

// Forward, non-contiguous, sized. Element/value type is TPair<Key, Value>.
template <class InKeyType, class InValueType, class SetAllocator, class KeyFuncs> class TMap {
public:
    using KeyType = InKeyType;
    using ValueType = InValueType;
    using ElementType = TPair<KeyType, ValueType>;
    using SizeType = int32;

    using TIterator = TFakeContainerIterator<typename std::vector<ElementType>::iterator>;
    using TConstIterator = TFakeContainerIterator<typename std::vector<ElementType>::const_iterator>;

    TMap() = default;
    TMap(std::initializer_list<ElementType> init)
        : storage_(init) {}

    int32 Num() const { return static_cast<int32>(storage_.size()); }

    void Reserve(int32) {}

    // UE: Add(const&/&&, const&/&&).
    ValueType& Add(const KeyType& key, const ValueType& value) { return Emplace_(key, value); }
    ValueType& Add(const KeyType& key, ValueType&& value) { return Emplace_(key, std::move(value)); }
    ValueType& Add(KeyType&& key, const ValueType& value) { return Emplace_(std::move(key), value); }
    ValueType& Add(KeyType&& key, ValueType&& value) { return Emplace_(std::move(key), std::move(value)); }
    ValueType& Add(const ElementType& pair) { return Add(pair.Key, pair.Value); }

    TIterator begin() { return TIterator{storage_.begin()}; }
    TIterator end() { return TIterator{storage_.end()}; }
    TConstIterator begin() const { return TConstIterator{storage_.begin()}; }
    TConstIterator end() const { return TConstIterator{storage_.end()}; }

private:
    template <class K, class V> ValueType& Emplace_(K&& key, V&& value) {
        for (ElementType& pair : storage_) {
            if (pair.Key == key) {
                pair.Value = std::forward<V>(value);
                return pair.Value;
            }
        }
        storage_.emplace_back(std::forward<K>(key), std::forward<V>(value));
        return storage_.back().Value;
    }

    std::vector<ElementType> storage_;
};

// As TMap; a key may repeat.
template <class InKeyType, class InValueType, class SetAllocator, class KeyFuncs> class TMultiMap {
public:
    using KeyType = InKeyType;
    using ValueType = InValueType;
    using ElementType = TPair<KeyType, ValueType>;
    using SizeType = int32;

    using TIterator = TFakeContainerIterator<typename std::vector<ElementType>::iterator>;
    using TConstIterator = TFakeContainerIterator<typename std::vector<ElementType>::const_iterator>;

    TMultiMap() = default;
    TMultiMap(std::initializer_list<ElementType> init)
        : storage_(init) {}

    int32 Num() const { return static_cast<int32>(storage_.size()); }

    void Reserve(int32) {}

    ValueType& Add(const KeyType& key, const ValueType& value) { return Emplace_(key, value); }
    ValueType& Add(const KeyType& key, ValueType&& value) { return Emplace_(key, std::move(value)); }
    ValueType& Add(KeyType&& key, const ValueType& value) { return Emplace_(std::move(key), value); }
    ValueType& Add(KeyType&& key, ValueType&& value) { return Emplace_(std::move(key), std::move(value)); }
    ValueType& Add(const ElementType& pair) { return Add(pair.Key, pair.Value); }

    TIterator begin() { return TIterator{storage_.begin()}; }
    TIterator end() { return TIterator{storage_.end()}; }
    TConstIterator begin() const { return TConstIterator{storage_.begin()}; }
    TConstIterator end() const { return TConstIterator{storage_.end()}; }

private:
    template <class K, class V> ValueType& Emplace_(K&& key, V&& value) {
        storage_.emplace_back(std::forward<K>(key), std::forward<V>(value));
        return storage_.back().Value;
    }

    std::vector<ElementType> storage_;
};

// UE 5.7 names both storages of TMap/TMultiMap (Map.h is stamped three times): same shape here.
template <class InKeyType, class InValueType, class SetAllocator, class KeyFuncs>
class TSparseMap : public TMap<InKeyType, InValueType, SetAllocator, KeyFuncs> {
public:
    using TMap<InKeyType, InValueType, SetAllocator, KeyFuncs>::TMap;
};
template <class InKeyType, class InValueType, class SetAllocator, class KeyFuncs>
class TCompactMap : public TMap<InKeyType, InValueType, SetAllocator, KeyFuncs> {
public:
    using TMap<InKeyType, InValueType, SetAllocator, KeyFuncs>::TMap;
};
template <class KeyType, class ValueType, class SetAllocator, class KeyFuncs>
class TSparseMultiMap : public TMultiMap<KeyType, ValueType, SetAllocator, KeyFuncs> {
public:
    using TMultiMap<KeyType, ValueType, SetAllocator, KeyFuncs>::TMultiMap;
};
template <class KeyType, class ValueType, class SetAllocator, class KeyFuncs>
class TCompactMultiMap : public TMultiMap<KeyType, ValueType, SetAllocator, KeyFuncs> {
public:
    using TMultiMap<KeyType, ValueType, SetAllocator, KeyFuncs>::TMultiMap;
};
