param(
    [switch]$SelfTest,
    [switch]$UiSmokeTest
)

Set-StrictMode -Version 3.0
$ErrorActionPreference = "Stop"

function Sort-SerialPortNames {
    param([string[]]$Names)

    return @($Names | Sort-Object {
        if ($_ -match '^COM([0-9]+)$') { [int]$Matches[1] } else { [int]::MaxValue }
    }, { $_ })
}

function ConvertFrom-FixtureTelemetryLine {
    param([string]$Line)

    if ([string]::IsNullOrWhiteSpace($Line)) { return $null }
    $text = $Line.Trim()
    if (-not $text.StartsWith('{') -or $text.IndexOf('"fixture_id"') -lt 0) {
        return $null
    }
    try {
        return $text | ConvertFrom-Json
    } catch {
        return $null
    }
}

function Get-TelemetryValue {
    param(
        [object]$Telemetry,
        [string]$Name,
        [object]$Default = $null
    )

    if ($null -eq $Telemetry) { return $Default }
    $property = $Telemetry.PSObject.Properties[$Name]
    if ($null -eq $property) { return $Default }
    return $property.Value
}

function Get-GoboEligibility {
    param([object]$Telemetry)

    if ($null -eq $Telemetry) {
        return [pscustomobject]@{ Allowed = $false; Reason = "Waiting for fixture telemetry." }
    }

    $fixtureId = [string](Get-TelemetryValue $Telemetry "fixture_id" "")
    if ($fixtureId -notmatch '^[0-9A-Fa-f]{6}$') {
        return [pscustomobject]@{ Allowed = $false; Reason = "The serial device did not report a valid fixture ID." }
    }

    $role = [string](Get-TelemetryValue $Telemetry "role" "")
    if ($role -ne "peer") {
        return [pscustomobject]@{ Allowed = $false; Reason = "The serial device is not fixture peer firmware." }
    }

    $fixtureClass = [string](Get-TelemetryValue $Telemetry "fixture_class" "")
    if ($fixtureClass -ne "perimeter") {
        $shownClass = if ($fixtureClass) { $fixtureClass } else { "unknown" }
        return [pscustomobject]@{
            Allowed = $false
            Reason = "Refused: fixture class is $shownClass, not perimeter."
        }
    }

    if ([bool](Get-TelemetryValue $Telemetry "deep_recovery_build" $false)) {
        return [pscustomobject]@{ Allowed = $false; Reason = "Refused: this is a deep-recovery firmware build." }
    }

    if (-not [bool](Get-TelemetryValue $Telemetry "pf_ready" $false)) {
        return [pscustomobject]@{ Allowed = $false; Reason = "Refused: PowerFeather initialization is not ready." }
    }

    $guardStage = [int](Get-TelemetryValue $Telemetry "guard_stage" 4)
    if ($guardStage -ge 4) {
        return [pscustomobject]@{ Allowed = $false; Reason = "Refused: the boot guard is in PROTECT." }
    }

    $powerTier = [int](Get-TelemetryValue $Telemetry "power_tier" 3)
    if ($powerTier -ge 2) {
        $tierName = if ($powerTier -eq 2) { "OFF" } else { "PROTECT" }
        return [pscustomobject]@{ Allowed = $false; Reason = "Refused: the power tier is $tierName." }
    }

    return [pscustomobject]@{
        Allowed = $true
        Reason = "Ready: exact perimeter fixture $($fixtureId.ToUpperInvariant())."
    }
}

