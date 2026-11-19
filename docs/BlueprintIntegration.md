# Cue C++ 기반 · Blueprint 연결 안내

2026년 11월 18일 KST · 실제 연출 wiring은 사용자 후속 작업

## 범위와 시작

Editor를 다시 열면 NetworkProject Runtime Module의 클래스가 로드된다. GameInstance/Pawn/GameMode 부모 교체는 필요 없다. Project Settings > Game > Cue Player에서 Server Base Url을 설정한다. 기본값 http://127.0.0.1:8001이며 /api/v1은 붙이지 않는다. Request Timeout Seconds 기본값은 10초다.

기존 CueServer와 현재 Show를 사용한다. demo.sqlite3으로 전환하지 않는다. 이 모듈은 기존 Blueprint/Widget/Map/Material/DataTable을 수정하거나 생성하지 않는다. GO/STOP/RESET 노드와 실행 커서는 제공하지 않는다.

## Subsystem 가져오기와 정확한 함수

Blueprint에서 Cue Player Subsystem을 검색해 GameInstance Subsystems 메뉴의 Get CuePlayerSubsystem 노드(그래프 본문 제목은 Cue Player Subsystem)를 사용한다. 반환값을 CuePlayerRef로 보관한다. 아래 노드는 CuePlayerRef에서 드래그해 검색한다. 함수 DisplayName은 C++ metadata로 고정했다.

| 검색할 노드 | C++ 함수 | 입력 핀 | 출력 핀 |
|---|---|---|---|
| Request Shows | RequestShows | Target: CuePlayerSubsystem Object Reference, 실행 | Return Value: Boolean, 실행 |
| Select Show | SelectShow | Target, Show Id: String, 실행 | Error: String, Return Value: Boolean, 실행 |
| Refresh Cue List | RefreshCueList | Target, 실행 | Return Value: Boolean, 실행 |
| Restore Selected Snapshot | RestoreSelectedSnapshot | Target, 실행 | Error: String, Return Value: Boolean, 실행 |
| Cancel Cue Requests | CancelRequests | Target, 실행 | 실행 |
| Get Cue Snapshot | GetSnapshot | Target | Snapshot: Cue Snapshot, Return Value: Boolean |
| Get Cue Shows | GetShows | Target | Return Value: Cue Show Info Array |
| Get Cue Player Status | GetStatus | Target | Return Value: Cue Player Status |
| Get Cue Server Base URL | GetServerBaseUrl | Target | Return Value: String |

조회/REFRESH의 Return Value는 요청을 시작했는지이며 다운로드 성공이 아니다. 결과는 delegate로 받는다. Select Show의 true는 선택 ID 저장 성공이다. 새 Show의 캐시가 없더라도 true일 수 있으므로 Get Cue Snapshot/Status로 데이터 여부를 확인한다. Restore Selected Snapshot은 현재 세션에서 선택한 Show의 디스크 캐시만 읽고 HTTP를 호출하지 않는다. Play 초기화는 미선택 상태이며 이전 Show ID를 자동 복원하지 않는다.

## BlueprintAssignable Delegate

| 프로퍼티 이름 | 바인딩 노드 검색 | 이벤트 매개변수 |
|---|---|---|
| OnShowsReceived | Bind Event to On Shows Received | bSuccess: Boolean, Shows: Cue Show Info Array, Error: String |
| OnSnapshotChanged | Bind Event to On Snapshot Changed | bHasSnapshot: Boolean, Snapshot: Cue Snapshot |
| OnRefreshCompleted | Bind Event to On Refresh Completed | bSuccess: Boolean, ShowId: String, Error: String |
| OnStatusChanged | Bind Event to On Status Changed | Status: Cue Player Status |

이벤트 핀은 Blueprint의 표시 옵션에 따라 bSuccess가 Success, bHasSnapshot이 Has Snapshot으로 표시될 수 있다. 위 표의 코드 식별자가 원본이다. 배열/구조체/String은 const 참조로 전달되며 Blueprint에서는 값 핀으로 읽는다.

성공 REFRESH는 파일 저장 후 OnSnapshotChanged, OnRefreshCompleted, OnStatusChanged를 보낸다. 실패 REFRESH는 기존 Snapshot을 바꾸거나 OnSnapshotChanged를 보내지 않는다. 실패 시 OnRefreshCompleted(false)와 Status만 갱신한다. Show 선택 변경은 캐시가 없을 때 OnSnapshotChanged(false, 기본 구조체)를 보낼 수 있다. 취소/무효화된 과거 요청은 완료 성공 이벤트를 보내지 않는다.

