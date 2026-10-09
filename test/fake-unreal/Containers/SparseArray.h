// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h" // ZR_FAKE_UE_RANGED_FOR_CHECKS

#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <vector>

struct FDefaultSparseArrayAllocator {};

// Array with holes (RemoveAt leaves a gap). Iterators skip holes, hold a container reference
// (not assignable, not default-constructible), and inherit operator++ from a base, so `++it`
// yields TBaseIterator&, not the iterator's own type. The ranged-for type switches with the
// configuration (UE's TSPARSEARRAY_RANGED_FOR_CHECKS follows TARRAY_RANGED_FOR_CHECKS).
template <class InElementType, class Allocator = FDefaultSparseArrayAllocator> class TSparseArray {
public:
    using ElementType = InElementType;
    using SizeType = int32;

    template <bool Const> class TBaseIterator {
        using ArrayType = std::conditional_t<Const, const TSparseArray, TSparseArray>;
        using ItElementType = std::conditional_t<Const, const ElementType, ElementType>;

    public:
        TBaseIterator(ArrayType& array, int32 index)
            : array_(array)
            , index_(index) {
            skip_();
        }

        ItElementType& operator*() const { return array_.elements_[static_cast<std::size_t>(index_)]; }

        TBaseIterator& operator++() {
            ++index_;
            skip_();
            return *this;
        }

        bool operator!=(const TBaseIterator& rhs) const { return index_ != rhs.index_; }

    protected:
        ArrayType& array_;

    private:
        void skip_() {
            while (index_ < static_cast<int32>(array_.allocated_.size()) && !array_.allocated_[static_cast<std::size_t>(index_)]) {
                ++index_;
            }
        }

        int32 index_;
    };

    class TIterator : public TBaseIterator<false> {
    public:
        using TBaseIterator<false>::TBaseIterator;
    };

    class TConstIterator : public TBaseIterator<true> {
    public:
        using TBaseIterator<true>::TBaseIterator;
    };

#if ZR_FAKE_UE_RANGED_FOR_CHECKS
    class TRangedForIterator : public TIterator {
    public:
        using TIterator::TIterator;
    };
    class TRangedForConstIterator : public TConstIterator {
    public:
        using TConstIterator::TConstIterator;
    };
#else
    using TRangedForIterator = TIterator;
    using TRangedForConstIterator = TConstIterator;
#endif

    TSparseArray() = default;
    TSparseArray(std::initializer_list<ElementType> init) {
        for (const ElementType& element : init) {
            Add(element);
        }
    }

    int32 Num() const { return num_; }

    void Reserve(int32) {}

    int32 Add(const ElementType& element) {
        elements_.push_back(element);
        allocated_.push_back(true);
        ++num_;
        return static_cast<int32>(elements_.size()) - 1;
    }

    void RemoveAt(int32 index) {
        allocated_[static_cast<std::size_t>(index)] = false;
        --num_;
    }

    TRangedForIterator begin() { return TRangedForIterator(*this, 0); }
    TRangedForIterator end() { return TRangedForIterator(*this, static_cast<int32>(elements_.size())); }
    TRangedForConstIterator begin() const { return TRangedForConstIterator(*this, 0); }
    TRangedForConstIterator end() const { return TRangedForConstIterator(*this, static_cast<int32>(elements_.size())); }

private:
    std::vector<ElementType> elements_;
    std::vector<bool> allocated_;
    int32 num_{};
};