function Invoke-SelfTest {
    function Assert-True {
        param([bool]$Value, [string]$Message)
        if (-not $Value) { throw "SELFTEST FAIL: $Message" }
    }

    $sorted = Sort-SerialPortNames @("COM10", "COM2", "COM1")
    Assert-True (($sorted -join ',') -eq "COM1,COM2,COM10") "COM port numeric sort"
    $single = @(Sort-SerialPortNames @("COM8"))
    Assert-True ($single.Count -eq 1 -and $single[0] -eq "COM8") "single COM port remains an array"
    $empty = @(Sort-SerialPortNames @())
    Assert-True ($empty.Count -eq 0) "empty COM port list remains an array"

    $good = ConvertFrom-FixtureTelemetryLine '{"fixture_id":"F2BDFC","role":"peer","fixture_class":"perimeter","deep_recovery_build":false,"pf_ready":true,"guard_stage":1,"power_tier":0}'
    Assert-True ($null -ne $good) "valid telemetry parse"
    Assert-True ((Get-GoboEligibility $good).Allowed) "healthy perimeter acceptance"

    $wrongClass = ConvertFrom-FixtureTelemetryLine '{"fixture_id":"F2BDFC","role":"peer","fixture_class":"downlight","pf_ready":true,"guard_stage":1,"power_tier":0}'
    Assert-True (-not (Get-GoboEligibility $wrongClass).Allowed) "non-perimeter refusal"

    $protected = ConvertFrom-FixtureTelemetryLine '{"fixture_id":"F2BDFC","role":"peer","fixture_class":"perimeter","pf_ready":true,"guard_stage":4,"power_tier":3}'
    Assert-True (-not (Get-GoboEligibility $protected).Allowed) "PROTECT refusal"

    $notJson = ConvertFrom-FixtureTelemetryLine 'fixture booting'
    Assert-True ($null -eq $notJson) "non-telemetry line rejection"

    Write-Host "perimeter_gobo_usb self-test PASS"
}

if ($SelfTest) {
    Invoke-SelfTest
    exit 0
}

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

[System.Windows.Forms.Application]::EnableVisualStyles()

$script:Serial = $null
$script:RxBuffer = ""
$script:LastTelemetry = $null
$script:ConnectedPort = ""
$script:NextTelemetryAt = [DateTime]::MinValue
$script:LastTelemetryAt = [DateTime]::MinValue
$script:GoboStartedByApp = $false
$script:ClosingAfterCleanup = $false

$form = New-Object System.Windows.Forms.Form
$form.Text = "Resonance Perimeter Gobo USB"
$form.ClientSize = New-Object System.Drawing.Size(650, 590)
$form.StartPosition = "CenterScreen"
$form.FormBorderStyle = "FixedDialog"
$form.MaximizeBox = $false
$form.Font = New-Object System.Drawing.Font("Segoe UI", 10)

$title = New-Object System.Windows.Forms.Label
$title.Text = "Perimeter Dancing Gobo - Offline USB Control"
$title.Font = New-Object System.Drawing.Font("Segoe UI Semibold", 17)
$title.AutoSize = $true
$title.Location = New-Object System.Drawing.Point(18, 14)
$form.Controls.Add($title)

$subtitle = New-Object System.Windows.Forms.Label
$subtitle.Text = "No WiFi or internet. Connect exactly one fixture by USB."
$subtitle.AutoSize = $true
$subtitle.ForeColor = [System.Drawing.Color]::DimGray
$subtitle.Location = New-Object System.Drawing.Point(21, 50)
$form.Controls.Add($subtitle)

$portLabel = New-Object System.Windows.Forms.Label
$portLabel.Text = "USB serial port"
$portLabel.AutoSize = $true
$portLabel.Location = New-Object System.Drawing.Point(20, 86)
$form.Controls.Add($portLabel)

$portPicker = New-Object System.Windows.Forms.ComboBox
$portPicker.DropDownStyle = "DropDownList"
$portPicker.Location = New-Object System.Drawing.Point(20, 108)
$portPicker.Size = New-Object System.Drawing.Size(170, 28)
$form.Controls.Add($portPicker)

$refreshButton = New-Object System.Windows.Forms.Button
$refreshButton.Text = "Refresh"
$refreshButton.Location = New-Object System.Drawing.Point(202, 106)
$refreshButton.Size = New-Object System.Drawing.Size(100, 32)
$form.Controls.Add($refreshButton)