## 실제 Struct와 Enum

C++ 이름의 F/U/E 접두사는 Blueprint 표시에서 생략될 수 있다. Break Cue Data, Break Cue Snapshot, Break Cue Show Info, Break Cue Player Status로 멤버를 읽는다. 네 연출 상태는 **Name**이며 기존 Blueprint Enum의 정수 인덱스가 아니다.

| C++ 형식 | 멤버와 타입 |
|---|---|
| FCueShowInfo | Id: FString, Name: FString, Revision: int32, CreatedAt: FDateTime, UpdatedAt: FDateTime |
| FCueData | Id: FString, Position: int32, Number: int32, Name: FString, AnimationState: FName, CameraState: FName, LightingState: FName, AppearancePreset: FName, CameraBlendMs: int32, LightingFadeMs: int32 |
| FCueSnapshot | SchemaVersion: int32, ShowId: FString, ShowName: FString, Revision: int32, PublishedAt: FDateTime, Cues: TArray<FCueData> |
| FCuePlayerStatus | SelectedShowId: FString, ShowListRequestState: ECueRequestState, CueListRequestState: ECueRequestState, bHasSnapshot: bool, bCanUseLocalSnapshot: bool, LastSuccessfulRefreshUtc: FDateTime, ShowListError: FString, CueListError: FString, CacheError: FString |
| ECueRequestState | NotRequested, Requesting, Succeeded, Failed |

Blueprint 타입 대응은 FString=String, FName=Name, int32=Integer, bool=Boolean, FDateTime=Date Time이다. 시간은 UTC다. PublishedAt은 서버 발행 시각이며 LastSuccessfulRefreshUtc는 마지막 다운로드·로컬 저장 성공 시각이다.

기존 E_CueItemState의 Pending/Next/Running/Applied/Stopped는 새 C++ Enum으로 중복 정의하지 않았다. 기존 Enum을 UI 함수의 InitialState/NewState에 그대로 쓴다. Shader Mode 멤버는 없다.

## WBP_Operator 초기화 순서

미선택 시작 상태와 정확한 버튼/ComboBox 연결은 [CueStartupBlueprint.md](CueStartupBlueprint.md)를 따른다.

Widget 변수로 CuePlayerRef, ShowOptions(Cue Show Info Array), ShowLabels(String Array), bUpdatingShowOptions(Boolean), 생성된 Cue Item 참조 배열을 둔다. 기존 Widget 이름은 사용자의 실제 Widget 참조를 사용한다.

```text
Event On Initialized
 → Get CuePlayerSubsystem → Set CuePlayerRef
 → Bind Event to On Shows Received
 → Bind Event to On Snapshot Changed
 → Bind Event to On Refresh Completed
 → Bind Event to On Status Changed
 → InitializeCueStartupUI (미선택, 빈 목록, STANDBY/NONE, GO/RESET 비활성)
 → Request Shows
```

바인딩을 요청보다 먼저 한다. 실패가 즉시 반환되는 경우도 있다. Event Pre Construct에서는 HTTP를 호출하지 않는다. 재생성/재부착으로 바인딩이 중복되지 않게 관리하고, Destruct 때 이 Widget의 이벤트만 Unbind한다. Unbind All로 다른 수신자의 바인딩을 지우지 않는다.

## Show 목록과 선택 순서

```text
OnShowsReceived(Success, Shows, Error)
 → Branch(Success)
     False: 기존 ComboBox/로컬 목록 유지, Error 표시
     True:
       Set bUpdatingShowOptions = true
       Set ShowOptions = Shows
       ComboBox Clear Options, ShowLabels Clear
       For Each Loop(Shows)
         → Break Cue Show Info
         → Label = Name + " [" + Id + "]"
         → Add(Label) to ShowLabels → ComboBox Add Option(Label)
       SelectedShowId가 비었으면 Clear Selection + Select Show 표시
       비어 있지 않으면 같은 Id의 옵션만 Set Selected Option (첫 옵션 자동 선택 금지)
       Set bUpdatingShowOptions = false
```

Show Name은 중복될 수 있으므로 이름만 키로 쓰거나 이름으로 ID를 추측하지 않는다. 전체 UUID가 붙은 Label과 ShowOptions의 같은 인덱스를 사용한다. 저장된 Show가 서버 목록에서 사라졌더라도 기존 로컬 Snapshot은 유지한다. 필요하면 복원된 Show의 이름/ID 옵션을 별도로 표시한다. 임의 Show나 demo Show를 자동 선택하지 않는다.

