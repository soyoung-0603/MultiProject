#pragma once

#include "CueTypes.h"

namespace CueJson
{
    constexpr int32 MaxBodyBytes = 4 * 1024 * 1024;
    bool ParseTimestamp(const FString& Input, FDateTime& Output);
    bool NormalizeShowId(const FString& Input, FString& Output);
    bool ParseShows(const FString& Json, TArray<FCueShowInfo>& Output, FString& Error);
    bool ParseSnapshot(const FString& Json, const FString& ExpectedShowId, FCueSnapshot& Output, FString& Error);
}
