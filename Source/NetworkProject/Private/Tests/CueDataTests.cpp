#include "CueJson.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCueJsonContractTest, "NetworkProject.Cue.JsonContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCueJsonContractTest::RunTest(const FString& Parameters)
{
    FString Body, Error;
    TestTrue(TEXT("Read shared server fixture"), FFileHelper::LoadFileToString(Body,
        *(FPaths::ProjectDir() / TEXT("CueServer/examples/cue-list.json"))));
    const FString ShowId = TEXT("f872900c-9595-44b7-b312-5c47e45c647b");
    FCueSnapshot Snapshot;
    if (!TestTrue(TEXT("Accept server schema v2 fixture"), CueJson::ParseSnapshot(Body, ShowId, Snapshot, Error)))
        return false;
    TestEqual(TEXT("Revision is preserved"), Snapshot.Revision, 6);
    TestEqual(TEXT("All cues retained"), Snapshot.Cues.Num(), 3);
    TestEqual(TEXT("Preset reference, not material values"), Snapshot.Cues[1].AppearancePreset, FName(TEXT("Preset_02")));

    // Number does not define the execution order.
    FString ReorderedNumbers = Body.Replace(TEXT("\"number\": 10"), TEXT("\"number\": 90"));
    TestTrue(TEXT("Accept descending display numbers"), CueJson::ParseSnapshot(ReorderedNumbers, ShowId, Snapshot, Error));
    TestEqual(TEXT("Position 1 stays first"), Snapshot.Cues[0].Number, 90);
    TestEqual(TEXT("Position 2 stays second"), Snapshot.Cues[1].Number, 20);

    const TArray<TPair<FString, FString>> Mutations = {
        { TEXT("\"schemaVersion\": 2"), TEXT("\"schemaVersion\": 1") },
        { TEXT("\"schemaVersion\": 2"), TEXT("\"schemaVersion\": true") },
        { TEXT("\"number\": 10"), TEXT("\"number\": 10.5") },
        { TEXT("\"number\": 20"), TEXT("\"number\": 10") },
        { TEXT("\"position\": 2"), TEXT("\"position\": 3") },
        { TEXT("\"name\": \"Standby\""), TEXT("\"name\": \"   \"") },
        { TEXT("\"cameraState\": \"Upper\""), TEXT("\"cameraState\": \"upper\"") },
        { TEXT("\"cameraBlendMs\": 1200"), TEXT("\"cameraBlendMs\": 30001") },
        { TEXT("\"lightingFadeMs\": 700"), TEXT("\"lightingFadeMs\": -1") },
        { TEXT("\"appearancePreset\": \"Preset_02\""), TEXT("\"appearancePreset\": null") },
        { TEXT("\"schemaVersion\": 2"), TEXT("\"shaderMode\": \"TOON\", \"schemaVersion\": 2") },
        { TEXT("2026-11-17T05:14:00.807491Z"), TEXT("not-a-date") },
        { TEXT("7b76404a-3675-49c7-bdbf-46c8edbc1ba9"), TEXT("e0504f2b-9857-49cf-9d0b-12670cc388db") }
    };
    for (const auto& Mutation : Mutations)
    {
        const FString Invalid = Body.Replace(*Mutation.Key, *Mutation.Value);
        TestFalse(*FString::Printf(TEXT("Reject mutation %s"), *Mutation.Value), CueJson::ParseSnapshot(Invalid, ShowId, Snapshot, Error));
        TestEqual(TEXT("Failed parse preserves previous snapshot"), Snapshot.Cues[0].Number, 90);
        TestFalse(TEXT("Failure has a diagnostic"), Error.IsEmpty());
    }
    TestFalse(TEXT("Reject another requested show"), CueJson::ParseSnapshot(Body,
        TEXT("00000000-0000-0000-0000-000000000001"), Snapshot, Error));
    TestFalse(TEXT("Reject malformed JSON"), CueJson::ParseSnapshot(TEXT("{broken"), ShowId, Snapshot, Error));

    const FString Empty = FString::Printf(TEXT("{\"schemaVersion\":2,\"showId\":\"%s\",\"showName\":\"Empty\",\"revision\":1,\"publishedAt\":\"2026-11-18T00:00:00Z\",\"cues\":[]}"), *ShowId);
    TestTrue(TEXT("Empty show is valid"), CueJson::ParseSnapshot(Empty, ShowId, Snapshot, Error));
    TestEqual(TEXT("Empty show has no playable cue"), Snapshot.Cues.Num(), 0);

    const FString Shows = FString::Printf(TEXT("[{\"id\":\"%s\",\"name\":\"Existing Show\",\"revision\":15,\"createdAt\":\"2026-11-17T00:00:00Z\",\"updatedAt\":\"2026-11-18T00:00:00Z\"}]"), *ShowId);
    TArray<FCueShowInfo> ParsedShows;
    TestTrue(TEXT("Parse show list array"), CueJson::ParseShows(Shows, ParsedShows, Error));
    TestEqual(TEXT("Show revision"), ParsedShows.IsEmpty() ? 0 : ParsedShows[0].Revision, 15);
    TestFalse(TEXT("Reject non-array show list"), CueJson::ParseShows(TEXT("{}"), ParsedShows, Error));
    TestEqual(TEXT("Bad list preserves old shows"), ParsedShows.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCueCameraContractTest, "NetworkProject.Cue.CameraContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCueCameraContractTest::RunTest(const FString& Parameters)
{
    FString Body, Error;
    if (!TestTrue(TEXT("Read shared fixture"), FFileHelper::LoadFileToString(Body,
        *(FPaths::ProjectDir() / TEXT("CueServer/examples/cue-list.json"))))) return false;
    const FString ShowId = TEXT("f872900c-9595-44b7-b312-5c47e45c647b");
    FCueSnapshot Snapshot;
    for (const TCHAR* Camera : {TEXT("Full"), TEXT("Upper"), TEXT("Close"), TEXT("Stage")})
    {
        // The third Cue reproduces a Stage failure after two valid Cues.
        FString CameraBody = Body;
        const int32 LastCamera = CameraBody.Find(TEXT("\"cameraState\": \"Full\""), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
        if (!TestTrue(TEXT("Find third Cue camera"), LastCamera != INDEX_NONE)) return false;
        CameraBody.RemoveAt(LastCamera, FString(TEXT("\"cameraState\": \"Full\"")).Len());
        CameraBody.InsertAt(LastCamera, FString::Printf(TEXT("\"cameraState\": \"%s\""), Camera));
        if (!TestTrue(*FString::Printf(TEXT("Accept Camera %s"), Camera),
            CueJson::ParseSnapshot(CameraBody, ShowId, Snapshot, Error))) continue;
        TestEqual(TEXT("Keep the complete Show"), Snapshot.Cues.Num(), 3);
        TestEqual(TEXT("Preserve Camera token spelling"), Snapshot.Cues[2].CameraState.ToString(), FString(Camera));
        TestEqual(TEXT("Keep earlier Camera"), Snapshot.Cues[1].CameraState.ToString(), FString(TEXT("Upper")));
    }
    // Seed a known valid snapshot so rejection must preserve it atomically.
    if (!TestTrue(TEXT("Seed previous snapshot"), CueJson::ParseSnapshot(Body, ShowId, Snapshot, Error))) return false;
    for (const TCHAR* Camera : {TEXT("Wide"), TEXT("stage"), TEXT("STAGE"), TEXT("Stage "), TEXT("")})
    {
        const FString Invalid = Body.Replace(TEXT("\"cameraState\": \"Upper\""),
            *FString::Printf(TEXT("\"cameraState\": \"%s\""), Camera), ESearchCase::CaseSensitive);
        TestFalse(*FString::Printf(TEXT("Reject invalid Camera '%s'"), Camera),
            CueJson::ParseSnapshot(Invalid, ShowId, Snapshot, Error));
        TestTrue(TEXT("Report offending Camera and Cue"), Error.StartsWith(TEXT("Cue position 2: Unsupported cameraState:")));
        TestEqual(TEXT("Failed parse preserves all previous Cues"), Snapshot.Cues.Num(), 3);
        TestEqual(TEXT("Failed parse preserves previous Camera"), Snapshot.Cues[1].CameraState.ToString(), FString(TEXT("Upper")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCueUnicodeTimestampTest, "NetworkProject.Cue.UnicodeTimestamp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCueUnicodeTimestampTest::RunTest(const FString& Parameters)
{
    FString Body, Error;
    FFileHelper::LoadFileToString(Body, *(FPaths::ProjectDir() / TEXT("CueServer/examples/cue-list.json")));
    const FString ShowId = TEXT("f872900c-9595-44b7-b312-5c47e45c647b");
    FCueSnapshot Snapshot;
    const FString NegativeOffset = Body.Replace(TEXT("2026-11-17T05:14:00.807491Z"), TEXT("2026-11-18T00:00:00-00:30"));
    TestTrue(TEXT("Accept negative sub-hour UTC offset"), CueJson::ParseSnapshot(NegativeOffset, ShowId, Snapshot, Error));
    TestEqual(TEXT("Preserve timezone sign when hour is zero"), Snapshot.PublishedAt, FDateTime(2026, 11, 18, 0, 30));
    for (const FString& Invalid : {FString(TEXT("2026/11/18T00:00:00ZjunkZ")), FString(TEXT("2026-11-18T00:00:00ZjunkZ")),
        FString(TEXT("2026-11-18T00:00:00+24:00")), FString(TEXT("2026-11-18T00:00:00+00:60")), FString(TEXT("2026-02-30T00:00:00Z"))})
    {
        TestFalse(*FString::Printf(TEXT("Reject invalid complete timestamp %s"), *Invalid),
            CueJson::ParseSnapshot(Body.Replace(TEXT("2026-11-17T05:14:00.807491Z"), *Invalid), ShowId, Snapshot, Error));
    }
    const FString Emoji = UTF8_TO_TCHAR("\xF0\x9F\x8E\xA4");
    FString Name;
    for (int32 Index = 0; Index < 100; ++Index) Name += Emoji;
    const FString ValidUnicode = Body.Replace(TEXT("\"name\": \"Standby\""), *FString::Printf(TEXT("\"name\": \"%s\""), *Name));
    TestTrue(TEXT("Accept 100 Unicode codepoints, not just 100 UTF16 units"), CueJson::ParseSnapshot(ValidUnicode, ShowId, Snapshot, Error));
    Name += Emoji;
    const FString TooLong = Body.Replace(TEXT("\"name\": \"Standby\""), *FString::Printf(TEXT("\"name\": \"%s\""), *Name));
    TestFalse(TEXT("Reject 101 Unicode codepoints"), CueJson::ParseSnapshot(TooLong, ShowId, Snapshot, Error));
    const FString EmbeddedNull = Body.Replace(TEXT("\"name\": \"Standby\""),
        *FString::Printf(TEXT("\"name\": \"A\\u0000%s\""), *FString::ChrN(101, TEXT('x'))));
    TestFalse(TEXT("Embedded NUL cannot hide characters beyond the name limit"), CueJson::ParseSnapshot(EmbeddedNull, ShowId, Snapshot, Error));
    return true;
}

#endif
