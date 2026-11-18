# Virtual Performer Studio · Cue 시스템 SDD

UE 5.8 · 최종 요구사항 반영 · 2026년 11월 18일 KST

## 책임과 이번 구현 경계

Director는 Web Cue Manager에서 방송별 Show와 Cue를 편집하고 Operator는 Unreal에서 실제 연출을 실행한다. 한 사람이 두 역할을 맡을 수 있다. 웹은 Editor 없는 편집, 다른 기기 접근, Show/Revision 관리와 재빌드 없는 방송 구성 변경을 위해 사용한다. 전체 상용 방송 자동화 제품 구현이 목적은 아니다.

```text
Director Browser → FastAPI / SQLite → HTTP REFRESH → 검증·영구 저장
                                                        ↓
Operator → Local Snapshot → Cue Player → 기존 Blueprint / DataTable
```

이번 C++ 모듈은 조회·검증·스냅샷·통신 상태·Blueprint API만 소유한다. 실제 GO/STOP/RESET과 UI wiring은 다음 단계다. 자동 다운로드·폴링·자동 연출은 없다. GameInstanceSubsystem은 기존 GameInstance/Pawn/GameMode 부모 변경 없이 자동 생성된다.

## 기존 구현 보존

WBP_Operator의 Cue Control과 WBP_CueItem, E_CueItemState, ST_CueItemStyle, DT_CueItemStyle을 그대로 사용한다. 신규 Bridge Blueprint, 별도 Cue Control Widget, 복제 Studio Map 방식은 폐기한다. 기존 에셋 수정·재부모화·저장·컴파일을 수행하지 않는다. 사용자가 이후 Blueprint를 연결한다.

BP_StudioManager의 모드 전환·Appearance 적용과 Original/Toon DataTable을 재사용한다. Cue는 세부 렌더링 값 대신 Animation/Camera/Lighting/Appearance 참조를 저장한다. Shader Mode는 Cue v2에 없고 Operator 독립 제어다. GO/RESET은 현재 모드의 같은 프리셋 ID를 적용한다. ResetAllState는 모드까지 변경하므로 Cue RESET으로 재사용하지 않는다.

RankingSubsystem(실습 WebAPIUnreal)의 UGameInstanceSubsystem, FHttpModule, BindUObject 비동기 콜백, 결과 delegate 방식을 참고한다. Ranking DTO·URL·CRUD와 ASP.NET/SQL Server 의존성은 가져오지 않는다.

## 기존 서버 계약

기본 주소는 http://127.0.0.1:8001, MCP는 127.0.0.1:8000이다. 이번 작업은 CueServer 코드·DB 구조·기존 Show/Cue를 변경하지 않는다. demo.sqlite3으로 전환하거나 기존 방송 순서를 번호순으로 재작성하지 않는다.

| API | 용도 |
|---|---|
| GET /api/v1/shows | Show 배열. id, name, revision, createdAt, updatedAt |
| GET /api/v1/shows/{showId}/cue-list | 아래 v2 스냅샷. HTTP 200만 성공으로 처리 |
| 기존 POST/PATCH/DELETE Show/Cue, PUT cue-order | Director 편집. Unreal 이번 단계에서 호출하지 않음 |

오류는 code/message/fieldErrors 구조다. 404, 409, 422, 503 등 비-200 응답은 성공 목록으로 파싱하지 않는다. 응답은 HTTP 수신 스트림에서 4 MiB 초과 전에 중단하며 요청 타임아웃은 설정한다. Camera Blend와 Lighting Fade는 0–30000ms 데이터를 유지하되 지정 시간 재생은 후순위다.

## 보존하는 서버 API와 SQLite 계약

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


## JSON Schema v2

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

필드명과 Enum은 대소문자를 구분한다. 추가 필드·필수 필드 누락·null·bool을 숫자로 해석하지 않는다. 이름 길이는 UTF-16 단위가 아닌 Unicode 코드 포인트로 센다. 타임스탬프는 전체 RFC3339 형식과 UTC 오프셋을 직접 검증하며 엔진의 느슨한 날짜 파서에 맡기지 않는다. 정수는 유한한 값이며 소수/문자열을 암묵 변환하지 않는다. UUID와 ISO 8601 날짜를 검증한다. 이름은 공백만으로 구성될 수 없다. 요청한 Show ID와 응답 Show ID가 일치해야 한다.

position은 배열에서 1부터 연속이고 Cue ID·Number는 유일해야 한다. Number로 정렬하지 않는다. 빈 Cue 배열은 유효한 스냅샷이지만 실행할 Cue는 없다. 전체 파싱이 성공하기 전에는 출력 구조체를 교체하지 않는다.

## C++ 구성과 Blueprint 계약

NetworkProject Runtime Module은 Core/CoreUObject/Engine/DeveloperSettings/HTTP/Json을 사용한다. JsonUtilities와 외부 런타임 의존성은 추가하지 않는다. 엔진 5.8의 Target 설정으로 Win64 Development Editor를 콜드 빌드한다. 기존 플러그인·맵·게임 클래스 설정을 유지한다.

