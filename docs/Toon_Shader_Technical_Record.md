# Bounty Hunter Toon Shader 기술 정리 — 실제 에셋 대조 수정본

두 번째 프로젝트 최종 발표 및 Technical Artist 포트폴리오 기술 자료

원문 작성 기준: 2026-11-18 / 저장된 프로젝트 대조: 2026-11-19 KST

## 검증 범위와 읽기 접근 결과

이 문서는 원문의 설명을 정답으로 삼지 않고 `C:\Work\Unreal_Engine\MultiProject`의 **저장된 에셋**을 UE 5.8의 읽기 API와 텍스트 Export로 조회한 결과를 우선한다. 문서 안의 과거 채팅 전달 지침은 과거 기록이며 이번 작업의 지시로 사용하지 않았다.

Unreal Asset의 생성·수정·저장·수동 Recompile·Play를 수행하지 않았다. `UnrealEditor-Cmd -run=pythonscript -EnablePlugins=PythonScriptPlugin -NullRHI`로 load/getter/텍스트 export만 실행했다. 결과는 `Saved/ToonAudit`에 저장했다. 엔진 로딩 중 자동 생성되는 로그·DDC와 에셋 저장은 구분한다. 화면 없는 조회이므로 **실제 시각 결과, 현재 열려 있는 Editor의 미저장 변경, 실행 중 MID/MPC 값은 확인하지 않았다.**

| 자료 | 내부 접근 결과 | 확정 가능한 범위 / 남은 제한 |
|---|---|---|
| M_Toon | 273개 Expression, Material 최종 입력, 입력 핀, Named Reroute Declaration을 조회·텍스트 Export | 저장된 출력 연결과 스위치별 도달성. 렌더링 결과·모든 외부 Material Function 내부는 별도 |
| M_Outline / M_Eyelashes / M_Vacant | Material 내부 연결·설정 조회 | 해당 Material 구현. 씬 적용 사실은 별도 |
| Toon Material Instance | Parent, 로컬 Override 배열, 상속을 포함한 유효 Scalar/Vector/Texture/Static Switch 값 조회 | 저장된 값. 런타임 MID 변경값은 별도 |
| Toon Texture 17개 | 설정과 엔진의 Size 조회 | sRGB/압축/Sampler/해상도 확인. 픽셀별 Mask 영역·UV 시각 비교는 미실행 |
| MPC_Toon | Scalar/Vector 정의·기본값 Export | 정의는 확인. 실시간 값과 모든 writer는 별도 |
| BP_BountyHunter / ToonTest / StudioManager / WBP_Operator | Blueprint Graph와 핀 연결을 텍스트 Export | 필요한 연결 일부를 추적. 전체 UI·씬 실행 검증 아님 |
| 캐릭터 CDO | 기본 CharacterMesh0의 Mesh·Material 배열, 기본 객체 Export | 기본 할당 확인. Construction/모드 전환 후 SCS 추가 Mesh·실행 인스턴스 할당으로 일반화 금지 |
| Studio 맵 | World 텍스트 일부 Export | World 조회의 Actor 배열이 비어 있어 완전한 씬 열람으로 간주하지 않음. Level Blueprint 접근도 실패 |
| 과거 G1·R1·P1·V1·V2·C1 | 원문에 적힌 식별자와 설명만 확보 | 원본 파일·과거 채팅·Library 링크는 재열람하지 않음. 이전 “작업 확인/화면 확인”은 원문 기록으로 한정 |

읽기 실행은 GameFeatureData Asset Manager 설정 오류를 로그에 남겼다. 따라서 commandlet 종료를 “프로젝트 전체 컴파일 정상”의 증거로 사용하지 않는다. 개별 JSON·Export 생성 완료와 오류 목록을 별도로 확인했다. 에셋을 수정해 이 오류를 해결하지 않았다.

### 상태 표기의 의미

- **구현 확인**: 현재 저장된 Graph에서 최종 Material 입력까지 연결을 추적했거나 저장된 설정을 직접 조회했다. 화면 적용을 자동 의미하지 않는다.
- **미사용 또는 폐기**: 현재 출력에 도달하지 않는 계산, 또는 과거에만 기록되고 현재 Graph에서 확인되지 않는 계산. 두 경우를 구분한다.
- **제안만 됨**: 원문에 있는 안내·목표로 현재 적용 근거가 없다.
- **확인 필요**: 실행 상태·자료 접근 제한으로 결론을 내릴 수 없다.
- **과거 작업 기록**: 기존 제작 과정·의도·완료 발언을 보존한 것이며 이번 검증에서 재입증하지 않았다.

## 0. 현재 에셋 기준 검증 결과

### 0.1 바로잡은 핵심 내용

| 항목 | 원문의 G1 기준 설명 | 현재 저장된 에셋 대조 결과 |
|---|---|---|
| 광원 방향 | 일반/SDF는 Custom, 헤어는 MPC 혼용 | **구현 확인:** 일반/SDF/헤어 모두 MPC_Toon.LightDir 경로. Custom `return View.DirectionalLightDirection;`는 존재하지만 최종 출력 미도달 |
| 기본 얼굴 그림자 | Min은 제안만 됨, Texture 없음 | **구현 확인:** T_DefaultShadow가 사용되며 **Min이 아닌 Multiply**. `(1 - DefaultShadow.R) * FaceLit` |
| 코 Highlight 그림자 차단 | 미연결 Multiply 존재 | 차단은 여전히 **미구현 경로**. 현재 OneMinus는 NoseHighlightMask를 직접 받음. 과거 노드 번호 239의 미연결 Multiply가 현재도 남았다고 단정하지 않음 |
| Hair Highlight Threshold | SmoothStep 계산이 남아 있으나 미사용 | 현재 Power가 IsHair? 선택을 통해 최종 Lerp Alpha에 도달. HairHighlightThreshold/HairHighlightSmooth와 해당 SmoothStep은 현재 Expression 목록에 없음. 과거 기록과 분리 |
| Hair 밝기 | Hair_Color × T_HairValue.R | **현재도 구현 확인.** DarkStrength/LightStrength Lerp는 현재도 출력 미도달 |
| EyeUnderHighlight | 미사용 | **현재도 출력 미도달.** EmissiveDepth/EyeEmissiveColor도 이 분기에서 출력 미도달 |
| 최종 밝기·색상 | OverallBrightness, LightColor 중심 | 현재 `Rim 결과 × LightColor.rgb × OverallHue.rgb × LightIntensity`가 Emissive로 감. OverallBrightness는 현재 Master 파라미터/연결에 없음. 일부 MI에 과거 Override가 남아 있어도 현재 기능으로 볼 수 없음 |
| 피부 Parameter | SkinHue/SkinDepth와 BP 이름 불일치 | 현재 Master는 **SkinHue_Global / SkinDepth_Global**. BP SetSkinColor의 Parameter 이름과 일치. 전체 런타임 색상 결과는 별도 |
| 의상 색 조절 | 기존 Hue/Depth 보존 미확인 | IsCloth? 분기와 ColorHue_Global/ColorDepth_Global/Color02_Hue/Color02_Depth 출력 연결 확인. 모든 원본 의상 기능의 완전 복제라는 뜻은 아님 |
| Master 설정 | Shading Model 미확인 | M_Toon은 **Surface / Opaque / Default Lit / Two Sided=false**. Emissive 중심이라고 Unlit으로 설명하면 틀림 |
| 얼굴 Normal | Master의 원본 T_Head_N 참조 | Master 기본은 원본이지만 **MI_Face_Toon 유효값은 T_Head_N_Toon**. MI_Body_Toon은 T_Body_N_Toon |
| Outline | Graph 최신본 미확보 | 별도 M_Outline의 최종 WPO/Mask/Emissive 및 거리 보정 연결 확인. 실제 씬 Mesh의 적용과 시각 두께는 별도 |
| 속눈썹 | M_Eyelashes_Toon / EyelashColor 안내 | 실제 Master 이름은 **M_Eyelashes**, Parameter는 **EyelashesColor**, MI_Eyelashes_Toon도 존재·Parent 확인. 본체 CDO Slot 1은 여전히 원본 MI_Eyelashes. 실제 실행 전환 할당은 확인 필요 |

### 0.2 최종 출력에서 역추적한 경로

대상 Asset: `/Game/ToonShader/Materials/M_Toon.M_Toon`. 아래 노드 식별자는 **현재 MaterialExpression 이름**이며 G1의 `MaterialGraphNode_번호`와 호환되는 번호가 아니다.

```text
Emissive Color
 ← Multiply_0( A=Multiply_38, B=MPC_Toon.LightIntensity )
 ← Multiply_38( A=Multiply_33, B=OverallHue )
 ← Multiply_33( A=StaticSwitchParameter_5[RimLight?], B=LightColor RGB Mask )
 ← RimLight? True: Lerp_14(Power_0, Multiply_32(Power_0,RimLightColor), DrawRimLight)
              False: Power_0
 ← Power_0( Lerp_3 결과, BaseColorPower )
 ← Lerp_3( 일반 Shadow/Highlight 합성, 그 결과 × HairSpecularColor,
            IsHair? True:Power_2 / False:0 )
 ← Lerp_1(Shadow 결과 × HighlightColor, Shadow 결과, DrawHighlight)
 ← Lerp_0(부위별 색 × ShadowColor, 부위별 색, DrawShadow)
```

`Normal`은 NamedRerouteUsage_5 → Declaration_5 → IsEye? → EyeNormal 또는 IsHair? → 고정 Normal/일반 Normal 경로다. `Base Color`는 Constant_0의 0, Opacity/Opacity Mask/WPO에는 입력 Expression이 없다. 루트 연결은 JSON과 T3D를 함께 대조했다.

Named Reroute Usage는 일반 입력 조회에서 입력이 없는 것으로 보였으나 T3D의 `Declaration=`을 추가 연결로 추적했다. 이 연결을 누락하면 실제 사용 중인 DrawShadow/Normal 등을 미사용으로 오판하므로, 근거 파일 `reachability.json`은 그 Declaration을 포함한다.

현재 도달성 계산은 모든 루트·스위치 양쪽을 먼저 검사하고, 별도로 MI의 유효 Static Switch 값을 적용했다. 외부 Material Function의 호출 입력까지 추적한 결과이며 GPU 최적화 결과·Instruction Count 측정은 아니다.

### 0.3 주요 연결과 미사용 계산

| 기능 | 현재 Expression 경로 | 판단 |
|---|---|---|
| 일반 명암 | MPC LightDir CollectionParameter_0 → Multiply_1(-1) → LightDirection Declaration_12 → DotProduct_0(PixelNormalWS) → Add_0(+1) → Multiply_2(0.5) → SmoothStep_0 | 최종 DrawShadow의 일반 분기에 연결 |
| 일반 Highlight | Multiply_2 → OneMinus_0 → SmoothStep_1 → IsFace? False → DrawHighlight → Lerp_1 | 출력 연결 |
| 얼굴 SDF | LightDirection.xy와 HeadForward.xy / HeadRight.xy 각각 Normalize·Dot → SmoothStep_3로 좌우 선택 → Lerp_15(SDF Left.R,Right.R) → Power_3(0.8) → SmoothStep_4 | IsFace? True 경로에서 출력 연결 |
| SDF 범위 | Lerp_16(GeneralLit,SDFLit,TextureSample_8 FaceShadowMask.R) | 마스크 안/밖 SDF와 일반 명암 선택 |
| 고정 그림자 | TextureSample_9(T_DefaultShadow.R) → OneMinus_5 → Multiply_39의 A, B=Lerp_16 → IsFace? True | **전체 얼굴 분기의 Lerp 뒤에 곱함. FaceShadowMask가 고정 그림자까지 제한하는 구조는 아님** |
| 코 Highlight | TextureSample_6/7 Left.R/Right.R → Lerp_13 → NoseHighlightMask → Usage_8 → OneMinus_7 → Lerp_17(FaceShadowMask.R) → DrawHighlight | Shadow 곱셈 없이 출력 사용 |
| 코 좌우 선택 | HeadRight 내적 DotProduct_10 → Add_3(-0.025) → Step_0 → Lerp_13.Alpha | SDF 좌우 선택의 SmoothStep과 다른 경로 |
| Hair 기본색 | VectorParameter_2(Hair_Color) × TextureSample_0(T_HairValue.R) → Multiply_31 → IsHair? | 출력 사용 |
| Hair Highlight | Cross(VertexNormalWS,VertexTangentWS) Normalize / Normalize(-MPC LightDir.rgb+CameraVectorWS) → Dot·Abs·OneMinus → Power_2(HairSpecularPower) → StaticSwitchParameter_11 → Lerp_3.Alpha | 현재 Power 직접 사용 |
| Eye | EyeColor Declaration_6(Lerp_8) → IsEye? StaticSwitchParameter_10.True, EyeNormal → Normal 분기 | 해당 MI 스위치에서 출력 사용. 원본과 알고리즘 완전 동일 여부는 별도 |
| EyeUnderHighlight | Lerp_9 → Declaration_10 | 어떤 Material 루트에도 도달하지 않음 |
| Hair Value 보정 | Lerp_12(DarkStrength,LightStrength,T_HairValue.R) | 루트 미도달 |
| Custom 광원 | Custom_0 `return View.DirectionalLightDirection;` | 루트 미도달. 현재 방향 공급으로 기술하지 않음 |
| HeadRight 잔여 Remap | Multiply_35 → Add_11 | 루트 미도달 |