$connectButton = New-Object System.Windows.Forms.Button
$connectButton.Text = "Connect"
$connectButton.Location = New-Object System.Drawing.Point(314, 106)
$connectButton.Size = New-Object System.Drawing.Size(110, 32)
$form.Controls.Add($connectButton)

$readButton = New-Object System.Windows.Forms.Button
$readButton.Text = "Read status"
$readButton.Location = New-Object System.Drawing.Point(436, 106)
$readButton.Size = New-Object System.Drawing.Size(120, 32)
$readButton.Enabled = $false
$form.Controls.Add($readButton)

$identityLabel = New-Object System.Windows.Forms.Label
$identityLabel.Text = "Fixture: not connected"
$identityLabel.Font = New-Object System.Drawing.Font("Consolas", 11)
$identityLabel.AutoSize = $false
$identityLabel.Location = New-Object System.Drawing.Point(20, 151)
$identityLabel.Size = New-Object System.Drawing.Size(610, 25)
$form.Controls.Add($identityLabel)

$powerLabel = New-Object System.Windows.Forms.Label
$powerLabel.Text = "Power: unknown"
$powerLabel.Font = New-Object System.Drawing.Font("Consolas", 10)
$powerLabel.AutoSize = $false
$powerLabel.Location = New-Object System.Drawing.Point(20, 177)
$powerLabel.Size = New-Object System.Drawing.Size(610, 25)
$form.Controls.Add($powerLabel)

$eligibilityLabel = New-Object System.Windows.Forms.Label
$eligibilityLabel.Text = "Waiting for fixture telemetry."
$eligibilityLabel.Font = New-Object System.Drawing.Font("Segoe UI Semibold", 11)
$eligibilityLabel.AutoSize = $false
$eligibilityLabel.Location = New-Object System.Drawing.Point(20, 205)
$eligibilityLabel.Size = New-Object System.Drawing.Size(610, 28)
$eligibilityLabel.ForeColor = [System.Drawing.Color]::DarkGoldenrod
$form.Controls.Add($eligibilityLabel)

$startButton = New-Object System.Windows.Forms.Button
$startButton.Text = "START DANCING GOBO"
$startButton.Font = New-Object System.Drawing.Font("Segoe UI Semibold", 15)
$startButton.Location = New-Object System.Drawing.Point(20, 244)
$startButton.Size = New-Object System.Drawing.Size(610, 62)
$startButton.BackColor = [System.Drawing.Color]::FromArgb(70, 150, 85)
$startButton.ForeColor = [System.Drawing.Color]::White
$startButton.Enabled = $false
$form.Controls.Add($startButton)

$stopButton = New-Object System.Windows.Forms.Button
$stopButton.Text = "STOP / LED RAIL OFF"
$stopButton.Font = New-Object System.Drawing.Font("Segoe UI Semibold", 12)
$stopButton.Location = New-Object System.Drawing.Point(20, 318)
$stopButton.Size = New-Object System.Drawing.Size(292, 48)
$stopButton.BackColor = [System.Drawing.Color]::FromArgb(175, 70, 60)
$stopButton.ForeColor = [System.Drawing.Color]::White
$stopButton.Enabled = $false
$form.Controls.Add($stopButton)

$normalButton = New-Object System.Windows.Forms.Button
$normalButton.Text = "RETURN TO NORMAL (REBOOT)"
$normalButton.Font = New-Object System.Drawing.Font("Segoe UI Semibold", 11)
$normalButton.Location = New-Object System.Drawing.Point(326, 318)
$normalButton.Size = New-Object System.Drawing.Size(304, 48)
$normalButton.Enabled = $false
$form.Controls.Add($normalButton)

