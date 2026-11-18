#include "CueHttpResponseBody.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCueResponseLimitTest, "NetworkProject.Cue.ResponseLimit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCueResponseLimitTest::RunTest(const FString& Parameters)
{
    FCueHttpResponseBody Body;
    TArray<uint8> Bytes;
    Bytes.Init('x', CueJson::MaxBodyBytes);
    int64 Length = Bytes.Num();
    Body.Receive(Bytes.GetData(), Length);
    TestEqual(TEXT("Accept exactly the limit"), Length, static_cast<int64>(CueJson::MaxBodyBytes));
    FString Text, Error;
    TestTrue(TEXT("Body at the boundary remains readable"), Body.Read(Text, Error));
    TestEqual(TEXT("Boundary body length"), Text.Len(), CueJson::MaxBodyBytes);
    Length = 1;
    Body.Receive(Bytes.GetData(), Length);
    TestEqual(TEXT("Reject overflow while receiving, before completion"), Length, static_cast<int64>(0));
    Text = TEXT("unchanged");
    TestFalse(TEXT("Overflow cannot become a parseable response"), Body.Read(Text, Error));
    TestEqual(TEXT("Failed read preserves previous output"), Text, FString(TEXT("unchanged")));
    TestFalse(TEXT("Overflow diagnostic"), Error.IsEmpty());
    return true;
}
#endif
