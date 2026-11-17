# Virtual Performer Studio · Cue 시스템 SDD

UE 5.8 두 번째 프로젝트 · 최종 설계 · 2026년 11월 17일 KST

## 구조와 데이터 흐름

```text
브라우저 편집 UI → FastAPI → SQLite (Show / Cue / Revision)
                               ↓ 발행된 Show 스냅샷
                         REFRESH (HTTP GET)
                               ↓ 검증 및 로컬 저장
Operator → GO/STOP/RESET → Unreal C++ Cue Player
                               ↓ 공통 Runtime 명령
기존 Operator 수동 제어 ──────→ StudioManager → BountyHunter / Camera / 조명 / Stage
                               ↑ 적용·완료·취소 결과
```

동일 PC에서 공식 Unreal MCP는 `http://127.0.0.1:8000/mcp`로 Editor 조사에 사용한다. FastAPI는 기본적으로 `127.0.0.1:8001`에 바인딩하고 HTML/CSS/JS와 API를 같은 Origin으로 제공한다. C++는 REFRESH, JSON 검증, 영구 캐시와 재생 상태를 관리한다. Blueprint는 기존 Studio 동작과 렌더링을 소유한다. 시연은 PIE와 Standalone을 우선한다.

Cue Player의 API Base URL은 변경 가능한 프로젝트 설정으로 관리한다. 기본값은 `http://127.0.0.1:8001`이다. LAN 시연에서는 CueServer가 바인딩한 IPv4 주소를 사용한다. 예를 들어 서버 주소가 `192.168.1.10`이면 `http://192.168.1.10:8001`을 지정한다. Show 목록과 REFRESH 요청은 설정값에 `/api/v1` 경로를 붙여 구성한다. 이 설정은 CueServer 요청에만 적용된다.

## 실습 소스 재사용

실습 원본은 읽기 전용으로 보존한다. 구현 시 필요한 코드를 격리 사본의 신규 Cue 코드에 선별해 옮기고, 실습의 DB·환경 설정·실행 파일은 가져오지 않는다.

| 기반 | 가져올 구조 | Cue에 맞게 작성할 부분 |
|---|---|---|
| `C:\Work\PythonAPIServer` | `app/main.py`의 FastAPI 앱 팩터리·라우터 등록·같은 Origin의 HTML 제공, `app/database.py`의 SQLAlchemy 세션·롤백 패턴, `app/ranking/`의 라우터·스키마·서비스 분리, `app/admin/index.html`의 fetch·폼·저장 상태, `tests/test_api.py`의 임시 DB 테스트 | 신규 `CueServer/`에 Show/Cue/Revision, 트랜잭션 기반 순서 갱신과 다운로드 스냅샷, Cue List·Inspector·복제·검증을 구현한다. 새 SQLite 파일과 `127.0.0.1:8001`을 사용한다. 실습 README의 8000은 MCP 포트로 남긴다. |
| `C:\Work\Unreal\WebAPIUnreal` | `Source/WebAPIUnreal/Private/WebAPI/RankingSubsystem.h`·`.cpp`의 `UGameInstanceSubsystem` 구조, `FHttpModule` 요청, `BindUObject` 완료 콜백, JSON 파싱과 Blueprint 결과 알림 | UE 5.8 Cue Player의 REFRESH에 적용한다. Ranking URL·DTO·CRUD 대신 8001의 Cue List GET, HTTP 상태와 스키마·Show ID·Revision 검증, 캐시 저장 후 활성화와 실패 시 기존 목록 보존을 연결한다. GO/STOP/RESET은 로컬 실행 경로를 유지한다. |
| `C:\Work\WebAPI_Project`, `C:\Work\WebAPI_Study` 및 백업 | ASP.NET Core Ranking Controller와 EF Core의 CRUD·계층 분리 참고 | C# 프로젝트와 SQL Server는 Cue 실행·배포 의존성에서 제외한다. |

## 기존 Blueprint 통합

MCP로 확인한 기준은 `/Game/Maps/Studio`의 매니저·스테이지 의존성과 `/Game/Maps/TestMap`의 두 테스트 Actor다. 레벨을 열어 본 시점의 맵과 기본 시작 맵은 TestMap이었다. Studio에서 실제 플레이 검증은 남아 있다.

