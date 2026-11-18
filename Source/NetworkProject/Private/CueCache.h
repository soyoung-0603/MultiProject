#pragma once

#include "CueTypes.h"

// Owns only the local cache directory, never the CueServer database.
class FCueCache
{
public:
    explicit FCueCache(FString InDirectory) : Directory(MoveTemp(InDirectory)) {}
    bool SaveSnapshot(const FString& Body, const FString& ShowId, FDateTime RefreshedAt, FString& Error) const;
    bool LoadSnapshot(const FString& ShowId, FCueSnapshot& Snapshot, FDateTime& RefreshedAt, FString& Error) const;
    bool SaveSelection(const FString& ShowId, FString& Error) const;
    bool LoadSelection(FString& ShowId, FString& Error) const;

private:
    FString Directory;
};
