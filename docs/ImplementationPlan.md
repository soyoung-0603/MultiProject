# Virtual Performer Studio · Cue 시스템 구현 계획

최종 설계 · 모든 작업은 미착수 · 2026년 11월 17일 KST

11월 18일 KST까지 Studio PIE와 Standalone에서 웹 편집, REFRESH, 오프라인 3 Cue 자동 적용 및 기본 STOP/RESET을 시연한다. 동일 PC의 Unreal MCP는 `127.0.0.1:8000`, FastAPI는 `127.0.0.1:8001`이다. C++는 HTTP/JSON·로컬 저장·재생 상태를, 기존 Blueprint는 Studio 런타임을 맡는다. [SDD](SDD.md)와 [PRD](PRD.md)의 계약을 적용한다.

## 작업 경계와 순서

아래 변경은 향후 구현 단계의 작업이다. 기존 Git 수정·삭제·미추적 파일을 보존한 격리 사본에서 시작한다. 커밋된 상태만 체크아웃하면 현재 Toon 파일 배치와 UE 5.8 플러그인 설정이 빠질 수 있으므로, 사본과 원래 작업 상태를 비교한다. 기존 Ranking 실습, Original Material/DataTable 및 원본 Level을 보존한다. 표의 신규 코드 경로·클래스·에셋명은 모두 제안이다.

| 순서 | 우선순위 | 산출물 및 수정 대상 | Codex 담당 | Unreal Editor 수동 작업 | 검증 및 통과 기준 |
|---|---|---|---|---|---|
| 0. Studio 기준선 | P0 | 기존 `/Game/Maps/Studio`, `BP_StudioManager`, `WBP_Operator`, `BP_BountyHunter`, ToonTest | 읽기 전용 MCP로 호출 노드와 에셋 경로 대조. 기존 변경을 보존한 사본에서 사용할 참조를 기록 | Studio를 열고 PIE에서 GameMode, Pawn, 매니저 초기화, 수동 Animation/Camera/Lighting/Appearance 실행. TestMap의 Toon 기준 화면 캡처 | Original/Toon 화면과 슬롯·조명 기준값 확보. 기존 이상은 기록하고 변경으로 생긴 회귀와 구분 |
| 1. 웹 Cue List | P0 | 신규 `CueServer/`와 API 테스트 | SQLite·Revision·CRUD·다운로드를 연결한다. Show 선택, 목록/Inspector, 위·아래 이동, 복제 초안, 저장 상태·입력 오류, 삭제 확인을 구현한다. | 없음 | 세 Cue 편집 및 정렬 후 서버 재시작. 순서·Revision 유지, 오류 및 키보드 동작 확인. |
| 2. Unreal 모듈과 캐시 | P0 | 격리 사본의 `.uproject`, 제안 `Source/NetworkProject/`와 C++ Cue Player | 심플한 HTTP GET, 검증, 임시 파일 교체, 정상 목록 유지와 시작 복원. 서버 결과와 로컬 상태를 별도 제공한다. | 호환 툴체인 확인, Development Editor 빌드와 Editor 재오픈 | REFRESH 성공, 연결 실패/불량 응답/쓰기 실패 뒤 이전 정상본 보존. 재시작 후 로컬 목록 사용. |
| 3. 기본 GO/STOP/RESET | P0 | 격리 사본의 `BP_StudioManager`, `WBP_Operator`, Camera Timeline, 복제 Studio 레벨 | 공통 명령, RunId, GO 중복 차단, 완료 결과와 Cue Control UI 상태 연결 | 기존 Operator 유지, 카메라 완료/중단, Stage View 자동 해제, 정확한 RESET 상태 연결 | 기본 3 Cue의 오프라인 실행, 중복 GO, STOP 즉시, RESET 확인 및 첫 NEXT 복귀. |
| 4. Shader/Appearance와 시연 UI | P0 | 격리 사본의 `BP_BountyHunter`, Operator, Toon Struct/DataTable | 슬롯 및 파라미터 호환표, 수동·Cue 공통 경로, Cue Control의 Preview·진행·MANUAL OVERRIDE 연결 | 보이는 보조 Mesh/Outline/MPC, MID 재연결, 다섯 프리셋·지원 의상 값 튜닝 | 세 Cue에서 ORIGINAL → TOON → ORIGINAL 자동 왕복. 수동 변경 표시와 RESET 외형 복원. PIE와 Standalone 증빙. |
| 5. 정교한 전환과 복구 | P1 | Cue Player, 매니저·Timeline·조명 BP, API·캐시 테스트 | 전환 시간, 복합 취소, 동시 편집 `If-Match`, 캐시 백업 복구를 추가한다. | 시간 보간 중 카메라·조명 정지와 월드 전환 확인 | 0 ms/지정 시간, 두 전환 중 STOP, 편집 충돌, 손상 캐시 복구를 검증한다. |
| 6. 발표 이후 확장 | P2 | OBS, YouTube 송출 설정과 통합 문서 | 별도 범위에서 연결 계획 작성 | 시연 세션 밖에서 송출 및 화면 캡처 검증 | 내부 리허설과 송출 검증 완료 후 채택 |

## P0에서 가져올 실습 코드