$note = New-Object System.Windows.Forms.Label
$note.Text = "Start sends RAM-only L1. Stop sends L0 and leaves the fixture dark. Return to Normal sends L0, then pulses USB reset so normal field behavior resumes."
$note.AutoSize = $false
$note.Location = New-Object System.Drawing.Point(20, 377)
$note.Size = New-Object System.Drawing.Size(610, 48)
$note.ForeColor = [System.Drawing.Color]::DimGray
$form.Controls.Add($note)

$logBox = New-Object System.Windows.Forms.TextBox
$logBox.Multiline = $true
$logBox.ReadOnly = $true
$logBox.ScrollBars = "Vertical"
$logBox.Font = New-Object System.Drawing.Font("Consolas", 9)
$logBox.Location = New-Object System.Drawing.Point(20, 434)
$logBox.Size = New-Object System.Drawing.Size(610, 135)
$form.Controls.Add($logBox)

function Add-LogLine {
    param([string]$Text)

    $stamp = [DateTime]::Now.ToString("HH:mm:ss")
    $logBox.AppendText("[$stamp] $Text`r`n")
}

function Close-SerialConnection {
    if ($null -ne $script:Serial) {
        try {
            if ($script:Serial.IsOpen) { $script:Serial.Close() }
        } catch {
        }
        try { $script:Serial.Dispose() } catch {}
    }
    $script:Serial = $null
    $script:ConnectedPort = ""
    $script:RxBuffer = ""
    $script:LastTelemetry = $null
    $readButton.Enabled = $false
    $startButton.Enabled = $false
    $stopButton.Enabled = $false
    $normalButton.Enabled = $false
    $connectButton.Text = "Connect"
    $identityLabel.Text = "Fixture: not connected"
    $powerLabel.Text = "Power: unknown"
    $eligibilityLabel.Text = "Waiting for fixture telemetry."
    $eligibilityLabel.ForeColor = [System.Drawing.Color]::DarkGoldenrod
}

function Send-FixtureCommand {
    param(
        [string]$Command,
        [string]$Description
    )

    if ($null -eq $script:Serial -or -not $script:Serial.IsOpen) {
        throw "No open serial connection."
    }
    $script:Serial.Write("$Command`n")
    Add-LogLine "$Description ($Command)"
    $script:NextTelemetryAt = [DateTime]::Now.AddMilliseconds(500)
}

function Update-TelemetryDisplay {
    param([object]$Telemetry)

    $script:LastTelemetry = $Telemetry
    $script:LastTelemetryAt = [DateTime]::Now

    $fixtureId = ([string](Get-TelemetryValue $Telemetry "fixture_id" "?")).ToUpperInvariant()
    $fixtureClass = [string](Get-TelemetryValue $Telemetry "fixture_class" "unknown")
    $firmware = [string](Get-TelemetryValue $Telemetry "fw" "unknown")
    $profile = [string](Get-TelemetryValue $Telemetry "profile" "unknown")
    $identityLabel.Text = "Fixture: $fixtureId  class=$fixtureClass  profile=$profile  fw=$firmware"

    $batteryV = Get-TelemetryValue $Telemetry "battery_v" $null
    $batteryText = if ($null -eq $batteryV) { "n/a" } else { "{0:N3} V" -f [double]$batteryV }
    $powerTier = [int](Get-TelemetryValue $Telemetry "power_tier" 3)
    $tierNames = @("FULL", "DIM", "OFF", "PROTECT")
    $tierText = if ($powerTier -ge 0 -and $powerTier -lt $tierNames.Count) { $tierNames[$powerTier] } else { [string]$powerTier }
    $guard = [int](Get-TelemetryValue $Telemetry "guard_stage" 4)
    $rail = [bool](Get-TelemetryValue $Telemetry "led_rail_on" $false)
    $smoke = [bool](Get-TelemetryValue $Telemetry "smoke_render" $false)
    $powerLabel.Text = "Power: VBAT=$batteryText  tier=$tierText  guard=$guard  rail=$rail  gobo=$smoke"

    $eligibility = Get-GoboEligibility $Telemetry
    $eligibilityLabel.Text = $eligibility.Reason
    $eligibilityLabel.ForeColor = if ($eligibility.Allowed) {
        [System.Drawing.Color]::DarkGreen
    } else {
        [System.Drawing.Color]::DarkRed
    }
    $verifiedPerimeter = $fixtureId -match '^[0-9A-F]{6}$' -and
        ([string](Get-TelemetryValue $Telemetry "role" "")) -eq "peer" -and
        $fixtureClass -eq "perimeter"
    $startButton.Enabled = [bool]$eligibility.Allowed
    $stopButton.Enabled = $verifiedPerimeter
    $normalButton.Enabled = $verifiedPerimeter
    $readButton.Enabled = $true
}