| 기존 에셋 | 재사용 및 최소 변경 제안 |
|---|---|
| `/Game/Blueprints/Core/BP_StudioManager` | `UpdateCharacterState`, `UpdateGestureState`는 상태 변경 부분으로 재사용한다. `PlayGesture`, `UpdateCameraState`, `SetStageView`, `UpdateLightingState`, `UpdateAppearancePresetState` → `ApplyAppearancePreset`를 기존 적용 경로로 사용한다. |
| `/Game/Blueprints/UI/WBP_Operator` | `ClickedCharacterState`의 이동·제스처 제한, `SetCharacterStateButtonState`의 알림, `SetGestureStateButtonState`의 `PlayGesture` 호출을 공통 진입점으로 옮긴다. 기존 버튼과 표시 갱신·바인딩을 연결한다. |
| `/Game/Blueprints/Character/BP_BountyHunter`, `/Game/Animations/ABP_BountyHunter` | Pawn과 AnimBP 유지. `CachMID`, 카메라 Timeline, `ApplyCharacterLighting`, 트래킹을 재사용한다. AnimBP는 매니저 상태를 계속 읽는다. |
| `/Game/Blueprints/Character/BP_BountyHunter_ToonTest` | Material Override, 보조 컴포넌트·Outline 및 MPC 동작의 참조. 직접 부모가 `Character`이고 메인 매니저의 `BountyHunterRef`는 `BP_BountyHunter_C` 타입이다. |

C++가 구현 세부에 의존하지 않도록, 매니저가 구현하는 C++ Blueprint 인터페이스 `UStudioCueRuntime`을 제안한다. 인터페이스와 다음 이름은 신규 제안이다. `ApplyStudioCommand(Command, RunId)`는 Cue와 수동 Animation·Camera·Lighting·Appearance 변경을 받는다. Shader Mode 전환은 Operator의 별도 수동 조작에서 처리한다. `CancelStudioCommand(RunId)`는 전환을 중단하고, `ResetStudioStandby(RunId)`는 현재 모드에서 Standby를 적용한다. 결과는 RunId와 함께 Started/Completed/Rejected/Cancelled/Failed를 반환 또는 통지한다. C++는 인터페이스의 Blueprint 디스패치를 사용한다. Blueprint 내부 함수의 이름 기반 호출을 전제로 삼지 않는다.

1. 매니저는 게임 스레드에서 초기화 완료, 권한 있는 월드, 참조, 이동·제스처 제한, Stage View 상태, Enum과 필요한 행/에셋을 먼저 확인한다. Stage View 중 Camera Cue가 있으면 정상 종료 경로를 선행하고, 실패하면 Cue를 거절한다. 수동 Camera 편집은 기존 Stage View 조건을 유지한다.
2. 실행할 상태와 적용 재료를 준비하고, 현재 Shader Mode의 DataTable에서 Appearance Preset ID를 해석한다. Appearance → Animation → Camera/Lighting 순서로 요청한다. Animation은 상태를 갱신한 뒤 기존 메시 위치·음악 연동에 필요한 이벤트를 전달한다. Camera와 Lighting은 준비 후 각각 목표 상태를 전달하고 P1에서 병렬로 전환한다.
3. 기존 내부 이벤트는 필요한 시점에 한 번만 전달한다. 외부 UI에 대한 명령 완료 통지는 두 전환이 모두 끝나고 실제 상태를 확정한 뒤 한 번 보낸다. WBP는 이 통지에서 표시만 갱신한다. 완료 전 상태를 읽는 AnimBP용 상태와 Cue의 완료 상태는 구분한다.
4. `DT_PresetData`의 통합 Preset은 세 상태를 한 명령으로 적용한다. `MatchPresetState`는 캐릭터·카메라·조명을 모두 비교하도록 조정하고, `OnPresetChanged` 수신에서 `SetPresetButtonState`가 런타임을 재호출하는 경로를 표시 갱신으로 바꾼다.

