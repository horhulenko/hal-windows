# Helpers shared by the scripts in this folder. Dot-source it: . "$PSScriptRoot\common.ps1"

$ErrorActionPreference = 'Stop'

function Get-RepoRoot {
    (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
}

function Get-DefaultDepsDir {
    Join-Path (Split-Path -Parent (Get-RepoRoot)) 'hal-windows-deps'
}

# Loads the MSVC x64 build environment (compiler, linker, CMake and Ninja of Visual Studio) into this session.
function Import-VsEnvironment {
    param([string]$VsDir)

    if (-not $VsDir) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
        if (-not (Test-Path $vswhere)) { throw 'Visual Studio with the C++ workload is required (vswhere.exe not found).' }
        $VsDir = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if (-not $VsDir) { throw 'No Visual Studio installation with the C++ tools found.' }
    }
    $vcvars = Join-Path $VsDir 'VC\Auxiliary\Build\vcvars64.bat'
    cmd /c "`"$vcvars`" >nul 2>&1 && set" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') { Set-Item -Path "env:$($Matches[1])" -Value $Matches[2] }
    }
}

# Downloads a file and refuses it when its checksum does not match. Pass -Sha256 or -Sha512Base64.
function Get-VerifiedFile {
    param(
        [Parameter(Mandatory)][string]$Url,
        [Parameter(Mandatory)][string]$Path,
        [string]$Sha256,
        [string]$Sha512Base64
    )
    if (-not (Test-Path $Path)) {
        New-Item -ItemType Directory -Force (Split-Path -Parent $Path) | Out-Null
        Write-Host "Downloading $Url"
        Invoke-WebRequest -Uri $Url -OutFile $Path -UseBasicParsing
    }
    if ($Sha256) {
        $actual = (Get-FileHash -Algorithm SHA256 $Path).Hash
        if ($actual -ne $Sha256.ToUpper()) { Remove-Item $Path; throw "SHA-256 mismatch for $Url`n expected $Sha256`n actual   $actual" }
    }
    if ($Sha512Base64) {
        $sha    = [System.Security.Cryptography.SHA512]::Create()
        $actual = [Convert]::ToBase64String($sha.ComputeHash([System.IO.File]::ReadAllBytes($Path)))
        if ($actual -ne $Sha512Base64) { Remove-Item $Path; throw "SHA-512 mismatch for $Url" }
    }
}

function Expand-Any {
    param([string]$Archive, [string]$Destination, [string]$Python = 'python')
    New-Item -ItemType Directory -Force $Destination | Out-Null
    if ($Archive -like '*.zip') {
        Expand-Archive -Path $Archive -DestinationPath $Destination -Force
    } elseif ($Archive -like '*.7z') {
        # the tar of Windows cannot read every .7z archive, py7zr can
        & $Python -m pip install --quiet --disable-pip-version-check py7zr
        if ($LASTEXITCODE) { throw 'Cannot install py7zr' }
        & $Python -c "import py7zr, sys; py7zr.SevenZipFile(sys.argv[1]).extractall(sys.argv[2])" $Archive $Destination
        if ($LASTEXITCODE) { throw "Cannot extract $Archive" }
    } else {
        tar -xf $Archive -C $Destination
        if ($LASTEXITCODE -ne 0) { throw "Cannot extract $Archive" }
    }
}
