// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"
#include "Containers/ContainersFwd.h"

#include <initializer_list>

// Sorted TArray of unique elements; inherits the array's configuration-dependent RangedForIteratorType
// (checked class in debug, raw pointer in Shipping), exactly like TSortedMap.
template <class InElementType, class ArrayAllocator, class SortPredicate> class TSortedSet {
public:
    using ElementType = InElementType;
    using ElementArrayType = TArray<ElementType, ArrayAllocator>;
    using SizeType = int32;

    using RangedForIteratorType = typename ElementArrayType::RangedForIteratorType;
    using RangedForConstIteratorType = typename ElementArrayType::RangedForConstIteratorType;

    TSortedSet() = default;
    TSortedSet(std::initializer_list<ElementType> init) {
        for (const ElementType& element : init) {
            Add(element);
        }
    }

    int32 Num() const { return elements_.Num(); }

    void Reserve(int32 n) { elements_.Reserve(n); }

    void Add(const ElementType& element) {
        int32 index = 0;
        while (index < elements_.Num() && elements_[index] < element) {
            ++index;
        }
        if (index == elements_.Num() || element < elements_[index]) {
            elements_.Insert(element, index);
        }
    }

    RangedForIteratorType begin() { return elements_.begin(); }
    RangedForIteratorType end() { return elements_.end(); }
    RangedForConstIteratorType begin() const { return elements_.begin(); }
    RangedForConstIteratorType end() const { return elements_.end(); }

private:
    ElementArrayType elements_;
};
