#include "CueJson.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <initializer_list>

namespace
{
    bool Fail(FString& Error, const FString& Message) { Error = Message; return false; }

    bool Fields(const TSharedPtr<FJsonObject>& Object, std::initializer_list<const TCHAR*> Required, FString& Error)
    {
        if (!Object.IsValid()) return Fail(Error, TEXT("Expected a JSON object."));
        for (const TCHAR* Key : Required)
            if (!Object->HasField(Key)) return Fail(Error, FString::Printf(TEXT("Missing field: %s"), Key));
        if (Object->Values.Num() != static_cast<int32>(Required.size()))
            return Fail(Error, TEXT("Unexpected JSON field (schema v2 only)."));
        return true;
    }

    bool String(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, FString& Out, FString& Error)
    {
        const auto Value = Object->TryGetField(Key);
        if (!Value.IsValid() || Value->Type != EJson::String)
            return Fail(Error, FString::Printf(TEXT("%s must be a string."), Key));
        Out = Value->AsString();
        return true;
    }

    bool Name(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, FString& Out, FString& Error)
    {
        if (!String(Object, Key, Out, Error)) return false;
        const FTCHARToUTF8 Utf8(*Out, Out.Len());
        int32 Codepoints = 0;
        for (int32 Index = 0; Index < Utf8.Length(); ++Index)
            if ((static_cast<uint8>(Utf8.Get()[Index]) & 0xC0) != 0x80) ++Codepoints;
        if (Codepoints < 1 || Codepoints > 100 || Out.TrimStartAndEnd().IsEmpty())
            return Fail(Error, FString::Printf(TEXT("%s must have 1-100 characters and not be blank."), Key));
        return true;
    }

    bool Integer(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, int32 Min, int32 Max, int32& Out, FString& Error)
    {
        const auto Value = Object->TryGetField(Key);
        if (!Value.IsValid() || Value->Type != EJson::Number)
            return Fail(Error, FString::Printf(TEXT("%s must be an integer."), Key));
        const double Number = Value->AsNumber();
        if (!FMath::IsFinite(Number) || Number < Min || Number > Max || Number != static_cast<double>(static_cast<int32>(Number)))
            return Fail(Error, FString::Printf(TEXT("%s must be an integer in [%d, %d]."), Key, Min, Max));
        Out = static_cast<int32>(Number);
        return true;
    }

    bool Uuid(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, FString& Out, FString& Error)
    {
        FString Raw;
        if (!String(Object, Key, Raw, Error)) return false;
        if (!CueJson::NormalizeShowId(Raw, Out))
            return Fail(Error, FString::Printf(TEXT("%s must be a hyphenated UUID."), Key));
        return true;
    }

    bool Date(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, FDateTime& Out, FString& Error)
    {
        FString Raw;
        if (!String(Object, Key, Raw, Error)) return false;
        if (!CueJson::ParseTimestamp(Raw, Out))
            return Fail(Error, FString::Printf(TEXT("%s must be an ISO 8601 timestamp with timezone."), Key));
        return true;
    }

    bool State(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, std::initializer_list<const TCHAR*> Allowed, FName& Out, FString& Error)
    {
        FString Raw;
        if (!String(Object, Key, Raw, Error)) return false;
        for (const TCHAR* Candidate : Allowed)
            if (Raw.Equals(Candidate, ESearchCase::CaseSensitive)) { Out = FName(*Raw); return true; }
        return Fail(Error, FString::Printf(TEXT("Unsupported %s: %s"), Key, *Raw));
    }

    bool BodySize(const FString& Body, FString& Error)
    {
        if (Body.Len() > CueJson::MaxBodyBytes || FTCHARToUTF8(*Body).Length() > CueJson::MaxBodyBytes)
            return Fail(Error, TEXT("JSON exceeds the 4 MiB limit."));
        return true;
    }
}