### 0.4 Master 기본값·MI Override·스위치 분리

부위별 MI는 대체로 `M_Toon → MI_BountyHunter → 부위별 MI`이며 얼굴/몸은 중간에 `MI_Skin_Toon`이 있다. 아래는 실행 MID가 아니라 **상속까지 해석한 저장 유효값**이다. 개별 MI JSON의 `scalar_parameter_values/texture_parameter_values`는 그 MI 자체에 저장된 Override이며 `effective`는 상속 결과다.

| 대상 | 켜진 부위 스위치 / Rim | BaseColorPower | Normal 유효값 |
|---|---|---|---|
| M_Toon 기본 | 전부 false / false | 1.5 | 원본 T_Head_N |
| MI_BountyHunter | 전부 false / false | 1.2 | 원본 T_Head_N |
| MI_Face_Toon | IsFace=true / false | 1.4 | T_Head_N_Toon |
| MI_Mouth_Toon | IsFace=true / false | 1.4 | T_Head_N_Toon |
| MI_Body_Toon | IsBody=true / true | 1.4 | T_Body_N_Toon |
| MI_Hair_Toon | IsHair=true / true | 2.044216 | getter는 원본 T_Head_N을 반환하지만 IsHair 분기에서는 일반 Normal을 사용하지 않음 |
| MI_Eye_Toon | IsEye=true / false | 1.2 | IsEye?에서 고정 TextureSample EyeNormal 선택 |
| MI_Cloth_Toon | IsCloth=true / true | 1.2 | 원본 T_Cloth_N |

MI_Hair_Toon의 RimLightRange2는 2.011303, RimLightColor는 RGB 15이며 Master의 1.43/RGB 50과 다르다. MI_Body_Toon의 RimeLightRange는 -0.095907이다. 값을 “문제” 또는 “의도적 보정”이라고 Graph만으로 단정하지 않는다. 조회한 위 계층의 BasePropertyOverrides에는 BlendMode/ShadingModel/TwoSided Override가 꺼져 있어 Master의 Opaque/Default Lit/단면을 따른다.

일부 MI에는 `OverallBrightness`의 과거 Scalar Override가 직렬화되어 있지만, 현재 M_Toon의 이름 목록과 출력 도달 파라미터에는 없다. Override 항목이 저장되었다는 사실은 효과 사용의 증거가 아니다.

### 0.5 Texture 설정 — 직접 조회

아래 에셋은 모두 `/Game/ToonShader/Texture/` 아래에 있으며 엔진 Size 조회값은 4096×4096이다. Imported 원본 파일과 픽셀별 동일성을 비교한 수치는 아니다.

| Texture | sRGB | Compression | Sampler/해석 |
|---|---|---|---|
| T_SDF_Left / Right | false | Grayscale | M_Toon Linear Grayscale, Filter Bilinear |
| T_HairValue | false | Default | M_Toon Linear Color, R 사용 |
| T_DefaultShadow | true | Default | M_Toon Color 샘플의 R 사용 |
| T_FaceShadowMask | true | Default | M_Toon Color 샘플의 R 사용 |
| T_Nose_Highlight_Left / Right | true | Default | M_Toon Color 샘플의 R 사용 |
| T_Head_Mask_Toon | true | Default | FaceMask 파라미터 샘플은 Color |
| T_Head_Toon / T_Body_Toon / T_Cloth_Toon / 눈 색 Texture | true | Default | 색상 Texture. 현재 값이 적절한지 시각 판정은 미실행 |
| T_Head_N_Toon / T_Body_N_Toon | false | Normalmap | Normalmap LOD Group |
| T_Eyelashes_Mask | false | Masks | M_Eyelashes Sampler Masks |

목록은 설정 사실을 기록한 것이며 마스크를 일괄 Linear로 고치거나 압축을 바꾸라는 지시가 아니다. sRGB=true인 마스크의 중간값은 선형 데이터 Texture와 같다고 가정하면 안 된다. 이번에는 에셋 설정을 변경하지 않았다.

### 0.6 Blueprint에서 확인한 MPC와 모드 경로

`BP_BountyHunter` EventGraph의 ReceiveTick → `bIsToonModeActive` Branch.True → `UpdateToonMPC` 호출 연결을 확인했다. 함수 내부는 Mesh.GetBoneTransform(`Head`, `RTS_World`) → BreakTransform.Rotation → 다음 방향 벡터를 LinearColor로 바꿔 MPC에 쓴다.

- `GetRightVector` → `HeadForward`.
- `GetUpVector` → `HeadRight`.
- `GetForwardVector` → `HeadUp` 쓰기 노드도 연결되어 있으나, **현재 MPC_Toon 정의에 HeadUp 항목이 없고 M_Toon에도 소비 노드가 없다.** 이 노드가 실제 상하 보정을 제공한다고 설명할 수 없다.

이는 World Rotation에서 계산한 벡터의 실제 축 매핑이다. 이름만 보고 Forward→HeadForward라고 바꾸어 쓰지 않는다. 이 매핑을 택한 이유와 Bone 좌표의 적절성은 제작자 의도·화면 검증 없이 단정하지 않는다.

`BP_BountyHunter.ApplyCharacterLighting`에는 `SetScalarParameterValue(Collection=MPC_Toon, ParameterName=LightIntensity)`가 exec 체인에 연결되어 있다. `BP_StudioManager` 조명 데이터 경로에는 ToonLightIntensity 값 전달 핀도 확인된다. 광원 **방향 LightDir와 색 LightColor를 최종적으로 쓰는 writer**는 이번에 추적한 Blueprint 범위에서 확정하지 못했다. Studio Level Blueprint 접근은 실패했으므로 남은 핵심 자료다.

Original/Toon 연결은 `BP_StudioManager.UpdateAppearanceModeState/ApplyAppearanceMode → BP_BountyHunter.ApplyAppearanceMode` 호출, 캐릭터 `OriginalMaterials/ToonMaterials` 참조와 `ApplyModeVisuals`의 `SetMaterial`, `CachMID_Toon`의 MID 생성 노드를 확인했다. **배열의 실제 생성 시점 값과 런타임 Slot→MI 대응 전체를 확인한 것은 아니다.** 기존 CDO의 CharacterMesh0는 Original 재질이며, ToonTest CDO에서는 얼굴/몸/의상 일부가 Toon이고 눈 2개 및 속눈썹 Slot은 원본이다. 따라서 “ToonTest에 눈/속눈썹 전용 Toon MI가 모두 적용됐다”고 쓰지 않는다.

### 0.7 Outline과 속눈썹의 구현·적용 구분

`M_Outline`은 Masked / Unlit / Two Sided=true이며 다음 연결이 있다.

```text
TwoSidedSign * -1 → Opacity Mask
OutlineColor → Emissive
Distance = Length(CameraPositionWS - AbsoluteWorldPosition)
Width = Saturate(Clamp(Distance * DistanceScale, OutlineThickness, 3.0) * OutlineThickness)
VertexNormalWS * Width → World Position Offset
```

같은 이름 OutlineThickness Scalar가 두 개이며 저장 기본값은 0과 0.5로 다르다. 따라서 Master 기본 두께를 단일 0.5로 단정하지 않는다. MI_Outline/MI_FaceOutline/MI_HairOutline의 유효 Override는 DistanceScale=0.0015, OutlineThickness=0.3이다. 색은 앞의 두 MI RGB 0.006, HairOutline RGB 0.022다. 실제 추가 Mesh의 Material/애니메이션 추종과 거리·FOV에 따른 화면 결과는 확인 필요다.

`M_Eyelashes`는 Masked / Unlit / Two Sided=true. `T_Eyelashes_Mask.TextureSample_0`의 **RGB output**이 Opacity Mask로, `EyelashesColor.VectorParameter_0`가 Emissive로 연결되어 있다. 원문의 “R 채널 연결”과 이름 `EyelashColor`는 현재 연결/철자와 다르다. MI_Eyelashes_Toon은 이 Master를 Parent로 하며 저장 EyelashesColor RGB는 0.014/0.010324/0.009552다. 최종 화면 적용은 별도 증거가 필요하다.

### 0.8 추가로 필요한 자료와 시각 검증

| 자료 | 구체적 요청 | 해결할 미확정 항목 |
|---|---|---|
| Studio Level Blueprint 또는 Light writer Graph 복사 텍스트 | Event/함수 진입부터 MPC_Toon의 LightDir·LightColor Set까지 exec와 value pin 전체. 광원 Actor 참조·GetForwardVector·부호·Tick/이벤트 포함 | 실제 광원 방향·색 공급자, 다중 광원 선택·갱신 주기 |
| 실제 Play 중 캐릭터 Component/Material 목록 | BP_BountyHunter의 본체·별도 Hair·Goggle·Outline 컴포넌트명, 각 Slot의 실제 Material/MID와 Parent, Original/Toon 전환 전후 | 저장 CDO와 런타임 적용 구분, 눈·속눈썹·Outline 적용 |
| Material 전환 초기화 Graph | BeginPlay/Construction → OriginalMaterials·ToonMaterials 초기화 → ApplyModeVisuals → CachMID_Toon의 exec·데이터 핀 | 실제 선택된 MI와 MID 재생성·캐시 타이밍 |
| Outline 컴포넌트 Details/Graph | Mesh 참조·Material 배열·Leader Pose/애니메이션 추종 설정 | Outline Shader와 실제 확장 Mesh 연결 |
| 얼굴 비교 실행 화면 | 같은 카메라/노출에서 정면·좌우·상하 광원, 기본 그림자/코 Highlight를 확인할 확대 영상 | Multiply 결합 결과, 코 차단 시각 현상, SDF 극단 방향 안정성 |
| 헤어·눈·속눈썹·Rim 실행 비교 | 동일 조건 기능 on/off와 근접 컷. 실제 MI/MID 설정을 함께 표시 | 연결 사실과 시각적 해결 결과의 구분 |
| Texture/UV 비교 | FaceShadowMask/DefaultShadow/NoseMask의 R 미리보기와 실제 얼굴 UV, 원본 작업 파일·최종 Import 정보 | 얼굴·귀·목 적용 범위, AI/직접 제작·최종 버전 일치 |

위 자료가 없다는 이유로 기능을 “없음”으로 단정하지 않는다. 저장된 연결은 확인한 범위만 확정하고, 시각적 개선·제작 의도·Maya 작업 원인은 과거 기록 또는 확인 필요로 남긴다. GPU/Draw Call/성능 수치는 측정하지 않았으며 추가하지 않는다.

### 0.9 검증 자료 위치와 재사용 주의

- 현재 Material Export: `Saved/ToonAudit/M_Toon.t3d`, `M_Outline.t3d`, `M_Eyelashes.t3d`, `MPC_Toon.t3d`.
- Material/MI/Texture 설정: 같은 디렉터리의 에셋명 `.json`. `effective`와 로컬 Override 배열을 구분한다.
- 출력 도달성: `Saved/ToonAudit/reachability.json` — Named Reroute 연결 보완, MI별 스위치 선택 결과 포함.
- Blueprint 근거: `BP_BountyHunter.t3d`, `BP_BountyHunter_ToonTest.t3d`, `BP_StudioManager.t3d`, `WBP_Operator.t3d`, 캐릭터 CDO Export.
- 접근 실패·범위: `index.json`, `extra.json`, `scene.json` 및 `audit*.log`. scene.json의 빈 actors는 실제 Studio에 Actor가 없다는 뜻이 아니다.

일부 T3D는 UTF-16, 일부는 UTF-8/ASCII다. BOM에 맞춰 읽어야 한다. 이 Export들은 Unreal 에셋 Import용 수정본이 아니라 검증 증거다. 현재 문서에 있는 Graph 번호를 과거 G1 번호와 혼합하지 않는다.

---

## 기존 설명과 제작 과정의 보존 범위

