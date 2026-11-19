# 예상 기술 질문 및 답변

슬라이드 번호는 최종 본편 1~16, 부록 A1~A8 기준 / 답변은 검증 문서의 근거 수준 유지

## 01. 첫 프로젝트와 이번 프로젝트의 차이는?

첫 프로젝트는 캐릭터와 무대의 Animation, Camera, Lighting, Appearance를 운영하는 Studio입니다. 이번에는 Toon 표현을 위해 Material, Texture, Hair Mesh, UV와 Normal을 수정하고 Web Cue 데이터 계층을 추가했습니다.

보여줄 슬라이드: 3, 4 / 근거: `PresentationPlan_Verified.md` §3.1, T13,T20

## 02. 왜 공용 M_Toon을 사용했나?

명암과 색 합성은 공유하면서 얼굴, 헤어와 눈의 다른 계산을 부위별 MI에서 선택하도록 구성했습니다. Draw Call이나 GPU 비용 감소는 측정하지 않았습니다.

보여줄 슬라이드: 5, A1 / 근거: `PresentationPlan_Verified.md` T01,T02

## 03. Unlit Material인가?

본체 M_Toon은 Surface / Opaque / Default Lit / 단면입니다. Base Color 입력은 0이고 최종 색은 Emissive 중심입니다. M_Outline과 M_Eyelashes는 별도 Unlit Material입니다.

보여줄 슬라이드: 5, A1 / 근거: `PresentationPlan_Verified.md` T02 / S2 §0.2, §0.7

## 04. 일반 Toon 명암의 계산은?

q=(dot(PixelNormalWS,-MPC_Toon.LightDir.rgb)+1)/2를 만든 뒤 ShadowRange±ShadowSmooth 구간의 SmoothStep을 적용합니다. 출력 1은 밝은 기본색, 0은 그림자색입니다. 전체 전환 구간 폭은 2×ShadowSmooth입니다.

보여줄 슬라이드: 6 / 근거: `PresentationPlan_Verified.md` T03

## 05. 정확한 3단계 또는 5단계 Ramp인가?

현재 근거는 일반 명암과 Highlight의 경계 계산입니다. 5개 Appearance Preset은 외형 배색이며 5단계 명암 구현으로 설명하지 않습니다.

보여줄 슬라이드: 6, 12 / 근거: `PresentationPlan_Verified.md` T03 / §6 슬라이드02 검수 조건

## 06. SDF는 직접 만든 알고리즘인가?

그림자 형태와 조건을 지정하고 AI 보조 자료를 활용했습니다. 적용 계산은 학습 자료를 참고해 구성했습니다. 엄밀한 거리 변환 생성기를 독자적으로 구현한 것은 아닙니다.

보여줄 슬라이드: 7, A2, A7 / 근거: `PresentationPlan_Verified.md` T04 / §3.5

## 07. SDF 좌우 선택과 Threshold는 어떻게 다른가?

HeadRight.xy와 LightDirection.xy의 내적으로 좌우 SDF를 보간하고, HeadForward.xy 내적을 remap 후 반전해 Threshold를 계산합니다. 선택 Texture에 Power 0.8을 적용한 뒤 SDF_Smooth 폭으로 SmoothStep을 수행합니다.

보여줄 슬라이드: A2 / 근거: `PresentationPlan_Verified.md` T04

## 08. FaceShadowMask와 DefaultShadow의 순서는?

FaceLit=Lerp(GeneralLit,SDFLit,T_FaceShadowMask.R) 이후 (1-T_DefaultShadow.R)을 곱합니다. 고정 그림자가 FaceShadowMask 안쪽으로만 제한되는 구조라고 설명하면 부정확합니다.

보여줄 슬라이드: 7, A2 / 근거: `PresentationPlan_Verified.md` T05

## 09. 광원 각도가 실제 Actor 수치와 동일한가?

슬라이드 7의 각도는 제공된 캡처 파일의 표기 기준입니다. 이번 발표 제작에서 실제 Actor 회전과 LightDir writer의 수치를 새로 측정한 것은 아닙니다.

보여줄 슬라이드: 7, A8 / 근거: `PresentationPlan_Verified.md` §0.5 / 제작 노트

## 10. 얼굴 상하 광원도 처리하나?

현재 확인 경로는 XY 비교입니다. HeadUp 쓰기 노드는 있어도 MPC 정의와 Material 소비 노드가 없어 상하 보정 완료로 설명할 수 없습니다.