P0 GO는 현재 Shader Mode를 유지하면서 Animation·Camera·Lighting·Appearance를 적용한다. 기존 카메라 블렌드와 즉시 조명 적용을 사용하며, CURRENT는 카메라 Timeline 완료와 조명 적용 이후 확정한다. P1에서 Camera Blend / Lighting Fade를 현재값에서 목표값으로 보간하며 0 ms는 즉시 적용한다. 기존 `UpdateLightingState`의 동일 상태 생략은 RESET 또는 수동 Shader Mode 변경 후 조명 재적용이 필요한 경우 강제 적용 경로로 처리한다.

## API와 SQLite

제안 경로는 `/api/v1` 기준이다. ID는 UUID 문자열이고 타임스탬프는 UTC ISO 8601을 사용한다.

| 메서드·경로 | 요청·결과 |
|---|---|
| GET `/shows`, GET `/shows/{showId}` | Show 목록 또는 상세. 상세에 현재 `revision`과 순서 있는 Cue 목록을 포함한다. |
| POST `/shows` | `{name}`으로 생성, 201. 최초 Revision은 1이며 목록은 비어 있다. |
| PATCH / DELETE `/shows/{showId}` | 이름 수정 또는 삭제. DELETE는 tombstone Revision을 기록하고 204를 반환한다. 삭제된 Show 조회와 다운로드는 404다. |
| POST `/shows/{showId}/cues` | Cue 필드 입력, 서버가 ID와 마지막 position 부여, 201. |
| GET / PATCH / DELETE `/shows/{showId}/cues/{cueId}` | 조회·부분 수정·삭제. 삭제 시 남은 position을 연속으로 정리한다. |
| PUT `/shows/{showId}/cue-order` | `{cueIds:[...]}`에 현재 Show 내 모든 ID를 중복 없이 완전한 순열로 제공한다. |
| GET `/shows/{showId}/cue-list` | 현재 발행된 스키마 v2 스냅샷. P1에서는 스키마 버전·Show ID·Revision으로 `ETag`를 구성하고 일치하는 `If-None-Match`에 304를 반환한다. |

P0의 모든 변경은 Revision을 증가시킨다. P1에서 수정·삭제·순서 변경과 Cue 생성에 Show `If-Match`를 요구한다. 누락은 428, 충돌은 412이며 412에서는 재조회 후 변경을 다시 적용할지 편집자가 선택한다. 미존재 404, 번호 중복 409, 검증 실패 422, 저장 불가 503은 P0에서 처리한다. 오류 응답은 `{"code":"invalid_cue","message":"…","fieldErrors":[]}` 형식으로 제공하며 UI는 읽기 쉬운 메시지를 표시한다.

| 테이블 | 키와 저장 규칙 |
|---|---|
| Show | `id PK, name, current_revision, created_at, updated_at, deleted_at`. 이름 1–100자. |
| Cue | `id PK, show_id FK, position, number, name, animation_state, camera_state, lighting_state, appearance_preset, camera_blend_ms, lighting_fade_ms`. Show 안에서 position과 number는 각각 유일하다. |
| Revision | `(show_id, revision) PK, created_at, snapshot_json, is_deleted`. 변경 당시 Show 이름과 순서 있는 Cue를 저장한다. 삭제 이력과 예전 스냅샷을 유지한다. |

SQLite 외래 키를 활성화한다. 입력 검증 → 변경·순서 재배치 → Revision 증가 → 스냅샷 저장을 한 트랜잭션으로 처리한다. 순서 갱신은 충돌을 피하도록 임시 위치를 거친다. P1의 Revision 비교도 같은 트랜잭션에서 실행한다. 복제는 Inspector에서 초안을 만들고 저장 시 기존 Cue 생성 API를 사용한다. 서버 시작 시 기존 v1 DB의 Cue 열과 모든 Revision 스냅샷에서 Shader Mode를 제거하고 v2로 변환한다. 이 변환은 한 트랜잭션으로 수행하며 ID, 순서, Revision 번호, 발행 시각과 삭제 이력을 유지한다. 변환 이후 다운로드는 발행된 단일 Revision을 읽고 실패한 변경은 롤백한다.

### 다운로드 JSON Schema v2

