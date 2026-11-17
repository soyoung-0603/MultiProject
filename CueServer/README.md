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

- [Cue 관리자](http://127.0.0.1:8001/admin)에서 Show를 만들고 Cue를 추가합니다. 번호는 표시용이며 **위로 / 아래로** 버튼으로 실행 순서를 변경합니다. **Cue 복제**는 제안 번호가 들어간 초안을 엽니다.
- [Swagger UI](http://127.0.0.1:8001/docs)에서 API를 확인합니다. `GET /api/v1/shows/{showId}/cue-list`는 현재 발행된 스키마 v2 JSON을 반환합니다. 관리자의 **JSON 다운로드**로 같은 응답을 저장할 수 있습니다.
- 기본 DB는 `data/cues.sqlite3`입니다. 별도 DB를 사용하려면 `CUE_DB_PATH`에 파일 경로를 지정하고 실행합니다. 관리자에서 편집한 Show와 Cue는 이 DB 파일에 유지됩니다.
- 기존 v1 DB는 서버 시작 시 Cue 테이블과 모든 Revision 스냅샷에서 Shader Mode를 제거하고 v2로 변환합니다. Show/Cue ID, 순서, Revision 번호, 발행 시각, 삭제 이력을 유지합니다. 로컬 시연 Show는 `CUE_DB_PATH`를 `data/demo.sqlite3`으로 지정해 열 수 있습니다.
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
