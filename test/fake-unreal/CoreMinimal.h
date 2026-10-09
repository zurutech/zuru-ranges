// Behavioural mirror of Unreal Engine's public API. Contains no Epic source. See README.md.
// Mirrors UE's CoreMinimal.h include set: TSortedMap/TSortedSet/TCompactSet/TStridedView are
// only forward-declared, and TDeque, TStaticArray, TIndirectArray, TChunkedArray, TRingBuffer,
// and the linked lists (List.h) are absent; include their headers explicitly.
#pragma once

#include "CoreTypes.h"
#include "Containers/ContainersFwd.h"
#include "Containers/Array.h"
#include "Containers/BitArray.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "Containers/SparseArray.h"
#include "Containers/StringView.h"
#include "Containers/UnrealString.h"
#include "Misc/Optional.h"
#include "Templates/SharedPointer.h"
#include "Templates/Tuple.h"