```json
{
  "type": "object",
  "additionalProperties": false,
  "required": ["schemaVersion", "showId", "showName", "revision", "publishedAt", "cues"],
  "properties": {
    "schemaVersion": {"const": 2},
    "showId": {"type": "string", "format": "uuid"},
    "showName": {"type": "string", "minLength": 1, "maxLength": 100},
    "revision": {"type": "integer", "minimum": 1, "maximum": 2147483647},
    "publishedAt": {"type": "string", "format": "date-time"},
    "cues": {
      "type": "array", "maxItems": 1000,
      "items": {
        "type": "object", "additionalProperties": false,
        "required": ["id", "position", "number", "name", "animationState", "cameraState", "lightingState", "appearancePreset", "cameraBlendMs", "lightingFadeMs"],
        "properties": {
          "id": {"type": "string", "format": "uuid"},
          "position": {"type": "integer", "minimum": 1, "maximum": 1000},
          "number": {"type": "integer", "minimum": 1, "maximum": 999999},
          "name": {"type": "string", "minLength": 1, "maxLength": 100},
          "animationState": {"enum": ["Idle", "Talk", "Dance"]},
          "cameraState": {"enum": ["Full", "Upper", "Close"]},
          "lightingState": {"enum": ["Neutral", "Focus", "Stage"]},
          "appearancePreset": {"enum": ["Preset_01", "Preset_02", "Preset_03", "Preset_04", "Preset_05"]},
          "cameraBlendMs": {"type": "integer", "minimum": 0, "maximum": 30000},
          "lightingFadeMs": {"type": "integer", "minimum": 0, "maximum": 30000}
        }
      }
    }
  }
}
```

배열은 position 오름차순이며 position은 1부터 연속이다. Cue Number는 화면 표시용으로 순서와 독립이다. API와 Unreal 양쪽에서 ID·번호 유일성, 배열 순서, 공백 이름, 요청 Show ID 일치, 범위와 Enum을 추가 검증한다. 정수 밀리초와 최대 30초는 제안값이다.

## 로컬 저장과 복구

- **P0** · `FPaths::ProjectSavedDir()/CuePlayer/<showId>.json`에 검증된 Cue List와 마지막 성공 REFRESH 시각을 저장한다. 선택 Show ID와 캐시는 재시작 후 복원하며 실행 커서는 Standby에서 시작한다. 파일명에는 검증한 UUID를 쓴다.
- **P0** · REFRESH는 사용자가 요청할 때 비동기 GET 한 건을 실행한다. Show 선택은 기본 목록 조회를 사용할 수 있다. HTTP 상태, JSON, 스키마 v2, Show ID, Revision과 배열을 검증하고 같은 디렉터리의 임시 파일에 기록한 뒤 이전 정상 파일을 교체한다. 교체 성공 후 활성 목록과 시각을 갱신한다. 실패하면 기존 파일·메모리 목록·성공 시각이 유지된다. 서버 접속 상태는 마지막 시도 결과로 표시한다. 유효한 v2 목록이 없으면 GO를 잠근다.
- **P0** · Running 중 REFRESH와 추가 GO를 잠근다. 진행한 Show의 목록을 바꾸려면 RESET 후 REFRESH한다. 새 목록은 첫 Cue부터 시작하며, 중단되거나 무효화된 다운로드 응답은 반영하지 않는다. GO/STOP/RESET은 HTTP를 호출하지 않는다.
- **P1** · `ETag`/304 최적화, 검증된 `.bak` 및 손상 파일에서 백업 복구, 오래된 Revision·동일 Revision의 다른 본문 감지를 추가한다.

## GO / STOP / RESET 상태 전환

| 현재 상태 | 명령·결과 | 다음 상태 |
|---|---|---|
| No List 또는 Standby | REFRESH → 유효 목록 저장 | Standby. CURRENT = Standby, NEXT = 첫 Cue 또는 비어 있음 |
| Standby / Ready | GO → NEXT를 고정하고 RunId 할당 | Running. 실행 중 Cue 표시. CURRENT와 NEXT 커서는 유지 |
| Running | 같은 RunId에서 Runtime 완료 | Ready 또는 Finished. 완료 Cue가 CURRENT, 다음 배열 요소가 NEXT |
| Running | 준비 전 거절 / 적용 도중 실패 | 준비 전에는 이전 상태와 커서를 유지. 일부 적용됐으면 Stopped, 커서 보존과 원인 표시 |
| Running / Ready / Standby / Finished | STOP → 추가 GO 잠금 및 활성 전환 취소 | Stopped. CURRENT는 마지막 완료 Cue, 중단 Cue는 별도 표시 |
| Stopped 또는 다른 상태 | RESET → 실행 무효화, Standby 강제 적용 성공 | 유효 목록이 있으면 Standby, 없으면 No List. 실패 시 Stopped |