| 연결 작업 | Codex 담당 | 검증 |
|---|---|---|
| 1. 웹 서버와 관리자 UI | `C:\Work\PythonAPIServer`의 앱 팩터리, SQLAlchemy 세션, 라우터·스키마·서비스 구조, 관리자 HTML의 fetch·폼 처리, 임시 DB 테스트를 신규 `CueServer/`에서 활용한다. Show/Cue/Revision, 순서 변경, Cue Inspector와 다운로드 스냅샷은 Cue 계약에 맞춘다. | 독립 임시 DB로 CRUD·순서·Revision·실패 롤백을 확인한다. 웹 UI와 API가 8001에서 제공되고 Unreal MCP가 8000을 유지하는지 확인한다. |
| 2. Unreal HTTP/JSON | `C:\Work\Unreal\WebAPIUnreal\Source\WebAPIUnreal\Private\WebAPI\RankingSubsystem.h`·`.cpp`의 서브시스템, `FHttpModule`, JSON 처리, 완료 알림 패턴을 UE 5.8 REFRESH에 적용한다. Cue DTO, 응답 검증과 로컬 캐시 저장을 연결한다. | 정상 다운로드, 연결·응답·저장 실패 후 기존 목록 유지, 서버 종료 후 3회 GO에서 HTTP 0건을 확인한다. |

C# Web API 실습은 CRUD 및 계층 구성의 참고 자료로 사용한다. 실습 원본과 백업, Ranking DB 및 설정은 보존한다. 시연과 테스트는 후속 구현 단계의 신규 코드 및 독립 DB에서 진행한다.

## 작업별 시작 조건

- [ ] **Studio 기준선** — 현재 변경이 포함된 사본을 확보하고 Studio에서 수동 기능을 확인한다. TestMap의 두 캐릭터를 비교하고 `BP_StudioManager.BountyHunterRef` 타입, 카메라 완료 핀, Toon의 보조 Mesh를 기록한다.
- [ ] **첫 작업 단위** — `CueServer`의 임시 SQLite DB에서 Show 생성 → Cue 3개 추가 → 재정렬 → 재접속 → `GET /api/v1/shows/{showId}/cue-list` 결과를 검사하는 테스트를 먼저 작성한다. 이 테스트를 통과시키는 스키마 v1·트랜잭션·다운로드 경로를 만들고, 같은 흐름을 웹 UI에 연결한다. 시연 Show와 최종 JSON을 Unreal 쪽 공용 검증 자료로 사용한다.
- [ ] **Unreal 접합** — 공용 JSON을 읽는 캐시 검증부터 통과시킨다. 이어서 Studio와 복제 시연 레벨에서 명령 하나를 수동 UI와 GO 양쪽으로 실행하고 세 Cue로 확장한다. 서버 중단 및 웹 편집 후 REFRESH 전/후를 로그로 비교한다.
- [ ] **Shader** — 우선 `Preset_01`로 모드 왕복이 가능한 구성을 만들고 `Preset_02`–`05`로 확장한다. Face/Body와 실제 보이는 Hair/Goggle의 MID를 검증한다. 목표 값이 없거나 복원에 실패하는 경로도 따로 실행한다.
- [ ] **P1** — P0 실증 이후 전환 시간 제어, 동시 보간 정교한 취소, 동시 편집 충돌, 고급 캐시 복구를 검증한다. 패키징은 시연 이후에 진행한다.

## P0 시연에서 줄일 범위

웹은 단순 목록과 Inspector, 위/아래 순서 버튼, 복제 초안으로 구성한다. Unreal은 기존 Operator에 Cue Control 패널 하나를 붙인다. 패키징, 실시간 서버 감시, 드래그 전용 편집, 복합 전환 및 고급 복구는 시연 경로에서 제외한다. 먼저 3개 Cue의 기존 카메라 타이밍과 즉시 조명 적용으로 전체 경로를 연결한다.

## 11월 18일 시연 증빙과 11월 19일 발표 준비

| 기한 (KST) | 작업과 증빙 |
|---|---|
| 11월 18일 전반 | Studio 기준선과 C++ 빌드 경로 확인. CRUD·순서·Revision 결과, 정상 Cue List 스냅샷 저장. Studio에서 첫 GO 실행 확인 |
| 11월 18일 후반 | PIE와 Standalone에서 3회 오프라인 GO, 자동 Shader·Appearance, Stage View 해제, 즉시 STOP·확인 RESET, MANUAL OVERRIDE와 웹 복제를 캡처한다. 서버 요청 기록과 상태 로그를 보관한다. |
| 11월 19일 | 발표 슬라이드, 로컬 Cue 사본, 캡처·녹화 자료를 준비하고 리허설. 확인된 시연 흐름으로 발표 |

일정은 목표다. C++ 콜드 빌드가 지연되거나 Original 복구가 검증되지 않으면 API·UI와 현재 검증된 Studio 화면을 따로 기록한다. 로컬 JSON이나 녹화 자료를 사용한 시연은 해당 방식으로 표시한다. Shader·Animation 회귀가 있는 상태에서 완료 판정을 내리지 않는다.

## 주요 위험과 사용자 결정

- **상태와 적용 시점이 분산됨.** 이동·제스처 검사와 이벤트는 보존하고, Preset 매칭 알림의 재호출과 상태 변경을 분리한다.
- **Toon 구성 이식.** Shader 슬롯 이름만 바꿔서는 Outline·SDF·보조 Mesh와 기존 MID를 복구할 수 없다. 먼저 모드 왕복 회귀 시험을 통과시킨다.
- **데이터와 실행의 경계.** 서버 쓰기 오류와 로컬 저장 실패에서 마지막 정상본을 보호한다. 완료 콜백이 늦어도 STOP 이후 커서가 움직이지 않아야 한다.
- **발표 일정.** 모듈·툴체인, Studio 수동 검증, Toon 프리셋 튜닝이 일정에 직접 영향을 준다. P0을 먼저 증빙하고 P1은 검증 가능한 범위만 포함한다.
- **현장 확인.** 기존 Standby 위치·입력 연결, Toon 의상 파라미터와 실제 화면을 확인한다. 판단이 덜 끝난 P1 기능은 시연 완료 목록에서 제외한다.

관련 문서 [PRD](PRD.md) · [SDD](SDD.md)
