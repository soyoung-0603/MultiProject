# Virtual Performer Studio · Cue 시스템 PRD

UE 5.8 두 번째 프로젝트 · 최종 설계 · 2026년 11월 17일 KST

## 제품 목표

웹에서 공연 순서를 편집하고, Unreal Operator가 명시적으로 내려받은 Cue 목록을 로컬에서 실행한다. 네트워크가 끊겨도 같은 목록으로 공연을 진행할 수 있어야 한다. 기존 Studio의 Animation, Camera, Lighting과 Original Appearance를 유지하면서 동일한 캐릭터에서 Toon 모드를 시연한다.

## 실행 환경

동일 PC에서 Unreal MCP는 `http://127.0.0.1:8000/mcp`, 웹 관리자와 Cue List API는 `http://127.0.0.1:8001`을 사용한다. Studio의 PIE와 Standalone을 시연 대상으로 삼는다.

## 사용자와 시나리오

| 사용자 | 시나리오 | 기대 결과 |
|---|---|---|
| 쇼 편집자 | 브라우저에서 Show 생성, Cue 추가·수정·삭제, 순서 변경 | 새로고침과 서버 재시작 뒤에도 편집 결과와 Revision이 유지된다. |
| Operator | Studio에서 Show 선택 후 REFRESH | 검증된 목록이 로컬에 저장되고 Revision과 NEXT가 표시된다. 기존 파일이 있다면 다운로드 실패 중에도 사용 가능하다. |
| Operator | Standby에서 GO를 세 번 실행 | 로컬 순서대로 한 Cue씩 적용된다. CURRENT는 완료된 Cue, NEXT는 실행할 Cue를 표시한다. 마지막 Cue 뒤에는 종료 상태가 표시된다. |
| Operator | 웹 편집 도중 기존 Show 실행 | 활성 목록의 내용과 순서는 유지된다. 새 Revision은 REFRESH를 통해 활성화한다. |
| Operator | 전환 도중 STOP, 이후 RESET | 현재 카메라·조명 전환이 멈추고 추가 GO가 차단된다. RESET은 Standby를 적용하고 목록 처음부터 다시 실행할 수 있게 한다. |
| 시연자 | Original 프리셋을 고른 뒤 TOON, ORIGINAL 순서로 변경 | 선택된 프리셋 ID가 유지되고 각 모드의 지정 표현을 복원한다. |

## 기능 요구사항과 완료 기준

| ID | 우선순위 | 요구사항 | 완료 기준 |
|---|---|---|---|
| W1 | P0 | FastAPI + SQLite 웹 관리자. Show/Cue CRUD, 순서 변경, 복제, Cue List API | 웹에서 편집한 세 Cue의 순서와 Revision이 서버 재시작 후 유지된다. 삭제 확인과 입력 오류 안내가 동작한다. |
| U1 | P0 | UE 5.8 C++ HTTP/JSON REFRESH와 로컬 영구 저장 | 정상 목록을 받은 뒤 Unreal을 재시작하고 서버를 꺼도 같은 Revision 및 Cue 순서를 읽는다. 실패한 다운로드 이후에도 기존 목록을 사용한다. |
| U2 | P0 | Cue Control 및 로컬 GO. Animation·Camera·Lighting·Shader·Appearance 자동 적용 | 오프라인에서 세 Cue를 순서대로 한 번씩 적용한다. Camera Cue는 Stage View를 먼저 해제한다. 화면에 CURRENT/NEXT와 다섯 가지 다음 값을 표시한다. |
| S1 | P0 | ORIGINAL / TOON 수동 전환, 기존 캐릭터 유지 | 동일 Pawn·AnimBP에서 외형 전환이 가능하다. Toon의 Outline·MPC 동작이 보이고 Original 슬롯·보조 구성·MID를 복구한다. |
| S2 | P0 | Toon Appearance DataTable과 `Preset_01`–`Preset_05` | 같은 ID가 각 모드의 행을 선택한다. Toon 컬러의 적용·생략 규칙과 실제 표현을 확인하고 Original의 다섯 프리셋을 회귀 검증한다. |
| U3 | P0 | 기본 STOP / RESET과 중복 GO 차단 | STOP은 즉시 기존 카메라 전환을 멈추고 추가 GO를 차단한다. 확인한 RESET은 ORIGINAL / Preset_01 / Idle / Full / Neutral 및 첫 NEXT를 복원한다. |
| W2 | P1 | 동시 편집 충돌과 고급 캐시 복구 | 충돌한 편집을 보호하고 손상된 로컬 파일을 검증된 백업에서 복구한다. P0에서도 Revision과 실패 시 이전 목록 보존은 필수다. |
| U4 | P1 | 지정 시간 전환 및 정교한 취소 | Camera Blend / Lighting Fade 지정값에 따른 전환, 0 ms 즉시 적용, 복합 전환의 중간 취소와 콜백 정리를 검증한다. |
| X1 | P2 | OBS 및 YouTube 송출, 추가 기능 | 별도 범위로 승인 후 검증한다. 발표 완료 조건은 P0 중심이다. |

