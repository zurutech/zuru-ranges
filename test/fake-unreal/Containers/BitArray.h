// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"

#include <cstddef>
#include <vector>

struct FDefaultBitArrayAllocator {};

// A bit is addressed through PROXIES: operator* and operator[] yield FBitReference prvalues, their
// const overloads FConstBitReference prvalues (a class, not bool), as UE. FBitReference's
// copy-assignment writes the referenced bit (it does not rebind) and operator=(bool) returns void,
// as UE.
class FBitReference {
public:
    explicit FBitReference(std::vector<bool>::reference bit)
        : bit_(bit) {}

    FBitReference(const FBitReference&) = default;
    FBitReference& operator=(const FBitReference& copy) {
        *this = static_cast<bool>(copy);
        return *this;
    }
    ~FBitReference() = default;

    operator bool() const { return bit_; }

    void operator=(bool value) { bit_ = value; }

private:
    std::vector<bool>::reference bit_;
};

class FConstBitReference {
public:
    explicit FConstBitReference(bool bit)
        : bit_(bit) {}

    operator bool() const { return bit_; }

private:
    bool bit_;
};

// Iterators hold a container reference: not assignable, not default-constructible.
template <class Allocator = FDefaultBitArrayAllocator> class TBitArray {
public:
    template <class ArrayType, class ReferenceType> class TIterator {
    public:
        TIterator(ArrayType& array, int32 index)
            : array_(array)
            , index_(index) {}

        ReferenceType operator*() const { return array_[index_]; }

        TIterator& operator++() {
            ++index_;
            return *this;
        }

        bool operator!=(const TIterator& rhs) const { return index_ != rhs.index_; }

    private:
        ArrayType& array_;
        int32 index_;
    };

    using FIterator = TIterator<TBitArray, FBitReference>;
    using FConstIterator = TIterator<const TBitArray, FConstBitReference>;

    TBitArray() = default;
    TBitArray(bool value, int32 numBits)
        : bits_(static_cast<std::size_t>(numBits), value) {}

    int32 Num() const { return static_cast<int32>(bits_.size()); }

    int32 Add(bool value) {
        bits_.push_back(value);
        return Num() - 1;
    }

    void Reserve(int32) {}

    FBitReference operator[](int32 index) { return FBitReference(bits_[static_cast<std::size_t>(index)]); }
    FConstBitReference operator[](int32 index) const { return FConstBitReference(bits_[static_cast<std::size_t>(index)]); }

#if ZR_FAKE_UE_VERSION >= 504 // ranged-for support, UE 5.4
    FIterator begin() { return FIterator(*this, 0); }
    FIterator end() { return FIterator(*this, Num()); }
    FConstIterator begin() const { return FConstIterator(*this, 0); }
    FConstIterator end() const { return FConstIterator(*this, Num()); }
#endif

private:
    std::vector<bool> bits_;
};