이하 원문의 유효한 설명·작업 과정을 유지하면서 현재와 달라진 핵심 절을 수정했다. G1·R1·P1·V1·V2·C1이라는 출처는 **과거 문서에 기록된 출처**이며 원본을 이번에 재열람했다는 뜻이 아니다. 문서 작성 의도·선택 이유·Maya/AI 작업 경력은 원문 작업 기록으로 보존하며 현재 Graph만으로 새로 확정하지 않는다. 과거 원문 그대로의 백업도 검증 폴더에 보존했다.


### 주요 자료 식별자

| 식별자 | 자료 | 이 문서에서의 용도 |
| --- | --- | --- |
| G1 | `붙여넣은 텍스트(1)(20261116-140809).txt` | 과거 원문 작성 시점의 그래프 식별자. 현재 에셋보다 우선하지 않음 |
| R1 | `Toon_Shader_공부.pdf`, 160페이지 | 참고 구현의 원리와 원본 Asset 분석, 프로젝트 완료 증거와 구분 |
| P1 | `두 번째 프로젝트 기획서.pdf`, 9페이지 | 두 번째 프로젝트 목표와 이전 프로젝트의 관계 |
| P2 | `Real-time Virtual Performer Studio 기획서 (Bounty Hunter 에셋 사용)(4).pdf` | 기존 캐릭터 및 운영 시스템의 기획 |
| I1 | `내가_한_것들.pdf`, 87페이지 | 첫 번째 프로젝트에서 구현한 Runtime Appearance와 MID Cache 등 |
| V1 | `image(20261117-131539).png` | 기존 Asset의 5가지 색상 조합 홍보 이미지 |
| V2 | `image(20261117-131733).png` | 실제 Studio 실행 화면, Toon 선택 상태와 Operator UI |
| C1 | 이 채팅의 작업 및 수정 대화 | Mesh 수정 완료, Rim 수정 결과, Texture 제작 요구와 후속 안내 구분 |

G1의 날짜 이후 대화에서 제안한 변경은 자동으로 G1에 적용되었다고 간주하지 않음


## 1 제작 목표 및 기획 의도

### 1.1 기존 Bounty Hunter의 시각적 특성

기존 Bounty Hunter는 이미 스타일화된 비율과 의상을 가진 캐릭터 Asset

원본을 완전한 실사 캐릭터라고 설명하는 것은 부정확

V1에서 확인되는 특징

- 가죽 의상과 금속 장식의 재질 차이
- 부드럽게 이어지는 명암과 표면 하이라이트
- 머리카락의 가는 결 표현
- 얼굴, 피부, 의상 Texture에 포함된 색상과 세부 묘사
- 머리카락과 의상 색을 바꾼 5가지 외형 예시

목표는 기존 디자인을 버리는 것이 아니라, **기존의 연속적인 재질 표현을 경계가 분명하고 형태가 읽히는 Toon 표현으로 재해석**하는 것

원본 Asset의 홍보 이미지는 제작자가 제공한 자료이며, 사용자의 작업 결과 이미지로 표기하지 않음

### 1.2 Toon Shader를 제작한 이유

첫 번째 프로젝트의 중심은 캐릭터를 실시간으로 운영하는 기능

- Animation 상태와 Gesture 제어
- Camera 구도와 추적
- Lighting 전환
- Appearance Parameter 및 Preset
- Operator UI와 Runtime 상태 연동

두 번째 프로젝트는 같은 캐릭터를 바탕으로 **캐릭터의 표현 자체를 개선하는 Rendering 작업**을 추가하는 방향

P1의 Rendering 목표

- Normal과 Light Direction에 따른 명암 계산
- 명암 경계와 색상 조절
- 피부와 헤어 등 부위별 표현 규칙
- 기존 표현과 Stylized 표현 비교

기획서의 `3단계 Toon Ramp`, 실제 그림자 검증, 모든 Parameter의 실시간 제어는 각각 구현 증거를 확인해야 하며 기획서에 적혔다는 이유만으로 완료 항목에 포함하지 않음

### 1.3 목표 그래픽 스타일

- 얼굴: 피부색을 유지하면서 코와 볼의 복잡한 굴곡에 끌려가지 않는 정돈된 그림자
- 헤어: 가는 Hair Card 결보다 덩어리와 띠 형태 하이라이트가 읽히는 표현
- 눈: 원본의 홍채와 동공 제어를 유지하면서 포스터에 가까운 스타일화된 색상과 반짝임
- 의상: 기존 색 배치와 디자인을 유지하면서 명암 대비 정리
- 실루엣: Outline과 Rim으로 캐릭터 외곽 강조

특정 상용 게임의 Rendering을 완전히 재현했다고 주장할 근거는 없음

기준은 제공된 포스터와 `Toon_Shader_공부.pdf`의 표현 방향을 Bounty Hunter에 맞추는 것

### 1.4 기존 캐릭터 및 연출 시스템과의 관계

| 재사용한 기반 | Toon 작업에서의 의미 |
| --- | --- |
| Bounty Hunter 몸과 얼굴, 의상 | 디자인과 기본 Mesh를 유지하고 Material과 일부 Texture 변경 |
| 기존 Animation 환경 | 캐릭터를 실제 운용 화면에서 확인하는 기반 |
| Camera와 Stage | Full / Upper / Close 등 거리와 구도에서 표현 확인 가능 |
| 기존 Appearance 시스템 | 색상 Parameter와 여러 Material Slot을 연동했던 기반 |
| Operator UI | V2에서 Original / Toon 버튼과 색상 Preset UI 확인 |

V2는 Toon 캐릭터가 기존 Studio 환경 안에서 표시되는 근거

다만 **기존 Appearance 시스템을 Toon에 완전히 호환시켰다는 증거와는 다름**

I1에서는 피부색 Parameter가 `SkinHue_Global`, `SkinDepth_Global`인데 G1은 `SkinHue`, `SkinDepth` 사용

따라서 기존 Blueprint의 Parameter 이름 갱신이나 별도 매핑 여부는 확인 필요

근거: P1 1~2페이지, P2 1~4페이지, I1 19~20페이지 및 86~87페이지, V1, V2

## 2 기존 Asset 분석

### 2.1 원본 Master Material과 Material Instance

P2의 제공 Asset 정보에는 Master Material 5개와 Material Instance 31개가 기록됨

실제 Master Material의 역할은 R1에서 기존 그래프를 대조한 내용으로 확인

| 원본 Material | 주요 역할 | 확인된 특징 |
| --- | --- | --- |
| `M_Hair` | 원본 헤어 표현 | `MakeMaterialAttributes`, Hair 색상, Opacity Mask, Normal, Pixel Depth Offset 등 |
| `M_Face` | 얼굴 피부와 눈썹, 입술 | Base Color와 Mask를 조합하고 피부색 및 눈썹, 입술을 별도 제어 |
| `M_Eyes` | 홍채와 흰자, 눈 표현 | 색상 합성 외에 기존 UV 보정 및 Emissive 관련 경로 존재 |
| `M_Cloth` | 의상 색상과 표면 표현 | Base Color, Normal, ORM과 Mask, Hue / Depth Parameter |
| `M_Body` | 몸 피부와 손톱 | 피부색 공통 Parameter와 손톱 영역 분리 |

31개 MI가 어떤 부모 계층과 최종 Override를 가지는지는 목록 전체가 확보되지 않아 확인 필요

원본의 5개 Master Material을 모두 새로 작성한 것으로 발표하지 않음

### 2.2 원본 Texture와 채널 역할

| Texture | 역할 | Toon 작업에서 주의한 의미 |
| --- | --- | --- |
| `T_Head_BC` | 얼굴 기본 색상과 디테일 | Toon 명암을 적용해도 기존 Texture의 명암이나 하이라이트가 자동으로 없어지지 않음 |
| `T_BodyModular_BC01` | 몸 기본 색상 | 얼굴과 몸의 피부색 일관성 확인 대상 |
| `T_Cloth_BC` | 의상 기본 색상 | 원본 디자인과 배색 유지 대상 |
| `T_Head_N`, `T_Body_N`, `T_Cloth_N` | 표면 방향의 세부 정보 | 명암 경계에 영향을 주므로 미세결이 강하면 Toon 표현도 복잡해질 수 있음 |
| `T_Head_ORM`, `T_BodyModular_ORM`, `T_Cloth_ORM` | R: AO, G: Roughness, B: Metallic | 원본 PBR 계산용 정보, 최신 Toon의 최종 출력에 그대로 사용한다고 가정하지 않음 |
| `T_Head_Mask` | 얼굴 영역 구분 | R: 눈썹 색상, G: 입술 첫 색상 및 거칠기, B: 입술 두 번째 색상 계산 |
| `T_ClothNails_Mask` | 여러 Material에서 공유하는 영역 정보 | R: 의상 색상, G: 원본 눈 Emissive 계산, B: 손톱 관련 계산 |
| `T_Hair_Alpha` | 원본 헤어 결과 불투명 영역 | 제공 PNG는 흑백 무늬가 RGB에 있고 A는 전부 255, 원본 그래프는 R 사용 |
| `T_Hair_Depth` | 원본 헤어 깊이 관련 데이터 | 이름만으로 최종 기능을 단정하지 않고 기존 연결 확인 |
| `T_Iris`, `T_Sclera` | 홍채와 흰자 색상 | 눈 UV와 원형 비율 보존 대상 |
| `T_Eyes_N`, `T_Eyes_N02` | 기존 눈 방향 계산 및 Normal | 눈 계산을 통합하면서 기존 데이터 재사용 |

**Texture 이름보다 실제 채널 내용과 그래프 연결을 기준으로 판단**

`Alpha`라는 이름이 있어도 A 채널을 사용해야 한다는 뜻은 아님

### 2.3 유지한 요소와 변경한 요소

| 구분 | 내용 |
| --- | --- |
| 유지 | Bounty Hunter의 몸과 얼굴, 의상 디자인, 기존 UV를 사용하는 주요 Texture 구조 |
| 재사용 | 얼굴의 눈썹 및 입술 마스크, 몸의 손톱 마스크, 기존 눈 UV 및 색상 계산 |
| 새로 구성 | 공용 `M_Toon`, Normal 명암, 얼굴 SDF 통합, 헤어 하이라이트, Rim 합성 |
| 변경 | Toon 색감에 맞춘 Base Color, 입체형 헤어와 UV, 헤어 밝기 Texture |
| 분리 | 본체 `M_Toon`과 Outline용 `M_Outline` |
| 일부 검토 | 미세 디테일을 완화한 Normal 및 Mask 파생 Texture |

생성된 Normal 파생 Texture의 최종 적용은 일괄 확인되지 않음

G1의 기본 `Normal` Texture는 여전히 원본 `T_Head_N`을 가리키며, `FaceMask`는 `T_Head_Mask_Toon`을 가리킴

이 값도 Master의 기본 참조이며 실제 MI Override와 구분

근거: R1 2~6페이지, G1 노드 6, 37, 75, 97, 146~148, 167, 213, 223

## 3 실제 제작 과정과 최종 구조

### 3.1 현재 M_Toon 출력 구조

현재 저장된 출력은 0.2절의 실제 Expression 경로를 따른다. 부위 색상 → Shadow/Highlight → Hair Highlight 선택 → BaseColorPower → RimLight? → LightColor.rgb → OverallHue.rgb → LightIntensity → Emissive 순서다. OverallBrightness는 현재 Master 출력 경로에 없다.

Surface/Opaque/Default Lit/단면 설정을 직접 조회했다. Base Color는 0, Normal은 별도 분기 입력, Opacity Mask와 WPO는 미연결이다. M_Toon 자체에 Outline 확장 경로가 없다. Emissive 중심 구성과 Unlit 설정은 같은 뜻이 아니다.

Named Reroute와 MI 스위치 선택까지 확인한 경로만 현재 구현으로 기술하며, 전체 엔진 Rendering Pass를 설명한 그림으로 일반화하지 않는다.

### 3.2 셀 셰이딩과 일반 부위 명암

#### 구현 목적

표면 방향에 따른 연속적인 밝기 변화를 경계와 범위를 조절할 수 있는 명암으로 변환

#### 실제 계산

현재 일반 명암은 `PixelNormalWS`와 `-MPC_Toon.LightDir`의 내적을 사용한다. LightDirection Named Reroute를 따라 최종 출력 도달을 확인했다.

과거 Custom 노드에는 아래 코드가 남아 있지만 **현재 출력에는 연결되지 않는다**. 이 코드를 현재 광원 공급 방식으로 소개하지 않는다.

```hlsl
return View.DirectionalLightDirection; // 현재 미사용 Custom 노드
```

설명용 수식

```text
q = (Dot(PixelNormalWS, (-MPC_Toon.LightDir.rgb)) + 1) × 0.5

GeneralLit = SmoothStep(
    ShadowRange - ShadowSmooth,
    ShadowRange + ShadowSmooth,
    q
)

ShadowedColor = Lerp(
    BasePartColor × ShadowColor,
    BasePartColor,
    DrawShadow
)
```

