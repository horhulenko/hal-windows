<#
.SYNOPSIS
    Packages a finished build into dist\HAL (portable folder), dist\HAL-portable.zip and dist\HALSetup.exe.

.DESCRIPTION
    The folder is self-contained: HAL, its plugins, the Qt libraries it uses, an embeddable Python 3.12 with jedi for code
    completion, the Visual C++ runtime and the editor files. Before anything is packed, every DLL is checked for dependencies
    that would be missing on a clean machine, and HAL is started from the folder with an empty PATH.

    HALSetup.exe is built with Inno Setup when it is installed (the runners of GitHub have it after "choco install innosetup").

.EXAMPLE
    .\tools\windows\build.ps1
    .\tools\windows\package.ps1 -Version 2026.11.21.5
#>
param(
    [string]$Build   = 'build-win',
    [string]$Out     = 'dist',
    [string]$Version = 'dev',
    [string]$Deps    = '',
    [string]$Qt      = '',
    [string]$Python  = '',
    [switch]$RequireSetup
)

. "$PSScriptRoot\common.ps1"
$root = Get-RepoRoot
Set-Location $root
if (-not $Deps) { $Deps = Get-DefaultDepsDir }
if (-not $Qt)   { $Qt = "$Deps\Qt\6.12.0\msvc2022_64" }
if (-not $Python) {
    $Python = if ($env:pythonLocation) { $env:pythonLocation } else { "$env:LOCALAPPDATA\Programs\Python\Python312" }
}
Import-VsEnvironment

$stage = Join-Path $root "$Out\HAL"
$bin   = Join-Path $stage 'bin'
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force $bin | Out-Null

# --- HAL itself: executables, DLLs, plugins, Python module; no debug files, logs or leftovers of a developer machine
Write-Host 'Copying the build'
robocopy "$Build\bin" $bin /E /NFL /NDL /NJH /NJS /NP `
    /XF *.pdb *.lib *.exp *.ilk *.txt *.log /XD jedi parso __pycache__ '*.dist-info' 'hal_project_*' | Out-Null
if ($LASTEXITCODE -ge 8) { throw 'Copying the build failed' }
robocopy "$Build\share\hal" (Join-Path $stage 'share\hal') /E /NFL /NDL /NJH /NJS /NP /XF .gitignore | Out-Null
if ($LASTEXITCODE -ge 8) { throw 'Copying the shared files failed' }

# --- Python: the embeddable distribution next to hal.exe, so HAL does not need a Python installation
Write-Host 'Adding Python'
$pythonZip = Join-Path $Deps '_download\python-3.12.10-embed-amd64.zip'
Get-VerifiedFile -Url 'https://www.python.org/ftp/python/3.12.10/python-3.12.10-embed-amd64.zip' -Path $pythonZip `
    -Sha256 '4acbed6dd1c744b0376e3b1cf57ce906f9dc9e95e68824584c8099a63025a3c3'
Expand-Archive -Path $pythonZip -DestinationPath $bin -Force
Remove-Item (Join-Path $bin 'python.exe'), (Join-Path $bin 'pythonw.exe') -ErrorAction SilentlyContinue

# code completion in the Python editor (pure Python packages, picked up through the folder of hal.exe)
& "$Python\python.exe" -m pip install --quiet --disable-pip-version-check --target $bin jedi parso
if ($LASTEXITCODE) { throw 'Installing jedi failed' }
Get-ChildItem $bin -Recurse -Directory -Filter '__pycache__' | Remove-Item -Recurse -Force

