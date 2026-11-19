# CueServer

Virtual Performer Studio의 웹 Cue 관리자와 Cue List API입니다. 관리자와 API는 같은 FastAPI 서버에서 제공되며, Show·Cue·발행 Revision은 SQLite에 저장됩니다.

## 실행

PowerShell에서 Python 3.11 이상으로 실행합니다.

```powershell
Set-Location 'C:\Work\Unreal\NetworkProject\CueServer'
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\.venv\Scripts\python.exe -m uvicorn app.main:create_app --factory --host 127.0.0.1 --port 8001
```

- [Cue 관리자](http://127.0.0.1:8001/admin)에서 Show를 만들고 Cue를 추가합니다. 번호는 현재 Position에 따라 서버가 1부터 자동 지정합니다. 화면에는 최소 세 자리(001, 002, 003)로 표시하며 1000 이상도 자르지 않습니다. **위로 / 아래로**와 드래그 정렬을 유지하며, 추가·삭제·정렬 후 전체 번호를 다시 맞춥니다. **Cue 복제**는 연출 값을 담은 새 초안을 만들고 저장 시 새 ID와 마지막 번호를 받습니다.
- [Swagger UI](http://127.0.0.1:8001/docs)에서 API를 확인합니다. `GET /api/v1/shows/{showId}/cue-list`는 현재 발행된 스키마 v2 JSON을 반환합니다. 관리자의 **JSON 다운로드**로 같은 응답을 저장할 수 있습니다.
- 기본 DB는 `data/cues.sqlite3`입니다. 별도 DB를 사용하려면 `CUE_DB_PATH`에 파일 경로를 지정하고 실행합니다. 관리자에서 편집한 Show와 Cue는 이 DB 파일에 유지됩니다.
- 기존 v1 DB는 서버 시작 시 Cue 테이블과 모든 Revision 스냅샷에서 Shader Mode를 제거하고 v2로 변환합니다. Show/Cue ID와 순서, 과거 Revision·발행 시각·삭제 이력을 보존합니다. 번호 정책 전환은 아래 규칙을 따릅니다. 로컬 시연 Show는 `CUE_DB_PATH`를 `data/demo.sqlite3`으로 지정해 열 수 있습니다.
- 실습 가상환경에 동일한 의존성이 이미 설치되어 있다면, `CueServer`에서 아래 명령으로 실행할 수 있습니다. `-B`는 실습 가상환경에 바이트코드 캐시를 쓰지 않도록 합니다.

```powershell
& 'C:\Work\PythonAPIServer\.venv\Scripts\python.exe' -B -m uvicorn app.main:create_app --factory --host 127.0.0.1 --port 8001
```

## 시연용 LAN 접속

기본 실행 주소는 `127.0.0.1:8001`입니다. 별도 기기로 시연할 때만 CueServer를 Windows PC의 LAN IPv4 주소에 바인딩합니다. Unreal MCP는 `http://127.0.0.1:8000/mcp`를 계속 사용합니다.

**보안 한계** — 현재 웹 관리자와 API에는 로그인·권한 검사와 HTTPS가 없습니다. 접속할 수 있는 기기는 Show와 Cue를 조회·수정·삭제할 수 있으며 HTTP 트래픽은 암호화되지 않습니다. 방화벽의 IP 제한은 사용자 인증을 제공하지 않습니다. 신뢰할 수 있는 사설 LAN에서 시연 중에만 사용하세요. 외부 인터넷 공개와 공유기 포트 포워딩은 금지합니다.

### 접속 허용

1. Windows PC에서 `ipconfig`로 같은 LAN에 연결된 어댑터의 IPv4를 확인합니다. `Get-NetConnectionProfile`에서 그 어댑터의 네트워크가 `Private`인지 확인하고, 접속할 시연 기기의 현재 IPv4도 확인합니다. 아래 예시의 `192.168.1.10`은 서버 PC, `192.168.1.25`는 허용할 기기입니다. 두 주소를 실제 값으로 바꾸세요.
2. Windows Defender 방화벽이 켜져 있고 Private 프로필에서 기본 인바운드 동작이 차단인지 확인합니다. `wf.msc`의 **인바운드 규칙**에서 Python 또는 TCP 8001을 더 넓게 허용하는 활성 규칙이 있는지도 확인하세요. 아래 규칙은 기존 허용 규칙의 범위를 좁히지 않으므로, 더 넓은 규칙이 적용되는 상태에서는 LAN 시연을 진행하지 마세요. Windows에서 Python에 대한 별도 접속 허용 창이 나타나면 취소하세요.
3. 서버 PC의 **관리자 PowerShell**에서 다음 규칙을 만듭니다. Private 프로필에서 지정한 기기 IP가 서버 LAN 주소의 TCP 8001로 보내는 인바운드 연결만 허용하는 규칙입니다. [Microsoft 방화벽 규칙 명령](https://learn.microsoft.com/ko-kr/powershell/module/netsecurity/new-netfirewallrule?view=windowsserver2025-ps)

   ```powershell
   $cueServerIp = '192.168.1.10'
   $cueDemoDeviceIp = '192.168.1.25'
   New-NetFirewallRule -Name 'CueServer-LAN-Demo-8001' -DisplayName 'CueServer LAN Demo TCP 8001' `
     -PolicyStore PersistentStore -Direction Inbound -Action Allow -Profile Private `
     -Protocol TCP -LocalAddress $cueServerIp -LocalPort 8001 -RemoteAddress $cueDemoDeviceIp
   ```

4. 이미 실행 중인 CueServer를 `Ctrl+C`로 종료합니다. 일반 PowerShell에서 `CueServer` 폴더로 이동한 뒤 시연용으로 실행합니다. `--host`에는 3단계에서 사용한 서버 IPv4를 지정합니다.

   ```powershell
   Set-Location 'C:\Work\Unreal\NetworkProject\CueServer'
   .\.venv\Scripts\python.exe -m uvicorn app.main:create_app --factory --host 192.168.1.10 --port 8001
   ```

5. 허용한 기기의 브라우저에서 `http://192.168.1.10:8001/admin`을 엽니다. 서버 PC에서도 LAN 시연 중에는 이 LAN 주소를 사용합니다. 시연 전에는 허용한 기기에서 열리는지와 다른 LAN 기기에서는 접속이 차단되는지를 확인합니다.

### 시연 종료

CueServer를 `Ctrl+C`로 종료한 뒤, **관리자 PowerShell**에서 시연용 규칙을 삭제합니다. [Microsoft 방화벽 규칙 삭제 명령](https://learn.microsoft.com/ko-kr/powershell/module/netsecurity/remove-netfirewallrule?view=windowsserver2025-ps)

```powershell
Remove-NetFirewallRule -Name 'CueServer-LAN-Demo-8001' -PolicyStore PersistentStore
```

이후 위의 기본 실행 명령(`--host 127.0.0.1 --port 8001`)으로 다시 시작합니다.

## 테스트

```powershell
.\.venv\Scripts\python.exe -m pip install -r requirements-test.txt
.\.venv\Scripts\python.exe -m pytest -q
```

테스트는 각각 임시 SQLite 파일을 사용합니다. `examples/cue-list.json`은 관리자에서 010·020·030 순서로 정렬한 시연 Show의 다운로드 예시입니다. Camera Blend와 Lighting Fade는 밀리초 단위로 저장됩니다. 지정 시간에 따른 재생과 동시 편집 충돌 제어는 P1 범위입니다.

## 자동 Cue 번호 정책 (2026-11-19)

- Cue의 고유 식별자는 UUID id이며 number는 position과 같은 표시용 정수입니다. 생성·수정 요청에 number를 보내지 않습니다. 직접 보내면 422로 거절합니다. 조회 응답의 number 필드와 schemaVersion 2는 유지합니다.
- 생성·삭제·순서 변경과 번호 재지정, 새 Revision 발행은 기존 SQLite 트랜잭션 안에서 함께 처리합니다. 실패하면 전체가 롤백됩니다.
- 서버 시작 시 기존 Cue를 저장된 Position 순서로 재번호화합니다. ID·이름·연출·Blend/Fade는 보존하고, 영향받은 Show만 새 Revision을 한 번 발행합니다. 재시작을 반복해도 추가 Revision을 만들지 않습니다. 과거 v2 Revision 스냅샷의 번호는 이력으로 보존합니다.
- Unreal은 새 REFRESH를 통해 새 Revision과 번호를 받습니다. 기존 로컬 스냅샷은 REFRESH 전까지 유지되며, CueData.Number는 Integer이고 기존 3자리 표시를 그대로 사용합니다. Blueprint나 C++ 변경은 필요하지 않습니다.
- examples/cue-list.json은 이전 번호 정책 스냅샷의 호환성 테스트 자료로 보존합니다. 최신 번호는 실제 Show의 cue-list API로 확인합니다. 기존 cues.sqlite3을 유지하며 demo.sqlite3으로 전환하지 않습니다.

## Camera Stage와 전환 시간 범위 (2026-11-19)

Camera는 Full / Upper / Close / Stage를 지원합니다. Stage는 기존 Unreal Stage View Override를 의미하며 E_CameraState에 새 Enum 값을 추가하는 설계가 아닙니다. 웹과 서버는 기존 cameraState에 문자열 Stage를 저장·조회·다운로드합니다.

Cue별 Camera Blend / Lighting Fade / Transition Time 입력과 지정 시간 실행 기능은 이번 개발 범위에서 제외합니다. Cue는 Animation·Camera·Lighting·Appearance 목표 상태를 전달합니다. 기존 Studio 카메라·조명 시스템은 변경하지 않습니다.

cameraBlendMs / lightingFadeMs는 호환 데이터로 DB와 API 응답에 유지합니다. 생성 요청에서 생략하면 0, PATCH에서 생략하면 기존 값을 보존합니다. 기존 클라이언트가 보낸 유효한 시간 값도 저장할 수 있습니다. 웹에서는 입력·수정을 제공하지 않고 복제 시에만 원본의 숨겨진 시간 값을 함께 보존합니다. DB 스키마 변경이나 기존 값 초기화는 없습니다.

현재 Unreal C++ CueJson 파서는 Camera를 Full/Upper/Close로 제한합니다. **Stage Cue가 포함된 목록은 C++ 허용 목록에 Stage를 추가하기 전까지 Unreal REFRESH 검증에 실패합니다.** 이번 웹·서버 범위에서는 이 파일이나 .uasset을 수정하지 않았습니다. 후속 연결에서 파싱 허용 후 CameraState == Stage이면 별도 Stage View 기능을 호출하고, 나머지 값은 Stage View 해제 후 기존 Camera State 경로로 전달합니다.
