$ErrorActionPreference = 'Stop'
$pidPath = Join-Path $env:TEMP 'resonance-camera-viewer.pid'

if (-not (Test-Path -LiteralPath $pidPath)) {
    Write-Host 'No recorded Resonance Camera Viewer server is running.'
    exit 0
}

$serverPidText = (Get-Content -Raw -LiteralPath $pidPath).Trim()
$serverPid = 0
if (-not [int]::TryParse($serverPidText, [ref]$serverPid)) {
    throw "Invalid viewer PID file: $pidPath"
}

$process = Get-CimInstance Win32_Process -Filter "ProcessId = $serverPid" -ErrorAction SilentlyContinue
if ($process -and $process.CommandLine -match 'http\.server' -and $process.CommandLine -match 'ops[\\/]camera') {
    Stop-Process -Id $serverPid
    Write-Host "Stopped Resonance Camera Viewer server (PID $serverPid)."
} else {
    Write-Host 'The recorded process is no longer the camera viewer; it was not stopped.'
}

Remove-Item -LiteralPath $pidPath -Force
