$rootPath = Split-Path -Parent $PSScriptRoot
$vpk = Join-Path $rootPath "artifacts\Release v0.55\PizzaTowerSAGE-v0.55.vpk"
if (-not (Test-Path $vpk)) {
    $vpk = Join-Path $rootPath "artifacts\current\PizzaTowerSAGE-v0.55.vpk"
}
if (-not (Test-Path $vpk)) {
    $vpk = Join-Path $rootPath "artifacts\Release v0.54\PizzaTowerSAGE-v0.54.vpk"
}
if (-not (Test-Path $vpk)) {
    $vpk = Join-Path $rootPath "artifacts\Release v0.51\PizzaTowerSAGE-v0.51.vpk"
}
if (-not (Test-Path $vpk)) {
    $vpk = Join-Path $rootPath "artifacts\Release v0.50\PizzaTowerSAGE-v0.50.vpk"
}
$destApp = "C:\Users\wolff\AppData\Roaming\Vita3K\Vita3K\ux0\app\PTSAGE001"
$destData = "C:\Users\wolff\AppData\Roaming\Vita3K\Vita3K\ux0\data\pizzatowersage"

if (Test-Path $destApp) { Remove-Item -Recurse -Force $destApp }
New-Item -ItemType Directory -Path $destApp -Force | Out-Null
tar -xf "$vpk" -C "$destApp"

if (-not (Test-Path $destData)) { New-Item -ItemType Directory -Path $destData -Force | Out-Null }
$destDataWin = Join-Path $destData "data.win"
if (-not (Test-Path $destDataWin)) {
    Copy-Item "C:\Users\wolff\Documents\SDKVita\PizzaTowerSAGE Vita\data.win" -Destination $destDataWin -Force
}

Write-Host "Staging complete."
Get-ChildItem $destApp
Get-ChildItem $destData
