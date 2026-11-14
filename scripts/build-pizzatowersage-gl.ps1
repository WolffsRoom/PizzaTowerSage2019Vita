param([ValidateSet('Debug','Release')][string]$Configuration = 'Release')

$ErrorActionPreference = 'Stop'
$rootPath = Split-Path -Parent $PSScriptRoot
$sdkVitaPath = Split-Path -Parent $rootPath
$project = Join-Path $rootPath 'src'

Write-Host "Iniciando compilação do Pizza Tower SAGE 2019 Demo para PS Vita (VitaGL)..."

$sdkVitaUnix = $sdkVitaPath.Replace('\', '/')
$sageUnix = $rootPath.Replace('\', '/')

docker run --rm `
    -v "${sdkVitaUnix}:/SDKVita" `
    -v "${sageUnix}:/sage" `
    -w "/sage/src" `
    atamanenko/vitasdk-softfp:latest sh -lc `
    "cmake -S . -B build-vita -DCMAKE_BUILD_TYPE=$Configuration && cmake --build build-vita -j2"

if ($LASTEXITCODE -ne 0) { throw "Build falhou com codigo $LASTEXITCODE." }

$output = Join-Path $rootPath 'artifacts\current\PizzaTowerSAGE-v0.55.vpk'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $output) | Out-Null
Copy-Item -Force (Join-Path $project 'build-vita\PizzaTowerSAGE-v0.55.vpk') $output

$releaseDir = Join-Path $rootPath 'artifacts\Release v0.55'
New-Item -ItemType Directory -Force -Path $releaseDir | Out-Null
Copy-Item -Force (Join-Path $project 'build-vita\PizzaTowerSAGE-v0.55.vpk') (Join-Path $releaseDir 'PizzaTowerSAGE-v0.55.vpk')

Write-Host "VPK gerado com sucesso: $output"
Write-Host "Copia de release: $(Join-Path $releaseDir 'PizzaTowerSAGE-v0.55.vpk')"
