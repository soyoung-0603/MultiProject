#include "CuePlayerSubsystem.h"
#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCueBlueprintSurfaceTest, "NetworkProject.Cue.BlueprintSurface",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCueBlueprintSurfaceTest::RunTest(const FString& Parameters)
{
    TArray<TSharedPtr<FJsonValue>> Surfaces;
    const auto Record = [&](UFunction* Function)
    {
        if (!Function) return;
        auto Object = MakeShared<FJsonObject>();
        Object->SetStringField(TEXT("name"), Function->GetName());
        Object->SetStringField(TEXT("node"), Function->GetDisplayNameText().ToString());
        TArray<TSharedPtr<FJsonValue>> Pins;
        for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
        {
            auto Pin = MakeShared<FJsonObject>();
            Pin->SetStringField(TEXT("name"), It->GetName());
            Pin->SetStringField(TEXT("type"), It->GetCPPType());
            Pin->SetStringField(TEXT("direction"), It->HasAnyPropertyFlags(CPF_ReturnParm) ? TEXT("Return") :
                (It->HasAnyPropertyFlags(CPF_OutParm) && !It->HasAnyPropertyFlags(CPF_ConstParm) ? TEXT("Output") : TEXT("Input")));
            Pins.Add(MakeShared<FJsonValueObject>(Pin));
        }
        Object->SetArrayField(TEXT("pins"), Pins);
        Surfaces.Add(MakeShared<FJsonValueObject>(Object));
    };
    for (const TCHAR* Name : {TEXT("RequestShows"), TEXT("SelectShow"), TEXT("RefreshCueList"), TEXT("RestoreSelectedSnapshot"),
        TEXT("CancelRequests"), TEXT("GetSnapshot"), TEXT("GetShows"), TEXT("GetStatus"), TEXT("GetServerBaseUrl")})
    {
        UFunction* Function = UCuePlayerSubsystem::StaticClass()->FindFunctionByName(Name);
        TestNotNull(Name, Function);
        Record(Function);
    }
    // Read reflected signatures only; do not call UI functions, compile or save assets.
    for (const auto& Pair : TArray<TPair<FString, FString>>{
        {TEXT("/Game/Blueprints/UI/WBP_Operator.WBP_Operator_C"), TEXT("AddCueItem")},
        {TEXT("/Game/Blueprints/UI/WBP_CueItem.WBP_CueItem_C"), TEXT("InitializeCueItem")},
        {TEXT("/Game/Blueprints/UI/WBP_CueItem.WBP_CueItem_C"), TEXT("SetCueItemState")}})
    {
        UClass* Class = LoadClass<UObject>(nullptr, *Pair.Key);
        if (!TestNotNull(*Pair.Key, Class)) continue;
        UFunction* Function = Class->FindFunctionByName(*Pair.Value);
        TestNotNull(*Pair.Value, Function);
        Record(Function);
    }
    auto Root = MakeShared<FJsonObject>();
    Root->SetArrayField(TEXT("functions"), Surfaces);
    FString Json;
    FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Json));
    TestTrue(TEXT("Export exact reflected Blueprint pins"), FFileHelper::SaveStringToFile(Json,
        *(FPaths::ProjectSavedDir() / TEXT("CueImplementation/blueprint-surface.json"))));
    return true;
}
#endif