function Process-SerialLine {
    param([string]$Line)

    $text = $Line.Trim()
    if (-not $text) { return }
    $telemetry = ConvertFrom-FixtureTelemetryLine $text
    if ($null -ne $telemetry) {
        Update-TelemetryDisplay $telemetry
        return
    }
    Add-LogLine "fixture: $text"
}

function Read-SerialAvailable {
    if ($null -eq $script:Serial -or -not $script:Serial.IsOpen) { return }
    if ($script:Serial.BytesToRead -le 0) { return }

    $script:RxBuffer += $script:Serial.ReadExisting()
    while ($true) {
        $newline = $script:RxBuffer.IndexOf("`n")
        if ($newline -lt 0) { break }
        $line = $script:RxBuffer.Substring(0, $newline)
        $script:RxBuffer = $script:RxBuffer.Substring($newline + 1)
        Process-SerialLine $line
    }
    if ($script:RxBuffer.Length -gt 65536) {
        $script:RxBuffer = $script:RxBuffer.Substring($script:RxBuffer.Length - 4096)
    }
}

function Request-Telemetry {
    if ($null -eq $script:Serial -or -not $script:Serial.IsOpen) { return }
    $script:Serial.Write("t`n")
    $script:NextTelemetryAt = [DateTime]::Now.AddSeconds(2)
}

function Refresh-PortPicker {
    param([switch]$AutoConnect)

    $previous = [string]$portPicker.SelectedItem
    # PowerShell unwraps a one-item pipeline result into a scalar unless the
    # caller re-wraps it. Keep Count/indexing valid for zero, one, or many COM
    # ports -- the normal field case is deliberately exactly one.
    $names = @(Sort-SerialPortNames ([System.IO.Ports.SerialPort]::GetPortNames()))
    $portPicker.Items.Clear()
    foreach ($name in $names) { [void]$portPicker.Items.Add($name) }

    if ($previous -and $names -contains $previous) {
        $portPicker.SelectedItem = $previous
    } elseif ($names.Count -eq 1) {
        $portPicker.SelectedIndex = 0
    } elseif ($names.Count -gt 0) {
        $portPicker.SelectedIndex = 0
    }

    if ($names.Count -eq 0) {
        Add-LogLine "No serial ports found. Plug in one fixture and press Refresh."
    } elseif ($names.Count -gt 1) {
        Add-LogLine "Found $($names.Count) serial ports. Select the fixture port explicitly."
    } else {
        Add-LogLine "Found sole serial port $($names[0])."
        if ($AutoConnect) { Open-SelectedPort }
    }
}

