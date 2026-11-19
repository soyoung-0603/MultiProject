# 최종 발표자료 검수와 근거

## 최종 구성

- 본편 16장 / 13:15 배정, 시연 75초 포함 / 15분 가정
- 질의응답용 부록 8장 / Stage 사례를 본편에서 부록 A5로 이동
- SDF 상세 계산은 A2로 보강 / 기존 설계의 기술적 범위 유지
- 표지 부제: Toon Shader & Web Cue System
- 표지 35pt, 슬라이드 제목 26pt, 소제목 18~19pt, 본문 16~17pt, 수식/Diagram 14~16pt, Caption 12~14pt, 보조 정보 10~11pt
- 표, 문구, 수식과 Diagram은 편집 가능한 PowerPoint 개체 / 전체 슬라이드를 이미지로 삽입하지 않음

## 기술 검수 기준

- 검증 문서 §0, §6, T01~T20을 우선 / 이전 기획과 과거 Graph를 최신 출력 사실로 대체하지 않음
- M_Toon Default Lit, Base Color=0 / Outline 별도 Material
- 일반 명암의 반전 LightDir, Remap, SmoothStep 폭과 Lerp 극성 유지
- SDF 좌우 선택과 Threshold 분리 / FaceShadowMask 혼합 뒤 DefaultShadow Multiply
- Hair Cross 방향축, Half Direction, Dot/Abs/OneMinus/Power와 Value 기본색 경로 분리
- Outline WPO와 Mask는 독립 입력 / Rim은 Fresnel 시점 효과
- 저장 MI 유효값과 Runtime Slot/MID 분리 / 미사용 경로를 완성 성과에 넣지 않음
- Web Cue 데이터 전달과 사용자 조작을 별도 영역으로 표시 / SelectShow 캐시와 Widget 후속 HTTP 요청 구분
- GO/END/RESET 소유는 Blueprint / C++은 HTTP/JSON/캐시/통지
- 자동 테스트 보존 기록과 사용자 실행 확인 분리 / 새 Play나 테스트 실행, 성능 개선 수치 주장 없음

## 이미지와 비교 조건

- 입력 이미지 74개(요청 75개와 1개 차이)는 앞선 전수 검수 결과 재사용 / 모두 열림, CRC 정상
- 최종 PPT 사용 원본 36종 / 원본 바이트와 해상도 유지 / PowerPoint Crop으로 비율 보존
- Original/Toon: UI의 Idle/Upper/Neutral 상태와 동일 viewport 좌표 Crop / 모든 노출 Parameter 동등성까지 주장하지 않음
- SDF 각도: 제공 캡처 파일명 기준 / 실제 Light Actor 수치와 writer를 새로 측정하지 않음
- 작업 화면은 현재 상태의 증거 / Soften Edge, Rim의 미확보 전후 장면을 재연하거나 생성하지 않음
- 웹, Cue 패널과 Studio 화면은 각각의 기능 화면 / 같은 순간 동기화 장면으로 제시하지 않음
- 발표 영상 미첨부 / 15번은 라이브 시연 안내

## 사용 이미지 배정

| 원본 파일 | 최종 슬라이드 |
|---|---|
| FaceColor 계산.png | 4 |
| HairHighlight.png | 10 |
| HairHighlight2.png | 10 |
| HairMesh.png | 4, 9 |
| HairValue 텍스처 앞.png | 9 |
| M_Toon 전체 Material Graph.png | A1 |
| NoRimLight.png | 11 |
| Original 얼굴 확대.png | 2 |
| QueControl.png | 13 |
| RimLight.png | 11 |
| SDF 계산.png | A2 |
| SDF_Step07.png | 4 |
| T_Body_Toon.png | 8 |
| T_Cloth_Toon.png | 8 |
| T_FaceShadowMask.png | 7 |
| T_HairValue.png | 10 |
| T_Head_Mask_Toon.PNG | 5 |
| T_Head_Toon.png | 5, 8 |
| T_Iris_Toon.png | 8 |
| T_SDF_Left.png | 7 |
| T_SDF_Right.png | 7 |
| T_Sclera_Toon.png | 8 |
| Toon 얼굴 확대.png | 1, 2, 4, 6 |
| Toon 외형 Preset 01.png | 12 |
| Toon 외형 Preset 02.png | 12 |
| Toon 외형 Preset 03.png | 12 |
| Toon 외형 Preset 04.png | 12 |
| Toon 외형 Preset 05.png | 12 |
| Toon 전신.png | 16 |
| ToonFace.png | 8 |
| WebCueEditor.png | 13 |
| 얼굴 그림자_광원 방향 0도.png | 7 |
| 얼굴 그림자_광원 방향 180도.png | 7 |
| 얼굴 그림자_광원 방향 90도.png | 7 |
| 일반 Toon 명암 계산 부분.png | 6 |
| 최신 Operator UI 전체 화면.png | 3, 15 |

