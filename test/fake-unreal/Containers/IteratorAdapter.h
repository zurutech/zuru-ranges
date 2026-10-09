// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"

#include <utility>

// Mirrors the shape of UE's TIteratorAdapter (TDeque's and TPagedArray's iterator): the base
// provides protected Dereference, Increment and Equals, optionally Decrement. `--`, `+=`, `-=`,
// `+` and `-` are declared for every base and compile only where the base has the operation:
// TDeque's base has no Decrement, so its iterator's `--` is declared but ill-formed.
template <typename BaseType> class TIteratorAdapter : public BaseType {
public:
    using ElementType = typename BaseType::ElementType;
    using SizeType = typename BaseType::SizeType;

    TIteratorAdapter() = default;

    template <typename... ArgTypes>
    TIteratorAdapter(EInPlace, ArgTypes&&... args)
        : BaseType(std::forward<ArgTypes>(args)...) {}

    ElementType& operator*() const { return this->Dereference(); }
    ElementType* operator->() const { return &this->Dereference(); }

    TIteratorAdapter& operator++() {
        this->Increment();
        return *this;
    }
    TIteratorAdapter operator++(int) {
        TIteratorAdapter temp = *this;
        ++*this;
        return temp;
    }
    TIteratorAdapter& operator--() {
        this->Decrement();
        return *this;
    }
    TIteratorAdapter operator--(int) {
        TIteratorAdapter temp = *this;
        --*this;
        return temp;
    }

    TIteratorAdapter& operator+=(SizeType offset) {
        this->Increment(offset);
        return *this;
    }
    TIteratorAdapter& operator-=(SizeType offset) {
        this->Decrement(offset);
        return *this;
    }
    TIteratorAdapter operator+(SizeType offset) const {
        TIteratorAdapter temp = *this;
        return temp += offset;
    }
    TIteratorAdapter operator-(SizeType offset) const {
        TIteratorAdapter temp = *this;
        return temp -= offset;
    }

    bool operator==(const TIteratorAdapter& rhs) const { return this->Equals(rhs); }
    bool operator!=(const TIteratorAdapter& rhs) const { return !this->Equals(rhs); }

protected:
    using BaseType::Dereference;
    using BaseType::Equals;
    using BaseType::Increment;
};
