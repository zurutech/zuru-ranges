// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"
#include "Containers/Array.h"
#include "Containers/ContainersFwd.h"

// FString (here), FAnsiString (AnsiString.h), FUtf8String (Utf8String.h): distinct classes over a
// null-terminated TArray of chars; CoreMinimal.h only forward-declares the latter two.
// Length is Len(), data is GetCharArray().GetData(). Ranged-for uses the array's
// configuration-dependent iterator, end() stepped back over the terminator.
template <class CharType> class TFakeString {
public:
    using ElementType = CharType;
    using RangedForIteratorType = typename TArray<CharType>::RangedForIteratorType;
    using RangedForConstIteratorType = typename TArray<CharType>::RangedForConstIteratorType;

    TFakeString() = default;
    TFakeString(const CharType* str) {
        for (const CharType* p = str; *p != CharType{}; ++p) {
            data_.Add(*p);
        }
        data_.Add(CharType{});
    }

    int32 Len() const { return data_.Num() ? data_.Num() - 1 : 0; }

    TFakeString& AppendChar(CharType c) {
        if (data_.Num()) {
            data_[data_.Num() - 1] = c;
        } else {
            data_.Add(c);
        }
        data_.Add(CharType{});
        return *this;
    }

    void Reserve(int32) {}

    // Case-INSENSITIVE, as in UE.
    friend bool operator==(const TFakeString& lhs, const TFakeString& rhs) {
        if (lhs.Len() != rhs.Len()) {
            return false;
        }
        for (int32 i = 0; i < lhs.Len(); ++i) {
            if (fold_(lhs.data_[i]) != fold_(rhs.data_[i])) {
                return false;
            }
        }
        return true;
    }

    TArray<CharType>& GetCharArray() { return data_; }
    const TArray<CharType>& GetCharArray() const { return data_; }

    RangedForIteratorType begin() { return data_.begin(); }
    RangedForConstIteratorType begin() const { return data_.begin(); }
    RangedForIteratorType end() {
        RangedForIteratorType it = data_.end();
        if (data_.Num()) {
            --it;
        }
        return it;
    }
    RangedForConstIteratorType end() const {
        RangedForConstIteratorType it = data_.end();
        if (data_.Num()) {
            --it;
        }
        return it;
    }

private:
    static CharType fold_(CharType c) { return (c >= CharType('A') && c <= CharType('Z')) ? static_cast<CharType>(c - CharType('A') + CharType('a')) : c; }

    TArray<CharType> data_;
};

class FString : public TFakeString<TCHAR> {
public:
    using TFakeString::TFakeString;
};