function Open-SelectedPort {
    if ($null -ne $script:Serial -and $script:Serial.IsOpen) { return }
    if ($null -eq $portPicker.SelectedItem) {
        [System.Windows.Forms.MessageBox]::Show(
            "No serial port is selected. Plug in one fixture and press Refresh.",
            "No fixture port",
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Warning
        ) | Out-Null
        return
    }

    $name = [string]$portPicker.SelectedItem
    try {
        $port = New-Object System.IO.Ports.SerialPort($name, 115200, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
        $port.DtrEnable = $false
        $port.RtsEnable = $false
        $port.ReadTimeout = 250
        $port.WriteTimeout = 1000
        $port.NewLine = "`n"
        $port.Open()
        $script:Serial = $port
        $script:ConnectedPort = $name
        $script:RxBuffer = ""
        $script:LastTelemetry = $null
        $script:LastTelemetryAt = [DateTime]::MinValue
        $script:NextTelemetryAt = [DateTime]::Now
        $connectButton.Text = "Disconnect"
        $readButton.Enabled = $true
        $identityLabel.Text = "Fixture: verifying $name..."
        $eligibilityLabel.Text = "Connected; waiting for read-only telemetry."
        $eligibilityLabel.ForeColor = [System.Drawing.Color]::DarkGoldenrod
        Add-LogLine "Connected to $name with DTR/RTS low."
    } catch {
        Close-SerialConnection
        Add-LogLine "Connect failed on ${name}: $($_.Exception.Message)"
        [System.Windows.Forms.MessageBox]::Show(
            "Could not open ${name}:`r`n$($_.Exception.Message)",
            "USB connection failed",
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Error
        ) | Out-Null
    }
}

function Return-FixtureToNormal {
    if ($null -eq $script:Serial -or -not $script:Serial.IsOpen) {
        throw "No open serial connection."
    }

    Send-FixtureCommand "L0" "Stopping gobo and forcing LED rail off before reset"
    [System.Threading.Thread]::Sleep(500)
    $script:Serial.DtrEnable = $false
    $script:Serial.RtsEnable = $true
    [System.Threading.Thread]::Sleep(150)
    $script:Serial.RtsEnable = $false
    $script:GoboStartedByApp = $false
    $script:LastTelemetry = $null
    $startButton.Enabled = $false
    $stopButton.Enabled = $false
    $normalButton.Enabled = $false
    $eligibilityLabel.Text = "USB reset sent; waiting for normal fixture telemetry."
    $eligibilityLabel.ForeColor = [System.Drawing.Color]::DarkGoldenrod
    $script:NextTelemetryAt = [DateTime]::Now.AddSeconds(2)
    Add-LogLine "USB RTS reset pulse sent; normal field behavior should resume."
}

$refreshButton.Add_Click({ Refresh-PortPicker })

$connectButton.Add_Click({
    if ($null -ne $script:Serial -and $script:Serial.IsOpen) {
        if ($script:GoboStartedByApp) {
            $choice = [System.Windows.Forms.MessageBox]::Show(
                "The dancing gobo may still be active. Return the fixture to normal before disconnecting?",
                "Gobo still active",
                [System.Windows.Forms.MessageBoxButtons]::YesNoCancel,
                [System.Windows.Forms.MessageBoxIcon]::Warning
            )
            if ($choice -eq [System.Windows.Forms.DialogResult]::Cancel) { return }
            if ($choice -eq [System.Windows.Forms.DialogResult]::Yes) {
                try { Return-FixtureToNormal } catch { Add-LogLine $_.Exception.Message }
            }
        }
        Add-LogLine "Disconnected from $($script:ConnectedPort)."
        Close-SerialConnection
    } else {
        Open-SelectedPort
    }
})

$readButton.Add_Click({
    try {
        Request-Telemetry
        Add-LogLine "Requested read-only telemetry."
    } catch {
        Add-LogLine "Telemetry request failed: $($_.Exception.Message)"
    }
})

$startButton.Add_Click({
    $eligibility = Get-GoboEligibility $script:LastTelemetry
    if (-not $eligibility.Allowed) {
        [System.Windows.Forms.MessageBox]::Show(
            $eligibility.Reason,
            "Gobo command refused",
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Warning
        ) | Out-Null
        return
    }
    try {
        Send-FixtureCommand "L1" "Starting one-white-pixel spiral"
        $script:GoboStartedByApp = $true
        $eligibilityLabel.Text = "Dancing gobo command sent. Watch the physical projection."
        $eligibilityLabel.ForeColor = [System.Drawing.Color]::DarkGreen
    } catch {
        Add-LogLine "Start failed: $($_.Exception.Message)"
    }
})

$stopButton.Add_Click({
    try {
        Send-FixtureCommand "L0" "Stopping gobo; LED rail forced off"
        $script:GoboStartedByApp = $false
        $eligibilityLabel.Text = "Stopped and dark. Press Return to Normal before leaving."
        $eligibilityLabel.ForeColor = [System.Drawing.Color]::DarkGoldenrod
    } catch {
        Add-LogLine "Stop failed: $($_.Exception.Message)"
    }
})

$normalButton.Add_Click({
    try {
        Return-FixtureToNormal
    } catch {
        Add-LogLine "Return to normal failed: $($_.Exception.Message)"
        [System.Windows.Forms.MessageBox]::Show(
            "USB reset failed. Use the fixture's physical reset or power-cycle it before leaving.",
            "Reset failed",
            [System.Windows.Forms.MessageBoxButtons]::OK,
            [System.Windows.Forms.MessageBoxIcon]::Error
        ) | Out-Null
    }
})

$timer = New-Object System.Windows.Forms.Timer
$timer.Interval = 200
$timer.Add_Tick({
    try {
        Read-SerialAvailable
        if ($null -ne $script:Serial -and $script:Serial.IsOpen -and [DateTime]::Now -ge $script:NextTelemetryAt) {
            Request-Telemetry
        }
        if ($null -ne $script:Serial -and $script:Serial.IsOpen -and
            $script:LastTelemetryAt -ne [DateTime]::MinValue -and
            ([DateTime]::Now - $script:LastTelemetryAt).TotalSeconds -gt 7) {
            $startButton.Enabled = $false
            $eligibilityLabel.Text = "Telemetry is stale; Start is disabled until the fixture responds."
            $eligibilityLabel.ForeColor = [System.Drawing.Color]::DarkRed
        }
    } catch {
        Add-LogLine "USB connection lost: $($_.Exception.Message)"
        Close-SerialConnection
    }
})

$form.Add_Shown({
    $timer.Start()
    Add-LogLine "Offline controller ready. Only read-only telemetry is sent automatically."
    Refresh-PortPicker -AutoConnect
})

$form.Add_FormClosing({
    param($sender, $eventArgs)

    if ($script:ClosingAfterCleanup) { return }
    if ($script:GoboStartedByApp -and $null -ne $script:Serial -and $script:Serial.IsOpen) {
        $choice = [System.Windows.Forms.MessageBox]::Show(
            "The dancing gobo may still be active.`r`n`r`nYes: stop and reset to normal, then close.`r`nNo: leave it active and close.`r`nCancel: keep the app open.",
            "Gobo still active",
            [System.Windows.Forms.MessageBoxButtons]::YesNoCancel,
            [System.Windows.Forms.MessageBoxIcon]::Warning
        )
        if ($choice -eq [System.Windows.Forms.DialogResult]::Cancel) {
            $eventArgs.Cancel = $true
            return
        }
        if ($choice -eq [System.Windows.Forms.DialogResult]::Yes) {
            try { Return-FixtureToNormal } catch {
                $eventArgs.Cancel = $true
                Add-LogLine "Cleanup failed: $($_.Exception.Message)"
                return
            }
        }
    }
    $timer.Stop()
    Close-SerialConnection
})

if ($UiSmokeTest) {
    $form.Show()
    [System.Windows.Forms.Application]::DoEvents()
    $form.Close()
    Write-Host "perimeter_gobo_usb UI smoke-test PASS"
    exit 0
}

[void][System.Windows.Forms.Application]::Run($form)
