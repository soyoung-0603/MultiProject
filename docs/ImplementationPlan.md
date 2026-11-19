# Virtual Performer Studio · Cue 구현 계획

2026년 11월 18일 KST · C++ 기반 단계 기록

2026년 11월 19일 자동 번호 정책은 마지막 추가 변경 절을 우선 적용한다. 기존 C++ 단계의 서버 변경 금지 범위는 이 추가 작업에 한해 번호 정책 변경을 허용한다.

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

## 추가 변경: Cue 자동 번호 (2026년 11월 19일 KST)

Cue 번호의 최종 정책은 1부터 연속 증가다. 서버가 현재 Position 순서대로 number = position을 저장하고 응답한다. 웹 화면은 최소 3자리(001, 002, 003)로 표시하며 1000 이상도 자르지 않는다. 010/020/030처럼 10씩 증가하는 정책은 사용하지 않는다.

웹 생성·수정·복제 초안에는 번호 입력이 없다. 추가·삭제·드래그/버튼 정렬 후 모든 번호를 다시 지정하고 서버 재조회 결과를 표시한다. 식별과 선택·정렬 요청은 UUID를 사용한다. ID·이름·Animation/Camera/Lighting/Appearance·Blend/Fade는 번호 변경으로 바뀌지 않는다.

POST/PATCH 입력에서 number를 제거하고 직접 입력은 422로 거절한다. GET 응답의 정수 number, schemaVersion 2, 기존 DB 테이블·UUID 구조는 유지한다. 두 UNIQUE 열은 트랜잭션 안에서 임시 음수로 옮긴 뒤 연속 Position/Number를 확정한다. 전체 실패는 롤백한다.

기존 데이터는 시작 시 저장된 순서대로 전환하며 영향받은 Show만 새 Revision을 발행한다. 과거 v2 Revision은 그대로 보존하고 다음 재시작에는 다시 발행하지 않는다. 구형 v1 스키마 변환과도 같은 트랜잭션으로 처리한다. 운영 DB는 전환 전에 별도로 백업한다.

Unreal의 CueData.Number는 계속 Integer다. 기존 Blueprint의 최소 3자리 표시는 유지하고 새 REFRESH 후 GO 001/002/003에 사용할 수 있다. 이전 로컬 스냅샷의 번호와 과거 Number/Position 독립 파싱 호환성은 유지한다. 기존 Blueprint/Map/Material/DataTable/C++는 이번 변경에서 수정하지 않는다.

### 자동 번호 변경 검증 결과

- 서버 pytest 31 passed. 기존 deprecation warning 1건. 생성·수정 제한·삭제·정렬·복제·동시 생성·재시작·1000 정수 응답·기존 데이터 전환·v1 동시 변환·실패 롤백을 검증했다.
- 별도 테스트 DB의 실제 브라우저에서 PERFORMANCE를 맨 앞으로 이동하면 001 PERFORMANCE / 002 OPENING / 003 MAIN TALK으로 갱신됐다. 추가·복제 Cue는 004/005, 새로고침 뒤에도 같은 결과다.
- 운영 cues.sqlite3 백업 후 같은 DB로 8001 서버를 재시작했다. Virtual Performer Showcase는 Revision 18→19, Test2는 10→11, 빈 Test는 Revision 1 유지다. 기존 Cue 7개 ID·정렬·연출과 과거 Revision 29개를 보존했다. demo.sqlite3은 변경하지 않았다.
- 백업·실제 전환 검증 결과는 Saved/CueNumbering/backup-path.txt와 migration-result.json에 기록했다. 사용자의 기존 에셋 수정은 보존한다. 작업 중 WBP_Operator에 추가 저장이 감지됐으며 이 에셋에는 쓰기 작업을 하지 않았다.
- 자동 번호 변경 파일: CueServer/app/cues/schemas.py, service.py, migrations.py, app/admin/index.html, app/admin/static/admin.js, tests/test_api.py, tests/test_numbering.py, README.md 및 PRD/SDD/ImplementationPlan/BlueprintIntegration의 Markdown·HTML. docs/render_docs.py는 새 하위 제목의 중복 anchor를 방지하도록 한 줄 수정했다. Unreal Source와 Asset에는 쓰기 작업을 하지 않았다.

## Camera Stage 및 전환 UI 범위 변경 (2026년 11월 19일 KST)

웹·서버 Camera 선택지는 Full / Upper / Close / Stage다. 기존 cameraState 필드에 Stage 문자열을 저장·반환한다. Stage는 Unreal의 별도 Stage View Override이며 E_CameraState에 Stage를 추가하거나 Camera DataTable을 변경하지 않는다. Full/Upper/Close의 의미와 기존 자동 번호·정렬·GO 구조는 유지한다.

Cue는 Animation·Camera·Lighting·Appearance 목표 상태에 집중한다. Cue별 Camera Blend, Lighting Fade, Blend/Fade Duration, Transition Time의 입력 UI와 지정 시간 실행은 이번 개발 범위 및 완료 기준에서 제외한다. 이전 문서의 P1 시간 제어 계획은 이번 범위에 포함되지 않는다. 정상 동작하는 Studio 카메라·조명·Timeline을 삭제하거나 리팩터링하지 않는다.

DB와 schema v2 응답의 cameraBlendMs/lightingFadeMs는 호환성을 위해 계속 저장·반환한다. 생성에서 생략 시 0, 수정에서 생략 시 기존 값 보존, 복제 시 원본 시간 값 보존으로 처리한다. 웹에서는 두 값을 입력하거나 수정하지 않는다. 기존 Cue ID·연출·순서·자동 Number와 Revision 이력을 임의로 바꾸지 않는다.

### Unreal 후속 연결 — 이번 웹·서버 작업에는 미포함

현재 Source/NetworkProject/Private/CueJson.cpp의 Camera 허용 목록은 Full/Upper/Close다. Stage를 추가하기 전에는 Stage Cue가 포함된 REFRESH가 스키마 검증에서 거절된다. 다음 Unreal 작업에서 C++ 허용 목록에 문자열 Stage를 추가해야 한다. 이는 데이터 파싱 변경이며 E_CameraState Enum 추가가 아니다.

그 후 Blueprint는 CameraState(Name) == Stage를 먼저 분기해 별도 Stage View를 활성화한다. Full/Upper/Close는 Stage View를 해제하고 기존 Enum·함수로 처리한다. 기존 Cue RESET, GO, Shader Mode 처리와 시간 보간은 이번 변경에서 연결하거나 완료로 판정하지 않는다. 이번에는 Unreal Source와 .uasset을 수정하지 않았다.

### Stage 웹·서버 변경 검증

서버 테스트 37 passed (기존 의존성 deprecation warning 1건). Stage 생성·수정·단건 조회·Show 상세·v2 다운로드·재시작, 기존 Camera 세 값, 생략 시 기본 시간 0, 기존 시간 값 보존과 자동 번호/정렬 회귀를 검증했다. 실제 브라우저 테스트에서 기존 시간 1234/5678을 가진 Cue를 Stage로 수정·복제해도 같은 값이 보존됐고 새 Stage Cue는 0/0으로 생성됐다.

변경 파일은 CueServer/app/cues/schemas.py, app/admin/index.html, app/admin/static/admin.js, 신규 tests/test_stage.py, README와 대응 설계·연결 문서다. Cue 정렬/번호 서비스·DB 마이그레이션·Unreal Source·.uasset에는 이번 단계의 쓰기 변경이 없다. 기존 8001 서버는 같은 DB로 재시작했고 원본 백업과 검증 자료는 Saved/CueStage에 남겼다.
