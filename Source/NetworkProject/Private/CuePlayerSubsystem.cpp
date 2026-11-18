#include "CuePlayerSubsystem.h"

#include "CueCache.h"
#include "CueJson.h"
#include "CueHttpResponseBody.h"
#include "CuePlayerSettings.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/Paths.h"

namespace
{
    bool ValidBaseUrl(const FString& Url)
    {
        const int32 SchemeLength = Url.StartsWith(TEXT("https://")) ? 8 : (Url.StartsWith(TEXT("http://")) ? 7 : 0);
        if (SchemeLength == 0 || Url.Len() <= SchemeLength || Url[SchemeLength] == TEXT('/') ||
            Url.Contains(TEXT("?")) || Url.Contains(TEXT("#")) || Url.Contains(TEXT("\\"))) return false;
        for (TCHAR Character : Url) if (FChar::IsWhitespace(Character)) return false;
        return true;
    }

    bool ConfigureRequest(const FHttpRequestPtr& Request, const FString& Url, const TSharedPtr<FCueHttpResponseBody, ESPMode::ThreadSafe>& Body)
    {
        Request->SetURL(Url);
        Request->SetVerb(TEXT("GET"));
        Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
        const float Timeout = GetDefault<UCuePlayerSettings>()->RequestTimeoutSeconds;
        Request->SetTimeout(FMath::IsFinite(Timeout) ? FMath::Clamp(Timeout, 0.1f, 120.0f) : 10.0f);
        Request->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);
        return Request->SetResponseBodyReceiveStreamDelegateV2(FHttpRequestStreamDelegateV2::CreateLambda(
            [Body](void* Data, int64& Length) { Body->Receive(Data, Length); }));
    }

    bool ReadResponse(const FHttpResponsePtr& Response, bool bConnected, const TSharedPtr<FCueHttpResponseBody, ESPMode::ThreadSafe>& Received, FString& Body, FString& Error)
    {
        if (!Received.IsValid() || !Received->Read(Body, Error)) return false;
        if (!bConnected || !Response.IsValid()) { Error = TEXT("Server connection failed or request timed out."); return false; }
        if (Response->GetResponseCode() != 200) { Error = FString::Printf(TEXT("Server returned HTTP %d."), Response->GetResponseCode()); return false; }
        return true;
    }
}

void UCuePlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CacheDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("CuePlayer"));
    FString Error;
    RestoreSelectedSnapshot(Error); // Local only; there is no automatic HTTP or scene command.
}

void UCuePlayerSubsystem::Deinitialize()
{
    bShuttingDown = true;
    CancelRequests();
    OnShowsReceived.Clear();
    OnSnapshotChanged.Clear();
    OnRefreshCompleted.Clear();
    OnStatusChanged.Clear();
    Super::Deinitialize();
}

FString UCuePlayerSubsystem::GetServerBaseUrl() const
{
    FString Url = GetDefault<UCuePlayerSettings>()->ServerBaseUrl.TrimStartAndEnd();
    while (Url.EndsWith(TEXT("/"))) Url.LeftChopInline(1);
    return Url;
}

void UCuePlayerSubsystem::PublishStatus()
{
    if (bShuttingDown) return;
    // Copy before broadcasting: a Blueprint listener can change selection synchronously.
    const FCuePlayerStatus Published = Status;
    OnStatusChanged.Broadcast(Published);
}

void UCuePlayerSubsystem::CancelShowRequest()
{
    ++ShowsGeneration;
    if (ShowsRequest.IsValid())
    {
        ShowsRequest->OnProcessRequestComplete().Unbind();
        ShowsRequest->CancelRequest();
        ShowsRequest.Reset();
    }
    ShowsBody.Reset();
    if (Status.ShowListRequestState == ECueRequestState::Requesting) Status.ShowListRequestState = ECueRequestState::NotRequested;
}

void UCuePlayerSubsystem::CancelCueRequest()
{
    ++CueGeneration;
    if (CueListRequest.IsValid())
    {
        CueListRequest->OnProcessRequestComplete().Unbind();
        CueListRequest->CancelRequest();
        CueListRequest.Reset();
    }
    CueBody.Reset();
    if (Status.CueListRequestState == ECueRequestState::Requesting) Status.CueListRequestState = ECueRequestState::NotRequested;
}

void UCuePlayerSubsystem::CancelRequests()
{
    CancelShowRequest();
    CancelCueRequest();
    PublishStatus();
}

