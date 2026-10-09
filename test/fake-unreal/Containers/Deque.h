// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"
#include "Containers/IteratorAdapter.h"

#include <deque>
#include <initializer_list>
#include <utility>

// TDeque's iterator base: Dereference, Increment, Equals, no Decrement (as UE). Stricter than UE
// in being neither default-constructible nor assignable.
template <class UnderlyingIt, class InElementType> class TFakeDequeIteratorBase {
public:
    using ElementType = InElementType;
    using SizeType = int32;

    TFakeDequeIteratorBase() = delete;
    TFakeDequeIteratorBase(const TFakeDequeIteratorBase&) = default;
    TFakeDequeIteratorBase(TFakeDequeIteratorBase&&) = default;
    TFakeDequeIteratorBase& operator=(const TFakeDequeIteratorBase&) = delete;
    TFakeDequeIteratorBase& operator=(TFakeDequeIteratorBase&&) = delete;
    ~TFakeDequeIteratorBase() = default;

protected:
    explicit TFakeDequeIteratorBase(UnderlyingIt it)
        : it_(std::move(it)) {}

    ElementType& Dereference() const { return *it_; }
    void Increment() { ++it_; }
    bool Equals(const TFakeDequeIteratorBase& rhs) const { return it_ == rhs.it_; }

private:
    UnderlyingIt it_;
};

// Random-access, non-contiguous, sized. Random access is through operator[] only: begin()/end()
// return a forward iterator whose `--` and arithmetic are declared but ill-formed, so a TDeque is
// indexed by range_traits::element_at.
template <class InElementType, class AllocatorType = FDefaultAllocator> class TDeque {
public:
    using ElementType = InElementType;
    using SizeType = int32;

    using TIterator = TIteratorAdapter<TFakeDequeIteratorBase<typename std::deque<ElementType>::iterator, ElementType>>;
    using TConstIterator = TIteratorAdapter<TFakeDequeIteratorBase<typename std::deque<ElementType>::const_iterator, const ElementType>>;

    TDeque() = default;
    TDeque(std::initializer_list<ElementType> init)
        : storage_(init) {}

    int32 Num() const { return static_cast<int32>(storage_.size()); }

    ElementType& operator[](int32 index) { return storage_[static_cast<std::size_t>(index)]; }
    const ElementType& operator[](int32 index) const { return storage_[static_cast<std::size_t>(index)]; }

    // No Add/Emplace: UE appends through PushLast/EmplaceLast.
    void PushLast(const ElementType& element) { storage_.push_back(element); }
    template <class... ArgsType> ElementType& EmplaceLast(ArgsType&&... args) { return storage_.emplace_back(std::forward<ArgsType>(args)...); }

    void Reserve(int32) {}

    TIterator begin() { return TIterator(InPlace, storage_.begin()); }
    TIterator end() { return TIterator(InPlace, storage_.end()); }
    TConstIterator begin() const { return TConstIterator(InPlace, storage_.begin()); }
    TConstIterator end() const { return TConstIterator(InPlace, storage_.end()); }

private:
    std::deque<ElementType> storage_;
};
