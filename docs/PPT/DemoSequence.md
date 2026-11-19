# 최종 시연 순서

본편 15번 / 기본 시연 75초 / 첫 프로젝트 운용 기반, Toon 결과, Web Cue 연동 순서

통합 시연 영상은 첨부되지 않아 PPT에는 실제 Screenshot과 라이브 시연 안내를 배치

## 75초 진행

| 시간 | 화면과 조작 | 설명 | 구분할 점 |
|---|---|---|---|
| 0~10초 | Studio에서 같은 Idle / Upper / Neutral 상태로 Original → Toon | Material, Texture와 Hair Mesh가 바뀐 결과 | 전환 사이 Camera를 움직이지 않음 / 모든 노출 수치가 같다는 추가 주장은 하지 않음 |
| 10~25초 | Close 또는 Upper로 얼굴 확인 / 기존 Camera 조작으로 헤어 시점 변화 확인 | 얼굴 명암 형태와 헤어 Highlight | Camera 변화는 헤어 시점 반응용 / 얼굴 SDF 광원 방향 변화는 본편 7번의 캡처와 구분 |
| 25~40초 | Full / Appearance Preset 1회 / Lighting 1회 변경 | 기존 운용 UI와 Toon의 연결 | 실제 사용 상태만 조작 / Cue가 Toon 모드까지 바꾼다고 설명하지 않음 |
| 40~50초 | Web Cue Editor에서 저장할 Cue의 네 필드 확인, 필요하면 저장 | Animation / Camera / Lighting / Appearance 참조 | DB의 실제 Cue를 사용 / 예시 이름과 값이 현재 DB에 있다고 가정하지 않음 |
| 50~60초 | Unreal Show 선택 / 이미 선택한 Show를 편집했다면 REFRESH | Widget의 조회 요청과 로컬 목록 갱신 | Show 선택 뒤 HTTP 조회가 자동 요청됨 / REFRESH가 매번 필수인 단계는 아님 |
| 60~75초 | GO로 준비한 Cue 2개 실행 / RESET | 로컬 Cue가 기존 Studio 상태 함수에 연결되는 결과 | GO 자체는 서버 호출이 아님 / RESET은 위치와 Original/Toon 모드까지 전부 초기화하지 않음 |

얼굴 광원을 직접 회전하는 조작은 현재 확보된 UI에 있다고 가정하지 않음 / 사용 가능한 기존 조작이 있을 때만 추가

## 시연 전 준비

1. 발표에 쓸 프로젝트 버전, 서버 주소, `CUE_DB_PATH`와 실제 Show 확인
2. Toon 모드를 직접 선택 / Cue 데이터에는 `shaderMode`가 없음
3. 처음 Play에서 Show 미선택 상태 확인 / 기존 사용자 정상 실행 확인을 새로 미완료로 취급하지 않음
4. 실제 Show에 있는 Cue 중 화면 변화가 분명한 항목 선정 / 현재 허용값 안에서 구성
5. Cue별 시간 필드는 실행과 별개이므로 설명에서 제외 / `cameraBlendMs`, `lightingFadeMs`를 동작 성과로 표시하지 않음
6. Show 선택과 REFRESH로 조회가 끝난 뒤 GO 가능 상태 확인
7. 같은 버전의 녹화본을 준비할 수 있으면 연결 문제 시 대체 / 현재 결과물에 미확보 영상을 삽입했다고 표시하지 않음

## 추가 질문용 동작

| 항목 | 조작과 정확한 설명 |
|---|---|
| Stage Camera | Stage Camera Cue 실행 → `ApplyCueCameraByName`의 `SetStageViewOn` 분기 / Lighting Stage와 별도 |
| END SHOW | 마지막 Cue가 실행 중일 때 같은 GO 버튼의 라벨이 END SHOW로 바뀜 / 클릭 시 `CompleteCurrentCue → ResetAllState(false)` |
| RESET | `ResetCueProgress → ResetAllState(false)` / 다음 Cue를 첫 항목 대기 상태로 초기화 |
| 위치와 표현 모드 | 위 두 경로의 `ResetPosition=false`에서는 위치 복귀와 Original 모드 전환까지 포함하지 않음 |
| 캐시 | 선택 Show의 유효한 로컬 Snapshot을 사용하는 데이터 계층 / 전체 공연의 무중단 보장은 별도 |

## 운영 대안

- 웹 연결이 불안정하면 준비해 둔 유효 Snapshot 상태에서 이미 확인한 동작만 시연
- 연결 실패, 잘못된 JSON과 캐시 오류를 즉석에서 유발하는 시연은 기본 75초에 포함하지 않음
- END SHOW까지 진행하면 시간이 길어질 수 있으므로 기본 시연은 Cue 2개와 RESET을 사용 / 마지막 Cue 동작은 질문 시 추가

근거: `PresentationPlan_Verified.md` §0.4, §12, K3/K4, U1/U2 / 본 문서는 조작 안내이며 Unreal 프로젝트나 DB를 수정하지 않음
