# 슬라이드별 발표 대본

기준: `PresentationPlan_Verified.md` 검수본과 확정 디자인 / 본편 16장, 기술 부록 8장

발표 설계 시간: 시연 75초 포함 13:15 / 15분을 가정한 여유 1:45 / 실제 발화 속도는 리허설로 조정

부록은 기본 발표 시간에서 제외 / 본편은 16번에서 종료하고 질문에 맞는 부록을 선택

## 발표 시간표

| 번호 | 제목 | 배정 | 누적 |
|---|---|---:|---:|
| 01 | Real-time Virtual Performer Studio | 15초 | 0:15 |
| 02 | Original / Toon 비교 | 35초 | 0:50 |
| 03 | 기존 Virtual Performer Studio | 40초 | 1:30 |
| 04 | 프로젝트 목표와 작업 범위 | 35초 | 2:05 |
| 05 | 공용 Toon Material | 50초 | 2:55 |
| 06 | Normal 기반 Toon 명암 | 55초 | 3:50 |
| 07 | SDF Face Shadow | 75초 | 5:05 |
| 08 | Texture 작업과 Eye 계산 | 45초 | 5:50 |
| 09 | Hair Mesh & UV | 65초 | 6:55 |
| 10 | Hair Highlight | 55초 | 7:50 |
| 11 | Outline & Rim Light | 55초 | 8:45 |
| 12 | Appearance 시스템 연동 | 50초 | 9:35 |
| 13 | Web Cue Editor | 40초 | 10:15 |
| 14 | Web Cue 시스템 아키텍처 | 60초 | 11:15 |
| 15 | 최종 시연 | 75초 | 12:30 |
| 16 | 구현 결과와 개선 과제 | 45초 | 13:15 |

## 01. Real-time Virtual Performer Studio

배정: 15초

안녕하세요. Real-time Virtual Performer Studio를 제작한 신소영입니다. 첫 프로젝트의 실시간 Studio를 기반으로 Toon Shader와 관련 데이터를 공부하고 수정했습니다. 추가로 웹에서 Cue를 준비하고 Unreal에서 조회해 실행하는 기능을 연결했습니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 01 / S1 §1, S2 §1

## 02. Original / Toon 비교

배정: 35초

먼저 기존 표현과 Toon 결과입니다. 두 화면은 UI에 표시된 Idle, Upper, Neutral 상태를 기준으로 같은 viewport 좌표를 확대했습니다. 기존 디자인과 배색을 활용하면서 명암 경계, 얼굴 그림자와 헤어 표현을 변경했습니다. Material만 교체한 비교가 아니라 Texture와 헤어 Mesh 변경까지 포함한 결과입니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 02 / T01~T04 / S2 §1.3, §2.3, §3.11

## 03. 기존 Virtual Performer Studio

배정: 40초

첫 프로젝트에서는 Animation과 Gesture, 위치, Camera, Lighting, Appearance를 한 화면에서 제어하는 Studio를 만들었습니다. UI 입력은 StudioManager를 거쳐 각 기능에 전달되고 Preset으로 여러 상태를 함께 바꿉니다. 이번 프로젝트는 이 기반을 활용해 캐릭터 표현을 수정하고 Cue 데이터 계층을 추가한 작업입니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 03 / T13 / S6 pp.1~7, 61~65, 83~94 / S1 §5

## 04. 프로젝트 목표와 작업 범위

배정: 35초

두 번째 프로젝트의 중심은 기존 Bounty Hunter를 Toon 표현으로 바꾸는 작업입니다. 원본 Material을 분석하고 Normal 명암과 얼굴 SDF를 구성한 뒤 Texture, 헤어 Mesh와 UV를 수정했습니다. 이 결과를 기존 Appearance와 Lighting 경로에 연결했습니다. 초기 기획의 Hair Physics, Foot IK와 Facial Tracking은 이번 완료 범위에 포함하지 않습니다. Web Cue는 교육과정 요구에 따라 추가했습니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 04 / T20 / S4 pp.1~9, S2 §1~3

## 05. 공용 Toon Material