P0 STOP은 확인 없이 GO를 잠그고 RunId를 무효화한 뒤 기존 카메라 Timeline과 해당 명령의 타이머를 중단한다. 카메라는 중단 시점에 머물고 이미 적용된 조명·Animation·Appearance는 유지된다. 늦은 완료는 무시한다. RESET은 확인 후 실행을 무효화하고 Stage View·활성 Montage·음악을 종료한다. 현재 Shader Mode를 유지하며 해당 DataTable의 `Preset_01`과 Idle / Full / Neutral, 기존 Standby 위치·입력 기준을 적용한다. 성공 후 첫 NEXT로 돌아간다. P1은 카메라와 조명이 동시에 보간 중일 때 현재 표시값 고정, 여러 콜백의 완료 합산 및 월드 전환·연속 취소 정리를 다룬다.

## UI 상태와 조작

| 영역 | P0 동작 |
|---|---|
| 웹 Show 선택·Cue List | Show/Revision/저장 상태를 위에 두고 선택한 행의 번호와 이름을 Inspector에도 표시한다. Cue 이동은 위/아래 버튼으로 제공하고 끝 위치에서는 해당 버튼을 비활성화한다. 이동 후 선택·포커스를 유지한다. |
| 웹 Cue Inspector | 필드마다 레이블을 표시한다. 잘못된 이름·중복 번호·범위를 필드 옆과 오류 요약에서 설명한다. 오류에서 필드로 이동하며 저장 실패 시 초안을 유지한다. 저장 전 다른 Cue나 Show로 이동할 때 저장·버리기·취소를 제공한다. 복제는 새 ID와 고유 번호를 받는 초안으로 시작하며, 삭제는 Show/Cue 이름을 명시한 확인에서 실행한다. 시간 값은 저장할 수 있고 P1 재생 항목으로 표시한다. |
| Unreal Cue Control | 기존 Operator에 Show 이름, Revision, 마지막 성공 REFRESH, CURRENT/NEXT를 표시하는 패널을 추가한다. Next Cue Preview는 Animation·Camera·Lighting·Appearance를 표시한다. 전체 Cue 목록은 읽기 전용이며 완료·다음·대기와 완료 수를 표시한다. Shader Mode는 Operator의 독립 수동 제어에 표시한다. |
| Unreal 상태와 명령 | 서버 상태는 미확인·조회 성공·조회 실패, 로컬 상태는 실행 가능·목록 없음·실행 잠금으로 독립 표시한다. 조작 영역에 GO/STOP/RESET/REFRESH를 고정한다. 잠금 이유를 텍스트로 표시하고 STOP은 확인 창 없이 실행한다. RESET 확인에는 대상 Show, 복원 값, 유지할 현재 모드를 표시한다. 기본 포커스는 취소에 두고 확인 중에도 STOP에 접근할 수 있게 한다. |
| MANUAL OVERRIDE | 실제 Animation·Camera·Lighting·Appearance Preset ID 또는 Stage View를 마지막 완료 Cue 기준과 비교한다. 완료 Cue 이전에는 Standby를 기준으로 삼는다. Shader Mode와 모드별 머티리얼 값은 판정에서 제외하며 모드 전환 자체로 표시가 켜지거나 해제되지 않는다. CURRENT/NEXT와 완료 수는 유지한다. 수동으로 기준 상태에 맞추거나 GO·RESET이 성공하면 해제한다. |

