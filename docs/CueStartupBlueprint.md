# Cue 시작 상태 Blueprint 연결

## C++ 경계

Play마다 새 CuePlayerSubsystem은 미선택 상태로 시작한다. Initialize는 이전 selection.json이나 Snapshot을 자동 복원하지 않는다. RequestShows는 옵션만 조회하고 선택하지 않는다.

RestoreSelectedSnapshot은 현재 세션에서 선택한 Show의 캐시만 복원한다. 미선택이면 false와 진단을 반환하며 저장된 Show ID를 읽지 않는다. SelectShow는 기존처럼 선택한 Show의 캐시를 즉시 복원하고, 캐시가 없으면 빈 Snapshot을 알린다. 서버 조회는 기존 RefreshCueList 경로를 사용한다.

캐시 파일과 selection.json, 서버/DB 데이터는 삭제하지 않는다. Stage Camera 허용 목록과 기존 Show 전환/실행 경로는 유지한다.

## WBP_Operator: Event On Initialized

아래 InitializeCueStartupUI는 새로 만들 Blueprint 함수 이름이다. 표시용 위젯 참조는 프로젝트의 실제 변수에 연결한다.

```text
Event On Initialized
 → Get CuePlayerSubsystem → Set CuePlayerRef
 → 기존 OnShowsReceived / OnSnapshotChanged / OnRefreshCompleted / OnStatusChanged 바인딩
 → InitializeCueStartupUI
 → Request Shows
```

초기 실행 체인에서 Restore Selected Snapshot 호출, 이전 ID로 Select Show 호출, ShowOptions[0] 선택, 첫 옵션 Set Selected Index(0)/Set Selected Option 연결을 제거한다. Get Cue Snapshot 결과로 이전 목록을 재생성하는 초기 복원 연결도 제거한다. 기존 Show 선택 이후의 RebuildCueList는 유지한다.

InitializeCueStartupUI 연결:

```text
Set bUpdatingShowOptions = true
 → Show ComboBox: Clear Selection
 → Show 표시 Text: Set Text("Select Show")
 → Cue ScrollBox: Clear Children
 → 생성된 Cue Item 참조 배열: Clear
 → CURRENT Text: Set Text("STANDBY")
 → NEXT Text: Set Text("NONE")
 → GO Text: Set Text("GO ---")
 → GO Button: Set Is Enabled(false)
 → Cue RESET Button: Set Is Enabled(false)
 → REFRESH Button: Set Is Enabled(true)
 → Set bUpdatingShowOptions = false
```

ComboBoxString의 미선택 표시를 직접 제어할 수 없다면 On Generate Widget과 별도로 선택 표시용 Text를 두고, 미선택일 때 Select Show를 표시한다. 가짜 Show나 UUID로 Select Show를 호출하지 않는다. 기존 런타임 초기화가 표시를 덮어쓴다면 그 초기화 뒤에서 위 함수를 한 번 호출한다. 상태 갱신 함수도 SelectedShowId가 빈 경우 같은 미선택 표시와 버튼 상태를 적용하고 Return하도록 한다. 진행 중 Show의 GO/END/RESET 상태 계산은 기존 분기를 유지한다.

## OnShowsReceived

```text
Success → bUpdatingShowOptions = true
 → 기존 ShowOptions / ShowLabels / ComboBox 옵션 채우기
 → Get Cue Player Status.SelectedShowId → Is Empty
     true: ComboBox Clear Selection + 표시 Text "Select Show"
     false: 현재 ID와 일치하는 옵션만 Set Selected Option
 → bUpdatingShowOptions = false
```

ForEachLoop의 Completed 핀에서 첫 옵션을 선택하거나 Select Show/Refresh Cue List를 호출하지 않는다. 기존 선택이 있는 상태에서 Show 목록을 재조회할 때는 그 선택을 유지한다.

## ComboBox On Selection Changed

```text
Branch(!bUpdatingShowOptions AND Selection Type != Direct)
 → Find Item(ShowLabels, Selected Item)
 → Is Valid Index(ShowOptions, Index)
 → ShowOptions[Index].Id
 → 기존 Select Show 및 선택 후 조회 흐름
```

Direct는 옵션 재구성/프로그램 선택 이벤트를 차단한다. 마우스·키보드로 운영자가 고른 유효한 옵션만 기존 선택 처리로 전달한다. SelectShow의 true는 다운로드 완료가 아니다. 캐시는 OnSnapshotChanged에서 표시하고, 새 서버 응답은 기존 RefreshCueList/OnRefreshCompleted로 처리한다. 기존에 직접 선택 직후 RefreshCueList를 연결했다면 그대로 유지한다.

## REFRESH On Clicked

```text
Get Cue Player Status.SelectedShowId → Is Empty
  true: Request Shows
  false: 기존 Refresh Cue List 경로
```

미선택 REFRESH는 옵션만 갱신하며 첫 Show를 선택하지 않는다. 초기 REFRESH 버튼은 활성화하되 기존 요청 중 중복 클릭 방지와 실행 중 REFRESH 잠금은 유지한다.

## 확인 순서

이전 Show와 캐시가 있는 상태에서 Stop → Play: Select Show / 빈 목록 / STANDBY / NONE / GO --- 비활성 / REFRESH 활성 / Cue RESET 비활성을 확인한다. 미선택 REFRESH 후에도 같은 상태여야 한다. 운영자가 Show를 선택하면 기존 캐시/서버 목록이 표시되어야 한다. Show 전환과 Stage GO → 일반 Camera GO, END SHOW, RESET은 기존 흐름으로 확인한다. 이 안내는 .uasset을 수정하지 않으며 실제 UI 연결은 운영자가 적용해야 한다.
