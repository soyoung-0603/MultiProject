# Virtual Performer Studio · Cue 시스템 PRD

UE 5.8 · 최종 요구사항 반영 · 2026년 11월 18일 KST

## 제품 목적

Virtual Performer Studio는 Unreal Engine 기반 실시간 버추얼 캐릭터 연출 및 제어 시스템이다. 기존 Operator의 Animation, Camera, Lighting, Appearance, Position, Gesture와 통합 Preset 제어를 유지하면서 방송 순서에 맞춰 준비한 연출을 순차 실행한다. 목표는 HTTP 연결 자체가 아니라 **연출 준비와 실제 실행을 분리하는 제작 파이프라인**이다.

QLab Cue List, Ross OverDrive, Unreal Motion Design Rundown의 구성·실행 분리 개념을 참고한다. 해당 제품 전체 기능이나 상용 방송 자동화 시스템을 재현하는 범위는 아니다.

## 웹서버를 사용하는 이유

한 Operator가 준비와 실행을 모두 담당하면 DataTable만으로도 충분하다. 웹 방식은 Editor 없는 브라우저 편집, 태블릿·다른 PC의 방송 구성 관리, 방송별 Show/Cue List, 추가·수정·삭제·복제·순서 변경, Director/Operator 역할 분리, Revision 관리, 프로젝트 재빌드 없는 구성 변경을 지원하기 위해 유지한다.

기존 DataTable은 카메라·조명·외형의 세부 값을 계속 소유한다. 외부 Cue에는 상태와 Appearance Preset 참조만 저장한다. Material Parameter, Camera Transform, Lighting Intensity를 직접 넣지 않는다.

## 역할과 데이터 흐름

| 역할 | 담당 | 경계 |
|---|---|---|
| Director — Web Cue Manager | Show 관리, Cue 추가·수정·삭제·복제, 순서와 네 연출 항목 구성 | 웹 편집으로 Unreal 실제 연출을 즉시 변경하지 않는다. |
| Operator — Unreal Engine | Show 선택, REFRESH, CURRENT/NEXT 확인, GO/STOP/RESET, 기존 수동 제어 | 내려받은 로컬 스냅샷으로 실제 연출을 실행한다. |

두 역할은 같은 사람이 수행할 수 있다.

```text
Director Browser → FastAPI / SQLite → HTTP REFRESH → Local Cue Snapshot
                                                        ↓
Operator → Cue Player → 기존 Blueprint / DataTable → 실제 연출
```

서버는 Show·Cue·Revision 데이터를 관리하고 실행은 Unreal 내부에서 처리한다. GO마다 서버를 요청하지 않는다. 유효한 스냅샷 이후에는 서버 장애 중에도 같은 목록을 실행한다. 웹 변경은 다음 REFRESH 전까지 활성 스냅샷에 영향을 주지 않는다.

## 현재 상태와 이번 단계

사용자가 WBP_Operator에 Show ComboBox, REFRESH, ScrollBox Cue List, NEXT 정보, GO/STOP/RESET을 구현했다. WBP_CueItem, E_CueItemState, ST_CueItemStyle, DT_CueItemStyle과 동적 생성 테스트도 존재한다. Shader Mode와 모드별 Appearance Preset은 기존 구현을 재사용한다.

**이번 단계는 C++ 데이터 기반 구현이다. 기존 UI와 Blueprint 연결은 사용자가 수행한다. GO/STOP/RESET 실제 연출, 실행 커서, RunId와 MANUAL OVERRIDE 판정은 다음 단계이며 이번 완료 범위가 아니다.**

| ID | 우선순위 | 전체 요구사항 | 이번 단계 |
|---|---|---|---|
| W1 | P0 | 웹 Show/Cue CRUD·복제·순서·Revision·영구 저장 | 기존 응답·DB 구조와 Show ID/연출 보존. 번호 정책은 추가 변경 참조 |
| U1 | P0 | HTTP 조회, v2 검증, 영구 스냅샷, 실패 시 정상본 유지 | 구현·테스트·Development Editor Win64 빌드 |
| U2 | P0 | CURRENT/NEXT, 로컬 순차 GO와 네 상태 적용 | UI는 존재. 실제 wiring과 실행은 다음 단계 |
| U3 | P0 | STOP 취소·시각 유지·늦은 완료 무시, 모드 유지 RESET | 다음 단계 |
| S1/S2 | P0 | 독립 수동 Shader Mode와 모드별 Appearance | 사용자 기존 구현 보존, 재구현하지 않음 |
| U4/W2 | P1 | 지정 시간 보간, 복합 취소, ETag/충돌 제어, 백업 복구 | 후순위 |
| X1 | P2 | 방송 송출·원격 영상 모니터링·확장 | 후순위 |