bool CueJson::ParseTimestamp(const FString& Input, FDateTime& Output)
{
    // Parse the complete RFC3339 shape. UE ParseIso8601 accepts trailing junk and loses -00:mm's sign.
    if (Input.Len() < 20 || Input[4] != TEXT('-') || Input[7] != TEXT('-') ||
        (Input[10] != TEXT('T') && Input[10] != TEXT('t')) || Input[13] != TEXT(':') || Input[16] != TEXT(':')) return false;
    const auto Digits = [&](int32 Start, int32 Count, int32& Result)
    {
        if (Start + Count > Input.Len()) return false;
        Result = 0;
        for (int32 Index = Start; Index < Start + Count; ++Index)
        {
            if (Input[Index] < TEXT('0') || Input[Index] > TEXT('9')) return false;
            Result = Result * 10 + Input[Index] - TEXT('0');
        }
        return true;
    };
    int32 Year, Month, Day, Hour, Minute, Second;
    if (!Digits(0, 4, Year) || !Digits(5, 2, Month) || !Digits(8, 2, Day) || !Digits(11, 2, Hour) ||
        !Digits(14, 2, Minute) || !Digits(17, 2, Second) || !FDateTime::Validate(Year, Month, Day, Hour, Minute, Second, 0)) return false;
    int32 Cursor = 19;
    int64 FractionTicks = 0;
    if (Input[Cursor] == TEXT('.'))
    {
        const int32 Start = ++Cursor;
        while (Cursor < Input.Len() && Input[Cursor] >= TEXT('0') && Input[Cursor] <= TEXT('9'))
        {
            if (Cursor - Start < 7) FractionTicks = FractionTicks * 10 + Input[Cursor] - TEXT('0');
            ++Cursor;
        }
        if (Cursor == Start) return false;
        for (int32 Index = Cursor - Start; Index < 7; ++Index) FractionTicks *= 10;
    }
    if (Cursor >= Input.Len()) return false;
    int32 OffsetMinutes = 0;
    if (Input[Cursor] == TEXT('Z') || Input[Cursor] == TEXT('z'))
    {
        if (Cursor + 1 != Input.Len()) return false;
    }
    else
    {
        const TCHAR Sign = Input[Cursor];
        int32 OffsetHour, OffsetMinute;
        if ((Sign != TEXT('+') && Sign != TEXT('-')) || Cursor + 6 != Input.Len() || Input[Cursor + 3] != TEXT(':') ||
            !Digits(Cursor + 1, 2, OffsetHour) || !Digits(Cursor + 4, 2, OffsetMinute) || OffsetHour > 23 || OffsetMinute > 59) return false;
        OffsetMinutes = (Sign == TEXT('-') ? -1 : 1) * (OffsetHour * 60 + OffsetMinute);
    }
    const int64 UtcTicks = FDateTime(Year, Month, Day, Hour, Minute, Second).GetTicks() + FractionTicks - FTimespan::FromMinutes(OffsetMinutes).GetTicks();
    if (UtcTicks < 0 || UtcTicks > FDateTime::MaxValue().GetTicks()) return false;
    Output = FDateTime(UtcTicks);
    return true;
}

bool CueJson::NormalizeShowId(const FString& Input, FString& Output)
{
    FGuid Id;
    if (!FGuid::ParseExact(Input, EGuidFormats::DigitsWithHyphens, Id)) return false;
    Output = Id.ToString(EGuidFormats::DigitsWithHyphensLower);
    return true;
}

bool CueJson::ParseShows(const FString& Json, TArray<FCueShowInfo>& Output, FString& Error)
{
    Error.Reset();
    if (!BodySize(Json, Error)) return false;
    TArray<TSharedPtr<FJsonValue>> Array;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Array))
        return Fail(Error, TEXT("Show response must be a JSON array."));
    TArray<FCueShowInfo> Parsed;
    TSet<FString> Ids;
    for (const auto& Value : Array)
    {
        if (!Value.IsValid() || Value->Type != EJson::Object) return Fail(Error, TEXT("Each Show must be an object."));
        const auto Object = Value->AsObject();
        FCueShowInfo Show;
        if (!Fields(Object, {TEXT("id"), TEXT("name"), TEXT("revision"), TEXT("createdAt"), TEXT("updatedAt")}, Error) ||
            !Uuid(Object, TEXT("id"), Show.Id, Error) || !Name(Object, TEXT("name"), Show.Name, Error) ||
            !Integer(Object, TEXT("revision"), 1, MAX_int32, Show.Revision, Error) ||
            !Date(Object, TEXT("createdAt"), Show.CreatedAt, Error) || !Date(Object, TEXT("updatedAt"), Show.UpdatedAt, Error)) return false;
        if (Ids.Contains(Show.Id)) return Fail(Error, TEXT("Duplicate Show ID."));
        Ids.Add(Show.Id);
        Parsed.Add(MoveTemp(Show));
    }
    Output = MoveTemp(Parsed);
    return true;
}