키보드 입력과 명시적인 포커스를 지원한다. 웹 저장 상태는 보조 기술이 읽을 수 있는 상태 영역을 사용한다. Unreal에서는 UMG 접근성 레이블과 실제 키보드 포커스 이동을 검사한다. 색상과 함께 텍스트·아이콘을 표시한다. 일반 텍스트 4.5:1, 큰 텍스트와 컨트롤 경계·포커스 3:1 대비를 목표로 검증한다.

## Shader Mode와 Toon Appearance

Operator가 ORIGINAL / TOON을 수동 선택하고 기존 `BP_BountyHunter`가 모드를 소유한다. GO, RESET, 수동 프리셋 선택은 현재 모드의 DataTable에서 요청한 ID를 조회한다. `SK_StylizedBountyHunter` 메인 Mesh는 두 테스트 Actor에서 확인되었다. `BodyModular`가 슬롯 5와 27에 중복되고 Toon의 슬롯 6–11은 `M_Vacant`를 사용한다. 모드별 매핑을 컴포넌트·슬롯 인덱스와 기대 슬롯명으로 저장한다. Original의 초기 Material Interface, 보조 컴포넌트 표시 상태 및 Override 유무를 MID 생성 전에 확보한다.

1. 수동 모드 전환 시 대상 모드에서 현재 프리셋 ID의 행과 필요한 MI·컴포넌트를 확인한다. 필수 에셋·슬롯 불일치가 있으면 기존 모드를 보존한다.
2. 메인 Mesh와 보조 Hair/Goggle/Outline의 Material 및 표시 상태를 적용한다. Toon은 `Mesh_Outline`의 Leader Pose와 `/Game/ToonShader/Materials/MPC_Toon`의 `HeadForward`, `HeadRight`, `HeadUp` 갱신을 연결한다. Original 복귀 시 Toon 경로를 끄고 초기 구성을 복원한다.
3. `CachMID`의 Original 슬롯 매핑과 기존 setter를 보존한다. Toon에서는 실제 보이는 컴포넌트의 유효 MI에서 MID를 만들고 캐시를 교체한다. 동일 모드에서 Appearance만 바뀌면 기존 MID를 재사용한다.
4. 모드별 DataTable에서 같은 프리셋 ID의 행을 적용한 뒤 결과를 게시한다. 필수 적용이 도중에 실패하면 이전 모드·MID와 외형을 복원하고, 복원 실패 시 추가 GO를 차단한다.

Appearance Preset은 ORIGINAL에서 `/Game/Blueprints/Data/DataTable/DT_AppearancePresetData`, TOON에서 `/Game/Blueprints/Data/DataTable/DT_AppearancePresetData_Toon`을 사용한다. 두 테이블의 행 이름은 `Preset_01`–`Preset_05`로 대응하며 행 값은 Editor에서 모드별로 튜닝한다. Toon 전용 행 Struct가 필요하면 `/Game/Blueprints/Data/Struct/ST_ToonAppearancePresetData`를 검토한다. 원본 테이블과 `ST_AppearancePresetData`를 그대로 보존한다.

| 영역 | 기존 값과 Toon 규칙 |
|---|---|
| 피부 | 기존 `SkinHue_Global`·`SkinDepth_Global` → 확인된 Toon `SkinHue`·`SkinDepth`. 대상은 실제 Face/Body MID이며 숫자 범위와 보정은 Editor에서 확정한다. |
| 머리·눈썹·눈·입술 | 행에 `Hair_Color`, `Eyebrows_Color`, `IrisColor`, `Lips_Color01`·`02`에 대응하는 선형 색 값을 둔다. 이름과 타입이 일치해도 화면에 기여하는 MID·정적 스위치를 확인한 후 연결한다. |
| 의상 | 기존 `ColorHue_Global`, `Color02_Hue`·`Color02_Depth`는 호환을 가정하지 않는다. 행에는 승인된 컴포넌트별 의상 값만 선택적으로 두고, 조사한 `MI_Dress_Toon`의 미매핑 항목은 기본값 유지로 시작한다. |

필수 행·에셋·파라미터가 없거나 타입이 다르면 사전 검사에서 명령을 거절한다. 선택 파라미터는 해당 MI의 기본값으로 유지 또는 복원하고 모드·슬롯·이름을 진단에 표시한다. 이전 프리셋의 선택 값이 남지 않도록 생략 대상도 초기화한다. 파라미터 존재 확인은 UE 5.8에서 열거 가능한 이름·타입을 기준으로 하며, 에디터에서만 확인 가능한 표현 차이는 수동 검증한다.