## Cue 데이터와 Shader 독립성

Cue는 ID, Position, Number, Name, Animation State, Camera State, Lighting State, Appearance Preset, Camera Blend ms, Lighting Fade ms를 갖는다. **Position이 실행 순서이고 Number는 현재 Position에서 자동 생성하는 표시값이다.** 시간 필드는 유지하되 세부 시간 제어는 후순위다.

Shader Mode는 Cue Schema에 포함하지 않는다. Operator가 ORIGINAL/TOON을 수동 선택한다. Appearance는 현재 모드의 DataTable에서 같은 Preset_01–Preset_05를 조회한다. GO와 Cue RESET은 모드를 바꾸지 않는다.

## 조작별 전체 시스템 완료 기준

| 조작 | 기대 동작 |
|---|---|
| REFRESH | 선택 Show 비동기 GET, v2·Show ID·Revision·목록 검증, 저장 성공 후 반영. 실패 시 정상 파일·메모리·성공 시각 유지. 통신과 로컬 사용 가능 상태 구분 |
| GO | 로컬 NEXT의 네 상태 적용, 중복 실행 차단, 실제 성공 후 CURRENT/NEXT·진행 갱신. HTTP 없음 |
| STOP | 진행 중 전환 취소, 중단 시점 시각 유지, 늦은 완료 무시. HTTP 없음 |
| RESET | 진행 초기화, Idle / Full / Neutral / Preset_01, 현재 Shader Mode 유지. HTTP 없음. 기존 ResetAllState 호출 금지 |

Full/Upper/Close Cue는 Stage View를 먼저 해제하고, Stage Cue는 별도 Stage View Override로 처리한다. STOP 이후 GO는 RESET까지 차단한다. 실행 중 REFRESH 잠금, 진행한 목록 교체, RunId와 실제 완료 감지는 다음 실행 단계에서 연결한다. 이번 로컬 사용 가능 표시는 연출 실행 허가나 완료를 뜻하지 않는다.

## 기존 UI 연결

Pending / Next / Running / Applied / Stopped는 기존 E_CueItemState로 유지하고 C++에서 중복 생성하지 않는다. InitializeCueItem, SetCueItemState, WBP_Operator.AddCueItem을 사용자가 연결한다.

이번 연결 안내는 Show 조회 → ID 기반 선택 → REFRESH → 검증된 Snapshot으로 목록 생성·첫 NEXT 표시까지다. 실패 때 빈 데이터로 UI를 덮어쓰지 않고 서버 실패와 로컬 준비를 동시에 표시한다. 표시만으로 Applied/Running 상태를 만들지 않는다. 키보드 포커스와 상태 텍스트를 유지한다.

## 보호 경계와 실행 환경

- 기존 Blueprint, Widget, Map, Material, DataTable을 수정하지 않는다. Studio 복제, 신규 Bridge Blueprint나 별도 Cue Control Widget도 만들지 않는다.
- CueServer 조회 응답과 DB 테이블 구조, 기존 Show/Cue ID·연출 데이터, 기존 Git 변경을 보존한다. 번호 입력 계약은 추가 변경에서 제거한다. demo.sqlite3으로 임의 전환하지 않는다.
- MCP는 127.0.0.1:8000, 기본 API는 http://127.0.0.1:8001. API Base URL은 프로젝트 설정에서 변경한다.
- 기존 GameMode/Pawn/Blueprint 부모와 에셋 참조를 유지한다. 첫 빌드 전 사용자가 Save All 후 Editor를 직접 종료한다.
- PIE·Standalone 실제 연출과 데이터 계층 테스트 결과를 구분한다. 11월 18일은 P0 우선 구현, 11월 19일은 검증된 결과로 발표·리허설한다. 미연결 기능을 완료로 표시하지 않는다.

관련 문서 [SDD](SDD.md) · [구현 계획](ImplementationPlan.md) · [Blueprint 연결 안내](BlueprintIntegration.md)

