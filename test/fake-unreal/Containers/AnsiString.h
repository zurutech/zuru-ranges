// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "Containers/UnrealString.h"

class FAnsiString : public TFakeString<ANSICHAR> {
public:
    using TFakeString::TFakeString;
};