## C++ 전환과 검증

현재 Blueprint-only다. 후속 구현에서는 자산과 현재 변경을 보존한 격리 사본에서 UE 5.8 C++ Class Wizard로 첫 런타임 클래스를 추가하거나 동등한 모듈을 생성한다. `.uproject` 모듈 선언, `Source`, 게임·Editor Target, Build.cs가 생긴다. 호환 Visual Studio·MSVC·Windows SDK를 확인하고 Development Editor를 콜드 빌드한다. `HTTP`·`Json`을 링크하고, DTO 매핑에 쓸 경우 `JsonUtilities`를 추가한다. GameMode 및 Blueprint 부모를 유지하고, Editor 재오픈 후 Studio PIE와 Standalone에서 초기화 및 Toon DataTable 로딩을 확인한다. 패키지 빌드와 쿠킹 검증은 시연 이후로 둔다. [Epic C++ Class Wizard](https://dev.epicgames.com/documentation/unreal-engine/using-the-cplusplus-class-wizard-in-unreal-engine) · [Epic FHttpModule](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/HTTP/FHttpModule?application_version=5.8)

| 검증 항목 | 통과 기준 |
|---|---|
| API와 웹 | CRUD·순서·동시 수정·서버 재시작. v1 DB 변환 후 같은 Show·Cue·Revision 이력과 순서가 유지되고 다운로드는 v2로 제공된다. 변환 및 편집 실패 시 전체 트랜잭션이 롤백된다. |
| 캐시 | P0에서 잘못된 JSON·스키마·Show ID 및 저장 실패 뒤 정상본 보존을 검증한다. P1에서 본 파일 손상과 백업 복구, 오래된 Revision을 검증한다. |
| Studio | `/Game/Maps/Studio`에서 먼저 기존 GameMode/Pawn/매니저와 Operator 초기화를 PIE로 확인한다. 복제한 시연 레벨에서 수동 조작과 Cue의 실제 결과를 비교하고 Standalone도 확인한다. MCP는 경로와 그래프를 재확인하고 런타임 검증은 화면·로그·서버 요청 기록으로 남긴다. |
| 오프라인 GO | REFRESH 후 서버를 종료하고 세 Cue를 실행한다. 각 모드에서 네 가지 Cue 상태, 현재 모드 유지, 해당 DataTable 행, Camera Cue의 Stage View 해제와 HTTP 0건을 확인한다. |
| 셰이더 회귀 | 수동 Original → Toon → Original 왕복에서 다섯 프리셋 ID, Outline/SDF, 헤어·고글, 두 Body 슬롯, 조명과 프리셋 적용을 비교한다. 미지원 파라미터 테스트에서 기본값과 진단을 확인한다. |
| 전환 중단 | P0은 기존 카메라 전환 도중 STOP, 중복 GO, 늦은 완료 무시를 확인한다. ORIGINAL과 TOON에서 각각 RESET을 실행해 모드 유지, 해당 DataTable의 `Preset_01`, Idle / Full / Neutral, 첫 NEXT를 확인한다. P1에서는 카메라·조명 동시 보간 취소를 확인한다. |
| UI와 수동 조작 | 복제 초안, 삭제/RESET 확인, 저장 실패, 포커스·대비, 서버 실패와 로컬 준비 동시 표시를 확인한다. 수동 모드 전환 시 MANUAL OVERRIDE가 유지되고 수동 프리셋 변경 시 ID 차이에 따라 판정되는지 확인한다. |

## 시연 전 미검증 항목

Studio GameMode, Blueprint 호출 타깃, Toon 보조 Mesh 및 승인 가능한 의상 매핑, RESET의 기존 위치·입력 연결을 PIE와 Standalone에서 확인한다. P1의 보간 가시성 정책과 파일 복구 테스트는 발표 범위 밖에서 확정한다.

관련 문서 [PRD](PRD.md) · [구현 계획](ImplementationPlan.md)
