# Runs from the root of the CRYPTOS drive, whatever letter it got.
# Started visibly by the chip; relaunches itself hidden with -Watch to close
# the kiosk on unplug.
param([switch]$Watch)

$root = $PSScriptRoot
$indexPath = Join-Path $root "dashboard\index.html"
$profileDir = Join-Path $env:TEMP "CryptoKiosk"

# Keep this window up until the chip has finished typing its Linux attempt
# (~5s after the Run command), so those keystrokes land here, not on the desktop.
$keystrokeGuardSeconds = 6

# All kiosk Edge processes, found by their dedicated profile dir. More reliable
# than one PID: Edge is multi-process and may hand off to an existing instance.
function Get-KioskProcesses {
    Get-CimInstance Win32_Process -Filter "Name='msedge.exe'" |
        Where-Object { $_.CommandLine -and $_.CommandLine.Contains("--user-data-dir=$profileDir") }
}

function Stop-Kiosk {
    Get-KioskProcesses | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }
}

if ($Watch) {
    # Close the kiosk once the drive is unplugged; stop if it was closed by hand.
    while (Test-Path $root) {
        Start-Sleep -Seconds 2
        if (-not (Get-KioskProcesses)) { exit }
    }
    Stop-Kiosk
    exit
}

$started = Get-Date

# A kiosk left over from an earlier session would absorb the new launch.
Stop-Kiosk

Start-Process msedge -ArgumentList "--kiosk", "--allow-file-access-from-files", "--user-data-dir=$profileDir", "file:///$indexPath"

# Wait for the kiosk window to appear.
$window = $null
for ($i = 0; $i -lt 40 -and -not $window; $i++) {
    Start-Sleep -Milliseconds 500
    $ids = @(Get-KioskProcesses | ForEach-Object { $_.ProcessId })
    if ($ids) {
        $window = Get-Process -Id $ids -ErrorAction SilentlyContinue |
            Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
    }
}
if (-not $window) { exit }

Start-Process powershell -WindowStyle Hidden -ArgumentList "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "`"$PSCommandPath`"", "-Watch"

$remaining = $keystrokeGuardSeconds - ((Get-Date) - $started).TotalSeconds
if ($remaining -gt 0) { Start-Sleep -Seconds $remaining }

# Bring the kiosk to the front. The Alt tap lifts Windows' focus-stealing
# lock so SetForegroundWindow isn't ignored.
Add-Type -Namespace CryptoKiosk -Name Win32 -MemberDefinition @"
[DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
[DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
"@
[CryptoKiosk.Win32]::keybd_event(0x12, 0, 0, [UIntPtr]::Zero)
[CryptoKiosk.Win32]::keybd_event(0x12, 0, 2, [UIntPtr]::Zero)
[void][CryptoKiosk.Win32]::SetForegroundWindow($window.MainWindowHandle)

# Exiting closes this console window; the hidden watcher carries on.
