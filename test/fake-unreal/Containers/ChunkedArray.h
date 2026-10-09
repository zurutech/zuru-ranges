// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"

#include <cstddef>
#include <deque>
#include <initializer_list>

// Ranged-for iterator is an aggregate with `void operator++()` and an ASYMMETRIC `!=`
// (`Count < Rhs.Count`): `end != it` is always false, so only `it != end` terminates.
template <class InElementType, uint32 TargetBytesPerChunk = 16384, class AllocatorType = FDefaultAllocator> class TChunkedArray {
public:
    using ElementType = InElementType;
    using SizeType = int32;

    template <class ContainerType, class ItElementType> struct TChunkedArrayIterator {
        ContainerType* Container = nullptr;
        uint32 Count = 0;

        ItElementType& operator*() const { return (*Container)[static_cast<std::size_t>(Count)]; }

        void operator++() { ++Count; }

        bool operator!=(const TChunkedArrayIterator& rhs) const { return Count < rhs.Count; }
    };

    using FIterType = TChunkedArrayIterator<std::deque<ElementType>, ElementType>;
    using FConstIterType = TChunkedArrayIterator<const std::deque<ElementType>, const ElementType>;

    TChunkedArray() = default;
    TChunkedArray(std::initializer_list<ElementType> init)
        : storage_(init) {}

    int32 Num() const { return static_cast<int32>(storage_.size()); }

    void Reserve(int32) {}

    ElementType& operator[](int32 index) { return storage_[static_cast<std::size_t>(index)]; }
    const ElementType& operator[](int32 index) const { return storage_[static_cast<std::size_t>(index)]; }

    int32 AddElement(const ElementType& element) {
        storage_.push_back(element);
        return Num() - 1;
    }

    // Appends count default-constructed elements; returns the first index.
    int32 Add(int32 count = 1) {
        const int32 first = Num();
        storage_.resize(storage_.size() + static_cast<std::size_t>(count));
        return first;
    }

    FIterType begin() { return {&storage_, 0}; }
    FIterType end() { return {&storage_, static_cast<uint32>(storage_.size())}; }
    FConstIterType begin() const { return {&storage_, 0}; }
    FConstIterType end() const { return {&storage_, static_cast<uint32>(storage_.size())}; }

private:
    std::deque<ElementType> storage_;
};