Cue 필드는 Cue Number, Name, Animation State, Camera State, Lighting State, Shader Mode, Appearance Preset, Camera Blend Time, Lighting Fade Time이다. P0에서도 전체 필드를 저장하고, 시간 제어는 P1로 표시한다. P0 GO는 기존 카메라 블렌드와 조명 적용 타이밍을 사용한다.

Hair Secondary Motion, Foot IK, Live Link는 발표 범위에서 제외한다. ASP.NET Core / SQL Server Ranking 실습은 참고용이며 보존한다.

## UI와 조작 기준

이미지 레퍼런스의 목록·Inspector 분리와 CURRENT/NEXT 배치를 사용한다. 기존 Operator의 디자인과 수동 기능을 유지하고 Cue Control 패널을 추가한다.

| 화면 | 정보 배치 | 조작과 오류 예방 |
|---|---|---|
| 웹 관리자 | 상단 Show 선택과 관리, Revision·저장 상태. Cue List에서 번호·이름·핵심 상태를 훑고, Cue Inspector에서 필드를 편집한다. 좁은 창에서는 목록 아래에 Inspector를 둔다. | 위/아래 버튼으로 순서를 바꾼다. 복제는 선택 Cue의 값을 담은 새 초안을 열고 고유 번호를 제안한다. 저장 전 변경, 저장 중, 저장 완료, 저장 실패를 텍스트로 구분한다. 입력 오류 위치와 해결 방법을 표시하고 삭제 전 대상 이름을 확인한다. |
| Unreal Cue Control | Show 이름, Revision, 마지막 성공 REFRESH 시각. CURRENT/NEXT, Animation·Camera·Lighting·Shader·Appearance의 Next Cue Preview, 읽기 전용 전체 목록과 완료 수·진행 상태. | 고정 조작 영역에 GO·STOP·RESET·REFRESH를 배치한다. GO 중복을 차단하고 STOP은 확인 없이 즉시 처리한다. RESET은 대상 쇼와 초기 상태를 표시한 확인을 거친다. |
| Unreal 상태 안내 | 마지막 서버 조회 결과와 로컬 실행 가능 여부를 각각 표시한다. 예를 들어 **서버 연결 실패 / 로컬 Cue 실행 가능**을 동시에 표시한다. | 수동 조작으로 실제 상태가 마지막 완료 Cue와 다르면 **MANUAL OVERRIDE**를 표시한다. CURRENT/NEXT는 유지하고 다음 GO가 다섯 상태를 적용한다. |

상태는 텍스트와 아이콘을 함께 사용하고 선택·오류·위험은 색상 외 형태와 레이블로 구분한다. 웹과 Unreal에서 키보드 탐색 순서, 보이는 포커스, 읽을 수 있는 글자 크기와 충분한 명도 대비를 검증한다. 일반 텍스트 대비 4.5:1, 큰 텍스트와 주요 컨트롤 경계·포커스 3:1을 설계 목표로 삼는다.

## 시연 완료 기준과 일정

- **11월 18일 KST까지** Studio PIE와 Standalone에서 웹 편집·복제, REFRESH, 오프라인 GO 3회, 자동 Shader·Appearance, Stage View 해제, 기본 STOP/RESET과 MANUAL OVERRIDE를 실증해 자료를 확보한다.
- 시연 Cue 예시는 010 Idle/Full/Neutral/ORIGINAL/Preset_01, 020 Talk/Upper/Focus/TOON/Preset_02, 030 Dance/Full/Stage/ORIGINAL/Preset_03이다. 수동 변경 및 RESET으로 상태 차이와 복구도 시연한다.
- **11월 19일 KST**은 발표 자료·시연 리허설에 사용한다. P1 기능이 미검증이면 계획 항목으로 표기한다. P0 결손이 있으면 성공한 경로만 시연하고 결손을 명시한다.

## 시연 운영 기준

STOP 잠금은 RESET으로 해제한다. Toon 의상은 대상 Material에서 실제로 지원하고 매핑한 파라미터만 적용한다. Original 에셋과 기존 Git 상태는 보존하고 후속 구현은 현 상태를 반영한 격리 사본에서 진행한다. 색상 튜닝 값과 기존 Standby 위치·입력 연동은 Studio에서 검증한다.

관련 문서 [SDD](SDD.md) · [구현 계획](ImplementationPlan.md)