- Dot: 표면 방향과 광원 방향 관계를 숫자로 변환
- Remap: -1~1을 0~1 범위로 옮김
- `ShadowRange`: 그림자 경계 위치
- `ShadowSmooth`: 경계 주변 전환 폭
- `ShadowColor`: 기본색에 곱할 그림자 색상 배율

`DrawShadow`는 이름과 달리 **1이 밝은 영역, 0이 그림자 영역**

이 규약은 얼굴 SDF, 고정 그림자 합성, 코 하이라이트 차단을 연결할 때 중요

일반 Highlight는 `1-q`를 별도 `SmoothStep`으로 처리하고 `HighlightColor`가 곱해진 색상과 일반 색상을 선택

현재 구조를 정확히 표현하면 **그림자와 하이라이트의 경계를 독립적으로 조정하는 Toon 명암 합성**

기획서의 3단계 Ramp를 그대로 구현한 전용 Ramp Texture나 정확히 5단계 명암을 구현했다고 설명하지 않음

5가지 외형 색상과 명암의 단계 수는 별개

#### Normal 제어

일반 부위는 평평한 Tangent Normal `(0,0,1)`과 Normal Texture를 `NormalStrength`로 혼합한 뒤 Normalize

`NormalMipBias`는 Normal Texture 샘플의 Mip Bias 입력에 연결

헤어는 별도 고정 Normal 경로, 눈은 원본 눈 Normal 경로를 사용

Parameter가 존재한다는 사실과 실제 MI에서 어느 값을 사용했는지는 구분

근거: G1 노드 11~41, 101, 229, Custom 0

### 3.3 얼굴 SDF Texture 제작과 적용

#### Normal 명암과 얼굴 SDF를 분리한 이유

얼굴은 코, 볼, 입과 눈 주변의 굴곡이 모여 있어 일반 Normal 명암만 쓰면 원하는 단순한 얼굴 그림자와 차이가 날 수 있음

얼굴의 모든 Normal을 원하는 그림자 모양에 맞게 수정하는 대신, Texture로 그림자 경계의 이동 형태를 설계하는 방향 선택

이는 얼굴을 물리적으로 정확하게 밝히는 작업보다 **캐릭터에 어울리는 그림자 형태를 조절하는 작업**에 가까움

#### 실제 자료 제작 과정

확인되는 작업 기록

1. 원본 포스터의 얼굴 특징과 피부색을 유지하도록 편집 조건 지정
2. 광원을 정면 0도, 화면 오른쪽 90도로 정의
3. 0~90도 사이 그림자 단계를 순차 제작하도록 요청
4. 단계 사이 변화가 부족한 구간은 중간 그림자 이미지 추가 요청
5. 90~180도 구간도 12단계 제작 요청
6. 좌우 및 반대 방향 얼굴 그림자 자료와 SDF Texture 생성 및 검증 요청
7. 얼굴에만 적용하기 위한 `FaceShadowMask` 제작 요청
8. 최종 확보 그래프에서는 `T_SDF_Left`, `T_SDF_Right`, `T_FaceShadowMask`가 사용됨

각도별 이미지 세트를 12장 모두 동일 조건으로 검증했는지, 이미지에서 어떤 수치 알고리즘으로 SDF를 생성했는지, 최종 Texture가 엄밀한 Signed Distance Field인지까지는 확인 필요

**각도별 그림자 형태를 설계하고 Threshold Texture로 활용한 작업**은 설명 가능

**정확한 거리 변환 알고리즘이나 자체 SDF 생성기를 구현했다는 주장**은 근거 부족

#### PDF 방식에서 바뀐 점

| 항목 | PDF의 참고 방식 | G1의 실제 방식 |
| --- | --- | --- |
| SDF 입력 | 한 장의 `SDFTexture` | `T_SDF_Left`, `T_SDF_Right` 두 장 |
| 좌우 구분 | 원본 UV와 U 반전 UV 선택 | 좌우 Texture 결과를 선택 |
| 좌우 경계 | Step | Dot 결과 -0.1~0.1에서 SmoothStep |
| Threshold 경계 | Step 중심 | Power와 SmoothStep |
| 적용 범위 | 얼굴용 분기 | `FaceShadowMask`와 `IsFace?`를 함께 사용 |
| 위아래 광원 보정 | `HeadUp` 경로 존재 | G1에는 없음 |
| 얼굴 그림자 강도 | `FaceShadowIntensity` | G1에는 해당 Parameter 없음 |

좌우 Texture 방식은 대칭 반전에 의존하지 않고 각 방향의 형태를 따로 지정할 수 있는 구조

다만 이 방식의 성능상 이점은 측정하지 않음

#### 최종 SDF 계산

다음 핵심 SDF 연결은 현재 에셋에서도 확인했다. 광원 공급과 기본 그림자 결합은 과거 G1 설명을 수정한 0.3절을 우선한다.

현재 확인 순서

1. `HeadForward`, `HeadRight`를 `MPC_Toon`에서 읽음
2. 머리 방향과 LightDirection(-MPC_Toon.LightDir)의 XY만 추출
3. 각각 Normalize 후 Dot Product
4. HeadRight 내적으로 좌우 Texture 선택 Alpha 계산
5. HeadForward 내적을 0~1로 변환하고 반전하여 Threshold 생성
6. 선택된 SDF 값에 `Power = 0.8` 적용
7. Threshold 주변 `SDF_Smooth` 범위로 SmoothStep
8. `FaceShadowMask` 안에서는 SDF, 밖에서는 일반 명암 사용
9. Lerp 결과에 `(1 - T_DefaultShadow.R)`를 곱한 뒤 `IsFace?`를 통해 얼굴 분기에 적용

실제 연결을 축약한 설명용 수식

```text
RightDot = Dot(Normalize(LightDirection.xy), Normalize(HeadRight.xy))
SideBlend = SmoothStep(-0.1, 0.1, RightDot)
SelectedSDF = Lerp(T_SDF_Left.R, T_SDF_Right.R, SideBlend)

ForwardDot = Dot(Normalize(LightDirection.xy), Normalize(HeadForward.xy))
Forward01 = 1 - (ForwardDot × 0.5 + 0.5)

SDFLit = SmoothStep(
    Forward01 - SDF_Smooth,
    Forward01 + SDF_Smooth,
    Power(SelectedSDF, 0.8)
)

FaceLit = Lerp(GeneralLit, SDFLit, FaceShadowMask.R)
DrawShadow = IsFace? ((1 - T_DefaultShadow.R) * FaceLit) : GeneralLit
```

`LightDirection`은 현재 MPC_Toon.LightDir에 -1을 곱한 Named Reroute 값이다. MPC의 실제 writer와 부호가 물리적 광원 방향에 어떻게 대응하는지는 추가 Graph·화면이 필요하다.

빛이 진행하는 방향과 표면에서 빛을 향하는 방향을 임의로 같은 뜻으로 바꾸지 않음

정면광에서 실제 흑백 결과를 확인하고 판단하는 방식 필요

#### 적용 범위 마스크가 필요한 이유

`IsFace?`는 Material 경로를 선택하지만 같은 Material에 묶인 영역 중 얼굴 피부만 정밀하게 구분하는 공간 마스크는 아님

`FaceShadowMask`는 피부 얼굴에만 SDF가 적용되도록 범위를 지정

마스크 밖의 귀, 목 등은 일반 명암 경로를 유지하는 구조

실제 마스크가 어떤 영역까지 흰색으로 포함하는지는 최종 Texture 원본과 Mesh UV에서 확인 필요

#### Blueprint 연동에서 확인되는 범위

현재 확인한 BP_BountyHunter.UpdateToonMPC는 `Head` Bone의 World Transform을 읽고 Rotation의 Right Vector를 HeadForward, Up Vector를 HeadRight로 쓴다. Toon 모드 Branch.True에서 Tick 호출된다. Forward Vector를 HeadUp으로 쓰는 노드도 있으나 현재 MPC 정의에 HeadUp이 없어 유효한 상하 보정으로 기록하지 않는다.

MPC LightIntensity는 ApplyCharacterLighting의 Scalar setter로 전달된다. LightDir/LightColor의 최종 writer와 Level Blueprint는 확인 필요다. 자세한 핀 경로와 접근 제한은 0.6절 참조.

근거: 현재 BP_BountyHunter.t3d 및 M_Toon.t3d. R1/G1은 과거 참고 기록이다.

### 3.4 고정 얼굴 그림자와 SDF 합성

#### 원래 요구와 제안 기록

기본 얼굴 그림자를 유지하면서 SDF 그림자가 넓어지는 부분을 추가하려는 요구가 있었고, 과거에는 아래 Min 방식을 제안했다. 이는 제안의 수학적 설명으로 보존하며 현재 적용 수식은 아니다.

```text
ProposedLit = Min(ExistingFaceLit, 1 - T_DefaultShadow.R)
```

#### 현재 실제 연결 — Min이 아닌 곱셈

```text
FaceLit = Lerp(GeneralLit, SDFLit, FaceShadowMask.R)
CombinedLit = (1 - T_DefaultShadow.R) * FaceLit
DrawShadow = IsFace? CombinedLit : GeneralLit
```

TextureSample_9 → OneMinus_5 → Multiply_39.A, Lerp_16 → Multiply_39.B, 결과 → IsFace? True → DrawShadow → 최종 Shadow 합성으로 이어진다. 현재 Expression 목록에 Min은 없다.

두 방식은 입력이 0/1인 일부 경우에는 같지만 중간값에서 다르다. 예를 들어 0.5와 0.5의 Min은 0.5, 곱은 0.25다. 따라서 Min 합성을 적용했다고 소개하면 틀린다. 현재 T_DefaultShadow는 sRGB=true Color Sample이다. 마스크 원본 극성과 중간값의 시각 결과는 별도 확인해야 한다.

FaceShadowMask는 앞의 SDF/일반 명암 선택 범위를 제어하며, 고정 그림자 곱셈은 그 뒤 전체 IsFace? 분기에 적용된다. 실제 귀·목까지 어떻게 보이는지는 Texture와 Mesh UV 화면으로 확인한다.

### 3.5 코 하이라이트

#### 구현된 부분

- `T_Nose_Highlight_Left.R`, `T_Nose_Highlight_Right.R` 읽기
- HeadRight와 광원 방향 내적을 Step으로 구분
- Lerp로 좌우 마스크 선택
- `NoseHighlightMask`로 재사용
- `OneMinus` 후 얼굴용 `DrawHighlight` 경로에 연결
- `FaceShadowMask`로 적용 범위 제한

일반 Normal Highlight만으로 코의 밝은 점 위치와 형태를 결정하지 않고 Texture로 모양 지정

`OneMinus`가 들어간 이유는 흰색 Nose 마스크와 기존 Highlight 선택 마스크의 극성이 다르기 때문

현재 일반 Highlight 합성은 Alpha가 낮을수록 Highlight 색상을 선택하는 구성

#### 미완료 부분

과거 G1 문서는 `NoseHighlightMask × DrawShadow` 노드 239가 미연결이라고 기록했다. 현재 Export에서는 해당 곱셈 경로가 확인되지 않고 OneMinus_7이 NoseHighlightMask Usage_8을 직접 받는다.

현재 OneMinus_7도 원래 NoseHighlightMask를 직접 사용한다. 현재 코 좌우 선택은 HeadRight 내적에 -0.025를 더한 뒤 Step_0으로 선택한다.

따라서 **현재에도 DrawShadow를 이용한 코 하이라이트 차단은 최종 색상 경로에 없다**. 이것이 실제 화면에서 어떤 문제를 만드는지는 실행 비교를 하지 않아 단정하지 않는다.

이후 수정 제안은 있었으나 완료 증거가 없어 개선 항목으로 유지

근거: G1 노드 137~139, 234~244

### 3.6 헤어 Mesh와 UV 수정

#### 형태 제작과 수정

기존 헤어를 그대로 사용하는 데 그치지 않고 Tripo에서 얻은 입체형 헤어를 바탕으로 Maya 수정 진행

확인되는 사용자 작업

- 헤어 덩어리별 분리
- UV 재전개
- 머리카락 흐름에 맞춘 UV 방향 정렬
- Unreal로 가져와 하이라이트 구현
- 앞머리 삼각형 명암에 `Soften Edge` 적용
- Substance Painter에서 Texture 작업

기여 구분

| 단계 | 제작 주체 |
| --- | --- |
| 헤어 초기 형태 | Tripo AI 생성 결과 활용 |
| 덩어리 분리 및 형태 정리 | 사용자 직접 수정 |
| UV 전개와 방향 정렬 | 사용자 직접 작업 |
| Vertex Normal 정리 | 사용자 직접 `Soften Edge` 적용 |
| Unreal Material 연결 | 사용자 직접 구성, AI 설명 보조 |