보여줄 슬라이드: A2, A8 / 근거: `PresentationPlan_Verified.md` T04,T14 / S2 §0.6

## 11. 실제 그림자를 수신하나?

방향 기반 명암과 실제 광원 차폐는 구분합니다. 벽이나 앞머리에 의한 차폐, 모든 광원의 수신까지 확인한 자료는 부족합니다.

보여줄 슬라이드: 6, A8 / 근거: `PresentationPlan_Verified.md` T03,T19

## 12. 코 Highlight도 그림자에서 사라지나?

좌우 코 Mask 선택과 Highlight 합성 경로는 있지만 DrawShadow로 차단하는 연결은 현재 출력 경로에 없습니다.

보여줄 슬라이드: A2 / 근거: `PresentationPlan_Verified.md` T06

## 13. 헤어는 전부 직접 모델링했나?

초기 Mesh는 Tripo 결과를 사용했습니다. Maya 덩어리 분리, UV 전개와 방향 정리, Vertex Normal 수정은 직접 작업 기록입니다.

보여줄 슬라이드: 9, A7 / 근거: `PresentationPlan_Verified.md` T09 / §3.5

## 14. UV와 Soften Edge는 어떤 차이인가?

UV는 Texture 위치와 방향 기준에 관계하고 Soften Edge는 인접 면의 Vertex Normal에 영향을 주는 작업입니다. 앞머리 삼각형 명암이 Soften Edge로 해결됐다는 기록이 있지만 최초 Import 원인을 새로 확정한 것은 아닙니다.

보여줄 슬라이드: 9 / 근거: `PresentationPlan_Verified.md` T09

## 15. 헤어 Highlight 수식은?

B=normalize(cross(VertexNormalWS,VertexTangentWS)), H=normalize(-LightDir+CameraVectorWS), m=pow(1-abs(dot(B,H)),HairSpecularPower)입니다. Cross 결과는 원본 Tangent 자체가 아니라 별도 방향축입니다.

보여줄 슬라이드: 10 / 근거: `PresentationPlan_Verified.md` T10

## 16. Hair Value와 방향성 Highlight는 같은 계산인가?

기본색은 Hair_Color×T_HairValue.R입니다. 방향성 마스크는 별도로 Lerp_3.Alpha에 연결됩니다. DarkStrength/LightStrength 보정 Lerp는 최종 출력에 도달하지 않습니다.

보여줄 슬라이드: 10, A1 / 근거: `PresentationPlan_Verified.md` T10,T11

## 17. 눈은 새로 만든 것인가?

기존 UV, 홍채와 동공 계산을 재사용하고 Toon Iris/Sclera와 색상 계산을 통합했습니다. 알고리즘 전체를 새로 작성한 것이 아니며 저장 MI 참조와 실제 실행 Eye Slot은 구분합니다.

보여줄 슬라이드: 8, A3 / 근거: `PresentationPlan_Verified.md` T07,T08

## 18. Outline은 Post Process인가?

확인한 M_Outline은 VertexNormalWS 기반 WPO, 거리 보정과 TwoSidedSign Mask를 쓰는 별도 Material입니다. 실제 추가 Mesh와 애니메이션 추종의 확인 범위는 별도입니다.

보여줄 슬라이드: 11, A1, A8 / 근거: `PresentationPlan_Verified.md` T12

## 19. Rim은 역광인가?

Fresnel과 Step으로 만든 시점 기반 마스크입니다. BaseColorPower 적용 후 색 C에 대해 Lerp(C,C×RimLightColor,DrawRimLight)를 사용합니다. 실제 뒤쪽 광원과 차폐를 계산하는 조명은 아닙니다.

보여줄 슬라이드: 11 / 근거: `PresentationPlan_Verified.md` T12

## 20. Original / Toon 전환은 Static Switch를 바꾸나?

확인한 Runtime 경로는 모드별 Material 교체와 MID 생성입니다. Static Switch는 부위별 MI 변형 선택이며 실시간 Scalar 변경과 구분합니다.

보여줄 슬라이드: 12, A3 / 근거: `PresentationPlan_Verified.md` T14

## 21. 웹에서 실시간 원격 조작하나?

웹은 Cue 편집과 저장을 담당합니다. Unreal에서 Show 선택 후 조회하거나 REFRESH로 다시 가져옵니다. GO는 WBP_Operator가 로컬 Snapshot을 기존 Studio에 적용하는 조작입니다. WebSocket이나 RPC/Replication 구조로 설명하지 않습니다.