배정: 50초

원본 Asset의 얼굴 Mask와 색상 제어, 눈 UV 계산을 재사용했습니다. 공용 M_Toon에서 부위별 규칙을 선택하고 부위별 Material Instance에서 Texture와 값을 설정합니다. 색상, 명암과 Highlight를 합성한 결과는 BaseColorPower와 선택적 Rim, 색상과 밝기 배율을 거쳐 Emissive로 출력합니다. M_Toon의 설정은 Opaque, Default Lit이며 Base Color 입력은 0입니다. Outline은 별도 Material입니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 05 / T01, T02 / S2 §0.2~0.4, §2, §3.1

## 06. Normal 기반 Toon 명암

배정: 55초

일반 명암은 PixelNormalWS와 MPC에서 받은 LightDir의 반대 방향을 내적해 계산합니다. 이 값을 더하기와 곱하기로 remap한 q를 SmoothStep에 넣습니다. ShadowRange는 경계 위치이고 ShadowSmooth는 경계 양쪽 폭이므로 전체 전환 구간은 두 배입니다. GeneralLit가 1이면 기본색, 0이면 그림자색을 선택합니다. 이 계산은 방향 기반 명암이며 장면의 실제 차폐 그림자를 계산하는 것과는 구분됩니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 06 / T03 / S2 §0.3, §3.2

## 07. SDF Face Shadow

배정: 75초

얼굴은 코와 볼의 굴곡을 따라 명암이 복잡하게 나뉘어, 그림자 형태를 Texture로 따로 구성했습니다. 상단은 광원 방향별 적용 결과입니다. 머리와 광원의 XY 방향을 비교해 좌우 SDF Texture를 선택하고 Threshold를 계산합니다. 선택한 값에 Power 0.8과 SmoothStep을 적용한 뒤 FaceShadowMask로 일반 명암과 혼합합니다. 마지막으로 1에서 DefaultShadow를 뺀 값을 곱합니다. 이 고정 그림자는 Mask 혼합 이후의 얼굴 분기에 적용됩니다. 상하 방향 보정이나 모든 광원 조건의 안정성까지 검증한 결과는 아닙니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 07 / T04~T06 / S2 §0.3, §3.3~3.5

## 08. Texture 작업과 Eye 계산

배정: 45초

Base Color에 남아 있는 고정 음영은 Shader의 명암과 중첩되기 때문에 Texture도 함께 조정했습니다. 기존 배색을 유지하면서 강한 명암을 정리하되, 지나치게 평평해진 결과에는 최소한의 형태 음영을 남기는 방향으로 수정 요청을 진행했습니다. 이 과정에는 AI 이미지 생성과 편집을 활용했습니다. 눈은 원본의 UV, 홍채와 동공 계산을 재사용하고 Toon Texture와 결합했습니다. 구성한 계산과 실제 눈 슬롯 적용 여부는 별도로 구분했습니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 08 / T07,T08 / S2 §2.2, §3.9~3.11, S9

## 09. Hair Mesh & UV

배정: 65초

헤어는 Tripo에서 생성한 초안을 활용해 Maya에서 덩어리를 분리하고 UV를 전개하고 정렬했습니다. 오른쪽 Painter 화면에서는 분리된 UV 섬과 밝기 패턴을 함께 볼 수 있습니다. UV는 Texture 위치와 방향 기준에 관계하고 Vertex Normal은 표면 명암과 Highlight 방향축에 관계하므로 서로 다른 작업입니다. 앞머리에 삼각형 면 단위 명암이 보였고 Soften Edge를 적용해 해결했다는 작업 기록이 있습니다. 여기에는 현재 작업 화면을 사용했으며 수정 전 화면을 새로 재현하지 않았습니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 09 / T09 / S2 §3.6, §4.4, §5.2

## 10. Hair Highlight

배정: 55초

