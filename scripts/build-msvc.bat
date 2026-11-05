@echo off
rem Configure, build, and test PhotoCarta with the machine-wide MSVC + Qt SDK.
rem
rem The Qt runtime DLLs are not on PATH by default with the Ninja generator, so
rem every step runs inside an MSVC developer environment with Qt's bin directory
rem prepended. Missing Qt6*.dll produces exit code 0xC0000135 (STATUS_DLL_NOT_FOUND).
rem
rem Usage:  scripts\build-msvc.bat [configure^|build^|test^|all]
rem         default: all
rem
rem Prints plain ASCII only. Do not expand paths containing "(" inside a
rem parenthesized block: cmd closes the block at the first ")" it sees.
rem
rem VSROOT and QTDIR may be set in the environment to match another layout;
rem the defaults below describe the project's reference machine.

setlocal EnableExtensions

if not defined VSROOT set "VSROOT=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools"
if not defined QTDIR set "QTDIR=D:\Qt\6.8.3\msvc2022_64"
set "CMAKEBIN=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
set "NINJADIR=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"

if not exist "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" goto no_vcvars
if not exist "%QTDIR%\lib\cmake\Qt6\Qt6Config.cmake" goto no_qt
if not exist "%~dp0..\CMakeUserPresets.json" goto no_preset

call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=%CMAKEBIN%;%NINJADIR%;%QTDIR%\bin;%PATH%"
where cmake >nul 2>nul
if errorlevel 1 goto no_cmake

pushd "%~dp0.."

set "STEP=%~1"
if "%STEP%"=="" set "STEP=all"
if /i "%STEP%"=="configure" goto configure
if /i "%STEP%"=="build" goto build
if /i "%STEP%"=="test" goto test
if /i "%STEP%"=="all" goto all
echo ERROR: unknown step "%STEP%". Use configure, build, test, or all.
goto fail

:configure
cmake --preset dev-local -G Ninja
if errorlevel 1 goto fail
if /i not "%STEP%"=="configure" goto build
goto done

:build
cmake --build --preset dev-local
if errorlevel 1 goto fail
if /i not "%STEP%"=="build" goto test
goto done

:test
ctest --preset dev-local
if errorlevel 1 goto fail
goto done

:all
cmake --preset dev-local -G Ninja
if errorlevel 1 goto fail
cmake --build --preset dev-local
if errorlevel 1 goto fail
ctest --preset dev-local
if errorlevel 1 goto fail
goto done

:no_vcvars
echo ERROR: vcvars64.bat not found under the Visual Studio Build Tools directory.
echo Install the "Desktop development with C++" workload, then edit VSROOT
echo or set VSROOT in the environment.
exit /b 1

:no_qt
echo ERROR: Qt 6 not found under the configured Qt directory.
echo Install it with:
echo   python -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 -O D:/Qt
echo Then edit QTDIR or set QTDIR in the environment.
exit /b 1

:no_preset
echo ERROR: CMakeUserPresets.json is missing; its dev-local preset is required.
echo Create it in the repository root with a dev-local preset that inherits
echo "default" and sets CMAKE_PREFIX_PATH to your Qt 6 SDK.
echo See docs/build-instructions.md for the expected contents.
exit /b 1

:no_cmake
echo ERROR: cmake was not found on PATH or in the Visual Studio installation.
echo Install CMake 3.22 or newer and put it on PATH.
exit /b 1

:fail
echo.
echo FAILED during step "%STEP%".
popd
exit /b 1

:done
popd
exit /b 0
