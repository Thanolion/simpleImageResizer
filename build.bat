@echo off
setlocal enabledelayedexpansion

REM --- Load build.env if present ---
if exist "%~dp0build.env" (
    for /f "usebackq tokens=1,* delims==" %%a in ("%~dp0build.env") do (
        set "LINE=%%a"
        if not "!LINE:~0,1!"=="#" if not "%%b"=="" set "%%a=%%b"
    )
)

REM --- Detect Visual Studio ---
if not defined VS_PATH (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo ERROR: vswhere.exe not found. Install Visual Studio Build Tools.
        exit /b 1
    )
    for /f "usebackq delims=" %%i in (`"!VSWHERE!" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_PATH=%%i"
    if not defined VS_PATH (
        echo ERROR: No Visual Studio with C++ tools found.
        exit /b 1
    )
)
call "%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

REM --- Detect CMake ---
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    set "CMAKE_FOUND="
    if defined QT_TOOLS_ROOT (
        if exist "!QT_TOOLS_ROOT!\Tools\CMake_64\bin\cmake.exe" (
            set "PATH=!QT_TOOLS_ROOT!\Tools\CMake_64\bin;!PATH!"
            set "CMAKE_FOUND=1"
        )
    )
    for %%d in ("C:\Qt\Tools\CMake_64\bin" "%LOCALAPPDATA%\Qt\Tools\CMake_64\bin" "C:\Programs\Qt\Tools\CMake_64\bin" "D:\Qt\Tools\CMake_64\bin") do (
        if not defined CMAKE_FOUND if exist "%%~d\cmake.exe" (
            set "PATH=%%~d;!PATH!"
            set "CMAKE_FOUND=1"
        )
    )
    if not defined CMAKE_FOUND (
        echo ERROR: cmake not found on PATH or in Qt Tools.
        exit /b 1
    )
)

REM --- Detect Ninja ---
where ninja >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    set "NINJA_FOUND="
    if defined QT_TOOLS_ROOT (
        if exist "!QT_TOOLS_ROOT!\Tools\Ninja\ninja.exe" (
            set "PATH=!QT_TOOLS_ROOT!\Tools\Ninja;!PATH!"
            set "NINJA_FOUND=1"
        )
    )
    for %%d in ("C:\Qt\Tools\Ninja" "%LOCALAPPDATA%\Qt\Tools\Ninja" "C:\Programs\Qt\Tools\Ninja" "D:\Qt\Tools\Ninja") do (
        if not defined NINJA_FOUND if exist "%%~d\ninja.exe" (
            set "PATH=%%~d;!PATH!"
            set "NINJA_FOUND=1"
        )
    )
    if not defined NINJA_FOUND (
        echo ERROR: ninja not found on PATH or in Qt Tools.
        exit /b 1
    )
)

REM --- Detect QTDIR ---
if not defined QTDIR (
    set "QTDIR_FOUND="
    if defined QT_TOOLS_ROOT (
        for /f "delims=" %%v in ('dir /b /ad /o-n "!QT_TOOLS_ROOT!\6.*" 2^>nul') do (
            if not defined QTDIR_FOUND if exist "!QT_TOOLS_ROOT!\%%v\msvc2022_64\bin\qmake.exe" (
                set "QTDIR=!QT_TOOLS_ROOT!\%%v\msvc2022_64"
                set "QTDIR_FOUND=1"
            )
        )
    )
    for %%r in ("C:\Qt" "%LOCALAPPDATA%\Qt" "C:\Programs\Qt" "C:\Programs\QT" "D:\Qt") do (
        if not defined QTDIR_FOUND if exist "%%~r" (
            for /f "delims=" %%v in ('dir /b /ad /o-n "%%~r\6.*" 2^>nul') do (
                if not defined QTDIR_FOUND if exist "%%~r\%%v\msvc2022_64\bin\qmake.exe" (
                    set "QTDIR=%%~r\%%v\msvc2022_64"
                    set "QTDIR_FOUND=1"
                )
            )
        )
    )
    if not defined QTDIR (
        echo ERROR: Could not detect QTDIR. Set it manually:
        echo   set QTDIR=C:\Qt\6.x.x\msvc2022_64
        exit /b 1
    )
)
echo Using QTDIR=%QTDIR%

REM --- Build ---
cd /d "%~dp0"
if exist out\build\x64-debug rmdir /s /q out\build\x64-debug
echo === CONFIGURING === > build.log 2>&1
cmake --preset x64-debug >> build.log 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo === CONFIGURE FAILED === >> build.log 2>&1
    exit /b 1
)
echo === BUILDING === >> build.log 2>&1
cmake --build out/build/x64-debug >> build.log 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo === BUILD FAILED === >> build.log 2>&1
    exit /b 1
)
echo === BUILD SUCCEEDED === >> build.log 2>&1
exit /b 0
