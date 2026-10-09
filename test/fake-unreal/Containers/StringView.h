// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "CoreTypes.h"
#include "Containers/ContainersFwd.h"

// Contiguous, raw-pointer iterators in every configuration; length is Len(), not Num(). The
// template head is constrained (UE::CCharType), as UE's.
template <UE::CCharType CharType> class TStringView {
public:
    using ElementType = CharType;

    constexpr TStringView() = default;
    constexpr TStringView(const CharType* data, int32 len)
        : data_(data)
        , len_(len) {}
    constexpr TStringView(const CharType* str)
        : data_(str) {
        while (str[len_] != CharType{}) {
            ++len_;
        }
    }

    constexpr const CharType* GetData() const { return data_; }
    constexpr int32 Len() const { return len_; }

    constexpr const CharType& operator[](int32 index) const { return data_[index]; }

    constexpr const CharType* begin() const { return data_; }
    constexpr const CharType* end() const { return data_ + len_; }

private:
    const CharType* data_ = nullptr;
    int32 len_ = 0;
};

using FStringView = TStringView<TCHAR>;
using FAnsiStringView = TStringView<ANSICHAR>;
using FUtf8StringView = TStringView<UTF8CHAR>;