헤어 전용 방향축은 Vertex Normal과 Vertex Tangent를 Cross한 뒤 정규화해 만듭니다. 광원 반대 방향과 CameraVector를 더해 정규화한 Half Direction과 이 축을 비교합니다. Dot의 절댓값을 1에서 빼고 Power를 적용해 띠 형태 Highlight 마스크를 만들었습니다. 기본색은 Hair_Color와 T_HairValue의 R 채널을 곱하는 별도 경로입니다. 즉 밝기 Texture와 방향성 Highlight는 서로 다른 입력입니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 10 / T10,T11 / S2 §0.3, §3.7~3.8

## 11. Outline & Rim Light

배정: 55초

Outline과 Rim은 역할이 다릅니다. M_Outline은 Vertex Normal 방향으로 위치를 확장하고 TwoSidedSign으로 면을 마스킹하는 별도 Material입니다. Rim은 Fresnel로 만든 시점 기반 마스크로 기존 색을 강조합니다. 회백색으로 뜨던 결과는 Add 방식 대신 기존 색과 배율의 곱을 Lerp하는 방식으로 수정한 기록이 있습니다. 실제 역광이나 차폐 계산은 아니며, 부위별 MI에 따라 Rim 활성 여부도 다릅니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 11 / T12 / S2 §0.4, §0.7, §3.12~3.13, §4.5

## 12. Appearance 시스템 연동

배정: 50초

Toon 결과는 기존 Studio의 Appearance 경로에 연결했습니다. 모드가 바뀌면 Manager에서 Character로 명령을 전달하고 Material을 교체한 뒤 MID를 생성하는 경로를 사용합니다. 색상 Preset은 부위별 Parameter를 바꾸며 Lighting에서는 LightIntensity를 MPC로 전달합니다. Toon 활성 상태의 Tick에서는 Head Bone의 World 방향을 MPC에 갱신해 얼굴 SDF 계산에 사용합니다. 화면은 제공된 다섯 외형 Preset 결과이며 실제 모든 슬롯과 광원 writer의 검증 범위는 부록에 구분했습니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 12 / T14 / S2 §0.6, §0.8

## 13. Web Cue Editor

배정: 40초

Web Cue는 공연 순서를 준비하는 추가 기능입니다. 웹에서 Show와 Cue를 편집하고 Animation, Camera, Lighting, Appearance 값을 저장합니다. Unreal에서는 Show 선택 후 목록을 조회하고 REFRESH로 다시 가져옵니다. GO는 가져온 로컬 Cue를 실행합니다. 웹 저장 자체가 씬을 바꾸지는 않습니다. 웹과 서버, 관련 C++ 코드 대부분은 Codex를 활용해 구현했고, 요구 기능과 작업 방향을 정하고 결과 확인과 연결 작업을 진행했습니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 13 / T15,T18 / §0.4 / K4,K5,U1,U2

## 14. Web Cue 시스템 아키텍처

배정: 60초

데이터 전달 흐름입니다. Browser의 편집 데이터는 FastAPI와 SQLite에 저장됩니다. Unreal은 HTTP로 Snapshot을 받아 JSON 전체 검증과 로컬 저장이 성공한 뒤 활성 데이터를 바꿉니다. 여기까지가 C++ Subsystem의 역할입니다. 아래는 사용자 조작 순서입니다. Show 선택 함수는 캐시를 읽고, 선택 성공 뒤 Widget이 별도로 조회를 요청합니다. GO 실행과 진행 상태는 WBP_Operator가 관리하며 Resolver와 상태 함수를 통해 Studio에 전달합니다. 서버가 GO를 보내거나 Actor를 직접 조작하는 구조는 아닙니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 14 / T16,T18 / §0.4 / S1 §6~8 / K3,K4

## 15. 최종 시연

배정: 75초

먼저 같은 상태에서 Original과 Toon을 전환하겠습니다. 얼굴은 광원과 머리의 상대 방향으로, 헤어는 Camera 변화에 따른 Highlight로 확인하겠습니다. 이어 외형 Preset과 Lighting을 변경하고, 웹 Cue를 Unreal에서 조회한 뒤 GO로 실행하겠습니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 16 / §12 / T18 / U1,K4

## 16. 구현 결과와 개선 과제

배정: 45초

