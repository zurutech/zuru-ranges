// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"
#include "Containers/ContainersFwd.h"
#include "Detail/FakeContainerIterator.h"

#include <initializer_list>
#include <vector>

// Forward, non-contiguous, sized. Iterators are non-semiregular, != only.
template <class InElementType, class KeyFuncs, class Allocator> class TSet {
public:
    using ElementType = InElementType;
    using SizeType = int32;

    using TIterator = TFakeContainerIterator<typename std::vector<ElementType>::iterator>;
    using TConstIterator = TFakeContainerIterator<typename std::vector<ElementType>::const_iterator>;

    TSet() = default;
    TSet(std::initializer_list<ElementType> init) {
        for (const ElementType& element : init) {
            Add(element);
        }
    }

    int32 Num() const { return static_cast<int32>(storage_.size()); }

    void Reserve(int32) {}

    // Keeps the existing element on a duplicate key.
    void Add(const ElementType& element) {
        for (const ElementType& existing : storage_) {
            if (existing == element) {
                return;
            }
        }
        storage_.push_back(element);
    }

    TIterator begin() { return TIterator{storage_.begin()}; }
    TIterator end() { return TIterator{storage_.end()}; }
    TConstIterator begin() const { return TConstIterator{storage_.begin()}; }
    TConstIterator end() const { return TConstIterator{storage_.end()}; }

private:
    std::vector<ElementType> storage_;
};

// UE 5.7 names both storages of TSet: TSparseSet (the default) and TCompactSet (CompactSet.h);
// TSet itself is stamped from one of them (UE_USE_COMPACT_SET_AS_DEFAULT). Same shape as TSet here.
template <class InElementType, class KeyFuncs, class Allocator> class TSparseSet : public TSet<InElementType, KeyFuncs, Allocator> {
public:
    using TSet<InElementType, KeyFuncs, Allocator>::TSet;
};
