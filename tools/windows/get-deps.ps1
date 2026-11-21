<#
.SYNOPSIS
    Downloads and prepares the third-party dependencies of the Windows build.

.DESCRIPTION
    Everything lands in one folder next to the repository (default: ..\hal-windows-deps), a step is skipped when its
    result is already there. Every download is checked against a pinned checksum.

      boost      header-only, used by the Boolean function parsers
      rapidjson  header-only JSON
      z3         SMT solver (prebuilt)
      zlib       built here, used by QuaZip for project archives
      webview2   Microsoft WebView2 SDK, hosts the Monaco code editor
      Qt         Qt 6.12 (base, SVG, Qt5Compat) from the official Qt repository

.EXAMPLE
    .\tools\windows\get-deps.ps1
#>
param(
    [string]$Deps   = '',
    [string]$Python = 'python'
)

. "$PSScriptRoot\common.ps1"
if (-not $Deps) { $Deps = Get-DefaultDepsDir }
New-Item -ItemType Directory -Force $Deps | Out-Null
$cache = Join-Path $Deps '_download'

# --- Boost headers
if (-not (Test-Path "$Deps\boost\boost\version.hpp")) {
    $archive = "$cache\boost-1.92.0-b2-nodocs.7z"
    Get-VerifiedFile -Url 'https://github.com/boostorg/boost/releases/download/boost-1.92.0/boost-1.92.0-b2-nodocs.7z' -Path $archive `
        -Sha256 '4d04137cc06b117701063f1e5e42875c52389158a8050f2845988f9871e04046'
    Expand-Any $archive "$cache\boost-x" $Python
    Move-Item "$cache\boost-x\boost-1.92.0" "$Deps\boost"
    Remove-Item "$cache\boost-x" -Recurse -Force
}

# --- RapidJSON (a pinned snapshot of the master branch)
if (-not (Test-Path "$Deps\rapidjson\include\rapidjson\rapidjson.h")) {
    $archive = "$cache\rapidjson.zip"
    Get-VerifiedFile -Url 'https://github.com/Tencent/rapidjson/archive/297637bc0a0f00dc504dbc1a47ebdaa577f8ae19.zip' -Path $archive `
        -Sha256 '59c2919b6ce918dbc8336e904873806fa8acda9372dc9a8c43434accd79818b8'
    Expand-Any $archive "$cache\rapidjson-x"
    New-Item -ItemType Directory -Force "$Deps\rapidjson" | Out-Null
    Move-Item (Get-ChildItem "$cache\rapidjson-x\rapidjson-*\include").FullName "$Deps\rapidjson\include"
    Remove-Item "$cache\rapidjson-x" -Recurse -Force
}

# --- Z3
if (-not (Test-Path "$Deps\z3\bin\libz3.lib")) {
    $archive = "$cache\z3-5.3.3-x64-win.zip"
    Get-VerifiedFile -Url 'https://github.com/Z3Prover/z3/releases/download/z3-5.3.3/z3-5.3.3-x64-win.zip' -Path $archive `
        -Sha256 '4e48624a6895b49606b3cf91ca4fbcc069fb2973b9706532170a144baa3fd798'
    Expand-Any $archive "$cache\z3-x"
    Move-Item "$cache\z3-x\z3-5.3.3-x64-win" "$Deps\z3"
    Remove-Item "$cache\z3-x" -Recurse -Force
}

# --- WebView2 SDK (NuGet package, only headers and the static loader are used)
if (-not (Test-Path "$Deps\webview2\build\native\include\WebView2.h")) {
    $archive = "$cache\webview2.zip"
    Get-VerifiedFile -Url 'https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/1.0.4370.26/microsoft.web.webview2.1.0.4370.26.nupkg' -Path $archive `
        -Sha512Base64 '6y+vmgBDP/1YeSZiQt9JRSj3w5ffdiL/uu/SwIQ3hSgyvqcnRGunhOvwGGG5WOrDp150G6MP8JjRG8AtW5FZ5w=='
    Expand-Any $archive "$Deps\webview2"
}

# --- zlib, built once with MSVC
if (-not (Test-Path "$Deps\zlib\lib\zlibstatic.lib")) {
    $archive = "$cache\zlib-1.3.1.tar.gz"
    Get-VerifiedFile -Url 'https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz' -Path $archive `
        -Sha256 '9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23'
    Expand-Any $archive "$cache\zlib-x"
    Import-VsEnvironment
    $prefix = $Deps -replace '\\', '/'
    cmake -S "$cache\zlib-x\zlib-1.3.1" -B "$cache\zlib-build" -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_INSTALL_PREFIX=$prefix/zlib"
    if ($LASTEXITCODE) { throw 'zlib configure failed' }
    cmake --build "$cache\zlib-build"
    if ($LASTEXITCODE) { throw 'zlib build failed' }
    cmake --install "$cache\zlib-build"
    if ($LASTEXITCODE) { throw 'zlib install failed' }
    Remove-Item "$cache\zlib-x", "$cache\zlib-build" -Recurse -Force
}

# --- Qt
if (-not (Test-Path "$Deps\Qt\6.12.0\msvc2022_64\bin\Qt6Core.dll")) {
    & $Python -m pip install --quiet py7zr
    if ($LASTEXITCODE) { throw 'Cannot install py7zr' }
    & $Python "$PSScriptRoot\get_qt.py" "$Deps\Qt"
    if ($LASTEXITCODE) { throw 'Qt download failed' }
}

Write-Host "Dependencies are ready in $Deps"
