#include "CuePlayerSubsystem.h"
#include "CuePlayerSettings.h"
#include "Engine/GameInstance.h"
#include "Async/TaskGraphInterfaces.h"
#include "HAL/FileManager.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

#if WITH_DEV_AUTOMATION_TESTS
struct FCueSubsystemTestAccess
{
    static void SetDirectory(UCuePlayerSubsystem* Player, const FString& Directory) { Player->CacheDirectory = Directory; }
    static uint64 GetGeneration(UCuePlayerSubsystem* Player) { return Player->CueGeneration; }
    static FHttpRequestPtr GetRequest(UCuePlayerSubsystem* Player) { return Player->CueListRequest; }
    static void DeliverOldCallback(UCuePlayerSubsystem* Player, FHttpRequestPtr Request, uint64 Generation, const FString& ShowId, const FString& Url)
    { Player->HandleCueResponse(Request, nullptr, false, Generation, ShowId, Url); }
};

namespace
{
    bool PumpUntil(TFunctionRef<bool()> Done, double Timeout = 6.0)
    {
        const double Start = FPlatformTime::Seconds();
        while (!Done() && FPlatformTime::Seconds() - Start < Timeout)
        {
            FHttpModule::Get().GetHttpManager().Tick(0.01f);
            FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
            FPlatformProcess::Sleep(0.01f);
        }
        return Done();
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCueHttpContractTest, "NetworkProject.Cue.HttpContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCueHttpContractTest::RunTest(const FString& Parameters)
{
    FString FixtureUrl;
    if (!FParse::Value(FCommandLine::Get(), TEXT("CueHttpTestUrl="), FixtureUrl))
    { AddError(TEXT("Start Tests/cue_http_fixture.py and pass -CueHttpTestUrl=http://127.0.0.1:18081")); return false; }
    auto* Settings = GetMutableDefault<UCuePlayerSettings>();
    const FString PreviousUrl = Settings->ServerBaseUrl;
    const float PreviousTimeout = Settings->RequestTimeoutSeconds;
    const FString Base = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation/CueHttpTests"));
    const FString Root = Base / FGuid::NewGuid().ToString(EGuidFormats::Digits);
    auto* Instance = NewObject<UGameInstance>();
    Instance->AddToRoot();
    auto* Player = NewObject<UCuePlayerSubsystem>(Instance);
    Player->AddToRoot();
    FCueSubsystemTestAccess::SetDirectory(Player, Root);
    ON_SCOPE_EXIT
    {
        Player->Deinitialize();
        Player->RemoveFromRoot();
        Instance->RemoveFromRoot();
        Settings->ServerBaseUrl = PreviousUrl;
        Settings->RequestTimeoutSeconds = PreviousTimeout;
        if (FPaths::IsUnderDirectory(Root, Base)) IFileManager::Get().DeleteDirectory(*Root, false, true);
    };
    const FString ShowId = TEXT("f872900c-9595-44b7-b312-5c47e45c647b");
    Settings->ServerBaseUrl = FixtureUrl / TEXT("normal");
    Settings->RequestTimeoutSeconds = 2.0f;
    FString Error;
    if (!TestTrue(TEXT("Select Show without making an HTTP request"), Player->SelectShow(ShowId, Error))) return false;
    TestFalse(TEXT("Selected Show initially has no snapshot"), Player->GetStatus().bHasSnapshot);
    if (!TestTrue(TEXT("Start real HTTP Show query"), Player->RequestShows())) return false;
    TestTrue(TEXT("Show query completes"), PumpUntil([&] { return Player->GetStatus().ShowListRequestState != ECueRequestState::Requesting; }));
    TestEqual(TEXT("Show query success"), Player->GetStatus().ShowListRequestState, ECueRequestState::Succeeded);
    TestEqual(TEXT("HTTP list parsed"), Player->GetShows().Num(), 1);
    if (!TestTrue(TEXT("Start REFRESH"), Player->RefreshCueList())) return false;
    TestTrue(TEXT("REFRESH completes"), PumpUntil([&] { return Player->GetStatus().CueListRequestState != ECueRequestState::Requesting; }));
    FCueSnapshot Snapshot;
    if (!TestTrue(TEXT("REFRESH activates a saved snapshot"), Player->GetSnapshot(Snapshot))) return false;
    TestEqual(TEXT("Received revision"), Snapshot.Revision, 6);
    const FDateTime LastSuccess = Player->GetStatus().LastSuccessfulRefreshUtc;
    TestTrue(TEXT("Local data available"), Player->GetStatus().bCanUseLocalSnapshot);

    const FString StageShowId = TEXT("00000000-0000-0000-0000-000000000002");
    Settings->ServerBaseUrl = FixtureUrl / TEXT("stage");
    if (!TestTrue(TEXT("Select Show containing Stage"), Player->SelectShow(StageShowId, Error))) return false;
    TestTrue(TEXT("Refresh Stage Show over HTTP"), Player->RefreshCueList());
    TestTrue(TEXT("Stage REFRESH completes"), PumpUntil([&] { return Player->GetStatus().CueListRequestState != ECueRequestState::Requesting; }));
    TestEqual(TEXT("Stage REFRESH succeeds"), Player->GetStatus().CueListRequestState, ECueRequestState::Succeeded);
    if (TestTrue(TEXT("Stage Show is available"), Player->GetSnapshot(Snapshot)) &&
        TestEqual(TEXT("Stage Show retains all Cues"), Snapshot.Cues.Num(), 3))
        TestEqual(TEXT("HTTP preserves Stage token"), Snapshot.Cues[2].CameraState.ToString(), FString(TEXT("Stage")));

    for (int32 Switch = 0; Switch < 3; ++Switch)
    {
        TestTrue(TEXT("Return to normal Show offline"), Player->SelectShow(ShowId, Error));
        if (TestTrue(TEXT("Normal Show restores from cache"), Player->GetSnapshot(Snapshot)) &&
            TestEqual(TEXT("Normal Show keeps all Cues"), Snapshot.Cues.Num(), 3))
            TestEqual(TEXT("Normal Camera survives switching"), Snapshot.Cues[2].CameraState.ToString(), FString(TEXT("Full")));
        TestTrue(TEXT("Return to Stage Show offline"), Player->SelectShow(StageShowId, Error));
        if (TestTrue(TEXT("Stage Show restores from cache"), Player->GetSnapshot(Snapshot)) &&
            TestEqual(TEXT("Cached Stage Show keeps all Cues"), Snapshot.Cues.Num(), 3))
            TestEqual(TEXT("Cache preserves Stage token"), Snapshot.Cues[2].CameraState.ToString(), FString(TEXT("Stage")));
    }
    TestTrue(TEXT("Return to original Show for failure tests"), Player->SelectShow(ShowId, Error));

    for (const FString& Scenario : {FString(TEXT("bad")), FString(TEXT("error")), FString(TEXT("wrong-show")), FString(TEXT("large")), FString(TEXT("invalid-camera"))})
    {
        Settings->ServerBaseUrl = FixtureUrl / Scenario;
        TestTrue(TEXT("Start failing REFRESH"), Player->RefreshCueList());
        TestTrue(TEXT("Failing REFRESH finishes"), PumpUntil([&] { return Player->GetStatus().CueListRequestState != ECueRequestState::Requesting; }));
        TestEqual(TEXT("Failure is separate from local readiness"), Player->GetStatus().CueListRequestState, ECueRequestState::Failed);
        TestTrue(TEXT("Old snapshot remains available"), Player->GetSnapshot(Snapshot));
        TestEqual(TEXT("Failure preserves revision"), Snapshot.Revision, 6);
        TestEqual(TEXT("Failure preserves last success"), Player->GetStatus().LastSuccessfulRefreshUtc, LastSuccess);
    }
    Settings->ServerBaseUrl = FixtureUrl / TEXT("bad");
    TestTrue(TEXT("Start invalid Show query"), Player->RequestShows());
    PumpUntil([&] { return Player->GetStatus().ShowListRequestState != ECueRequestState::Requesting; });
    TestEqual(TEXT("Invalid query preserves prior Show list"), Player->GetShows().Num(), 1);

    Settings->ServerBaseUrl = FixtureUrl / TEXT("timeout");
    Settings->RequestTimeoutSeconds = 0.1f;
    TestTrue(TEXT("Start timeout request"), Player->RefreshCueList());
    TestTrue(TEXT("Timeout ends request"), PumpUntil([&] { return Player->GetStatus().CueListRequestState != ECueRequestState::Requesting; }));
    TestEqual(TEXT("Timeout is failure"), Player->GetStatus().CueListRequestState, ECueRequestState::Failed);
    TestTrue(TEXT("Timeout preserves cached data"), Player->GetStatus().bCanUseLocalSnapshot);

    Settings->RequestTimeoutSeconds = 2.0f;
    Settings->ServerBaseUrl = FixtureUrl / TEXT("slow");
    Player->RefreshCueList();
    const auto OldRequest = FCueSubsystemTestAccess::GetRequest(Player);
    const auto OldGeneration = FCueSubsystemTestAccess::GetGeneration(Player);
    Settings->ServerBaseUrl = FixtureUrl / TEXT("new");
    Player->RefreshCueList();
    PumpUntil([&] { return Player->GetStatus().CueListRequestState != ECueRequestState::Requesting; });
    FCueSubsystemTestAccess::DeliverOldCallback(Player, OldRequest, OldGeneration, ShowId, FixtureUrl / TEXT("slow"));
    Player->GetSnapshot(Snapshot);
    TestEqual(TEXT("Superseded callback cannot replace newer revision"), Snapshot.Revision, 7);
    TestEqual(TEXT("Old failure cannot replace new server success"), Player->GetStatus().CueListRequestState, ECueRequestState::Succeeded);

    Settings->ServerBaseUrl = FixtureUrl / TEXT("slow");
    Player->RefreshCueList();
    const auto SelectedRequest = FCueSubsystemTestAccess::GetRequest(Player);
    const auto SelectedGeneration = FCueSubsystemTestAccess::GetGeneration(Player);
    const FString OtherId = TEXT("00000000-0000-0000-0000-000000000001");
    TestTrue(TEXT("Change selection during request"), Player->SelectShow(OtherId, Error));
    FCueSubsystemTestAccess::DeliverOldCallback(Player, SelectedRequest, SelectedGeneration, ShowId, Settings->ServerBaseUrl);
    TestFalse(TEXT("Old Show is not exposed as new selection"), Player->GetSnapshot(Snapshot));
    TestEqual(TEXT("Selection stays on new Show"), Player->GetStatus().SelectedShowId, OtherId);
    TestTrue(TEXT("Select previous Show offline"), Player->SelectShow(ShowId, Error));
    TestTrue(TEXT("Previous Show restores from disk without HTTP"), Player->GetSnapshot(Snapshot));
    TestEqual(TEXT("Disk retains newest verified revision"), Snapshot.Revision, 7);

    auto* Restarted = NewObject<UCuePlayerSubsystem>(Instance);
    FSubsystemCollection<UGameInstanceSubsystem> Collection;
    Restarted->Initialize(Collection);
    TestTrue(TEXT("Initialize starts without a selected Show"), Restarted->GetStatus().SelectedShowId.IsEmpty());
    TestFalse(TEXT("Initialize starts without a snapshot"), Restarted->GetSnapshot(Snapshot));
    FCueSubsystemTestAccess::SetDirectory(Restarted, Root);
    TestFalse(TEXT("Fresh subsystem cannot restore an unselected Show"), Restarted->RestoreSelectedSnapshot(Error));
    TestTrue(TEXT("Saved selection is not automatically selected"), Restarted->GetStatus().SelectedShowId.IsEmpty());
    TestFalse(TEXT("Fresh subsystem has no displayed snapshot"), Restarted->GetSnapshot(Snapshot));
    TestFalse(TEXT("Fresh subsystem has no local readiness"), Restarted->GetStatus().bCanUseLocalSnapshot);
    TestTrue(TEXT("Fresh subsystem requests Show options"), Restarted->RequestShows());
    PumpUntil([&] { return Restarted->GetStatus().ShowListRequestState != ECueRequestState::Requesting; });
    TestEqual(TEXT("Show options still load"), Restarted->GetShows().Num(), 1);
    TestTrue(TEXT("Show options do not select the first Show"), Restarted->GetStatus().SelectedShowId.IsEmpty());
    TestFalse(TEXT("Loading options keeps Cue List empty"), Restarted->GetSnapshot(Snapshot));
    TestTrue(TEXT("Operator can explicitly select cached Show"), Restarted->SelectShow(ShowId, Error));
    TestTrue(TEXT("Explicit selection restores cached snapshot"), Restarted->GetSnapshot(Snapshot));
    TestEqual(TEXT("Fresh subsystem restored revision"), Snapshot.Revision, 7);
    TestTrue(TEXT("Explicitly selected Show can still be restored"), Restarted->RestoreSelectedSnapshot(Error));
    Restarted->Deinitialize();

    Player->RefreshCueList();
    Player->CancelRequests();
    TestEqual(TEXT("Explicit cancel releases request state"), Player->GetStatus().CueListRequestState, ECueRequestState::NotRequested);
    TestTrue(TEXT("Cancel preserves snapshot"), Player->GetStatus().bCanUseLocalSnapshot);
    return true;
}
#endif