| 공개 형식 | 역할 |
|---|---|
| ECueRequestState | NotRequested / Requesting / Succeeded / Failed. 기존 E_CueItemState와 별개인 통신 상태 |
| FCueShowInfo | ID·이름·Revision·생성/수정 UTC |
| FCueData | Cue ID·Position·Number·Name·네 상태 FName 참조·두 ms 정수 |
| FCueSnapshot | 스키마·Show ID/이름·Revision·발행 UTC·Cue 배열 |
| FCuePlayerStatus | 선택 Show, 두 요청 상태/오류, 캐시 오류, 스냅샷 존재/로컬 데이터 사용 가능, 마지막 성공 REFRESH UTC |
| UCuePlayerSettings | Config=Game, Project Settings > Game > Cue Player. Base URL과 타임아웃 |
| UCuePlayerSubsystem | RequestShows, SelectShow, RefreshCueList, RestoreSelectedSnapshot, CancelRequests, GetSnapshot/GetShows/GetStatus/GetServerBaseUrl |

네 상태는 기존 Blueprint Enum의 정수값을 가정하지 않는 FName이다. 이후 사용자가 기존 Enum/함수에 명시적으로 매핑한다. Pending/Next/Running/Applied/Stopped나 재생 커서는 이 모듈에서 구현하지 않는다. 정확한 노드·핀·delegate는 [Blueprint 연결 안내](BlueprintIntegration.md)에 기록한다.

## 비동기 수명과 상태

Show 목록 요청과 Cue List 요청은 독립 요청 세대 번호를 갖는다. 같은 종류의 새 요청은 이전 요청을 취소·무효화한다. Show 선택 변경, 취소, Deinitialize에서도 Cue 요청을 무효화한다. 콜백은 세대 번호·실제 요청 포인터·선택 Show·설정 주소가 모두 일치할 때만 반영한다. UObject 수명 바인딩과 게임 스레드 콜백을 사용한다.

REFRESH는 HTTP 200 → 전체 JSON 검증 → 디스크 커밋 → 활성 메모리 교체 순서다. 저장 전 UI에 성공 Snapshot을 알리지 않는다. 실패는 마지막 통신 결과만 갱신하고 기존 유효 Snapshot·성공 시각은 유지한다. Show 목록 실패도 기존 목록을 보존한다. 취소되거나 무효화된 응답에는 완료 성공 이벤트를 내보내지 않는다.

서버 통신 실패와 로컬 데이터 사용 가능은 독립이다. bCanUseLocalSnapshot은 유효한 비어 있지 않은 데이터 존재를 뜻하며 GO 실행 허가가 아니다. 실행 중 잠금과 진행 후 REFRESH 정책은 실제 Cue Player 연결 단계에서 구현한다.

## 영구 캐시와 복원

Saved/CuePlayer/<검증한 UUID>.json에 cacheVersion, lastSuccessfulRefreshUtc, snapshot을 가진 로컬 envelope를 저장한다. snapshot은 서버 v2 본문이며 캐시 메타데이터를 서버 Schema에 추가하지 않는다. 선택 ID는 같은 디렉터리 selection.json에 저장한다. 경로에 임의 문자열을 넣지 않는다.

정상본을 삭제한 뒤 덮어쓰지 않는다. 같은 디렉터리의 고유 임시 파일에 UTF-8 기록·flush 후 원자적 교체한다. 실패 시 임시 파일만 정리하고 기존 파일·메모리·성공 시각을 유지한다. 로딩 때 envelope·v2·Show ID를 다시 검증한다. 검증된 선택 ID와 스냅샷을 시작 시 복원하며 HTTP나 실제 연출은 하지 않는다.

다른 Show를 선택했는데 캐시가 없거나 손상됐으면 해당 Show는 스냅샷 없음이다. 이전 Show를 새 Show 데이터처럼 노출하지 않는다. 이전 Show 캐시는 디스크에 남는다. 고급 .bak 복구, ETag/304, 오래된 서버 Revision/같은 Revision 본문 충돌 판정은 후순위다. 비동기 요청의 오래됨과 서버 Revision의 오래됨을 구분한다.

## 다음 단계의 로컬 실행

GO는 position 기준 NEXT를 고정하고 네 연출 참조를 기존 Blueprint에 적용한다. 실제 성공 후 커서를 갱신한다. STOP은 진행 중 전환과 타이머를 취소하고 시각을 유지하며 RunId로 늦은 완료를 무시한다. RESET은 Idle/Full/Neutral/Preset_01, 현재 Shader Mode 유지, 첫 NEXT 복귀다. 세 조작은 HTTP를 호출하지 않는다.

Stage View 선행 해제, 이동·제스처 제한, 카메라 Timeline 완료·중단, 동일 조명 강제 재적용, 입력/음악 정리, MANUAL OVERRIDE는 실제 연결 시 검증한다. 이 항목들은 이번 데이터 모듈의 구현 완료로 표시하지 않는다.

## 검증과 운영

파서: 공용 예제·빈 Show·Number와 Position 독립·잘못된 타입/범위/Enum/중복/순서/Show ID. 캐시: 저장/복원·선택 복원·불량 본문/저장 실패 후 보존. HTTP: 정상·비-200·불량 JSON·지연 응답·선택 변경·취소 후 무시. 테스트는 임시 캐시와 별도 loopback fixture 서버를 사용하고 기존 DB에 쓰지 않는다.

첫 빌드는 사용자가 Save All 후 Editor를 직접 종료한 뒤 실행한다. UHT와 Development Editor Win64 빌드 및 native automation 결과를 보고한다. 실제 WBP wiring·Studio PIE/Standalone 연출·송출·패키징은 별도 검증이다. 서버와 UI 기능을 구현했다는 것만으로 전체 방송 시연 성공을 주장하지 않는다.

관련 문서 [PRD](PRD.md) · [구현 계획](ImplementationPlan.md) · [Blueprint 연결 안내](BlueprintIntegration.md)
