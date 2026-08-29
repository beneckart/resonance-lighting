param(
    [int]$Port = 8768
)

$ErrorActionPreference = 'Stop'
$appDir = (Resolve-Path -LiteralPath $PSScriptRoot).Path
$url = "http://127.0.0.1:$Port/"
$pidPath = Join-Path $env:TEMP 'resonance-camera-viewer.pid'

function Test-CameraViewer {
    try {
        $response = Invoke-WebRequest -UseBasicParsing -Uri $url -TimeoutSec 1
        return $response.Content -match '<title>Resonance Camera Bench</title>'
    } catch {
        return $false
    }
}

if (-not (Test-CameraViewer)) {
    $python = Get-Command py -ErrorAction SilentlyContinue
    if (-not $python) {
        $python = Get-Command python -ErrorAction SilentlyContinue
    }
    if (-not $python) {
        throw 'Python was not found. Install Python 3 or add it to PATH.'
    }

    $server = Start-Process -FilePath $python.Source `
        -ArgumentList @('-m', 'http.server', $Port, '--bind', '127.0.0.1', '--directory', $appDir) `
        -WindowStyle Hidden -PassThru
    Set-Content -LiteralPath $pidPath -Value $server.Id -Encoding ascii

    $ready = $false
    for ($attempt = 0; $attempt -lt 30; $attempt++) {
        Start-Sleep -Milliseconds 100
        if (Test-CameraViewer) {
            $ready = $true
            break
        }
        if ($server.HasExited) {
            break
        }
    }
    if (-not $ready) {
        throw "Camera viewer did not start on $url"
    }
}

$edgeCandidates = @(@(
    (Join-Path ${env:ProgramFiles(x86)} 'Microsoft\Edge\Application\msedge.exe'),
    (Join-Path $env:ProgramFiles 'Microsoft\Edge\Application\msedge.exe')
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) })

if ($edgeCandidates.Count -gt 0) {
    Start-Process -FilePath $edgeCandidates[0] -ArgumentList @("--app=$url", '--start-maximized')
} else {
    Start-Process $url
}

Write-Host "Resonance Camera Viewer opened at $url"