보여줄 슬라이드: 13, 14 / 근거: `PresentationPlan_Verified.md` T15,T16,T18

## 22. SelectShow()가 HTTP를 요청하나?

C++ SelectShow 자체는 선택 Show의 캐시를 처리합니다. 현재 Widget은 선택 성공 후 RequestCueRefresh→RefreshCueList를 호출해 HTTP 조회를 요청합니다.

보여줄 슬라이드: 14 / 근거: `PresentationPlan_Verified.md` §0.4 / K3,K4

## 23. Subsystem이 Cue 진행도 소유하나?

C++은 HTTP, JSON 검증, 캐시와 통지를 담당합니다. CurrentCueIndex, NextCueIndex와 bCueRunning, Resolver와 GO/종료 흐름은 WBP_Operator가 관리합니다.

보여줄 슬라이드: 14, A4 / 근거: `PresentationPlan_Verified.md` T16,T18

## 24. END SHOW와 RESET은 같은가?

END SHOW는 마지막 Cue 실행 중 GO 버튼의 다른 라벨로, CompleteCurrentCue와 Studio 초기값 적용을 호출합니다. RESET은 ResetCueProgress로 진행을 첫 Cue 대기 상태로 돌립니다. 두 경로 모두 ResetAllState(false)를 호출하며 위치와 Original/Toon 모드까지 초기화하지 않습니다.

보여줄 슬라이드: A4 / 근거: `PresentationPlan_Verified.md` §0.4 / K4

## 25. Camera Blend / Lighting Fade는 구현됐나?

v2 시간 필드는 검증하고 보존하지만 Cue 지정 시간에 따른 전환 실행은 현재 범위에서 제외했습니다. 기존 Camera Timeline 전환과 혼동하지 않습니다.

보여줄 슬라이드: A4, A8 / 근거: `PresentationPlan_Verified.md` T20

## 26. Stage Cue 오류는 어떻게 수정했나?

서버 Camera 허용값에는 Stage가 있었지만 C++ 목록에는 누락되어 목록 검증이 실패했습니다. 허용 목록을 맞추었고 보존된 RED/GREEN 기록으로 확인했습니다. 현재 코드에도 Stage가 있습니다.

보여줄 슬라이드: A5 / 근거: `PresentationPlan_Verified.md` T17

## 27. 테스트 결과는 어느 범위인가?

보존 기록에서 Unreal 7개 성공 중 경고 포함 1개, 서버 37개 성공과 경고 1개입니다. 자동 테스트는 데이터 계층을, GO/END SHOW/RESET/Stage는 사용자 실행 확인을 근거로 합니다. 이번 PPT 제작에서 테스트를 다시 실행한 것은 아닙니다.

보여줄 슬라이드: A5, A6 / 근거: `PresentationPlan_Verified.md` T17,T18 / K6,U1

## 28. 서버가 끊겨도 계속 가능한가?

선택 Show에 유효한 로컬 Snapshot이 있으면 데이터 계층에서 사용할 수 있습니다. 전체 공연의 무중단 동작이나 모든 오류 조건을 보장하는 검증은 아닙니다.

보여줄 슬라이드: A6 / 근거: `PresentationPlan_Verified.md` T16 / S1 §8~10

## 29. 직접 작성한 코드는 어디인가?

웹, Python, JavaScript와 관련 C++ 데이터 계층 대부분은 Codex를 활용했습니다. 요구 기능과 방향 결정, 에디터 작업과 Blueprint 연결, 결과 확인은 기록 범위로 설명합니다. 근거 없이 함수별 작성 주체를 나누지 않습니다.

보여줄 슬라이드: A7 / 근거: `PresentationPlan_Verified.md` U2 / §3.5

## 30. 성능이 향상됐나?

동일 조건의 GPU frame time과 Draw Call 비교가 없어 성능 개선은 주장하지 않습니다. 이번 결과는 표현 방식과 제어 범위의 변경입니다.

보여줄 슬라이드: 16, A8 / 근거: `PresentationPlan_Verified.md` T19

## 31. 다음으로 개선할 것은?

실행 슬롯과 MID, LightDir/LightColor writer, Outline 추종을 먼저 대조하고 극단 조명과 반복 운용을 확인합니다. 그다음 같은 조건의 성능 계측으로 개선 필요 지점을 판단합니다.

보여줄 슬라이드: 16, A8 / 근거: `PresentationPlan_Verified.md` T19 / §0.5