## 추가 변경: Cue 자동 번호 (2026년 11월 19일 KST)

Cue 번호의 최종 정책은 1부터 연속 증가다. 서버가 현재 Position 순서대로 number = position을 저장하고 응답한다. 웹 화면은 최소 3자리(001, 002, 003)로 표시하며 1000 이상도 자르지 않는다. 010/020/030처럼 10씩 증가하는 정책은 사용하지 않는다.

웹 생성·수정·복제 초안에는 번호 입력이 없다. 추가·삭제·드래그/버튼 정렬 후 모든 번호를 다시 지정하고 서버 재조회 결과를 표시한다. 식별과 선택·정렬 요청은 UUID를 사용한다. ID·이름·Animation/Camera/Lighting/Appearance·Blend/Fade는 번호 변경으로 바뀌지 않는다.

POST/PATCH 입력에서 number를 제거하고 직접 입력은 422로 거절한다. GET 응답의 정수 number, schemaVersion 2, 기존 DB 테이블·UUID 구조는 유지한다. 두 UNIQUE 열은 트랜잭션 안에서 임시 음수로 옮긴 뒤 연속 Position/Number를 확정한다. 전체 실패는 롤백한다.

기존 데이터는 시작 시 저장된 순서대로 전환하며 영향받은 Show만 새 Revision을 발행한다. 과거 v2 Revision은 그대로 보존하고 다음 재시작에는 다시 발행하지 않는다. 구형 v1 스키마 변환과도 같은 트랜잭션으로 처리한다. 운영 DB는 전환 전에 별도로 백업한다.

Unreal의 CueData.Number는 계속 Integer다. 기존 Blueprint의 최소 3자리 표시는 유지하고 새 REFRESH 후 GO 001/002/003에 사용할 수 있다. 이전 로컬 스냅샷의 번호와 과거 Number/Position 독립 파싱 호환성은 유지한다. 기존 Blueprint/Map/Material/DataTable/C++는 이번 변경에서 수정하지 않는다.

## Camera Stage 및 전환 UI 범위 변경 (2026년 11월 19일 KST)

웹·서버 Camera 선택지는 Full / Upper / Close / Stage다. 기존 cameraState 필드에 Stage 문자열을 저장·반환한다. Stage는 Unreal의 별도 Stage View Override이며 E_CameraState에 Stage를 추가하거나 Camera DataTable을 변경하지 않는다. Full/Upper/Close의 의미와 기존 자동 번호·정렬·GO 구조는 유지한다.

Cue는 Animation·Camera·Lighting·Appearance 목표 상태에 집중한다. Cue별 Camera Blend, Lighting Fade, Blend/Fade Duration, Transition Time의 입력 UI와 지정 시간 실행은 이번 개발 범위 및 완료 기준에서 제외한다. 이전 문서의 P1 시간 제어 계획은 이번 범위에 포함되지 않는다. 정상 동작하는 Studio 카메라·조명·Timeline을 삭제하거나 리팩터링하지 않는다.

DB와 schema v2 응답의 cameraBlendMs/lightingFadeMs는 호환성을 위해 계속 저장·반환한다. 생성에서 생략 시 0, 수정에서 생략 시 기존 값 보존, 복제 시 원본 시간 값 보존으로 처리한다. 웹에서는 두 값을 입력하거나 수정하지 않는다. 기존 Cue ID·연출·순서·자동 Number와 Revision 이력을 임의로 바꾸지 않는다.

### Unreal 후속 연결 — 이번 웹·서버 작업에는 미포함

현재 Source/NetworkProject/Private/CueJson.cpp의 Camera 허용 목록은 Full/Upper/Close다. Stage를 추가하기 전에는 Stage Cue가 포함된 REFRESH가 스키마 검증에서 거절된다. 다음 Unreal 작업에서 C++ 허용 목록에 문자열 Stage를 추가해야 한다. 이는 데이터 파싱 변경이며 E_CameraState Enum 추가가 아니다.

그 후 Blueprint는 CameraState(Name) == Stage를 먼저 분기해 별도 Stage View를 활성화한다. Full/Upper/Close는 Stage View를 해제하고 기존 Enum·함수로 처리한다. 기존 Cue RESET, GO, Shader Mode 처리와 시간 보간은 이번 변경에서 연결하거나 완료로 판정하지 않는다. 이번에는 Unreal Source와 .uasset을 수정하지 않았다.
