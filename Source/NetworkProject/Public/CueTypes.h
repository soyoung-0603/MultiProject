#pragma once

#include "CoreMinimal.h"
#include "CueTypes.generated.h"

// Transport state only. Existing Blueprint E_CueItemState owns the visual item states.
UENUM(BlueprintType)
enum class ECueRequestState : uint8
{
    NotRequested,
    Requesting,
    Succeeded,
    Failed
};

USTRUCT(BlueprintType)
struct NETWORKPROJECT_API FCueShowInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Cue") FString Id;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FString Name;
    UPROPERTY(BlueprintReadOnly, Category="Cue") int32 Revision = 0;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FDateTime CreatedAt;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FDateTime UpdatedAt;
};

USTRUCT(BlueprintType)
struct NETWORKPROJECT_API FCueData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Cue") FString Id;
    UPROPERTY(BlueprintReadOnly, Category="Cue") int32 Position = 0;
    UPROPERTY(BlueprintReadOnly, Category="Cue") int32 Number = 0;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FString Name;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FName AnimationState;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FName CameraState;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FName LightingState;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FName AppearancePreset;
    UPROPERTY(BlueprintReadOnly, Category="Cue") int32 CameraBlendMs = 0;
    UPROPERTY(BlueprintReadOnly, Category="Cue") int32 LightingFadeMs = 0;
};

USTRUCT(BlueprintType)
struct NETWORKPROJECT_API FCueSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Cue") int32 SchemaVersion = 2;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FString ShowId;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FString ShowName;
    UPROPERTY(BlueprintReadOnly, Category="Cue") int32 Revision = 0;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FDateTime PublishedAt;
    UPROPERTY(BlueprintReadOnly, Category="Cue") TArray<FCueData> Cues;
};

USTRUCT(BlueprintType)
struct NETWORKPROJECT_API FCuePlayerStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Cue") FString SelectedShowId;
    UPROPERTY(BlueprintReadOnly, Category="Cue") ECueRequestState ShowListRequestState = ECueRequestState::NotRequested;
    UPROPERTY(BlueprintReadOnly, Category="Cue") ECueRequestState CueListRequestState = ECueRequestState::NotRequested;
    UPROPERTY(BlueprintReadOnly, Category="Cue") bool bHasSnapshot = false;
    // Data availability, not permission to GO. Runtime execution is a later stage.
    UPROPERTY(BlueprintReadOnly, Category="Cue") bool bCanUseLocalSnapshot = false;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FDateTime LastSuccessfulRefreshUtc;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FString ShowListError;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FString CueListError;
    UPROPERTY(BlueprintReadOnly, Category="Cue") FString CacheError;
};
