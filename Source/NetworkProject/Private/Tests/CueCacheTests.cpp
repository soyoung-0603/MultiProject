#include "CueCache.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCueCacheContractTest, "NetworkProject.Cue.CacheContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCueCacheContractTest::RunTest(const FString& Parameters)
{
    const FString TestBase = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation/CueTests"));
    const FString Root = TestBase / FGuid::NewGuid().ToString(EGuidFormats::Digits);
    ON_SCOPE_EXIT { if (FPaths::IsUnderDirectory(Root, TestBase)) IFileManager::Get().DeleteDirectory(*Root, false, true); };
    FCueCache Cache(Root);
    FString Body, Error;
    FFileHelper::LoadFileToString(Body, *(FPaths::ProjectDir() / TEXT("CueServer/examples/cue-list.json")));
    const FString ShowId = TEXT("f872900c-9595-44b7-b312-5c47e45c647b");
    const FDateTime Time(2026, 11, 18, 1, 2, 3);
    if (!TestTrue(TEXT("Persist a verified snapshot"), Cache.SaveSnapshot(Body, ShowId, Time, Error))) return false;
    TestTrue(TEXT("Persist selection"), Cache.SaveSelection(ShowId, Error));

    FCueCache Restarted(Root);
    FString Selected;
    TestTrue(TEXT("Restore selection in a new cache instance"), Restarted.LoadSelection(Selected, Error));
    TestEqual(TEXT("Restored UUID"), Selected, ShowId);
    FCueSnapshot Snapshot;
    FDateTime RestoredAt;
    TestTrue(TEXT("Restore snapshot after restart"), Restarted.LoadSnapshot(Selected, Snapshot, RestoredAt, Error));
    TestEqual(TEXT("Revision persists"), Snapshot.Revision, 6);
    TestEqual(TEXT("Refresh time persists separately from publication"), RestoredAt, Time);

    TestFalse(TEXT("Invalid JSON cannot overwrite cache"), Cache.SaveSnapshot(TEXT("{}"), ShowId, Time + FTimespan::FromDays(1), Error));
    TestFalse(TEXT("Prevent path traversal"), Cache.SaveSelection(TEXT("../../other"), Error));
    TestTrue(TEXT("Normal file remains readable"), Restarted.LoadSnapshot(ShowId, Snapshot, RestoredAt, Error));
    TestEqual(TEXT("Failed save keeps success time"), RestoredAt, Time);

    const FString CachePath = Root / (ShowId + TEXT(".json"));
#if PLATFORM_WINDOWS
    // Deny replacement while permitting reads, to exercise the real OS failure path.
    HANDLE Lock = CreateFileW(*CachePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (TestTrue(TEXT("Lock the normal cache file"), Lock != INVALID_HANDLE_VALUE))
    {
        const FString NewBody = Body.Replace(TEXT("\"revision\": 6"), TEXT("\"revision\": 7"));
        TestFalse(TEXT("Replacement failure is reported"), Cache.SaveSnapshot(NewBody, ShowId, Time + FTimespan::FromDays(1), Error));
        CloseHandle(Lock);
        TestTrue(TEXT("Failed replacement leaves previous normal file"), Restarted.LoadSnapshot(ShowId, Snapshot, RestoredAt, Error));
        TestEqual(TEXT("Failed replacement preserves revision"), Snapshot.Revision, 6);
        TestEqual(TEXT("Failed replacement preserves timestamp"), RestoredAt, Time);
    }
#endif
    FFileHelper::SaveStringToFile(TEXT("broken"), *CachePath);
    TestFalse(TEXT("Reject corrupt file on startup"), Restarted.LoadSnapshot(ShowId, Snapshot, RestoredAt, Error));
    TestEqual(TEXT("Failed load preserves caller output"), Snapshot.Revision, 6);
    return true;
}
#endif