이번 결과물에는 Toon Material과 관련 Texture, 헤어 데이터 수정, 기존 Studio 연결, Web Cue 데이터 연동이 포함됩니다. 표현 문제를 Material 값뿐 아니라 Texture, UV와 Normal, Runtime 연결까지 나누어 확인한 과정이 중심 작업이었습니다. 웹과 서버 코드는 대부분 Codex를 활용했습니다. 남은 과제는 실제 슬롯과 광원 writer의 대조, 극단 광원과 반복 운용 검증입니다. GPU 비용과 프레임시간은 측정하지 않았으므로 성능 개선은 주장하지 않습니다. 이상으로 발표를 마치겠습니다.

근거: `PresentationPlan_Verified.md` §6 슬라이드 17 / T19,T20 / S1 §10.3, §12~13 / S2 §5.5

## A1. M_Toon 최종 출력

배정: 75초 / 질문이 있을 때만

현재 출력에 연결된 경로를 기준으로 정리한 부록입니다. DrawHighlight는 일반 명암의 q를 반전한 별도 마스크이며 Lerp 입력의 순서도 그림자와 다릅니다. Hair Highlight 뒤에 BaseColorPower와 Rim을 적용하고 LightColor, OverallHue와 LightIntensity를 곱합니다. Normal은 별도 루트입니다. EyeUnderHighlight, HairValue의 DarkStrength와 LightStrength 보정 Lerp, Custom 광원은 최종 출력에 도달하지 않습니다.

근거: `PresentationPlan_Verified.md` §7 A1 / T01,T02,T08,T10~T12 / S2 §0.2~0.3, §0.7

## A2. SDF 방향과 Threshold 계산

배정: 75초 / 질문이 있을 때만

L은 MPC LightDir를 반전한 LightDirection의 XY 정규화 값입니다. HeadRight와의 내적을 SmoothStep -0.1부터 0.1에 넣어 좌우 Texture를 보간하고 HeadForward와의 내적은 remap 후 반전해 Threshold를 만듭니다. 선택 SDF에 Power 0.8을 적용하고 Threshold 양쪽 SDF_Smooth 폭으로 SmoothStep을 계산합니다. FaceShadowMask로 GeneralLit와 혼합한 뒤 DefaultShadow를 곱합니다. Blueprint 실제 축 매핑은 이름과 다르게 RightVector가 HeadForward, UpVector가 HeadRight로 기록되어 있습니다.

근거: `PresentationPlan_Verified.md` §7 A1 확장 / T04~T06,T14 / S2 §0.3, §0.6, §3.3~3.5

## A3. Master / MI / MID

배정: 60초 / 질문이 있을 때만

Master 기본값과 부위별 MI의 상속 유효값, 실행 중 MID 값은 서로 다른 근거입니다. 표는 저장된 MI 유효값입니다. 얼굴과 눈은 Rim이 꺼져 있고 Body, Hair, Cloth는 켜져 있습니다. 얼굴과 몸은 파생 Normal Texture를 참조합니다. Hair의 Normal getter가 원본을 반환해도 실제 Hair 분기의 선택 결과와 동일하게 해석하면 안 됩니다. 실제 실행 슬롯과 MID 전체는 추가 대조 범위입니다.

근거: `PresentationPlan_Verified.md` §7 A2 / T07,T12,T14 / S2 §0.4~0.7, L1e

## A4. Cue v2 계약과 Blueprint 실행

배정: 75초 / 질문이 있을 때만

왼쪽은 Cue 객체 중 네 상태와 시간 필드를 발췌한 예시입니다. id, position, number, name 등 필수 필드는 생략했으므로 이 자체가 전체 API 요청은 아닙니다. 시간 값은 검증과 보존 대상이지만 Cue별 Camera Blend와 Lighting Fade 실행은 이번 범위가 아닙니다. 오른쪽은 WBP_Operator가 Resolver로 문자열을 해석해 Studio 상태 함수로 전달하는 경로입니다. Stage Camera는 별도 Stage View 분기입니다.

근거: `PresentationPlan_Verified.md` §7 A3 / T16,T18,T20 / §0.4 / K3,K4