처음부터 전부 직접 모델링한 헤어로 표현하지 않음

최종 헤어의 Skin Weight, Bone 연결, 기존 Hair Secondary Motion 연동 완료 여부는 확인 필요

#### UV 정리의 의미

UV는 Texture 위치뿐 아니라 Mesh Tangent 기준과도 연관됨

헤어 결 방향과 UV 방향을 정리한 작업은 Texture 페인팅 위치와 방향성 Highlight의 일관성을 맞추는 데 관련

UV를 정렬한 작업과 Normal을 부드럽게 만든 작업은 서로 다른 수정

- UV: Texture 좌표와 방향 기준
- `Soften Edge`: 인접 면의 Vertex Normal 관계

DiffuseUV 확인과 복제 Mesh UV 표시 문제를 처리한 기록은 있으나, 최종 UV Set 수와 `Current UV Set`, 모든 해결 조작 순서까지는 원본 화면 확인 필요

### 3.7 헤어 방향성 하이라이트

#### 선택한 방식

헤어는 일반 구형 하이라이트보다 머리카락 흐름에 따라 생기는 띠 형태의 반짝임을 목표로 설정

PDF의 이방성 Highlight 계산을 참고해 `M_Toon`의 `IsHair?` 경로에 통합

설명용 수식

```text
FlowAxis = Normalize(Cross(VertexNormalWS, VertexTangentWS))
HalfDirection = Normalize(-MPC_Toon.LightDir.rgb + CameraVectorWS)

HairMask = Power(
    1 - Abs(Dot(FlowAxis, HalfDirection)),
    HairSpecularPower
)

HairResult = Lerp(
    ExistingToonColor,
    ExistingToonColor × HairSpecularColor,
    HairMask
)
```

- Cross: Normal과 Tangent에 수직인 방향 생성
- 광원 방향과 카메라 방향 조합: 보는 각도와 조명에 따라 하이라이트 위치 변화
- Abs: 방향 부호에 대한 대칭 처리
- OneMinus: 수직에 가까운 관계에서 값이 커지도록 변환
- Power: 띠의 집중 정도 조정
- Lerp: 하이라이트 영역에서 색상 배율이 적용된 결과 선택

`Cross(Normal, Tangent)` 결과를 무조건 원본 Tangent라고 부르지 않음

UV와 Tangent에 의해 결정되는 별도의 방향축이며, 실제 모발 방향과 일치하는지는 Mesh에서 검증 필요

완전한 물리 기반 Hair BRDF나 Groom Shader를 새로 구현한 것은 아님

#### 현재 경로와 과거 경계 조절

현재 Power_2가 StaticSwitchParameter_11(IsHair?)의 True 입력을 통해 Lerp_3.Alpha에 직접 연결된다. HairHighlightThreshold/HairHighlightSmooth와 해당 Hair용 SmoothStep은 현재 Expression 목록에서 확인되지 않는다.

G1 당시 “SmoothStep이 남아 있으나 미연결”이라는 기록은 보존하되, 현재에도 그 노드가 존재한다고 설명하지 않는다.

### 3.8 헤어 밝기 Texture

Substance Painter에서 `T_Hair_Value`를 제작하고 연결했다는 사용자 완료 기록 존재

G1의 에셋 참조명은 `T_HairValue`

목적은 머리카락의 기본 색상과 덩어리별 밝기 변화를 분리하는 것

현재 출력 연결(Multiply_31의 B는 R 채널)

```text
HairBase = Hair_Color.rgb × T_HairValue.R
```

흑백 Texture는 공간별 밝기 패턴을 지정하고, `Hair_Color`는 전체 색상 변경에 사용

색을 바꾸어도 동일한 밝기 패턴을 재사용할 수 있는 구조

다만 검은 픽셀은 곱셈 결과가 0이므로 Texture가 완전히 검으면 해당 영역의 기본 색도 검정이 됨

#### 이전 설명과 달랐던 점

G1에는 아래 계산도 존재

```text
Lerp(DarkStrength, LightStrength, T_HairValue.R)
```

현재 LinearInterpolate_12 출력도 어떤 최종 Material 입력에 도달하지 않는다. 과거 G1의 노드 번호 105와 현재 Expression 이름을 구분한다.

따라서 `DarkStrength = 0.55`, `LightStrength = 1.15`가 실제 헤어 밝기 범위를 제어한다고 발표하면 부정확

근거: G1 노드 43~69, 104~108, 사용자 11월 11일 및 13일 완료 기록

### 3.9 눈 표현과 기존 계산 재사용

#### 기본 선택

원본 `M_Eyes`의 계산을 모두 버리기보다 필요한 계산을 `M_Toon`의 `IsEye?` 분기로 가져오는 방향

G1에서 확인되는 구성

1. 카메라 방향과 Vertex Normal로 보는 방향 계산
2. 원본 눈 Normal과 Transform으로 눈의 로컬 방향 기준 구성
3. 시점에 따른 UV Offset 계산
4. `IrisDiameter`로 홍채 좌표 범위 조절
5. `PupilScale`로 동공 주변 UV 변형
6. `T_Iris_Toon`과 `T_Sclera_Toon` 샘플
7. `IrisColor`, `ScleraColor`, `ScleraContrast` 적용
8. 홍채 경계 마스크로 홍채와 흰자 합성
9. `IsEye?`로 눈 색상과 Normal 선택

단순히 눈 Texture를 교체한 작업보다, **원본 Asset의 눈 표현 구조를 유지하며 공용 Toon Material에 통합한 작업**으로 설명 가능

다만 원본 눈 UV 및 시점 보정 알고리즘을 사용자가 처음부터 발명한 것으로 설명하지 않음

#### Texture 제작 방향

- 포스터 여성 캐릭터의 눈 색감과 스타일을 기준으로 `T_Iris`, `T_Sclera` 변형 요청
- 기존 UV와 원형 비율 유지 요구
- 홍채 상부 음영, 외곽의 어두운 링, 하이라이트의 표현 방법 검토
- 실제 색상은 Parameter와 Texture를 조합하는 방향

G1에 Toon Texture 참조가 있으므로 파생 눈 Texture를 사용하는 계산은 확인됨

여러 차례 생성한 파일 중 어느 버전이 최종 에셋과 정확히 같은지는 확인 필요

#### 직접 추가한 것과 재사용한 것

| 항목 | 구분 |
| --- | --- |
| 원본 눈 Normal과 기존 시점 기반 UV 계산 | Asset에서 재사용 |
| `IsEye?`를 통한 공용 Shader 통합 | 프로젝트에서 구성 |
| Toon용 눈 색상 Texture 제작 | AI 보조 생성 및 사용자 선택과 적용 |
| 눈 색상과 대비 조정 | Material Parameter 활용 |
| 시선 추적 Animation이나 Live Link | 이 눈 Material 그래프만으로 완료 판단 불가 |

Material의 `CameraVectorWS` 기반 UV 보정과 캐릭터가 카메라를 바라보는 Look Tracking은 다른 기능

#### 확인된 미사용 출력

`T_ClothNails_Mask.G`, `EmissiveDepth`, `EyeEmissiveColor`를 이용하는 `EyeUnderHighlight` 계산은 존재

현재에도 LinearInterpolate_9 → EyeUnderHighlight Declaration_10이 루트 미도달이다. 과거 G1 번호 233과 현재 이름은 구분한다.

따라서 해당 하이라이트가 최종 눈에 적용된다고 설명하지 않음

실제 눈의 2D Catchlight 마스크나 별도 눈 하이라이트 Mesh 최종 적용 여부도 확인 필요

근거: G1 노드 145~233

### 3.10 피부와 의상 표현

#### 피부

G1 당시 이름은 SkinDepth/SkinHue로 기록되어 있으나 현재는 `SkinDepth_Global`, `SkinHue_Global`이다. Multiply_4 → HueShift → SkinColor → 부위별 색상 경로가 연결된다. BP Parameter 이름과 일치한다.

얼굴은 `T_Head_Mask_Toon`의 채널로 눈썹과 두 종류 입술 색상을 합성

몸은 손톱 마스크 B 채널을 사용해 피부와 손톱색 분리

목표는 Toon 명암을 적용하면서도 눈썹, 입술, 손톱 같은 원본의 색상 제어를 유지하는 것

얼굴과 몸의 최종 피부색이 모든 Preset에서 일치하는지는 전체 비교 이미지 필요

#### 의상

공용 Shader에서 얼굴, 몸, 헤어가 아닌 일반 부위는 기본 Texture를 사용해 일반 Toon 명암으로 들어가는 경로 존재

가죽과 금속에 독립적인 BRDF나 별도 금속 전용 하이라이트를 구현했다는 근거는 없음

기획서의 부위별 재질 표현 목표와 최종 그래프의 공통 Highlight를 구분

현재 IsCloth? 분기에서 ColorDepth_Global/ColorHue_Global 및 Color02_Hue/Color02_Depth의 출력 연결을 확인했다. 이것만으로 원본 M_Cloth의 모든 기능과 런타임 적용이 동일하다고 결론내리지 않는다.

`M_Cloth_Toon`은 사용자가 잘못 적은 항목으로 삭제 요청한 명칭이므로 별도 구현 성과로 기록하지 않음

### 3.11 Base Color 재작업과 파생 Texture

#### 작업 의도

기존 Texture의 실사풍 세부 묘사와 기존 명암이 Toon Shader 위에 남으면, Shader의 명암과 겹쳐 결과가 복잡해질 수 있음

기존 UV를 유지한 채 포스터의 색상 설계에 가까운 스타일화 Texture 제작 요청

최초 요구는 강한 음영과 하이라이트를 Texture에 넣지 않고 Material에서 처리하는 방향

이후 지나치게 평평한 결과에 대해 최소한의 음영은 필요하다고 수정

이 변화는 명암을 전부 제거하는 일보다 **기본색의 형태 정보와 동적 셰이딩의 역할을 조절한 과정**으로 설명

#### 제작 기록

- 얼굴, 몸, 의상 Base Color의 Toon 스타일 변환
- 얼굴과 몸 피부톤 일관성 요구
- 회색 헤어가 포함된 Head Texture 변형 요청
- 눈 Texture 2종 변형
- Normal의 미세 디테일 완화와 Mask 채널 유지 파생본 생성 기록
- ORM은 불필요하다고 판단하여 추가 제작 범위에서 제외

생성 파일의 해상도, UV의 픽셀 단위 보존, 모든 RGB Mask 채널의 완전 일치 여부를 이 문서에서 재검증한 것은 아님

AI 생성 이미지를 바로 정확한 게임 데이터라고 간주하지 않음

최종 사용 버전과 Unreal 설정이 확인되는 파일만 최종 산출물로 표기할 필요

### 3.12 Outline

현재 별도 `/Game/ToonShader/Materials/M_Outline`의 출력 연결과 MI_Outline/MI_FaceOutline/MI_HairOutline 설정을 직접 조회했다. Masked/Unlit/양면이며 TwoSidedSign×-1 → Opacity Mask, OutlineColor → Emissive, VertexNormalWS×보정 두께 → WPO다.

거리 보정은 `Saturate(Clamp(Distance * DistanceScale, OutlineThickness, 3.0) * OutlineThickness)`로 연결된다. Master에는 같은 이름 OutlineThickness 노드 두 개의 기본값이 다르다. 조회한 Outline MI의 유효값은 Thickness=0.3, DistanceScale=0.0015다. 자세한 기본값·Override는 0.7절을 따른다.

원문의 거리 보정 완료 발언과 제작 과정은 과거 기록으로 유지한다. 현재 확인한 것은 Shader 연결과 저장 설정이며, 실제 추가 Mesh의 애니메이션 추종·Material 할당, 헤어 외곽선과 FOV별 두께는 아직 화면 검증하지 않았다. 일정 픽셀 두께나 성능 이점은 보장하지 않는다.

### 3.13 Rim Light

#### 문제와 최종 선택

초기 안내는 흰 Rim 색상을 기존색에 더하는 방식이었으나, 머리카락 외곽이 회색으로 떠 보이는 결과 발생

사용자가 PDF 방식으로 바꾼 뒤 결과가 좋아졌다고 확인

최종 G1은 PDF와 같은 기본 합성 원리 사용

```text
RimMask = Step(Fresnel, 0.5)
RimColorResult = ExistingColor × RimLightColor.rgb
RimResult = Lerp(ExistingColor, RimColorResult, RimMask)

FinalColor = RimLight? RimResult : ExistingColor
Emissive = FinalColor × MPC_Toon.LightColor.rgb × OverallHue.rgb × MPC_Toon.LightIntensity
```