```text
ComboBox On Selection Changed(Selected Item, Selection Type)
 → Branch(NOT bUpdatingShowOptions)
 → Find Item(ShowLabels, Selected Item) → Is Valid Index(ShowOptions)
 → Get ShowOptions[Index] → Break Cue Show Info.Id
 → Select Show(Show Id = Id)
 → Branch(Return Value)
     False: Error 표시, 실제 Get Cue Player Status.SelectedShowId에 맞게 선택 표시 복원
     True: OnSnapshotChanged에서 해당 Show 캐시 또는 빈 상태 표시
```

Show 선택은 다운로드하지 않는다. 서버의 새 목록 활성화는 사용자가 REFRESH를 눌렀을 때만 한다.

## REFRESH와 Cue List 생성 순서

```text
REFRESH Button On Clicked
 → CuePlayerRef.Refresh Cue List
 → 반환값으로 다운로드 성공 UI를 만들지 않음
 → OnRefreshCompleted에서 결과 메시지 표시
```

```text
OnSnapshotChanged(Has Snapshot, Snapshot)
 → ScrollBox Cue List: Clear Children
 → 생성된 Cue Item 참조 배열 Clear
 → Branch(Has Snapshot)
     False: NEXT 비우기, 목록 없음 표시
     True:
       Break Cue Snapshot → Show Name / Revision 표시
       For Each Loop(Cues)  [원래 배열 순서 유지]
         → Break Cue Data
         → Array Index == 0 ? 기존 E_CueItemState.Next : Pending
         → 기존 WBP_Operator.AddCueItem
              CueNember = Cue Data.Number
              CueName = Cue Data.Name
              InitialState = 위 상태
         → 출력 CueItem (WBP_CueItem Object Reference)을 참조 배열에 보관
       Cues Is Valid Index(0)
         True: Cues[0]의 Number/Name/AnimationState/CameraState/LightingState/AppearancePreset으로 NEXT 표시
         False: 비어 있는 Show 표시, NEXT 없음
```

실제 AddCueItem 입력 이름은 현재 그래프에서 **CueNember**로 확인됐다. 출력 핀 이름은 CueItem이며 타입은 WBP_CueItem Object Reference다. 기존 InitializeCueItem은 CueNumber 정수를 3자리 표시로 변환하므로 001 문자열을 별도로 만들 필요 없다. 서버가 현재 Position에 따라 1, 2, 3을 자동 부여하며 기존 3자리 표시는 그대로 사용한다. CueNumber가 아니라 **Position 순서 배열**을 순회한다. REFRESH 성공마다 Clear Children 후 다시 만들어 중복을 방지한다. 실패 REFRESH에서 목록을 지우지 않는다.

새로 받은 첫 항목의 Next 표시는 초기 Preview다. 아직 실행 커서나 실제 GO가 구현된 것이 아니다. 실행 완료 전 Applied로 바꾸거나 GO/STOP/RESET을 연결 완료로 표시하지 않는다.

## 상태 표시와 향후 연출 연결

OnStatusChanged → Break Cue Player Status로 서버 요청 상태, 오류, 로컬 사용 가능, 마지막 성공 시각을 각각 표시한다. 예: CueListRequestState=Failed이고 bCanUseLocalSnapshot=true이면 서버 조회 실패 / 로컬 Cue 데이터 사용 가능을 동시에 표시한다. bHasSnapshot=true여도 Cues가 비면 bCanUseLocalSnapshot=false다.

같은 종류의 새 요청은 이전 요청을 취소한다. Show 변경/Cancel/Deinitialize/주소 변경 후 과거 응답은 현재 스냅샷을 덮어쓰지 않는다. UI에서 Requesting 중 REFRESH를 비활성화해도 되지만 C++도 교체 요청을 방어한다. 현재 bCanUseLocalSnapshot은 데이터 존재만 뜻한다. 이후 GO 허용은 Running/Stopped/모드별 적용 가능 등의 실행 조건과 함께 판단해야 한다.

후속 단계는 기존 Blueprint의 상태 Enum에 네 FName을 명시적으로 매핑하고, 현재 Shader Mode를 유지하는 GO/RESET, Timeline 실제 완료·STOP·RunId·Stage View 해제를 연결한다. 기존 ResetAllState는 Cue RESET에 호출하지 않는다.