## A5. Stage Cue 데이터 검증 수정

배정: 60초 / 질문이 있을 때만

Stage Camera를 포함한 목록이 거부되던 사례입니다. 서버는 Stage를 허용했지만 C++ Camera 허용 목록에는 누락되어 Snapshot 전체 검증이 실패했습니다. 현재 코드에는 Stage가 허용값으로 들어 있습니다. 수정 결과는 보존된 실패와 통과 기록으로 확인했으며, 이번 발표 제작에서 테스트를 다시 실행한 것은 아닙니다. 자동 테스트와 사용자 Unreal 실행 확인은 구분합니다.

근거: `PresentationPlan_Verified.md` 원 설계 슬라이드 15 → 부록 / T17 / S1 §9~10 / L1b,d,g,h

## A6. 오류 처리와 검증 범위

배정: 60초 / 질문이 있을 때만

수신 데이터는 응답과 JSON 검증, 캐시 저장이 성공한 뒤 활성 Snapshot으로 교체합니다. 세대 번호와 선택 Show 비교는 이전 요청의 늦은 응답이 현재 선택을 덮지 않도록 하는 보호입니다. 보존 자동 테스트는 HTTP, JSON과 캐시 등의 데이터 경로를 확인합니다. GO, END SHOW, RESET과 Stage의 기본 동작은 사용자 실행 확인입니다. 이 둘을 합쳐 모든 공연 조건이나 장시간 안정성이 검증되었다고 설명하지 않습니다.

근거: `PresentationPlan_Verified.md` §7 A4 / T16,T17,T18 / S1 §8~10 / K3,K4,U1

## A7. 작업 기여와 AI 활용

배정: 60초 / 질문이 있을 때만

작업별 기여 범위입니다. 캐릭터의 몸, 얼굴과 의상은 외부 Bounty Hunter Asset입니다. Toon 노드 구성과 프로젝트 연결, Maya와 UV, Normal 수정은 학습 자료와 AI 설명을 참고한 직접 작업 기록이 있습니다. 헤어 초기 Mesh는 Tripo, 일부 Texture와 SDF 이미지는 AI 생성과 편집을 활용했습니다. 웹, 서버와 관련 C++ 대부분은 Codex 구현입니다. 요구사항과 방향 결정, 결과 확인과 Blueprint 연결은 작업 기록 범위로 설명하며 함수별 작성자를 근거 없이 나누지는 않습니다.

근거: `PresentationPlan_Verified.md` §7 A5 / §3.5 / U2 / S2 §5.2

## A8. 검증 환경과 후속 과제

배정: 60초 / 질문이 있을 때만

검증 문서의 환경은 Windows PC와 Unreal Engine 5.8입니다. 저장된 Graph와 제공 Screenshot, 사용자 실행과 보존 자동 테스트가 각각 다른 근거입니다. 다음 단계는 실제 슬롯과 MID, LightDir와 LightColor writer를 대조하고 극단 조명과 반복 운용을 확인하는 것입니다. 성능은 같은 조건의 GPU frame time과 Draw Call을 계측해야 판단할 수 있습니다. 인증과 외부 배포, Cue별 시간 전환이나 초기 Motion 목표는 현재 완료 범위에 포함하지 않습니다.

근거: `PresentationPlan_Verified.md` §7 A6 / T19,T20 / K1 / S1 §10.3, §12~13 / S2 §0.8, §5.5

## 사용 방법

- PPT 발표자 노트에도 같은 대본과 근거 기록
- 문장 전체를 화면에 읽히는 속도로 낭독하기보다 이미지와 계산을 가리키며 설명
- 15번은 영상이 삽입된 슬라이드가 아니라 라이브 시연 안내 화면
- 웹, 서버 및 관련 C++ 코드는 대부분 Codex 구현으로 설명 / 함수별 직접 작성 범위는 근거 없이 추가하지 않음
- 자동 테스트 수치는 보존 기록 / 이번 PPT 제작에서 Unreal Play, 빌드, 성능 테스트를 새로 수행한 결과가 아님