## 슬라이드별 근거

S1=`PresentationTechnicalReport.md`, S2=`Toon_Shader_Technical_Record.md`, S6=`내가_한_것들.pdf` 118페이지 판본 / K3=C++ 선택/수신 경로, K4=WBP 저장 Export, U1=사용자 실행 확인, U2=Codex 활용 범위

### 01. Real-time Virtual Performer Studio

`PresentationPlan_Verified.md` §6 슬라이드 01 / S1 §1, S2 §1

검증 문서의 해당 슬라이드 범위 사용

### 02. Original / Toon 비교

`PresentationPlan_Verified.md` §6 슬라이드 02 / T01~T04 / S2 §1.3, §2.3, §3.11

이미지: Original 얼굴 확대.png, Toon 얼굴 확대.png. 동일 UI 상태와 동일 좌표 Crop. 노출 및 모든 런타임 Parameter의 수치 동일성까지 확인한 비교는 아님.

### 03. 기존 Virtual Performer Studio

`PresentationPlan_Verified.md` §6 슬라이드 03 / T13 / S6 pp.1~7, 61~65, 83~94 / S1 §5

이미지: 최신 Operator UI 전체 화면.png. Cue 패널이 포함된 현재 통합 화면이며 모든 UI가 첫 프로젝트에 있었다는 의미가 아님. 최종 검수 환경 UE 5.8 / Windows PC.

### 04. 프로젝트 목표와 작업 범위

`PresentationPlan_Verified.md` §6 슬라이드 04 / T20 / S4 pp.1~9, S2 §1~3

검증 문서의 해당 슬라이드 범위 사용

### 05. 공용 Toon Material

`PresentationPlan_Verified.md` §6 슬라이드 05 / T01, T02 / S2 §0.2~0.4, §2, §3.1

M_Toon Surface/Opaque/Default Lit/단면. Normal은 별도 루트. Static Switch는 MI 변형 선택이며 실시간 연속 토글 설명 금지. 최적화 수치 없음.

### 06. Normal 기반 Toon 명암

`PresentationPlan_Verified.md` §6 슬라이드 06 / T03 / S2 §0.3, §3.2

q의 0~1 범위는 단위 방향 입력 전제. Custom View.DirectionalLightDirection 노드는 미사용. Graph Crop은 일반 명암 사용 경로를 확대하며 미사용 Custom은 포함하지 않음.

### 07. SDF Face Shadow

`PresentationPlan_Verified.md` §6 슬라이드 07 / T04~T06 / S2 §0.3, §3.3~3.5

각도는 제공 파일명 표기 기준. 실제 Light Actor 각도 수치를 새로 측정하지 않음. 결과는 전체 Material 적용 화면. FaceLit=Lerp(GeneralLit,SDFLit,T_FaceShadowMask.R). FinalFaceLit=(1−T_DefaultShadow.R)×FaceLit. Nose Highlight에는 DrawShadow 차단 없음. 자세한 방향/Threshold 수식은 A2.

### 08. Texture 작업과 Eye 계산

`PresentationPlan_Verified.md` §6 슬라이드 08 / T07,T08 / S2 §2.2, §3.9~3.11, S9

AI 보조 Texture 변형, 사용자 목표 지정/수정 요청/선택/적용 기록. EyeUnderHighlight 미사용. T_Head_N_Toon/T_Body_N_Toon은 저장 MI 유효 참조. 전후 Texture 픽셀 동등 비교는 수행 안 함.

### 09. Hair Mesh & UV

`PresentationPlan_Verified.md` §6 슬라이드 09 / T09 / S2 §3.6, §4.4, §5.2

현재 Maya/Painter 화면은 확보. Soften Edge 전후 원본은 없음. 최초 Import 원인이나 단일 원인을 추가 단정하지 않음. Hair Skinning/Physics 완료 주장 없음.