# --- Qt: the libraries HAL's GUI uses, platform and style plugins
Write-Host 'Adding Qt'
& "$Qt\bin\windeployqt.exe" --release --no-translations --no-opengl-sw --no-system-d3d-compiler --no-compiler-runtime `
    --dir $bin (Join-Path $bin 'hal_plugins\hal_gui.pyd')
if ($LASTEXITCODE) { throw 'windeployqt failed' }

# --- Visual C++ runtime, application-local so no redistributable has to be installed
Write-Host 'Adding the Visual C++ runtime'
$redist = Join-Path $env:VCToolsRedistDir 'x64'
foreach ($folder in Get-ChildItem $redist -Directory | Where-Object { $_.Name -match 'CRT$|OpenMP$' }) {
    Copy-Item (Join-Path $folder.FullName '*.dll') $bin -Force
}

# --- licenses
$licenses = Join-Path $stage 'licenses'
New-Item -ItemType Directory -Force $licenses | Out-Null
Copy-Item 'LICENSE' (Join-Path $stage 'LICENSE.txt')
Copy-Item 'deps\monaco\LICENSE' (Join-Path $licenses 'monaco-editor.txt')
Copy-Item 'deps\monaco\ThirdPartyNotices.txt' (Join-Path $licenses 'monaco-editor-third-party.txt')
Copy-Item 'plugins\gui\resources\icons\LICENSE-lucide.txt' (Join-Path $licenses 'lucide-icons.txt')

# --- every dependency of every binary has to be in the folder or be a system DLL
Write-Host 'Checking dependencies'
$present = @{}
Get-ChildItem $bin -Recurse -Include *.dll, *.pyd | ForEach-Object { $present[$_.Name.ToLower()] = $true }
$missing = @{}
Get-ChildItem $bin -Recurse -Include *.dll, *.pyd, *.exe | ForEach-Object {
    $binary = $_
    dumpbin /nologo /dependents $binary.FullName | ForEach-Object {
        if ($_ -match '^\s+(\S+\.(dll|pyd))\s*$') {
            $name = $Matches[1].ToLower()
            if ($name -like 'api-ms-win-*' -or $name -like 'ext-ms-win-*' -or $present[$name]) { return }
            if (Test-Path (Join-Path $env:SystemRoot "System32\$name")) { return }
            $missing[$name] = $binary.Name
        }
    }
}
if ($missing.Count) {
    $missing.GetEnumerator() | ForEach-Object { Write-Host "  missing: $($_.Key) (needed by $($_.Value))" }
    throw 'The package is missing libraries'
}

# --- start HAL from the folder with nothing but the system on PATH
Write-Host 'Starting HAL from the package'
$savedPath = $env:PATH
$env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
try {
    $sample = Join-Path $root "$Out\sample"
    if (Test-Path $sample) { Remove-Item $sample -Recurse -Force }
    Expand-Archive -Path (Join-Path $root 'examples\simple_alu.zip') -DestinationPath $sample -Force
    $output = & (Join-Path $bin 'hal.exe') -p (Join-Path $sample 'simple_alu') --no-log 2>&1 | Out-String
    $exit = $LASTEXITCODE

    # the embedded Python with the HAL module and jedi
    $script = Join-Path $root "$Out\smoke.py"
    Set-Content -Path $script -Encoding ascii -Value @(
        'import sys, hal_py, jedi',
        'print("SMOKE python", sys.version_info.major, sys.version_info.minor, "jedi", jedi.__version__, "hal_py ok")'
    )
    $pythonOutput = & (Join-Path $bin 'hal.exe') --python-script $script --no-log 2>&1 | Out-String
    $pythonExit = $LASTEXITCODE
} finally {
    $env:PATH = $savedPath
}
Remove-Item (Join-Path $root "$Out\sample") -Recurse -Force -ErrorAction SilentlyContinue
if ($exit -ne 0 -or $output -match 'cannot load library' -or $output -match 'terminating due to') {
    Write-Host $output
    throw 'HAL does not start from the package'
}
if ($output -notmatch 'deserialized') { Write-Host $output; throw 'HAL did not open the sample project' }
Remove-Item (Join-Path $root "$Out\smoke.py") -ErrorAction SilentlyContinue
if ($pythonExit -ne 0 -or $pythonOutput -notmatch 'SMOKE python 3 12 jedi') { Write-Host $pythonOutput; throw 'The embedded Python does not work' }

# --- the portable archive and the setup
Write-Host 'Creating the archives'
$zip = Join-Path $root "$Out\HAL-portable.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path $stage -DestinationPath $zip -CompressionLevel Optimal

$iscc = Get-Command iscc.exe -ErrorAction SilentlyContinue
if (-not $iscc) {
    $candidate = Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'
    if (Test-Path $candidate) { $iscc = Get-Item $candidate }
}
if ($iscc) {
    $setupVersion = ($Version -replace '\+.*$', '')
    & $iscc.Source /Qp "/DAppVersion=$setupVersion" "/DSourceDir=$stage" "/DOutputDir=$(Join-Path $root $Out)" `
        "/DIconFile=$(Join-Path $root 'app\hal.ico')" (Join-Path $root 'packaging\windows\hal.iss')
    if ($LASTEXITCODE) { throw 'Inno Setup failed' }
} elseif ($RequireSetup) {
    throw 'Inno Setup is not installed (choco install innosetup)'
} else {
    Write-Warning 'Inno Setup is not installed, only the portable folder and zip were created.'
}

Get-ChildItem (Join-Path $root $Out) -File | ForEach-Object { Write-Host ('{0,-20} {1,8:N1} MB' -f $_.Name, ($_.Length / 1MB)) }
