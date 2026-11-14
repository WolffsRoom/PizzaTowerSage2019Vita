param(
    [int]$Duration = 25
)

$vita3kPath = "C:\Users\wolff\Documents\Vita3k\Vita3K.exe"
Write-Host "Launching Vita3K with PTSAGE001..."
$proc = Start-Process -FilePath $vita3kPath -ArgumentList "-r", "PTSAGE001" -PassThru

$waited = 0
while ($waited -lt $Duration -and -not $proc.HasExited) {
    Start-Sleep -Seconds 2
    $waited += 2
}

if (-not $proc.HasExited) {
    Write-Host "Stopping Vita3K after $waited seconds..."
    Stop-Process -Id $proc.Id -Force
} else {
    Write-Host "Vita3K exited on its own with code $($proc.ExitCode)."
}