bool UCuePlayerSubsystem::RequestShows()
{
    if (bShuttingDown) return false;
    CancelShowRequest();
    const FString BaseUrl = GetServerBaseUrl();
    Status.ShowListError.Reset();
    if (!ValidBaseUrl(BaseUrl))
    {
        Status.ShowListRequestState = ECueRequestState::Failed;
        Status.ShowListError = TEXT("Set a valid http(s) Server Base URL in Project Settings > Game > Cue Player.");
        const auto PublishedShows = Shows;
        const FString Error = Status.ShowListError;
        OnShowsReceived.Broadcast(false, PublishedShows, Error);
        PublishStatus();
        return false;
    }
    const FHttpRequestPtr Request = FHttpModule::Get().CreateRequest();
    ShowsRequest = Request;
    const uint64 Generation = ShowsGeneration;
    ShowsBody = MakeShared<FCueHttpResponseBody, ESPMode::ThreadSafe>();
    const bool bConfigured = ConfigureRequest(Request, BaseUrl + TEXT("/api/v1/shows"), ShowsBody);
    Request->OnProcessRequestComplete().BindUObject(this, &UCuePlayerSubsystem::HandleShowsResponse, Generation, BaseUrl);
    Status.ShowListRequestState = ECueRequestState::Requesting;
    const bool bStarted = bConfigured && Request->ProcessRequest();
    if (!bStarted) HandleShowsResponse(Request, nullptr, false, Generation, BaseUrl);
    else PublishStatus();
    return bStarted;
}

void UCuePlayerSubsystem::HandleShowsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected, uint64 Generation, FString BaseUrl)
{
    if (bShuttingDown || Generation != ShowsGeneration || Request != ShowsRequest) return;
    ShowsRequest.Reset();
    const auto Received = MoveTemp(ShowsBody);
    if (BaseUrl != GetServerBaseUrl())
    {
        Status.ShowListRequestState = ECueRequestState::NotRequested;
        PublishStatus();
        return;
    }
    FString Body, Error;
    TArray<FCueShowInfo> Parsed;
    const bool bSuccess = ReadResponse(Response, bConnected, Received, Body, Error) && CueJson::ParseShows(Body, Parsed, Error);
    if (bSuccess) Shows = MoveTemp(Parsed);
    Status.ShowListRequestState = bSuccess ? ECueRequestState::Succeeded : ECueRequestState::Failed;
    Status.ShowListError = Error;
    const auto PublishedShows = Shows;
    OnShowsReceived.Broadcast(bSuccess, PublishedShows, Error);
    PublishStatus();
}

bool UCuePlayerSubsystem::SelectShow(const FString& ShowId, FString& Error)
{
    Error.Reset();
    if (bShuttingDown) { Error = TEXT("Cue subsystem is shutting down."); return false; }
    FString Normalized;
    if (!CueJson::NormalizeShowId(ShowId, Normalized)) { Error = TEXT("Show ID must be a hyphenated UUID."); return false; }
    if (Status.SelectedShowId == Normalized) return true;
    FCueCache Cache(CacheDirectory);
    if (!Cache.SaveSelection(Normalized, Error)) { Status.CacheError = Error; PublishStatus(); return false; }
    CancelCueRequest();
    Status.SelectedShowId = Normalized;
    Status.CueListRequestState = ECueRequestState::NotRequested;
    Status.CueListError.Reset();
    Status.LastSuccessfulRefreshUtc = FDateTime();
    Status.bHasSnapshot = false;
    Status.bCanUseLocalSnapshot = false;
    ActiveSnapshot = FCueSnapshot();
    FCueSnapshot Cached;
    FDateTime RefreshedAt;
    Status.CacheError.Reset();
    if (Cache.LoadSnapshot(Normalized, Cached, RefreshedAt, Status.CacheError))
    {
        ActiveSnapshot = MoveTemp(Cached);
        Status.LastSuccessfulRefreshUtc = RefreshedAt;
        Status.bHasSnapshot = true;
        Status.bCanUseLocalSnapshot = !ActiveSnapshot.Cues.IsEmpty();
    }
    const FCueSnapshot Published = ActiveSnapshot;
    OnSnapshotChanged.Broadcast(Status.bHasSnapshot, Published);
    PublishStatus();
    return true;
}

