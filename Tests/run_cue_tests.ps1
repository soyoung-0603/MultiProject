param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$Python = 'python',
    [int]$Port = 18081,
    [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$RunName = 'final',
    [switch]$SkipBuild
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$evidence = Join-Path $projectRoot 'Saved\CueImplementation'
New-Item -ItemType Directory -Path $evidence -Force | Out-Null
$project = Join-Path $projectRoot 'NetworkProject.uproject'
$reportFile = Join-Path $evidence "$RunName-report\index.json"
if (Test-Path -LiteralPath $reportFile) { Remove-Item -LiteralPath $reportFile }
if (!$SkipBuild) {
    & (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat') NetworkProjectEditor Win64 Development "-Project=$project" "-Log=$evidence\build-$RunName.log" -WaitMutex -NoHotReloadFromIDE -NoUBA *> "$evidence\build-$RunName-console.log"
    if ($LASTEXITCODE -ne 0) { Get-Content "$evidence\build-$RunName-console.log" -Tail 45; exit $LASTEXITCODE }
}
$pythonExe = (Get-Command $Python -ErrorAction Stop).Source
$fixture = Start-Process -FilePath $pythonExe -ArgumentList @('-B', ('"' + (Join-Path $PSScriptRoot 'cue_http_fixture.py') + '"'), '--port', $Port) -WindowStyle Hidden -PassThru -RedirectStandardOutput "$evidence\fixture-$RunName.log" -RedirectStandardError "$evidence\fixture-$RunName-error.log"
try {
    $ready = $false
    for ($attempt = 0; $attempt -lt 30; $attempt++) {
        if ($fixture.HasExited) { throw "Fixture exited. See $evidence\fixture-$RunName-error.log" }
        try { $null = Invoke-WebRequest "http://127.0.0.1:$Port/normal/api/v1/shows" -TimeoutSec 1; $ready = $true; break } catch { Start-Sleep -Milliseconds 100 }
    }
    if (!$ready) { throw 'Cue HTTP fixture did not become ready.' }
    & (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe') $project /Engine/Maps/Entry -NullRHI -unattended -nosplash -nosound -nop4 "-CueHttpTestUrl=http://127.0.0.1:$Port" '-ExecCmds=Automation RunTests NetworkProject.Cue' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$evidence\$RunName-report" "-abslog=$evidence\$RunName-automation.log" *> "$evidence\$RunName-console.log"
    if ($LASTEXITCODE -ne 0) { Get-Content "$evidence\$RunName-automation.log" -Tail 45; exit $LASTEXITCODE }
    $report = Get-Content -LiteralPath $reportFile -Raw | ConvertFrom-Json
    $report.tests | Select-Object fullTestPath,state | Format-Table -AutoSize
    # Unreal TestExit may return 0 even when a test failed; inspect the report.
    if ($report.failed -ne 0 -or $report.notRun -ne 0 -or $report.tests.Count -eq 0) { exit 1 }
    Write-Output "Cue tests passed. Report: $evidence\$RunName-report\index.json"
} finally {
    if (!$fixture.HasExited) { Stop-Process -Id $fixture.Id }
}
