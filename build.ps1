# --- Load build.env if present ---
$envFile = Join-Path $PSScriptRoot "build.env"
if (Test-Path $envFile) {
    Get-Content $envFile | ForEach-Object {
        if ($_ -match "^\s*([^#][^=]+)=(.+)$") {
            [System.Environment]::SetEnvironmentVariable($matches[1].Trim(), $matches[2].Trim(), "Process")
        }
    }
}

# --- Detect Visual Studio ---
if (-not $env:VS_PATH) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        Write-Error "vswhere.exe not found. Install Visual Studio Build Tools."; exit 1
    }
    $env:VS_PATH = & $vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $env:VS_PATH) {
        Write-Error "No Visual Studio with C++ tools found."; exit 1
    }
}

# Capture VS environment
$vcvarsall = Join-Path $env:VS_PATH "VC\Auxiliary\Build\vcvarsall.bat"
$vsEnv = cmd /c "`"$vcvarsall`" x64 && set" 2>&1
foreach ($line in $vsEnv) {
    if ($line -match "^([^=]+)=(.*)$") {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}

# --- Detect CMake ---
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    $qtRoots = @("C:\Qt", "$env:LOCALAPPDATA\Qt", "C:\Programs\Qt", "D:\Qt")
    if ($env:QT_TOOLS_ROOT) { $qtRoots = @($env:QT_TOOLS_ROOT) + $qtRoots }
    $cmakePath = $qtRoots | ForEach-Object { Join-Path $_ "Tools\CMake_64\bin" } |
        Where-Object { Test-Path (Join-Path $_ "cmake.exe") } | Select-Object -First 1
    if (-not $cmakePath) { Write-Error "cmake not found on PATH or in Qt Tools."; exit 1 }
    $env:PATH = "$cmakePath;$env:PATH"
}

# --- Detect Ninja ---
if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
    $qtRoots = @("C:\Qt", "$env:LOCALAPPDATA\Qt", "C:\Programs\Qt", "D:\Qt")
    if ($env:QT_TOOLS_ROOT) { $qtRoots = @($env:QT_TOOLS_ROOT) + $qtRoots }
    $ninjaPath = $qtRoots | ForEach-Object { Join-Path $_ "Tools\Ninja" } |
        Where-Object { Test-Path (Join-Path $_ "ninja.exe") } | Select-Object -First 1
    if (-not $ninjaPath) { Write-Error "ninja not found on PATH or in Qt Tools."; exit 1 }
    $env:PATH = "$ninjaPath;$env:PATH"
}

# --- Detect QTDIR ---
if (-not $env:QTDIR) {
    $qtRoots = @("C:\Qt", "$env:LOCALAPPDATA\Qt", "C:\Programs\Qt", "D:\Qt")
    if ($env:QT_TOOLS_ROOT) { $qtRoots = @($env:QT_TOOLS_ROOT) + $qtRoots }
    $qtdir = $null
    foreach ($root in $qtRoots) {
        if (-not (Test-Path $root)) { continue }
        $versions = Get-ChildItem $root -Directory -Filter "6.*" |
            Sort-Object { [version]$_.Name } -Descending
        foreach ($ver in $versions) {
            $candidate = Join-Path $ver.FullName "msvc2022_64"
            if (Test-Path (Join-Path $candidate "bin\qmake.exe")) {
                $qtdir = $candidate; break
            }
        }
        if ($qtdir) { break }
    }
    if (-not $qtdir) {
        Write-Error "Could not detect QTDIR. Set it manually via build.env or:"
        Write-Error "  `$env:QTDIR = 'C:\Qt\6.x.x\msvc2022_64'"
        exit 1
    }
    $env:QTDIR = $qtdir
}
Write-Host "Using QTDIR=$env:QTDIR"

# --- Build ---
Set-Location $PSScriptRoot

if (Test-Path "out\build\x64-debug") {
    Remove-Item -Recurse -Force "out\build\x64-debug"
}

Write-Host "=== CONFIGURING ==="
& cmake --preset x64-debug
if ($LASTEXITCODE -ne 0) {
    Write-Host "=== CONFIGURE FAILED ==="
    exit 1
}

Write-Host "=== BUILDING ==="
& cmake --build out/build/x64-debug
if ($LASTEXITCODE -ne 0) {
    Write-Host "=== BUILD FAILED ==="
    exit 1
}

Write-Host "=== BUILD SUCCEEDED ==="
