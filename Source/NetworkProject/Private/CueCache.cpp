#include "CueCache.h"

#include "CueJson.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

namespace
{
    bool Fail(FString& Error, const FString& Message) { Error = Message; return false; }

    bool AtomicWrite(const FString& Path, const TSharedRef<FJsonObject>& Object, FString& Error)
    {
        Error.Reset();
        auto& Files = FPlatformFileManager::Get().GetPlatformFile();
        const FString Absolute = FPaths::ConvertRelativePathToFull(Path);
        if (!Files.CreateDirectoryTree(*FPaths::GetPath(Absolute))) return Fail(Error, TEXT("Cannot create the Cue cache directory."));
        FString Json;
        if (!FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Json))) return Fail(Error, TEXT("Cannot serialize the Cue cache."));
        const FString Temp = Absolute + TEXT(".") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".tmp");
        bool bWritten = false;
        {
            TUniquePtr<IFileHandle> Handle(Files.OpenWrite(*Temp));
            const FTCHARToUTF8 Utf8(*Json);
            bWritten = Handle && Handle->Write(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length()) && Handle->Flush(true);
        }
        if (!bWritten)
        {
            Files.DeleteFile(*Temp);
            return Fail(Error, TEXT("Cannot write/flush the Cue cache temporary file. Previous cache preserved."));
        }
        bool bReplaced = false;
#if PLATFORM_WINDOWS
        // Same-directory rename, without deleting the last normal file first.
        bReplaced = !!MoveFileExW(*Temp, *Absolute, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
#endif
        if (!bReplaced)
        {
            Files.DeleteFile(*Temp);
            return Fail(Error, TEXT("Cannot atomically replace the Cue cache (Win64). Previous cache preserved."));
        }
        return true;
    }

    bool ReadObject(const FString& Path, TSharedPtr<FJsonObject>& Object, FString& Error)
    {
        const int64 Size = FPlatformFileManager::Get().GetPlatformFile().FileSize(*Path);
        if (Size < 0) return Fail(Error, TEXT("No local cache file for this selection."));
        if (Size > CueJson::MaxBodyBytes + 4096) return Fail(Error, TEXT("Local cache exceeds the size limit."));
        FString Body;
        if (!FFileHelper::LoadFileToString(Body, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Object) || !Object.IsValid())
            return Fail(Error, TEXT("Cannot read a valid local Cue cache object."));
        return true;
    }
}

bool FCueCache::SaveSnapshot(const FString& Body, const FString& ShowId, FDateTime RefreshedAt, FString& Error) const
{
    FCueSnapshot Validated;
    if (!CueJson::ParseSnapshot(Body, ShowId, Validated, Error)) return false;
    TSharedPtr<FJsonObject> Snapshot;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Snapshot)) return Fail(Error, TEXT("Cannot serialize the validated snapshot."));
    const auto Envelope = MakeShared<FJsonObject>();
    Envelope->SetNumberField(TEXT("cacheVersion"), 1);
    Envelope->SetStringField(TEXT("lastSuccessfulRefreshUtc"), RefreshedAt.ToIso8601());
    Envelope->SetObjectField(TEXT("snapshot"), Snapshot);
    return AtomicWrite(Directory / (Validated.ShowId + TEXT(".json")), Envelope, Error);
}

bool FCueCache::LoadSnapshot(const FString& ShowId, FCueSnapshot& Snapshot, FDateTime& RefreshedAt, FString& Error) const
{
    Error.Reset();
    FString Normalized;
    if (!CueJson::NormalizeShowId(ShowId, Normalized)) return Fail(Error, TEXT("Invalid cache Show UUID."));
    TSharedPtr<FJsonObject> Envelope;
    if (!ReadObject(Directory / (Normalized + TEXT(".json")), Envelope, Error)) return false;
    const auto Version = Envelope->TryGetField(TEXT("cacheVersion"));
    const auto Time = Envelope->TryGetField(TEXT("lastSuccessfulRefreshUtc"));
    const TSharedPtr<FJsonObject>* Object = nullptr;
    FDateTime ParsedTime;
    if (Envelope->Values.Num() != 3 || !Version.IsValid() || Version->Type != EJson::Number || Version->AsNumber() != 1 ||
        !Time.IsValid() || Time->Type != EJson::String || !Time->AsString().EndsWith(TEXT("Z")) ||
        !CueJson::ParseTimestamp(Time->AsString(), ParsedTime) || !Envelope->TryGetObjectField(TEXT("snapshot"), Object))
        return Fail(Error, TEXT("Invalid local cache envelope or timestamp."));
    FString Body;
    if (!FJsonSerializer::Serialize(Object->ToSharedRef(), TJsonWriterFactory<>::Create(&Body))) return Fail(Error, TEXT("Invalid cached snapshot."));
    FCueSnapshot Parsed;
    if (!CueJson::ParseSnapshot(Body, Normalized, Parsed, Error)) return false;
    Snapshot = MoveTemp(Parsed);
    RefreshedAt = ParsedTime;
    return true;
}

bool FCueCache::SaveSelection(const FString& ShowId, FString& Error) const
{
    FString Normalized;
    if (!CueJson::NormalizeShowId(ShowId, Normalized)) return Fail(Error, TEXT("Selected Show ID must be a UUID."));
    const auto Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("showId"), Normalized);
    return AtomicWrite(Directory / TEXT("selection.json"), Object, Error);
}

bool FCueCache::LoadSelection(FString& ShowId, FString& Error) const
{
    Error.Reset();
    TSharedPtr<FJsonObject> Object;
    if (!ReadObject(Directory / TEXT("selection.json"), Object, Error)) return false;
    FString Raw, Parsed;
    const auto Value = Object->TryGetField(TEXT("showId"));
    if (Object->Values.Num() != 1 || !Value.IsValid() || Value->Type != EJson::String || !CueJson::NormalizeShowId(Value->AsString(), Parsed))
        return Fail(Error, TEXT("Invalid local Show selection."));
    ShowId = MoveTemp(Parsed);
    return true;
}
