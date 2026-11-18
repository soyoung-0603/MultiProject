#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CuePlayerSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Cue Player"))
class NETWORKPROJECT_API UCuePlayerSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    virtual FName GetCategoryName() const override { return TEXT("Game"); }

    // Server origin or URL prefix, without /api/v1. No request is sent automatically.
    UPROPERTY(Config, EditAnywhere, Category="Server")
    FString ServerBaseUrl = TEXT("http://127.0.0.1:8001");

    UPROPERTY(Config, EditAnywhere, Category="Server", meta=(ClampMin="0.1", ClampMax="120.0", Units="s"))
    float RequestTimeoutSeconds = 10.0f;
};
