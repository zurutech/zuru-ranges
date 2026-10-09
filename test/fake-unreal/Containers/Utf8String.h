// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See ../README.md.
#pragma once

#include "Containers/UnrealString.h"

class FUtf8String : public TFakeString<UTF8CHAR> {
public:
    using TFakeString::TFakeString;
};