위 Step 표기는 Unreal Material의 핀 순서 기준이며 `X = Fresnel`, `Y = 0.5`, `X >= Y`일 때 1

#### 왜 결과가 달라졌는가

기존색에 흰색을 더하면 RGB 각 채널에 같은 양이 추가되어 원래 색상의 상대적인 차이가 작아질 수 있음

최종 방식은 기존색에 배율을 곱한 값을 Rim 영역에서 선택

`RimLightColor`가 같은 RGB 배율이면 입력 색상의 채널 비율을 곱셈 단계에서 유지

다만 이후 HDR 출력, 노출, Tonemapping, Bloom에 따라 화면 색상은 다시 달라질 수 있으므로 색 보존을 무조건 보장하는 공식은 아님

#### 기능의 정확한 의미

Fresnel은 시선 방향과 표면 방향의 관계를 이용

현재 Rim은 광원의 뒤쪽 위치나 실제 역광 차폐를 계산하지 않음

**시점에 따른 실루엣 강조 효과**로 설명하는 것이 정확

G1의 기본값

| Parameter | Master 기본값 |
| --- | --- |
| `RimeLightRange` | 0.03 |
| `RimLightRange2` | 1.43 |
| `RimLightColor` | RGB 각각 50 |
| Step Y | 0.5 |

`RimeLightRange`는 현재 파일의 실제 오탈자 포함 이름

일부 저장 MI의 유효값은 현재 0.4절에서 확인했다. 런타임 MID 값과 화면 결과는 별도 확인 필요다.

근거: R1 152~160페이지, G1 노드 245~271, C1의 PDF 방식 수정 결과 확인

### 3.14 속눈썹

원문에는 사용자가 마스크를 직접 그렸다는 기록이 있으며 제작 주체는 그 기록으로 유지한다. 이번 분석에서 원본 제작 파일이나 과거 완료 대화를 재열람한 것은 아니다.

현재 실제 이름은 M_Eyelashes와 MI_Eyelashes_Toon이다. M_Eyelashes는 Masked/Unlit/양면이다. T_Eyelashes_Mask의 RGB 출력이 Opacity Mask에 연결되며 EyelashesColor가 Emissive에 연결된다. 마스크는 sRGB=false / Masks, 4096×4096이다.

과거 안내의 M_Eyelashes_Toon/EyelashColor/R 입력을 현재 에셋 이름·연결과 구분한다. Blueprint 기본 CharacterMesh0의 Slot 1은 원본 MI_Eyelashes이므로 Toon 속눈썹이 실제 플레이 캐릭터에 적용됐다는 결론은 추가 Mesh/모드 전환 후 Slot 목록과 확대 화면을 받은 뒤 내린다.

## 4 기술적 문제 및 해결 과정

> 이 절의 문제 현상·의도·완료 발언은 원문의 과거 작업 기록이다. 현재 Graph에서 확인된 연결은 0절 및 수정된 3절을 우선하며, 시각적 해결 결과는 이번에 재실행하지 않았다.

### 4.1 원본 Texture와 Toon 명암의 중첩

| 항목 | 내용 |
| --- | --- |
| 문제 현상 | 원본 Texture의 세부 음영이 남아 Toon 스타일로 정리하려는 방향과 차이 발생 |
| 발생 원인 | Material에서 명암을 나누어도 Base Color에 그려진 세부 묘사는 그대로 유지 |
| 시도한 방법 | 음영과 하이라이트를 최대한 제거한 스타일화 Texture 생성 요청 |
| 최종 선택 방향 | UV와 배색을 유지하면서 최소한의 형태 음영은 허용하도록 요구 수정 |
| 해결 결과 | Texture 변형본 제작과 Toon Texture 참조는 확인, 모든 부위의 최종 품질은 추가 비교 필요 |
| 얻은 이해 | Texture의 고정 정보와 Shader의 동적 명암을 나누어 설계해야 원하는 스타일을 조절 가능 |

강한 무음영 Texture가 항상 최종 목표에 더 적합한 것은 아니라는 판단 과정

### 4.2 얼굴 굴곡에 종속되는 그림자

| 항목 | 내용 |
| --- | --- |
| 문제 현상 | 일반 Normal 기반 얼굴 그림자가 원하는 단순한 형태와 다름 |
| 발생 원인 | 얼굴의 세부 Normal이 광원 내적 결과에 직접 반영 |
| 시도한 방법 | PDF의 단일 SDF와 반전 UV 방식 학습, 각도별 그림자 형태 설계 |
| 최종 구현 | 좌우 SDF 두 장, HeadForward / HeadRight 비교, Power와 SmoothStep, FaceShadowMask |
| 해결 결과 | 얼굴 전용 그림자 계산이 공용 명암에 연결된 구조 확인 |
| 얻은 이해 | 표면 Normal의 명암과 Texture로 설계한 얼굴 명암은 다른 제어 방식이며, 적용 범위 마스크가 별도로 필요 |

완전한 360도 및 상하 광원 안정성까지 검증되었다는 의미는 아님

### 4.3 SDF의 밝음과 그림자 방향 혼동

| 항목 | 내용 |
| --- | --- |
| 문제 현상 | 흑백 마스크에서 기대한 밝음과 어둠 방향이 반대로 해석될 가능성 |
| 발생 원인 | SDF Texture, Threshold, Step 출력, Lerp Alpha의 규약이 서로 다름 |
| 시도한 방법 | 마스크 미리보기와 OneMinus를 이용한 방향 수정 검토 |
| 최종 확인 내용 | G1의 `Forward01`에 OneMinus 존재, `DrawShadow`는 1이 밝음, 0이 그림자 |
| 해결 결과 | 현재 연결의 극성은 추적 가능, 과거 어느 한 노드가 유일한 원인이었는지는 원본 전후 자료 확인 필요 |
| 얻은 이해 | 이름보다 수식과 최종 Lerp의 A / B를 따라가야 마스크 의미를 정확히 판단 가능 |

최신 파일은 단순히 SDF 결과 마지막을 반전한 구조와 다르므로, 이전 안내 문장을 최종 구조로 복사하지 않음

### 4.4 앞머리에 삼각형 명암이 드러남

| 항목 | 내용 |
| --- | --- |
| 문제 현상 | 뒷머리와 달리 앞머리에서 삼각형 면 단위 명암이 보임 |
| 발생 원인 | `Soften Edge`로 해결된 결과에 비추어 면 사이 Vertex Normal 불연속이 직접 관련된 것으로 판단 |
| 시도한 방법 | 헤어 UV 방향 정리 후에도 앞머리 문제를 별도로 확인 |
| 최종 해결 | Maya에서 앞머리에 `Soften Edge` 적용 |
| 해결 결과 | 사용자가 문제 해결 확인 |
| 얻은 이해 | Material 수식 문제처럼 보이는 현상도 Mesh Normal 문제일 수 있으며, UV 수정만으로 모든 명암 문제가 해결되지는 않음 |

정확히 어떤 Hard Edge나 Import 옵션이 최초 원인이었는지는 확인 필요

`Soften Edge`는 화면 미리보기 전용 효과가 아니라 Vertex Normal 정보를 바꾸는 작업

### 4.5 Rim이 회색으로 떠 보임

| 항목 | 내용 |
| --- | --- |
| 문제 현상 | 주황색 헤어 외곽에 회백색 빛이 나타나 색이 따로 노는 느낌 |
| 발생 원인 | 이전 Add 합성과 PDF 방식의 차이, 흰색을 더하는 과정에서 색 대비가 줄 수 있음 |
| 시도한 방법 | Rim 강도를 0으로 해 원인 구분, 주황색 Rim과 경계값 조절 안내 |
| 최종 해결 | 사용자가 PDF 방식으로 수정, 기존색에 RimLightColor를 곱하고 Lerp로 선택 |
| 해결 결과 | 사용자 개선 확인, G1에서 해당 최종 연결 확인 |
| 얻은 이해 | 같은 Fresnel 마스크라도 Add와 Multiply / Lerp의 색 합성 방식에 따라 결과가 달라짐 |

Outline이 함께 영향을 주었는지와 노출의 정량적 기여까지 분리 측정된 것은 아님

따뜻한 Rim 색상 제안은 최종 채택 방식으로 기록하지 않음

### 4.6 광원 색상 곱셈의 float3와 float4 오류

> 현재 대조: 현재 출력은 LightColor RGB Mask 뒤에 OverallHue.rgb와 LightIntensity까지 곱한다. 과거 에러 문구와 당시 수정 경로는 작업 기록이며 현재 전체 컴파일 성공을 뜻하지 않는다.

| 항목 | 내용 |
| --- | --- |
| 문제 현상 | 그래프에 `Arithmetic between types float3 and float4 are undefined` 오류 기록 |
| 발생 원인 | RGB 색상과 RGBA 벡터의 차원 불일치 |
| 시도한 방법 | MPC Vector의 RGB만 추출하도록 Component Mask 연결 |
| 최종 연결 | 노드 257 `LightColor` → 노드 271 RGB Mask → 노드 270 Multiply |
| 해결 결과 | 수정 연결은 확인, 내보낸 텍스트에는 ErrorMsg가 남아 있어 에디터 Apply / Compile 성공 여부 확인 필요 |
| 얻은 이해 | Vector Parameter의 전체 출력과 RGB 출력은 타입이 다르며 실제 핀의 성분 수를 맞춰야 함 |

오류 문구가 남았다는 사실만으로 현재도 컴파일 실패라고 단정하지 않음

### 4.7 코 하이라이트가 그림자에서 제거되지 않음

> 현재 대조: 현재도 Shadow 차단 경로는 없지만 G1의 미연결 Multiply가 지금도 존재한다는 뜻은 아니다. 0.3절 및 수정된 3.5절을 우선한다.

| 항목 | 내용 |
| --- | --- |
| 문제 현상 | 그림자에 들어간 코 하이라이트를 지우려는 의도가 최종 계산에 반영되지 않은 상태 |
| 발생 원인 | `NoseHighlightMask × DrawShadow` 출력이 미사용 |
| 시도한 방법 | 곱셈 결과를 기존 OneMinus 입력으로 연결하도록 수정 안내 |
| 최종 해결 | 적용 완료 확인 필요 |
| 해결 결과 | 현재 문서에서는 미완료로 분류 |
| 얻은 이해 | 노드가 그래프 안에 존재해도 출력이 연결되지 않으면 결과에 영향을 주지 않음 |

현상이 나타난 전후 실행 비교가 없어 실제 화면 오류를 정량적으로 확인한 사례로 과장하지 않음

### 4.8 고정 얼굴 그림자 보존

> 현재 대조: 현재에는 T_DefaultShadow가 실제 연결되어 있고 Min이 아니라 곱셈이다. 아래 제안·당시 미확정 기록은 과거 과정이며 현재 결론은 3.4절을 따른다.

| 항목 | 내용 |
| --- | --- |
| 문제 현상 | SDF가 작아지더라도 코와 입 주변의 기본 그림자를 남기고 싶다는 요구 |
| 발생 원인 | 두 마스크를 단순 교체하면 기본 그림자가 SDF 결과에 의해 사라질 수 있음 |
| 시도한 방법 | 밝은 영역 마스크의 규약을 맞춘 뒤 Min 합성 안내 |
| 최종 해결 | `Min(ExistingFaceLit, 1 - DefaultShadow.R)` 적용 제안 |
| 해결 결과 | 실제 적용 완료 확인 필요 |
| 얻은 이해 | 전체 면적 비교 없이 픽셀별 Min / Max로 원하는 영역 합성 규칙을 만들 수 있음 |

### 4.9 Outline의 거리별 두께 변화

| 항목 | 내용 |
| --- | --- |
| 문제 현상 | 거리 변화에 따라 외곽선의 화면상 두께가 달라지는 문제 |
| 발생 원인 | 일정한 월드 공간 확장량을 원근 카메라에서 보면 화면상 크기가 달라짐 |
| 시도한 방법 | 카메라와 표면 거리로 확장량 보정 |
| 최종 해결 | 별도 Outline Material에 거리 보정 적용 |
| 해결 결과 | 사용자 완료 발언 존재, 최종 값과 모든 FOV 검증은 확인 필요 |
| 얻은 이해 | 월드 공간 두께와 화면 픽셀 두께가 다르며, 카메라 조건을 함께 고려해야 함 |

### 4.10 의미가 있어도 완료 성과로 넣지 않을 항목

