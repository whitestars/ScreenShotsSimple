param(
    [string]$ExePath = "build/bin/Release/ScreenShotApp.exe",
    [int]$SettleSeconds = 3,
    [string]$LaunchArgs = "",
    [switch]$NoSession
)

$ErrorActionPreference = 'Stop'

$exe = (Resolve-Path -LiteralPath $ExePath).Path
$exeBytes = (Get-Item -LiteralPath $exe).Length

function Read-Memory([int]$ProcessId) {
    $process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
    if (-not $process) { return $null }
    $process.Refresh()
    return [pscustomobject]@{
        WorkingSetMB = [math]::Round($process.WorkingSet64 / 1MB, 2)
        PrivateMB    = [math]::Round($process.PrivateMemorySize64 / 1MB, 2)
        Handles      = $process.HandleCount
    }
}

function Invoke-Flag([string]$Flag) {
    Start-Process -FilePath $exe -ArgumentList $Flag -PassThru -Wait | Out-Null
}

$process = if ($LaunchArgs) {
    Start-Process -FilePath $exe -ArgumentList $LaunchArgs -PassThru
} else {
    Start-Process -FilePath $exe -PassThru
}
try {
    Start-Sleep -Seconds $SettleSeconds
    $idle = Read-Memory $process.Id

    $peak = $null
    $reclaimed = $null
    if (-not $NoSession -and $idle) {
        Invoke-Flag '--snapshot'
        Start-Sleep -Seconds 2
        $peak = Read-Memory $process.Id

        Invoke-Flag '--cancel'
        Start-Sleep -Seconds 4
        $reclaimed = Read-Memory $process.Id
    }

    $pass = if ($idle -and $idle.WorkingSetMB -lt 20) { 'True' } else { 'False' }

    Write-Output "[ScreenShotApp metrics]"
    Write-Output ("ExePath: {0}" -f $exe)
    Write-Output ("ExeBytes: {0}" -f $exeBytes)
    Write-Output ("ExeKB: {0}" -f [math]::Round($exeBytes / 1KB, 2))
    if ($idle) {
        Write-Output ("IdleWorkingSetMB: {0}" -f $idle.WorkingSetMB)
        Write-Output ("IdlePrivateMB: {0}" -f $idle.PrivateMB)
        Write-Output ("IdleHandles: {0}" -f $idle.Handles)
    }
    if ($peak) {
        Write-Output ("PeakWorkingSetMB: {0}" -f $peak.WorkingSetMB)
        Write-Output ("PeakPrivateMB: {0}" -f $peak.PrivateMB)
    }
    if ($reclaimed) {
        Write-Output ("ReclaimedWorkingSetMB: {0}" -f $reclaimed.WorkingSetMB)
        Write-Output ("ReclaimedPrivateMB: {0}" -f $reclaimed.PrivateMB)
    }
    Write-Output ("Pass20MB: {0}" -f $pass)
}
finally {
    if (Get-Process -Id $process.Id -ErrorAction SilentlyContinue) {
        Invoke-Flag '--quit'
        Start-Sleep -Milliseconds 600
        if (Get-Process -Id $process.Id -ErrorAction SilentlyContinue) {
            Stop-Process -Id $process.Id -Force
        }
    }
}
