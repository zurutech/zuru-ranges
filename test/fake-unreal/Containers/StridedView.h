// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"
#include "Containers/ContainersFwd.h"

#include <type_traits>

// Elements InBytesBetweenElements apart. The iterator is an aggregate (default-constructible,
// assignable) whose operator* is NON-CONST: a const iterator is not dereferenceable.
template <class InElementType, class InSizeType> class TStridedView {
public:
    using ElementType = InElementType;
    using SizeType = InSizeType;

    struct FIterator {
        const TStridedView* Owner;
        SizeType Index;

        FIterator& operator++() {
            ++Index;
            return *this;
        }

        ElementType& operator*() { return (*Owner)[Index]; }

        bool operator==(const FIterator& other) const { return Owner == other.Owner && Index == other.Index; }
        bool operator!=(const FIterator& other) const { return !(*this == other); }
    };

    TStridedView() = default;
    TStridedView(SizeType bytesBetweenElements, ElementType* first, SizeType num)
        : first_(first)
        , stride_(bytesBetweenElements)
        , num_(num) {}

    SizeType Num() const { return num_; }

    ElementType& operator[](SizeType index) const {
        using ByteType = std::conditional_t<std::is_const_v<ElementType>, const unsigned char, unsigned char>;
        return *reinterpret_cast<ElementType*>(reinterpret_cast<ByteType*>(first_) + (index * stride_));
    }

    FIterator begin() const { return FIterator{this, 0}; }
    FIterator end() const { return FIterator{this, num_}; }

private:
    ElementType* first_ = nullptr;
    SizeType stride_ = 0;
    SizeType num_ = 0;
};
