# Virtual Performer Studio · Cue 구현 계획

2026년 11월 18일 KST · 최종 사용자 요구사항 우선

## 목표와 보호 경계

이번 단계는 C++ Runtime Module, UCuePlayerSubsystem, HTTP 비동기 조회, v2 검증, 영구 캐시, 오래된 응답 차단, Blueprint API, 테스트와 Development Editor Win64 빌드다. [PRD](PRD.md)와 [SDD](SDD.md)가 전체 요구와 이번 경계를 정의한다.

기존 Cue Control UI와 Shader/Appearance를 보존한다. 기존 Blueprint/Widget/Map/Material/DataTable 수정, 신규 Bridge/Widget/복제 맵, CueServer/API/DB 변경, demo.sqlite3 전환은 하지 않는다. UI wiring과 GO/STOP/RESET 실제 연출·실행 커서·RunId·MANUAL OVERRIDE는 후속 작업이다.

## 파일 구성

| 대상 | 책임 |
|---|---|
| NetworkProject.uproject, Source/*.Target.cs, Source/NetworkProject/NetworkProject.Build.cs | Runtime 모듈과 Editor/Game Target |
| Public/CueTypes.h, Public/CuePlayerSettings.h | 데이터·통신 상태·프로젝트 설정 |
| Public/CuePlayerSubsystem.h, Private/CuePlayerSubsystem.cpp | 요청·선택·수명·활성화·Blueprint API |
| Private/CueJson.h/.cpp | JSON 전체 검증 |
| Private/CueCache.h/.cpp | 임시 기록·flush·교체·복원 |
| Private/Tests/ | JSON·캐시·HTTP·지연 응답 자동화 테스트 |
| docs/*.md, 대응 HTML | 요구·설계·진행 상태·정확한 Blueprint 연결 안내 |

Source 하위 경로는 NetworkProject 모듈 기준이다. 기존 Blueprint Enum과 C++ 데이터 Enum을 중복 구현하지 않는다.

## 작업 순서와 완료 기준

| 단계 | 작업 | 검증 |
|---|---|---|
| 0 | 최신 커밋·UI·스키마, DB 해시, 기존 테스트 확인 | b58d86e 기준. 기존 서버 테스트 24 passed, upstream deprecation warning 1건 |
| 1 | 모듈·DTO·JSON 계약 테스트 작성 | 사용자 Editor 종료 후 콜드 빌드, 미구현 파서에 의한 RED 확인 |
| 2 | JSON 파서 구현 | 공용 v2 예제/빈 Show/독립 Number 허용. 누락·null·bool·소수·범위·Enum·중복·position·Show ID 거절 |
| 3 | 캐시 테스트와 구현 | 저장/복원, 불량 데이터·쓰기 실패 후 정상본 유지, 선택 Show 복원 |
| 4 | Subsystem·HTTP | 조회·REFRESH, 비-200/불량 JSON/타임아웃, 선택 변경·취소·새 요청 뒤 오래된 콜백 무시 |
| 5 | 빌드·회귀 검증 | Development Editor Win64 ExitCode 0, native tests, 기존 서버 tests, 기존 자산/DB 보존 |
| 6 | 연결 안내와 결과 보고 | 정확한 노드·핀·delegate·형식·WBP_Operator 연결 순서 |

## 검증 중점

- bool/소수/문자열을 정수로 암묵 변환하지 않는다. 전체 검증 전 출력이나 활성 목록을 바꾸지 않는다.
- 선택 Show와 응답 Show가 일치해야 한다. 새 Show에 캐시가 없으면 이전 Show를 사용 가능 데이터로 노출하지 않는다.
- 통신·파싱·저장 실패 시 파일·메모리·성공 시각을 보존한다. 서버 상태와 로컬 데이터 상태는 독립이다.
- 복원은 자동 HTTP/자동 실행이 아니다. 빈 Show는 유효하나 실행할 Cue가 없다.
- HTTP 테스트는 별도 loopback 포트와 임시 캐시를 쓴다. 기존 CueServer나 Show를 대체하지 않는다.

## 다음 단계

사용자가 기존 WBP_Operator에 조회·선택·REFRESH·목록 생성 wiring을 연결한다. 이후 기존 Blueprint의 GO/STOP/RESET을 연결하고 실행 잠금·RunId·완료·취소·모드 유지 RESET·MANUAL OVERRIDE를 검증한다. 기존 ResetAllState는 Cue RESET에 사용하지 않는다.

전체 P0는 기존 Show의 position 순서대로 서버 중단 중 Cue 실행, STOP/RESET, Shader Mode 유지까지 실제 Studio PIE/Standalone에서 확인한 뒤 완료로 판정한다. 이번 기반 모듈 빌드 성공은 전체 P0 시연 성공과 별개다. 세부 시간 보간, 충돌/고급 복구, 패키징·송출·원격 영상은 후순위다.

## 진행 기록

구현 브랜치 codex/cue-player-foundation. 초기 기존 CueServer 테스트 24 passed. Development Editor Win64 빌드 성공, native 6개 통과, 기존 CueServer 24개 통과를 확인했다. Native HTTP 테스트의 타임아웃/수신 제한 경고는 의도한 실패 경로이며 서버 테스트에는 기존 Starlette deprecation warning 1건이 있다.

## 구현 결과와 남은 작업

- 완료: Runtime Module, 자동 생성 GameInstanceSubsystem, 비동기 Show/Cue GET, 엄격한 schema v2·Unicode·RFC3339 검증, 수신 중 4 MiB 제한, 저장 후 활성화, 선택/스냅샷 복원, 실패 시 정상본 유지, 오래된 응답 차단, Blueprint API와 설정.
- 검증: BlueprintSurface, CacheContract, HttpContract, JsonContract, ResponseLimit, UnicodeTimestamp 모두 통과. 기존 CueServer pytest 24 passed. Development Editor Win64 build ExitCode 0. HTML 구조/링크와 git diff --check 통과.
- 보존: 두 SQLite DB와 사용자 Save All로 변경된 WBP_Operator/WBP_CueItem의 SHA256 일치. 다른 기존 에셋·CueServer 소스/API/DB 구조 수정 없음.
- 미구현: GO/STOP/RESET 실제 연출, 실행 커서/RunId/실행 잠금, MANUAL OVERRIDE, 실제 Widget wiring, Studio PIE/Standalone 전체 시연, 지정 시간 보간, 패키징·송출·원격 영상.
- Editor: 사용자가 종료한 상태에서 빌드했고 interactive Editor를 다시 시작하지 않았다. NetworkProject.uproject를 다시 열어 새 노드를 연결한다. 기존 Blueprint 부모는 유지한다.
- Git: codex/cue-player-foundation 브랜치, 변경은 검토 가능한 미커밋 상태다. 사용자 에셋 변경을 되돌리거나 커밋하지 않았다.

## dotnet 오류 진단

첫 제한 실행 2회는 엔진 번들 dotnet.exe가 UBT 로그 백업 정리에서 System.UnauthorizedAccessException으로 종료했다. 경로는 AppData/Local/UnrealBuildTool/Trace-backup-2026.11.13-02.35.14.uba, 스택은 FileSystem.DeleteFile → FileReference.Delete → Log.BackupLogFile → UnrealBuildTool.Main이다. 종료 코드 -532462766은 0xe0434352와 같다. Application Error/.NET Runtime의 해당 시점 이벤트는 조회되지 않았고, 실제 예외는 직접 실행 콘솔에서 재현했다.

동일 UBT의 제한 밖 도움말 및 전체 빌드는 성공했다. 설치 손상이나 새 C++의 실행 예외가 아니라 도구 실행 제한에 의한 로그 접근 실패다. .NET/Visual Studio 재설치, ACL 수정, 기존 로그 수동 삭제는 하지 않았다. 빌드는 승인된 제한 밖 실행을 사용한다. 상세 증거는 Saved/CueImplementation/dotnet-diagnosis.md와 ubt-sandbox-diagnostic-console.log에 남겼다.

## 생성·수정한 파일

수정한 기존 파일은 NetworkProject.uproject와 PRD/SDD/ImplementationPlan의 Markdown 및 대응 HTML이다. 아래는 신규 파일 전체다. 사용자 Save All로 수정된 두 Widget은 이 변경 목록에 포함하지 않는다.

```text
Source/NetworkProject.Target.cs
Source/NetworkProject/NetworkProject.Build.cs
Source/NetworkProject/Private/CueCache.cpp
Source/NetworkProject/Private/CueCache.h
Source/NetworkProject/Private/CueHttpResponseBody.h
Source/NetworkProject/Private/CueJson.cpp
Source/NetworkProject/Private/CueJson.h
Source/NetworkProject/Private/CuePlayerSubsystem.cpp
Source/NetworkProject/Private/NetworkProject.cpp
Source/NetworkProject/Private/Tests/CueBlueprintSurfaceTests.cpp
Source/NetworkProject/Private/Tests/CueCacheTests.cpp
Source/NetworkProject/Private/Tests/CueDataTests.cpp
Source/NetworkProject/Private/Tests/CueHttpTests.cpp
Source/NetworkProject/Private/Tests/CueResponseLimitTests.cpp
Source/NetworkProject/Public/CuePlayerSettings.h
Source/NetworkProject/Public/CuePlayerSubsystem.h
Source/NetworkProject/Public/CueTypes.h
Source/NetworkProjectEditor.Target.cs
Tests/cue_http_fixture.py
Tests/run_cue_tests.ps1
docs/BlueprintIntegration.html
docs/BlueprintIntegration.md
docs/render_docs.py
```

## 검증 자료와 재현

Tests/run_cue_tests.ps1은 빌드, 임시 fixture, native suite, JSON 결과 판정과 fixture 종료를 수행한다. 기본 fixture는 127.0.0.1:18081이며 실제 CueServer 8001이나 DB를 사용하지 않는다. -SkipBuild로 바이너리 검증만 할 수 있다.

최종 빌드 로그는 Saved/CueImplementation/build-final.log, native 결과는 Saved/CueImplementation/final-report/index.json, Blueprint reflection은 Saved/CueImplementation/blueprint-surface.json이다. 전체 연결 계약과 실제 핀 타입은 BlueprintIntegration.md를 기준으로 한다.

관련 문서 [PRD](PRD.md) · [SDD](SDD.md) · [Blueprint 연결 안내](BlueprintIntegration.md)