- 눈의 `EyeUnderHighlight`: 계산만 있고 최종 미연결
- 헤어 Threshold / Smooth 조절: 계산만 있고 최종 미연결
- 헤어 `DarkStrength` / `LightStrength`: Lerp 출력 미연결
- 앞머리 투영 그림자와 G-Buffer 후처리: 참고 PDF 기능이며 현재 구현 항목에서 제외
- 눈썹과 아이라인의 별도 투과 표시: 참고 예시와 실제 Asset 구조가 달라 최종 기능으로 확인되지 않음
- UV 표시 및 복제 문제: 작업 기록은 있으나 재현 가능한 원인과 최종 조작 순서 증거 부족

## 5 기술적 성과와 기여 범위

### 5.1 직접 구성하거나 수정한 기술

| 성과 | 근거와 의미 |
| --- | --- |
| 공용 Toon Material 구성 | 부위별 색상과 명암 계산을 Static Switch로 나누어 통합 |
| 일반 Toon 명암 | Normal과 광원 방향, Threshold와 Smooth 범위로 명암 경계 조절 |
| 얼굴 SDF 적용 | 원본 반전 UV 방식에서 좌우 Texture를 선택하는 방식으로 변경 |
| 얼굴 영역 제한 | FaceShadowMask와 부위 분기를 조합 |
| 코 하이라이트 | 좌우 Texture를 광원 방향에 따라 선택하는 계산 추가 |
| 헤어 데이터 수정 | AI 초안을 Maya에서 분리하고 UV 방향과 Normal 정리 |
| 헤어 방향성 Highlight | Mesh의 방향 기준과 광원 및 카메라 방향을 이용하는 계산 연결 |
| 헤어 밝기 페인팅 | Substance Painter의 Value Texture와 색상 Parameter를 분리 |
| 기존 눈 통합 | 기존 UV와 홍채, 동공 계산을 공용 Toon Material에 연결 |
| Outline 거리 보정 | 별도 Outline 구조에서 카메라 거리에 따른 두께 보정 |
| Rim 합성 수정 | 원하는 색상 결과를 위해 참고 방식과 실제 합성 차이를 확인하고 수정 |

이 중 기본 Toon, Outline, 헤어 Highlight, Rim은 참고 자료의 구현을 학습하고 프로젝트에 맞게 적용한 기술

독자적인 렌더링 알고리즘을 새로 발명한 것으로 표현하지 않음

### 5.2 외부 Asset과 AI 사용 구분

| 항목 | 외부 및 AI 활용 | 사용자 직접 기여 | 표기 시 주의 |
| --- | --- | --- | --- |
| 캐릭터 몸, 얼굴, 의상 | Bounty Hunter Asset | 구조 분석, Material 적용과 재구성 | 캐릭터 전체 직접 제작 주장 X |
| Skeleton과 기본 UV | 기존 Asset | 필요한 구조 확인과 일부 데이터 수정 | 리깅 전체 신규 제작 주장 X |
| 헤어 초기 Mesh | Tripo 결과 | 덩어리 분리, UV 재전개, 방향 정렬, Normal 수정 | AI 초안 기반임을 명시 |
| Base Color 파생 Texture | AI 이미지 편집 및 변형 | 목표 색상 지정, 결과 평가와 수정 요구, 선택 및 적용 | 모든 픽셀 직접 Hand Painting 주장 X |
| 각도별 얼굴 그림자 | AI 보조 이미지 생성 | 각도와 고정 조건 정의, 중간 단계 요구와 결과 검토 | 정확한 SDF 생성 알고리즘 직접 구현 주장 X |
| FaceShadowMask와 좌우 SDF | AI 보조 제작 기록 | Material에서 적용 범위와 연결 구성 | Texture별 후처리 제작 주체는 확인 필요 |
| `T_HairValue` | Substance Painter 사용 | 직접 페인팅 및 연결 완료 기록 | 최종 그래프는 원시 R 채널 곱셈 |
| 속눈썹 마스크 | 앞선 AI 생성은 실패 | 사용자 직접 그림 | 전용 Material 최종 적용은 별도 확인 |
| 눈 UV 알고리즘 | 원본 `M_Eyes` | Toon 통합, Texture와 색상 조정 | 알고리즘 전체 독자 개발 주장 X |
| Shader 제작 설명 | PDF와 AI 안내 참고 | 에디터에서 노드 구성, 판단, 조정과 검증 | 튜토리얼 및 AI 지원을 숨기지 않음 |

### 5.3 이전 프로젝트 대비 개선된 부분

| 첫 번째 프로젝트의 중심 | 두 번째 프로젝트에서 추가한 작업 |
| --- | --- |
| 기존 캐릭터의 상태와 외형 Parameter 운용 | 명암 및 하이라이트 계산 자체를 수정 |
| 기존 헤어 표현 활용 | 입체 헤어 데이터와 UV 및 Normal 수정 |
| 기존 눈과 피부 Material 활용 | 눈 계산 통합과 얼굴 전용 SDF 적용 |
| 제공된 외형을 Preset으로 제어 | 원하는 스타일을 위한 Texture와 Shader 조합 조정 |
| 운영 기능 통합 | Rendering 데이터의 문제를 분석하고 수정 |

개선은 주로 **표현 제어 범위와 기술 작업 범위가 넓어진 점**

객관적인 미적 우월성, FPS 증가, 비용 감소가 측정된 것은 아님

### 5.4 Technical Artist 포트폴리오에서 강조할 역량

#### Asset 분석

Material 이름만 보는 것이 아니라 최종 색상 경로와 Texture 채널을 추적

기존 눈, 얼굴, 몸의 마스크 계산에서 재사용할 부분과 교체할 부분 분리

#### 아트 요구를 계산 규칙으로 변환

- 얼굴 그림자 형태 → SDF Threshold Texture
- 기본 그림자 유지 → 밝은 영역 규약을 맞춘 Min 합성 제안
- 헤어 덩어리 밝기 → 흑백 Texture와 색상 Parameter 분리
- 회색으로 뜨는 Rim → Add와 Multiply / Lerp 합성 비교

#### DCC와 Unreal 사이의 데이터 이해

Mesh, UV, Vertex Normal, Tangent, Texture와 Shader를 연결해 문제 판단

앞머리 삼각형 명암을 Shader 숫자만 바꾸지 않고 Maya Normal 수정으로 해결한 과정이 대표 사례

#### 검증과 범위 판단

공용 Material의 여러 부위 계산을 최종 출력까지 추적하고, 미사용 Parameter를 실제 기능과 구분

참고 PDF에 있지만 프로젝트에서 제외한 기능을 명확히 설명

### 5.5 현재 구현의 한계

| 한계 | 현재 상태 | 개선 가능성 |
| --- | --- | --- |
| 광원 writer | 현재 일반/SDF/헤어는 MPC LightDir, Custom은 미사용 | 실제 LightDir/LightColor writer와 갱신 경로 추가 확인 |
| 얼굴 상하 조명 | XY만 비교, HeadUp 보정 없음 | 위아래 광원과 머리 기울임에서 결과 검증 |
| XY 투영 Normalize | 수평 성분이 작아지는 조건의 별도 처리 미확인 | 극단적인 방향에서 안전성과 전환 확인 |
| 실제 차폐 그림자 | 벽이나 앞머리 차폐 검증 근거 없음 | 방향 명암과 별도 항목으로 검증 |
| 다중 캐릭터 | 머리 방향을 공용 MPC에서 읽음 | 캐릭터마다 다른 값이 필요할 경우 전달 구조 검토 |
| 미사용/폐기 | 헤어 보정 Lerp·EyeUnderHighlight·Custom은 미사용, Hair SmoothStep/코 차단 곱셈은 현재 경로 없음 | 현재 상태와 과거 잔여 노드를 구분. 이번에는 에셋 수정 안 함 |
| Runtime 호환 | 현재 SkinHue_Global/SkinDepth_Global은 BP와 이름 일치 | 실제 모드별 MID와 화면 반영은 추가 검증 |
| Static Switch | MI 변형 선택용 | Runtime 연속 Parameter와 전환 방식을 구분 |
| Outline | 거리 보정 완료, 최신 그래프 미확보 | 카메라 거리와 FOV, 애니메이션에서 비교 |
| AI Texture | 최종 버전과 정량 품질 검증 일부 미확보 | UV, 해상도, 마스크 채널, 이음새 확인 |
| 최종 컴파일 | G1에 오류 기록 잔존 | 에디터 Apply 성공과 최신 Export 확보 |
| 성능 | GPU 시간, Instruction Count, Draw Call 비교 없음 | 같은 조건의 Before / After 계측 |
| 5종 Toon 외형 | 요청과 UI는 있으나 완성 5종 이미지 미확보 | 같은 구도에서 5종 결과 확보 |

개선 가능성은 차기 작업 후보이며 완료 성과에 포함하지 않음

### 5.6 발표용 성과 문장

사용 가능한 문장

> 기존 Bounty Hunter Asset의 Material과 Texture 채널을 분석하고, Normal 기반 Toon 명암과 얼굴 전용 SDF, 헤어 방향성 하이라이트를 공용 Material에 통합
>
> Tripo로 생성한 헤어 초안을 Maya에서 분리하고 UV와 Vertex Normal을 수정하여 Unreal에서 원하는 스타일의 명암과 하이라이트가 나타나도록 조정
>
> 얼굴과 헤어의 시각 문제를 Shader 연결뿐 아니라 Texture, UV, Mesh Normal의 관점에서 구분해 해결

피해야 할 문장

- 캐릭터 전체와 모든 Texture를 직접 제작
- 독자적인 Toon Rendering 엔진 개발
- 실시간 물리 기반 Hair Shader 완성
- 모든 조명과 카메라에서 안정적인 얼굴 그림자 보장
- Shader 통합으로 Draw Call과 GPU 비용 감소
- AI로 만든 SDF가 수학적으로 정확함을 검증
- 기획한 3단계 또는 5단계 Ramp 전체 구현 완료

## 6 발표자료 구성 제안

> 원문 구성안을 보존한다. 아래 G1 기반 Custom 광원·과거 수식·미사용 노드 설명은 현재 발표에 그대로 복사하지 말고 0절의 현재 경로로 대체한다. 필요한 영상은 실제 촬영 후 사용한다.

전체 프로젝트 발표 안에서 Toon 파트를 구성하는 안

아래 10장은 기술 상세형 구성이고 발표 시간이 짧으면 뒤의 축약안 사용

### 슬라이드 1 기존 캐릭터를 Toon 스타일로 재해석

| 항목 | 내용 |
| --- | --- |
| 핵심 메시지 | 기존 캐릭터의 디자인을 유지하며 표현 방식을 바꾸는 Look Development 작업 |
| 기술 설명 | 원본 재질과 Texture의 연속적인 명암, 목표 Toon 명암의 차이 |
| 필요한 이미지 | 같은 카메라와 조명에서 Original / Toon 전신 비교, 얼굴 확대 |
| Diagram | 불필요, 결과 비교를 크게 배치 |
| 발표 강조 | 외부 캐릭터 Asset과 직접 작업한 Rendering 부분 구분 |

V1은 참고 Asset 이미지로 표시하고, 정식 전후 비교는 실제 프로젝트에서 같은 조건으로 캡처

### 슬라이드 2 Asset 구조 분석과 재사용 범위

| 항목 | 내용 |
| --- | --- |
| 핵심 메시지 | 기존 Asset의 데이터 구조를 파악한 뒤 필요한 계산을 재사용 |
| 기술 설명 | 5개 원본 Master, 주요 Texture와 Mask 채널, 눈 계산과 색상 Parameter |
| 필요한 이미지 | Material Slot 목록, 원본 Face / Eyes 그래프 일부, RGB Mask 분리 이미지 |
| Diagram | 원본 Mesh와 Texture → 재사용 계산 → M_Toon / M_Outline 관계 |
| 발표 강조 | 모든 원본을 버리지 않고 교체와 유지의 기준을 정했다는 점 |

MI 31개를 모두 보여주기보다 얼굴과 눈 두 사례를 중심으로 설명

### 슬라이드 3 부위별 규칙을 가진 공용 Toon Material

| 항목 | 내용 |
| --- | --- |
| 핵심 메시지 | 공통 명암은 공유하고 얼굴, 헤어, 눈은 별도 계산 선택 |
| 기술 설명 | 부위별 Static Switch, Named Reroute, 최종 Emissive 흐름 |
| 필요한 이미지 | 전체 그래프 축소 이미지와 핵심 경로만 확대, 부위별 MI 설정 |
| Diagram | 이 문서 3.1의 전체 구조를 발표용으로 단순화 |
| 발표 강조 | Static Switch와 Runtime Parameter의 차이를 정확히 설명 |

전체 노드를 읽게 하기보다 색상, 명암, 헤어, 얼굴, 출력의 다섯 영역을 구분

### 슬라이드 4 Normal에서 명암 경계 만들기

