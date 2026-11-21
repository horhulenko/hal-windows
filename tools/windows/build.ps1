<#
.SYNOPSIS
    Configures and builds HAL natively on Windows (MSVC + Ninja).

.DESCRIPTION
    Needs Visual Studio with the C++ workload, Python 3.12 and the dependencies of get-deps.ps1.
    Extra arguments are passed on to CMake, e.g. -DPL_SIMULATOR=ON.

.EXAMPLE
    .\tools\windows\get-deps.ps1
    .\tools\windows\build.ps1
    .\tools\windows\build.ps1 -Run
#>
param(
    [string]$Deps   = '',
    [string]$Qt     = '',
    [string]$Python = '',
    [string]$Build  = 'build-win',
    [switch]$Run
)

. "$PSScriptRoot\common.ps1"
$root = Get-RepoRoot
Set-Location $root
if (-not $Deps) { $Deps = Get-DefaultDepsDir }
if (-not $Qt)   { $Qt = "$Deps\Qt\6.12.0\msvc2022_64" }
if (-not $Python) {
    # the Python of actions/setup-python, otherwise the usual per-user location
    $Python = if ($env:pythonLocation) { $env:pythonLocation } else { "$env:LOCALAPPDATA\Programs\Python\Python312" }
}
foreach ($required in @($Qt, $Deps, $Python)) {
    if (-not (Test-Path $required)) { throw "Not found: $required (run tools\windows\get-deps.ps1 first)" }
}

Import-VsEnvironment

function ToCMake($path) { $path -replace '\\', '/' }

# Other toolchains on the machine (MinGW from Strawberry Perl or Git) ship libraries that must not leak into an MSVC build
$ignore = @('C:/Strawberry/c', 'C:/Programs/Strawberry/c', 'C:/Program Files/Git/mingw64', 'C:/msys64/mingw64', 'C:/mingw64') -join ';'

cmake -S . -B $Build -G Ninja -DCMAKE_BUILD_TYPE=Release `
    "-DCMAKE_PREFIX_PATH=$(ToCMake $Qt);$(ToCMake $Deps)/zlib" `
    "-DCMAKE_IGNORE_PREFIX_PATH=$ignore" `
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
    $env:PATH = "$Qt\bin;$Python;$env:PATH"
    $env:PYTHONHOME = $Python
    & (Join-Path $Build 'bin\hal-gui.exe')
}
