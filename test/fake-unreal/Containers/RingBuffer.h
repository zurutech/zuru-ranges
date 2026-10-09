// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"

#include <cstddef>
#include <deque>
#include <initializer_list>

// Iterator holds a container reference (not assignable, not default-constructible) and has
// post-increment and decrement, as UE's. Random access is operator[]; Reserve takes the
// unsigned SizeType while Num() returns the signed IndexType, as UE.
template <class T, class AllocatorT = FDefaultAllocator> class TRingBuffer {
public:
    using ElementType = T;
    using IndexType = int32;
    using SizeType = uint32;

    template <class ContainerType, class ItElementType> class TRingBufferIterator {
    public:
        TRingBufferIterator(ContainerType& container, IndexType index)
            : container_(container)
            , index_(index) {}

        ItElementType& operator*() const { return container_[index_]; }

        TRingBufferIterator& operator++() {
            ++index_;
            return *this;
        }
        TRingBufferIterator operator++(int) {
            TRingBufferIterator old(*this);
            ++index_;
            return old;
        }
        TRingBufferIterator& operator--() {
            --index_;
            return *this;
        }
        TRingBufferIterator operator--(int) {
            TRingBufferIterator old(*this);
            --index_;
            return old;
        }

        bool operator!=(const TRingBufferIterator& rhs) const { return index_ != rhs.index_; }

    private:
        ContainerType& container_;
        IndexType index_;
    };

    using TIterator = TRingBufferIterator<TRingBuffer, ElementType>;
    using TConstIterator = TRingBufferIterator<const TRingBuffer, const ElementType>;

    TRingBuffer() = default;
    TRingBuffer(std::initializer_list<ElementType> init)
        : storage_(init) {}

    IndexType Num() const { return static_cast<IndexType>(storage_.size()); }

    IndexType Add(const ElementType& element) {
        storage_.push_back(element);
        return Num() - 1;
    }
    IndexType AddFront(const ElementType& element) {
        storage_.push_front(element);
        return 0;
    }

    void Reserve(SizeType) {}

    ElementType& operator[](IndexType index) { return storage_[static_cast<std::size_t>(index)]; }
    const ElementType& operator[](IndexType index) const { return storage_[static_cast<std::size_t>(index)]; }

    TIterator begin() { return TIterator(*this, 0); }
    TIterator end() { return TIterator(*this, Num()); }
    TConstIterator begin() const { return TConstIterator(*this, 0); }
    TConstIterator end() const { return TConstIterator(*this, Num()); }

private:
    std::deque<ElementType> storage_;
};