| 항목 | 내용 |
| --- | --- |
| 핵심 메시지 | 광원과 표면 방향의 관계를 조절 가능한 Toon 명암으로 변환 |
| 기술 설명 | Dot → Remap → SmoothStep, ShadowRange와 ShadowSmooth, 그림자색 합성 |
| 필요한 이미지 | Dot 연속값, 흑백 명암 마스크, 최종 색상 결과, Parameter 비교 |
| Diagram | Dot와 Threshold를 이용한 마스크 생성 관계 |
| 발표 강조 | 단순 색 필터가 아니라 광원 방향에 따라 바뀌는 명암 계산 |

3단계 Ramp 완성이라고 적지 않고 실제 명암 및 Highlight 합성 구조로 설명

### 슬라이드 5 얼굴 형태를 제어하는 SDF 그림자

| 항목 | 내용 |
| --- | --- |
| 핵심 메시지 | 얼굴의 세부 굴곡보다 설계한 그림자 형태를 우선 |
| 기술 설명 | 좌우 SDF, 머리 상대 방향, Threshold, FaceShadowMask |
| 필요한 이미지 | 일반 Normal 얼굴 그림자 / SDF 얼굴 그림자, 좌우 SDF Texture, 정면과 좌우 조명 결과 |
| Diagram | HeadForward / HeadRight + 광원 → 좌우 선택 및 Threshold → 얼굴 마스크 → DrawShadow |
| 발표 강조 | PDF의 단일 반전 UV 방식에서 두 Texture 선택 방식으로 바뀐 이유와 실제 범위 |

각도별 AI 이미지 자료와 실제 엔진 결과를 분리해 표시

상하 조명 보정 미구현은 질의 시 설명할 한계로 준비

### 슬라이드 6 헤어 데이터를 Shader에 맞게 수정

| 항목 | 내용 |
| --- | --- |
| 핵심 메시지 | 원하는 셰이딩을 얻기 위해 Mesh, UV, Normal까지 수정 |
| 기술 설명 | Tripo 초안, Maya 덩어리 분리, UV 방향 정렬, Soften Edge, Value Texture |
| 필요한 이미지 | 초기 헤어 / 수정 헤어, UV 전후, 삼각형 명암 전후, Painter Value Texture |
| Diagram | 필요하면 Mesh / UV / Normal이 Shader 입력에 연결되는 관계 |
| 발표 강조 | AI 생성 결과를 그대로 사용한 것이 아니라 Unreal 표현에 맞게 직접 정리한 과정 |

이 슬라이드는 TA 관점에서 가장 강한 문제 해결 근거 중 하나

### 슬라이드 7 헤어 하이라이트와 눈 재질 통합

| 항목 | 내용 |
| --- | --- |
| 핵심 메시지 | 부위의 특징에 맞춰 서로 다른 계산을 사용 |
| 기술 설명 | 헤어 방향축과 Half Direction, Power 마스크 / 눈의 기존 UV 계산과 홍채 및 동공 조절 |
| 필요한 이미지 | 카메라 또는 광원 변화에 따른 헤어 띠, 눈 확대, IrisDiameter와 PupilScale 비교 |
| Diagram | 헤어 계산과 눈 계산을 각각 작은 그림으로 분리 |
| 발표 강조 | 헤어는 방향성 계산, 눈은 원본 기능 재사용과 스타일 변경이라는 차이 |

시간이 충분하면 헤어와 눈을 각각 한 장으로 분리

Hair SmoothStep은 현재 목록에서 확인되지 않고 EyeUnderHighlight는 현재 미사용이다. 기능 목록에서 제외한다.

### 슬라이드 8 실루엣 표현과 Rim 문제 해결

| 항목 | 내용 |
| --- | --- |
| 핵심 메시지 | 경계 마스크와 색상 합성 방식을 함께 설계해야 원하는 색을 얻음 |
| 기술 설명 | 별도 Outline과 거리 보정, Fresnel Rim, Add에서 Multiply / Lerp로 변경 |
| 필요한 이미지 | 회색 Rim 문제 / 수정 결과, Rim 마스크, 가까운 거리 / 먼 거리 Outline |
| Diagram | Add 방식과 최종 Multiply / Lerp 방식을 나란히 비교 |
| 발표 강조 | 실제 역광 계산이 아니라 시점 기반 실루엣 강조라는 점, PDF 기준으로 재검토한 판단 |

Outline과 Rim은 외곽에서 보이지만 서로 다른 계산이므로 효과를 각각 껐을 때의 비교가 있으면 설명이 명확

### 슬라이드 9 기존 Studio에서의 적용과 기여 범위

| 항목 | 내용 |
| --- | --- |
| 핵심 메시지 | Rendering 결과를 기존 캐릭터 운영 환경 안에서 확인 |
| 기술 설명 | Material 적용과 Operator UI, 재사용한 Camera / Stage / Appearance 기반 |
| 필요한 이미지 | V2 실행 화면, 검증된 경우에만 Original / Toon 전환 영상과 색상 변경 |
| Diagram | UI → Material 적용 경로는 최신 Blueprint 확보 후 작성 |
| 발표 강조 | 기존 시스템의 재사용과 이번 Toon 작업을 분리, AI 및 외부 Asset 기여 표기 |

버튼이 보인다는 이유만으로 모든 전환이 정상 동작한다고 말하지 않음

### 슬라이드 10 구현 범위와 다음 검증

| 항목 | 내용 |
| --- | --- |
| 핵심 메시지 | 실제 완료한 범위와 남은 제약을 근거로 설명 |
| 기술 설명 | 완료 기능, 미사용 노드, 상하 광원 및 실제 차폐, 성능 미측정 |
| 필요한 이미지 | 대표 최종 결과, 완료 / 확인 필요 항목의 짧은 표 |
| Diagram | 불필요 |
| 발표 강조 | 성능 개선이나 모든 조건의 안정성을 과장하지 않고 다음 검증 기준 제시 |

### 발표 시간이 짧은 경우의 6장 구성

| 순서 | 묶을 내용 |
| --- | --- |
| 1 | 목표와 Original / Toon 결과 |
| 2 | 원본 Asset 분석과 M_Toon 구조 |
| 3 | 얼굴 SDF와 적용 범위 마스크 |
| 4 | 헤어 Mesh, UV, Normal 수정과 방향성 Highlight |
| 5 | 눈 재사용, Outline, Rim 수정 사례 |
| 6 | 직접 기여, AI 활용, 구현 한계 |

Shader의 모든 노드보다 **문제 → 기술 판단 → 바뀐 결과**가 보이도록 구성

## 7 발표 전 증거 보완 목록

> 아래는 원문의 과거 요청 목록이다. 현재 확보한 M_Toon/Outline/MI/Texture/일부 Blueprint Export를 제외한 **남은 자료의 구체적인 요청은 0.8절**을 따른다.

추가 기능을 반드시 만들어야 한다는 뜻이 아니라, 현재 기능을 정확히 설명하기 위해 필요한 자료

| 우선순위 | 자료 | 확인하려는 내용 |
| --- | --- | --- |
| 높음 | 최종 `M_Toon` 전체 Export | G1 이후 변경, 기본 그림자 Min과 코 차단 적용 여부 |
| 높음 | Material Details와 Compile 성공 화면 | Shading Model, Blend Mode, 잔존 오류 상태 |
| 높음 | `M_Outline` 전체 그래프와 Mesh 구성 | 별도 Outline의 실제 연결과 거리 보정 |
| 높음 | `MPC_Toon`을 갱신하는 Blueprint | LightDir / LightColor / HeadForward / HeadRight, 축과 좌표계 |
| 높음 | 같은 조건 Original / Toon 비교 | 시각 변화의 근거 |
| 높음 | 헤어 Soften Edge 전후 | 직접 해결한 문제의 전후 증거 |
| 중간 | 최종 부위별 MI와 Slot 목록 | 어떤 기능이 실제 어느 부위에 켜졌는지 |
| 중간 | 좌우 및 정면, 상하 광원 얼굴 영상 | SDF와 극단적 방향의 안정성 |
| 중간 | Original / Toon 전환 Blueprint 및 실행 | UI 외 실제 전환과 MID 재생성 및 갱신 |
| 중간 | 최종 Texture 파일명 및 버전 목록 | AI 결과와 직접 제작, 최종 적용 파일 구분 |
| 선택 | GPU 및 Draw Call 비교 | 성능 이야기를 할 경우에만 필요 |

## 8 발표·포트폴리오에 재사용할 현재 기준

이 절은 과거의 “다음 채팅 지침”을 현재 검증 요약으로 수정한 것이다. 실제 요청보다 우선하는 지시가 아니다.

1. 외부 Bounty Hunter 기반과 직접 제작·AI 보조 작업을 구분한다. 과거 완료 발언은 이번 검증에서 재확인한 화면과 구분한다.
2. 현재 M_Toon 최종 출력과 Static Switch별 경로는 0.2–0.4절을 기준으로 한다. 과거 G1 번호를 현재 노드 번호로 사용하지 않는다.
3. 일반/SDF/헤어는 현재 MPC_Toon.LightDir를 사용하며 Custom 방향 노드는 미사용이다.
4. 얼굴 SDF는 좌우 Texture·FaceShadowMask 경로가 연결된다. 고정 그림자 합성은 Min이 아니라 곱셈이다.
5. 코 Highlight의 DrawShadow 차단은 현재 최종 출력에 없다. 화면 현상은 별도 검증한다.
6. Hair_Color×T_HairValue.R과 Power Highlight는 현재 사용된다. DarkStrength/LightStrength Lerp와 EyeUnderHighlight는 출력 미도달이다.
7. 현재 Hair Threshold/Smooth Parameter 경로는 확인되지 않는다. HeadUp 쓰기 노드만으로 상하 보정을 구현했다고 하지 않는다.
8. Master 기본과 MI 상속값·로컬 Override·런타임 MID를 각각 구분한다. OverallBrightness 잔여 Override는 현재 출력 기능이 아니다.
9. Outline/속눈썹 Shader 구현과 실제 Mesh 적용은 별도다. CDO는 실행 인스턴스가 아니다.
10. 실제 광원 writer, 실제 Slot 적용, 시각 결과, 다섯 Toon 프리셋 전체, 성능은 확보한 추가 증거 범위에서만 발표한다.

## 9 원본 자료 링크와 상세 근거

### 원문에 남아 있는 외부 원본 링크 — 이번 접근·확인하지 않음

- [최신 M_Toon 그래프 G1](https://chatgpt.com/library?file_id=libfile_b7f5789f2f1081919f7be3becedf79e1&account_id=personal)
- [기존 Bounty Hunter 5종 배색 참고 V1](https://chatgpt.com/library?file_id=libfile_cc1b4298a4588191a625c4fe21e6c802&account_id=personal)
- [Studio의 Toon 실행 화면 V2](https://chatgpt.com/library?file_id=libfile_5da7d90e074c81918539a611f4b816f3&account_id=personal)
- [첫 번째 프로젝트 기획서 P2](https://chatgpt.com/library?file_id=libfile_ee9a60877c308191b113224c6105d3fd&account_id=personal)

### 과거 원문이 G1에 대해 기록한 주요 노드 — 이번 원본 재열람 아님

아래 번호는 `MaterialGraphNode_` 뒤 숫자

| 내용 | 노드 |
| --- | --- |
| 일반 명암 | 13~25, Custom 0 |
| 일반 Highlight | 26~36 |
| Normal | 37~41, 4~5, 101, 229~230 |
| 헤어 방향성 Highlight | 45~64 |
| 미사용 헤어 SmoothStep | 65~69 |
| 피부색과 얼굴 마스크 | 70~89 |
| 손톱과 몸 색상 | 90~100 |
| 헤어 Value | 104~108, 실제 사용 Multiply 106, 미사용 Lerp 105 |
| 얼굴 SDF | 109~144, Custom 1~2 |
| 눈 | 145~233 |
| 코 하이라이트 | 234~244, 미사용 Multiply 239 |
| Rim과 광원 색상 | 245, 247~248, 257, 260~271 |
| 최종 출력 | Root의 Base Color 0, Emissive 270, Normal 1 |

### 참고 PDF를 실제 완료와 혼동하지 않을 범위

- R1 2~6페이지: 원본 Asset Material과 Texture 분석
- R1 58~88페이지: 얼굴 SDF와 머리 방향의 참고 구현
- R1 107~126페이지: 헤어 방향성 Highlight 참고 구현
- R1 152~160페이지: Rim과 LightColor 합성
- PDF에 포함된 앞머리 그림자, 별도 눈썹 표시, G-Buffer 후처리는 프로젝트 완료 목록과 별개

이 문서의 핵심 근거는 외부 일반 이론이 아니라 제공된 그래프, 자료 및 작업 기록이며, 확인되지 않은 사항을 보충 이론으로 채우지 않음
