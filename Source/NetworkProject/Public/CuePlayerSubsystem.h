#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "CueTypes.h"
#include "CuePlayerSubsystem.generated.h"

class FCueHttpResponseBody;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCueShowsReceived, bool, bSuccess, const TArray<FCueShowInfo>&, Shows, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCueSnapshotChanged, bool, bHasSnapshot, const FCueSnapshot&, Snapshot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCueRefreshCompleted, bool, bSuccess, const FString&, ShowId, const FString&, Error);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCueStatusChanged, const FCuePlayerStatus&, Status);

// Data foundation only: no GO/STOP/RESET, scene commands, or UI creation.
UCLASS(BlueprintType)
class NETWORKPROJECT_API UCuePlayerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="Cue Player", meta=(DisplayName="Request Shows"))
    bool RequestShows();

    // Persists only the Show ID. A new selection may have no local snapshot.
    UFUNCTION(BlueprintCallable, Category="Cue Player", meta=(DisplayName="Select Show"))
    bool SelectShow(const FString& ShowId, FString& Error);

    UFUNCTION(BlueprintCallable, Category="Cue Player", meta=(DisplayName="Refresh Cue List"))
    bool RefreshCueList();

    // Restores only the Show explicitly selected in this session; never loads a saved selection.
    UFUNCTION(BlueprintCallable, Category="Cue Player", meta=(DisplayName="Restore Selected Snapshot"))
    bool RestoreSelectedSnapshot(FString& Error);

    UFUNCTION(BlueprintCallable, Category="Cue Player", meta=(DisplayName="Cancel Cue Requests"))
    void CancelRequests();

    UFUNCTION(BlueprintPure, Category="Cue Player", meta=(DisplayName="Get Cue Snapshot"))
    bool GetSnapshot(FCueSnapshot& Snapshot) const;

    UFUNCTION(BlueprintPure, Category="Cue Player", meta=(DisplayName="Get Cue Shows"))
    TArray<FCueShowInfo> GetShows() const { return Shows; }

    UFUNCTION(BlueprintPure, Category="Cue Player", meta=(DisplayName="Get Cue Player Status"))
    FCuePlayerStatus GetStatus() const { return Status; }

    UFUNCTION(BlueprintPure, Category="Cue Player", meta=(DisplayName="Get Cue Server Base URL"))
    FString GetServerBaseUrl() const;

    UPROPERTY(BlueprintAssignable, Category="Cue Player") FOnCueShowsReceived OnShowsReceived;
    UPROPERTY(BlueprintAssignable, Category="Cue Player") FOnCueSnapshotChanged OnSnapshotChanged;
    UPROPERTY(BlueprintAssignable, Category="Cue Player") FOnCueRefreshCompleted OnRefreshCompleted;
    UPROPERTY(BlueprintAssignable, Category="Cue Player") FOnCueStatusChanged OnStatusChanged;

private:
    friend struct FCueSubsystemTestAccess;

    UPROPERTY(Transient) TArray<FCueShowInfo> Shows;
    UPROPERTY(Transient) FCueSnapshot ActiveSnapshot;
    UPROPERTY(Transient) FCuePlayerStatus Status;
    FString CacheDirectory;
    FHttpRequestPtr ShowsRequest;
    FHttpRequestPtr CueListRequest;
    TSharedPtr<FCueHttpResponseBody, ESPMode::ThreadSafe> ShowsBody;
    TSharedPtr<FCueHttpResponseBody, ESPMode::ThreadSafe> CueBody;
    uint64 ShowsGeneration = 0;
    uint64 CueGeneration = 0;
    bool bShuttingDown = false;

    void CancelShowRequest();
    void CancelCueRequest();
    void PublishStatus();
    void HandleShowsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected, uint64 Generation, FString BaseUrl);
    void HandleCueResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected, uint64 Generation, FString ShowId, FString BaseUrl);
};