### 10. Hair Highlight

`PresentationPlan_Verified.md` §6 슬라이드 10 / T10,T11 / S2 §0.3, §3.7~3.8

B=normalize(cross(VertexNormalWS,VertexTangentWS)); H=normalize(-MPC_Toon.LightDir.rgb+CameraVectorWS); m=pow(1-abs(dot(B,H)),HairSpecularPower). m은 IsHair?를 거쳐 Lerp_3.Alpha. HairValue DarkStrength/LightStrength Lerp는 출력 미도달.

### 11. Outline & Rim Light

`PresentationPlan_Verified.md` §6 슬라이드 11 / T12 / S2 §0.4, §0.7, §3.12~3.13, §4.5

Rim C는 BaseColorPower 적용 후 색. NoRimLight/RimLight는 제공 ON/OFF 캡처이며 노출/모든 값의 수치 동일성 새 검증 없음. Outline Shader 연결은 확인, 실제 추가 Mesh/슬롯/애니메이션 추종은 확인 제한. 정확한 Width 수식 A1.

### 12. Appearance 시스템 연동

`PresentationPlan_Verified.md` §6 슬라이드 12 / T14 / S2 §0.6, §0.8

실제 함수: UpdateAppearanceModeState/ApplyAppearanceMode→BP_BountyHunter.ApplyAppearanceMode→ApplyModeVisuals SetMaterial/CachMID_Toon. SetSkinColor: SkinHue_Global/SkinDepth_Global. UpdateToonMPC GetRightVector→HeadForward/GetUpVector→HeadRight. LightDir/LightColor writer 미확정. Cue에는 shaderMode 없음.

### 13. Web Cue Editor

`PresentationPlan_Verified.md` §6 슬라이드 13 / T15,T18 / §0.4 / K4,K5,U1,U2

Cue Inspector와 Cue Control은 각각의 기능 화면이며 동일 순간 동기화 증거가 아님. 사용자 조작 순서를 데이터 경로와 구분. 복제는 초안 생성/저장 단계를 구분. Drag/drop 완료나 다운로드 완료를 새로 주장하지 않음.

### 14. Web Cue 시스템 아키텍처

`PresentationPlan_Verified.md` §6 슬라이드 14 / T16,T18 / §0.4 / S1 §6~8 / K3,K4

SelectShow 자체는 HTTP 요청 아님. Widget RequestCueRefresh→RefreshCueList. C++ HandleCueResponse ReadResponse→CueJson::ParseSnapshot→FCueCache.SaveSnapshot→ActiveSnapshot. GO CurrentCueIndex/NextCueIndex/bCueRunning 소유는 WBP_Operator. HTTP 연동, RPC/Replication 아님.

### 15. 최종 시연

`PresentationPlan_Verified.md` §6 슬라이드 16 / §12 / T18 / U1,K4

시연 영상 미첨부. 본 슬라이드는 실제 제공 Screenshot을 이용한 라이브 시연 안내 화면이며 영상 삽입 완료로 표시하지 않음. 시연 순서는 별도 DemoSequence.md. 기존 사용자 정상 실행 확인과 이번 제작자의 새 Play 실행을 구분.

### 16. 구현 결과와 개선 과제

`PresentationPlan_Verified.md` §6 슬라이드 17 / T19,T20 / S1 §10.3, §12~13 / S2 §5.5

Cue 기본 동작은 사용자 정상 실행 확인 완료로 유지. 미확인 슬롯/광원 writer/장시간 및 성능만 후속 범위로 표시. 새 Play/빌드/성능측정 미실시.

### A1. M_Toon 최종 출력

`PresentationPlan_Verified.md` §7 A1 / T01,T02,T08,T10~T12 / S2 §0.2~0.3, §0.7

M_Toon.t3d, reachability.json. Lerp_0/1/3, Power_0, Lerp_14, Multiply_33/38/0. Outline Width=Saturate(Clamp(Distance*DistanceScale,OutlineThickness,3.0)*OutlineThickness). 모든 switch 양쪽 연결 존재와 해당 MI 선택을 구분.

### A2. SDF 방향과 Threshold 계산

`PresentationPlan_Verified.md` §7 A1 확장 / T04~T06,T14 / S2 §0.3, §0.6, §3.3~3.5

