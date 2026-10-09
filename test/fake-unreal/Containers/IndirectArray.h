// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"

#include <utility>

// Wraps the TArray<void*> ranged-for iterator and dereferences twice. `!=` only, not
// default-constructible; copy-assignable only when the wrapped iterator is (Shipping), so
// assignability is configuration-dependent.
template <class ElementType, class IteratorType> class TDereferencingIterator {
public:
    explicit TDereferencingIterator(IteratorType it)
        : it_(std::move(it)) {}

    ElementType& operator*() const { return *static_cast<ElementType*>(*it_); }

    TDereferencingIterator& operator++() {
        ++it_;
        return *this;
    }

    bool operator!=(const TDereferencingIterator& rhs) const { return it_ != rhs.it_; }

private:
    IteratorType it_;
};

// Array of owned, heap-allocated elements (stable addresses).
template <class T, class Allocator = FDefaultAllocator> class TIndirectArray {
    using PointerArrayType = TArray<void*, Allocator>;

public:
    using ElementType = T;
    using SizeType = int32;

    using RangedForIteratorType = TDereferencingIterator<ElementType, typename PointerArrayType::RangedForIteratorType>;
    using RangedForConstIteratorType = TDereferencingIterator<const ElementType, typename PointerArrayType::RangedForConstIteratorType>;

    TIndirectArray() = default;
    TIndirectArray(const TIndirectArray&) = delete;
    TIndirectArray& operator=(const TIndirectArray&) = delete;
    TIndirectArray(TIndirectArray&& other) noexcept
        : pointers_(std::move(other.pointers_)) {
        other.pointers_ = PointerArrayType();
    }
    TIndirectArray& operator=(TIndirectArray&&) = delete;
    ~TIndirectArray() {
        for (void* p : pointers_) {
            delete static_cast<ElementType*>(p);
        }
    }

    int32 Num() const { return pointers_.Num(); }

    void Reserve(int32 n) { pointers_.Reserve(n); }

    ElementType& operator[](int32 index) { return *static_cast<ElementType*>(pointers_[index]); }
    const ElementType& operator[](int32 index) const { return *static_cast<const ElementType*>(pointers_[index]); }

    // Takes ownership.
    void Add(ElementType* element) { pointers_.Add(element); }

    RangedForIteratorType begin() { return RangedForIteratorType(pointers_.begin()); }
    RangedForIteratorType end() { return RangedForIteratorType(pointers_.end()); }
    RangedForConstIteratorType begin() const { return RangedForConstIteratorType(pointers_.begin()); }
    RangedForConstIteratorType end() const { return RangedForConstIteratorType(pointers_.end()); }

private:
    PointerArrayType pointers_;
};