## 검증 자료와 주의

정확한 reflected 함수/핀은 Saved/CueImplementation/blueprint-surface.json으로 내보낸다. Native tests는 Saved/CueImplementation의 report에 기록한다. 테스트용 HTTP fixture는 Tests/cue_http_fixture.py이며 기존 CueServer DB를 열지 않는다. Editor에서 실제 버튼 wiring, PIE/Standalone 연출 결과, 키보드 포커스는 사용자의 연결 후 별도 검증한다.

## 빌드와 테스트 재현

Editor를 Save All 후 종료한 상태에서 PowerShell로 Tests/run_cue_tests.ps1을 실행한다. 기본 엔진 경로는 UE_5.8이며 다른 위치는 -EngineRoot로 지정한다. 이 스크립트는 Development Editor Win64 빌드, 임시 loopback fixture 서버 기동, 6개 native test, 결과 JSON 판정과 fixture 종료를 수행한다. UBT 로그 정리를 차단하는 제한 실행 환경에서는 승인된 제한 밖 실행을 사용한다. .NET/VS 설치나 시스템 ACL을 바꾸지 않는다.

```powershell
.\Tests\run_cue_tests.ps1
# 이미 빌드한 바이너리만 검증:
.\Tests\run_cue_tests.ps1 -SkipBuild
```

기본 fixture 포트는 18081로 기존 CueServer 8001과 다르다. -Port로 테스트 포트만 바꿀 수 있다. 테스트 캐시는 Saved/Automation 하위의 임시 UUID 디렉터리다. 기존 Saved/CuePlayer 스냅샷을 테스트 데이터로 덮어쓰지 않는다. 일부 Unreal TestExit는 실패해도 OS 종료 코드 0을 반환하므로 스크립트는 report의 실패/미실행 수까지 검사한다.

관련 문서 [PRD](PRD.md) · [SDD](SDD.md) · [구현 계획](ImplementationPlan.md)

## Camera Stage 및 전환 UI 범위 변경 (2026년 11월 19일 KST)

웹·서버 Camera 선택지는 Full / Upper / Close / Stage다. 기존 cameraState 필드에 Stage 문자열을 저장·반환한다. Stage는 Unreal의 별도 Stage View Override이며 E_CameraState에 Stage를 추가하거나 Camera DataTable을 변경하지 않는다. Full/Upper/Close의 의미와 기존 자동 번호·정렬·GO 구조는 유지한다.

Cue는 Animation·Camera·Lighting·Appearance 목표 상태에 집중한다. Cue별 Camera Blend, Lighting Fade, Blend/Fade Duration, Transition Time의 입력 UI와 지정 시간 실행은 이번 개발 범위 및 완료 기준에서 제외한다. 이전 문서의 P1 시간 제어 계획은 이번 범위에 포함되지 않는다. 정상 동작하는 Studio 카메라·조명·Timeline을 삭제하거나 리팩터링하지 않는다.

DB와 schema v2 응답의 cameraBlendMs/lightingFadeMs는 호환성을 위해 계속 저장·반환한다. 생성에서 생략 시 0, 수정에서 생략 시 기존 값 보존, 복제 시 원본 시간 값 보존으로 처리한다. 웹에서는 두 값을 입력하거나 수정하지 않는다. 기존 Cue ID·연출·순서·자동 Number와 Revision 이력을 임의로 바꾸지 않는다.

### Unreal 후속 연결 — 이번 웹·서버 작업에는 미포함

현재 Source/NetworkProject/Private/CueJson.cpp의 Camera 허용 목록은 Full/Upper/Close다. Stage를 추가하기 전에는 Stage Cue가 포함된 REFRESH가 스키마 검증에서 거절된다. 다음 Unreal 작업에서 C++ 허용 목록에 문자열 Stage를 추가해야 한다. 이는 데이터 파싱 변경이며 E_CameraState Enum 추가가 아니다.

그 후 Blueprint는 CameraState(Name) == Stage를 먼저 분기해 별도 Stage View를 활성화한다. Full/Upper/Close는 Stage View를 해제하고 기존 Enum·함수로 처리한다. 기존 Cue RESET, GO, Shader Mode 처리와 시간 보간은 이번 변경에서 연결하거나 완료로 판정하지 않는다. 이번에는 Unreal Source와 .uasset을 수정하지 않았다.