bool UCuePlayerSubsystem::RestoreSelectedSnapshot(FString& Error)
{
    Error.Reset();
    if (bShuttingDown) { Error = TEXT("Cue subsystem is shutting down."); return false; }
    CancelCueRequest();
    FCueCache Cache(CacheDirectory);
    if (Status.SelectedShowId.IsEmpty() && !Cache.LoadSelection(Status.SelectedShowId, Error))
    {
        Status.CacheError = Error;
        PublishStatus();
        return false;
    }
    FCueSnapshot Cached;
    FDateTime RefreshedAt;
    if (!Cache.LoadSnapshot(Status.SelectedShowId, Cached, RefreshedAt, Error))
    {
        Status.CacheError = Error;
        PublishStatus();
        return false;
    }
    ActiveSnapshot = MoveTemp(Cached);
    Status.bHasSnapshot = true;
    Status.bCanUseLocalSnapshot = !ActiveSnapshot.Cues.IsEmpty();
    Status.LastSuccessfulRefreshUtc = RefreshedAt;
    Status.CacheError.Reset();
    const FCueSnapshot Published = ActiveSnapshot;
    OnSnapshotChanged.Broadcast(true, Published);
    PublishStatus();
    return true;
}

bool UCuePlayerSubsystem::RefreshCueList()
{
    if (bShuttingDown) return false;
    CancelCueRequest();
    const FString BaseUrl = GetServerBaseUrl();
    const FString ShowId = Status.SelectedShowId;
    Status.CueListError.Reset();
    if (ShowId.IsEmpty() || !ValidBaseUrl(BaseUrl))
    {
        Status.CueListRequestState = ECueRequestState::Failed;
        Status.CueListError = TEXT("Select a valid Show and configure an http(s) Server Base URL first.");
        const FString Error = Status.CueListError;
        OnRefreshCompleted.Broadcast(false, ShowId, Error);
        PublishStatus();
        return false;
    }
    const FHttpRequestPtr Request = FHttpModule::Get().CreateRequest();
    CueListRequest = Request;
    const uint64 Generation = CueGeneration;
    CueBody = MakeShared<FCueHttpResponseBody, ESPMode::ThreadSafe>();
    const bool bConfigured = ConfigureRequest(Request, BaseUrl + TEXT("/api/v1/shows/") + ShowId + TEXT("/cue-list"), CueBody);
    Request->OnProcessRequestComplete().BindUObject(this, &UCuePlayerSubsystem::HandleCueResponse, Generation, ShowId, BaseUrl);
    Status.CueListRequestState = ECueRequestState::Requesting;
    const bool bStarted = bConfigured && Request->ProcessRequest();
    if (!bStarted) HandleCueResponse(Request, nullptr, false, Generation, ShowId, BaseUrl);
    else PublishStatus();
    return bStarted;
}

void UCuePlayerSubsystem::HandleCueResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected,
    uint64 Generation, FString ShowId, FString BaseUrl)
{
    if (bShuttingDown || Generation != CueGeneration || Request != CueListRequest || ShowId != Status.SelectedShowId) return;
    CueListRequest.Reset();
    const auto Received = MoveTemp(CueBody);
    if (BaseUrl != GetServerBaseUrl())
    {
        Status.CueListRequestState = ECueRequestState::NotRequested;
        PublishStatus();
        return;
    }
    FString Body, Error;
    FCueSnapshot Parsed;
    bool bSuccess = ReadResponse(Response, bConnected, Received, Body, Error) && CueJson::ParseSnapshot(Body, ShowId, Parsed, Error);
    const FDateTime RefreshedAt = FDateTime::UtcNow();
    if (bSuccess)
    {
        bSuccess = FCueCache(CacheDirectory).SaveSnapshot(Body, ShowId, RefreshedAt, Error);
        if (!bSuccess) Status.CacheError = Error;
    }
    Status.CueListRequestState = bSuccess ? ECueRequestState::Succeeded : ECueRequestState::Failed;
    Status.CueListError = Error;
    if (bSuccess)
    {
        ActiveSnapshot = MoveTemp(Parsed);
        Status.bHasSnapshot = true;
        Status.bCanUseLocalSnapshot = !ActiveSnapshot.Cues.IsEmpty();
        Status.LastSuccessfulRefreshUtc = RefreshedAt;
        Status.CacheError.Reset();
        const FCueSnapshot Published = ActiveSnapshot;
        OnSnapshotChanged.Broadcast(true, Published);
    }
    if (Generation == CueGeneration && ShowId == Status.SelectedShowId && !bShuttingDown)
        OnRefreshCompleted.Broadcast(bSuccess, ShowId, Error);
    PublishStatus();
}

bool UCuePlayerSubsystem::GetSnapshot(FCueSnapshot& Snapshot) const
{
    Snapshot = Status.bHasSnapshot ? ActiveSnapshot : FCueSnapshot();
    return Status.bHasSnapshot;
}