원본 SDF 계산.png 상단 HeadForward와 하단 HeadRight 구간 확인. nose side Step(r−0.025)와 SDF side SmoothStep을 혼동하지 않음. HeadUp은 MPC 정의/Material 소비 없음.

### A3. Master / MI / MID

`PresentationPlan_Verified.md` §7 A2 / T07,T12,T14 / S2 §0.4~0.7, L1e

Face/Mouth BaseColorPower1.4, Body1.4, Hair2.044216, Eye1.2, Cloth1.2. Master1.5/MI_BountyHunter1.2. Texture설정은Audit의직접조회기록. sRGB mask 중간값을 linear로가정금지.

### A4. Cue v2 계약과 Blueprint 실행

`PresentationPlan_Verified.md` §7 A3 / T16,T18,T20 / §0.4 / K3,K4

아래 JSON은 필드 발췌 예시,실제DB존재주장없음. Camera Stage: SetStageViewOn,일반유효값:SetStageViewOff→SetCameraButtonState. END/RESET ResetAllState(false) 위치/표현모드초기화까지확대금지.

### A5. Stage Cue 데이터 검증 수정

`PresentationPlan_Verified.md` 원 설계 슬라이드 15 → 부록 / T17 / S1 §9~10 / L1b,d,g,h

실제발췌 Source/NetworkProject/Private/CueJson.cpp State(cameraState). Unreal7개성공중1개경고포함, 서버37passed1warning. 이숫자를새실행결과로표기금지.

### A6. 오류 처리와 검증 범위

`PresentationPlan_Verified.md` §7 A4 / T16,T17,T18 / S1 §8~10 / K3,K4,U1

코드발췌 UCuePlayerSubsystem::HandleCueResponse. 아래는 원본 제어 흐름을 설명용으로 재구성한 의사코드,원문인용아님. 실제라인코드별첨메모부록. 보존기록37+7,새실행없음.

### A7. 작업 기여와 AI 활용

`PresentationPlan_Verified.md` §7 A5 / §3.5 / U2 / S2 §5.2

AI활용을이유로실제구현축소하지않으며독자알고리즘발명/전체캐릭터제작/모든코드직접작성주장없음. 참고PDF To on_Shader_공부.pdf학습원본의개별페이지사용은현재선택근거검증문서범위.

### A8. 검증 환경과 후속 과제

`PresentationPlan_Verified.md` §7 A6 / T19,T20 / K1 / S1 §10.3, §12~13 / S2 §0.8, §5.5

검증기준GitHEAD 4ab28a985b13e5b9cd005d9f144f19b8e3f02e5c. 이번자료제작중새Git/Play/테스트재검증없음. Screenshot원본74개,PNG73BMP1정상열림/75개요청대비1개차이. 본문에누락추정성과추가없음.

## 제작 및 검수 기록

- 구조 검사와 슬라이드 규격, 기본 글꼴 검사 통과
- 실제 Microsoft PowerPoint에서 24장 PNG 렌더링 / 각 장 개별 시각 검수 후 이미지 Crop 보완
- 원본 자료에 없는 생성 이미지나 인터넷 이미지 미사용
- Notion, Unreal 프로젝트와 DB 수정 없음
- Presentations의 Spaces/Artifact runtime은 이 환경에서 사용할 수 없어 Open XML 편집형 PPTX와 실제 PowerPoint 출력으로 제작

## 최종 출력 확인

- PowerPoint PNG 24장 생성, 개별 시각 검수 완료
- 수정 후 24장 재렌더링, 변경 7장 재확인 / 나머지 17장은 이전 렌더와 SHA-256 동일
- PowerPoint PDF 내보내기 24페이지 / Poppler로 24페이지 렌더링 성공
- PDF의 표지, 일반 명암, SDF 수식, MI 표에서 한국어와 수식 렌더 확인
- PPTX 구조 및 16:9 규격, 글꼴 검사 통과 / 표는 native PowerPoint table

| 파일 | SHA-256 |
|---|---|
| Virtual_Performer_Studio.pptx | 19dd11d9a9dc6dbc9f6e5d2686cab9aa3c93574a6a638f404e6c9659712172c3 |
| Virtual_Performer_Studio.pdf | c1f6e06e54c0360abedb815e7c379ae304b7e70ae175e7cb0e36df42856cbc3e |
