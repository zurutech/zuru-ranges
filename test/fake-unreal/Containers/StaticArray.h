// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"

#include <cstddef>

// C4324 (padded due to alignas) is the point here; UBT disables it too (/wd4324).
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
namespace UE::Core {
template <class ElementType, uint32 Alignment> struct alignas(Alignment) TAlignedElement {
    ElementType Element;
};
} // namespace UE::Core
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// Fixed-size array; raw-pointer iterators in every configuration.
template <class InElementType, uint32 NumElements, uint32 Alignment = uint32(-1)> class TStaticArray {
public:
    using ElementType = InElementType;

    InElementType& operator[](uint32 index) { return elements_[index]; }
    const InElementType& operator[](uint32 index) const { return elements_[index]; }

    int32 Num() const { return static_cast<int32>(NumElements); }

    InElementType* GetData() { return elements_; }
    const InElementType* GetData() const { return elements_; }

    InElementType* begin() { return elements_; }
    InElementType* end() { return elements_ + NumElements; }
    const InElementType* begin() const { return elements_; }
    const InElementType* end() const { return elements_ + NumElements; }

private:
    InElementType elements_[NumElements]{};
};

// Over-aligned: elements are padded (TAlignedElement), so GetData() is not an ElementType*
// and the storage is not contiguous over ElementType. Class iterator, `!=` only.
template <class InElementType, uint32 NumElements, uint32 Alignment>
    requires(Alignment != uint32(-1))
class TStaticArray<InElementType, NumElements, Alignment> {
    using StorageType = UE::Core::TAlignedElement<InElementType, Alignment>;

public:
    using ElementType = InElementType;

    template <class ItElementType, class ItStorageType> class FRangedForIterator {
    public:
        explicit FRangedForIterator(ItStorageType* ptr)
            : ptr_(ptr) {}

        ItElementType& operator*() const { return ptr_->Element; }

        FRangedForIterator& operator++() {
            ++ptr_;
            return *this;
        }

        bool operator!=(const FRangedForIterator& rhs) const { return ptr_ != rhs.ptr_; }

    private:
        ItStorageType* ptr_;
    };

    using RangedForIteratorType = FRangedForIterator<InElementType, StorageType>;
    using RangedForConstIteratorType = FRangedForIterator<const InElementType, const StorageType>;

    InElementType& operator[](uint32 index) { return elements_[index].Element; }
    const InElementType& operator[](uint32 index) const { return elements_[index].Element; }

    int32 Num() const { return static_cast<int32>(NumElements); }

    StorageType* GetData() { return elements_; }
    const StorageType* GetData() const { return elements_; }

    RangedForIteratorType begin() { return RangedForIteratorType(elements_); }
    RangedForIteratorType end() { return RangedForIteratorType(elements_ + NumElements); }
    RangedForConstIteratorType begin() const { return RangedForConstIteratorType(elements_); }
    RangedForConstIteratorType end() const { return RangedForConstIteratorType(elements_ + NumElements); }

private:
    StorageType elements_[NumElements]{};
};