bool CueJson::ParseSnapshot(const FString& Json, const FString& ExpectedShowId, FCueSnapshot& Output, FString& Error)
{
    Error.Reset();
    if (!BodySize(Json, Error)) return false;
    FString Expected;
    if (!NormalizeShowId(ExpectedShowId, Expected)) return Fail(Error, TEXT("Requested Show ID is not a UUID."));
    TSharedPtr<FJsonObject> Object;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Object)) return Fail(Error, TEXT("Invalid Cue List JSON."));
    FCueSnapshot Parsed;
    if (!Fields(Object, {TEXT("schemaVersion"), TEXT("showId"), TEXT("showName"), TEXT("revision"), TEXT("publishedAt"), TEXT("cues")}, Error) ||
        !Integer(Object, TEXT("schemaVersion"), 2, 2, Parsed.SchemaVersion, Error) ||
        !Uuid(Object, TEXT("showId"), Parsed.ShowId, Error) || !Name(Object, TEXT("showName"), Parsed.ShowName, Error) ||
        !Integer(Object, TEXT("revision"), 1, MAX_int32, Parsed.Revision, Error) || !Date(Object, TEXT("publishedAt"), Parsed.PublishedAt, Error)) return false;
    if (Parsed.ShowId != Expected) return Fail(Error, TEXT("Response Show ID does not match the selected Show."));
    const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
    if (!Object->TryGetArrayField(TEXT("cues"), Array) || !Array || Array->Num() > 1000)
        return Fail(Error, TEXT("cues must be an array with at most 1000 entries."));
    TSet<FString> Ids;
    TSet<int32> Numbers;
    for (const auto& Value : *Array)
    {
        if (!Value.IsValid() || Value->Type != EJson::Object) return Fail(Error, TEXT("Each Cue must be an object."));
        const auto CueObject = Value->AsObject();
        FCueData Cue;
        if (!Fields(CueObject, {TEXT("id"), TEXT("position"), TEXT("number"), TEXT("name"), TEXT("animationState"), TEXT("cameraState"),
            TEXT("lightingState"), TEXT("appearancePreset"), TEXT("cameraBlendMs"), TEXT("lightingFadeMs")}, Error) ||
            !Uuid(CueObject, TEXT("id"), Cue.Id, Error) ||
            !Integer(CueObject, TEXT("position"), 1, 1000, Cue.Position, Error) ||
            !Integer(CueObject, TEXT("number"), 1, 999999, Cue.Number, Error) || !Name(CueObject, TEXT("name"), Cue.Name, Error) ||
            !State(CueObject, TEXT("animationState"), {TEXT("Idle"), TEXT("Talk"), TEXT("Dance")}, Cue.AnimationState, Error) ||
            !State(CueObject, TEXT("cameraState"), {TEXT("Full"), TEXT("Upper"), TEXT("Close"), TEXT("Stage")}, Cue.CameraState, Error) ||
            !State(CueObject, TEXT("lightingState"), {TEXT("Neutral"), TEXT("Focus"), TEXT("Stage")}, Cue.LightingState, Error) ||
            !State(CueObject, TEXT("appearancePreset"), {TEXT("Preset_01"), TEXT("Preset_02"), TEXT("Preset_03"), TEXT("Preset_04"), TEXT("Preset_05")}, Cue.AppearancePreset, Error) ||
            !Integer(CueObject, TEXT("cameraBlendMs"), 0, 30000, Cue.CameraBlendMs, Error) ||
            !Integer(CueObject, TEXT("lightingFadeMs"), 0, 30000, Cue.LightingFadeMs, Error))
        {
            Error = FString::Printf(TEXT("Cue position %d: %s"), Parsed.Cues.Num() + 1, *Error);
            return false;
        }
        if (Cue.Position != Parsed.Cues.Num() + 1) return Fail(Error, TEXT("Cue positions must be ordered and consecutive from 1."));
        if (Ids.Contains(Cue.Id) || Numbers.Contains(Cue.Number)) return Fail(Error, TEXT("Duplicate Cue ID or number."));
        Ids.Add(Cue.Id);
        Numbers.Add(Cue.Number);
        Parsed.Cues.Add(MoveTemp(Cue));
    }
    Output = MoveTemp(Parsed);
    return true;
}
