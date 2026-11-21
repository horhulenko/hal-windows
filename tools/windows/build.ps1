<#
.SYNOPSIS
    Configures, builds and optionally runs HAL natively on Windows (MSVC + Ninja).

.EXAMPLE
    .\tools\windows\build.ps1
    .\tools\windows\build.ps1 -Run
#>
param(
    [string]$Qt     = 'C:\Programs\Qt\6.12.0\msvc2022_64',
    [string]$Deps   = (Join-Path (Split-Path -Parent (Resolve-Path "$PSScriptRoot\..\..")) 'hal-windows-deps'),
    [string]$Python = "$env:LOCALAPPDATA\Programs\Python\Python312",
    [string]$VsDir  = 'C:\Program Files\Microsoft Visual Studio\2022\Community',
    [string]$Build  = 'build-win',
    [switch]$Run
)

$ErrorActionPreference = 'Stop'
$root = Resolve-Path "$PSScriptRoot\..\.."
Set-Location $root

# Import the MSVC x64 environment
$vcvars = Join-Path $VsDir 'VC\Auxiliary\Build\vcvars64.bat'
cmd /c "`"$vcvars`" >nul 2>&1 && set" | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') { Set-Item -Path "env:$($Matches[1])" -Value $Matches[2] }
}

function ToCMake($p) { $p -replace '\\', '/' }

# Expected layout of the dependency folder: boost, rapidjson, z3, zlib and webview2 (the unpacked NuGet package Microsoft.Web.WebView2)
cmake -S . -B $Build -G Ninja -DCMAKE_BUILD_TYPE=Release `
    "-DCMAKE_PREFIX_PATH=$(ToCMake $Qt);$(ToCMake $Deps)/zlib" `
    "-DBOOST_ROOT=$(ToCMake $Deps)/boost" `
    "-DRAPIDJSON_ROOT=$(ToCMake $Deps)/rapidjson" `
    "-DZ3_ROOT=$(ToCMake $Deps)/z3" `
    "-DWEBVIEW2_ROOT=$(ToCMake $Deps)/webview2" `
    "-DPython3_ROOT_DIR=$(ToCMake $Python)" -DPython3_FIND_STRATEGY=LOCATION @args
if ($LASTEXITCODE) { throw 'configure failed' }

cmake --build $Build
if ($LASTEXITCODE) { throw 'build failed' }

# libz3 has to sit next to the HAL DLLs
Copy-Item (Join-Path $Deps 'z3\bin\libz3.dll') (Join-Path $Build 'bin') -Force

if ($Run) {
    $env:PATH       = "$Qt\bin;$Python;$env:PATH"
    $env:PYTHONHOME = $Python
    & (Join-Path $Build 'bin\hal.exe') --gui
}
