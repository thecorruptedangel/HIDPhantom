@echo off
setlocal EnableDelayedExpansion

:: ============================================================
:: HIDPhantom Build Script
:: Installs prerequisites and builds RP2040 firmware
:: Usage: build.bat [clean]
:: ============================================================

set "PICO_SDK_VERSION=2.3.1"
set "SDK_ROOT=%USERPROFILE%\.pico-sdk\sdk\%PICO_SDK_VERSION%"
set "ARM_BIN=C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\12.2 mpacbti-rel1\bin"
set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
set "VCVARSALL=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%build"

:: ============================================================
:: 1. Check / install prerequisites
:: ============================================================
echo [1/5] Checking prerequisites...

:: ARM GCC
if not exist "%ARM_BIN%\arm-none-eabi-gcc.exe" (
    echo Installing ARM GNU Toolchain...
    winget install --id Arm.ArmGnuToolchain --silent --accept-package-agreements --accept-source-agreements
    if errorlevel 1 ( echo ERROR: ARM GCC install failed. & exit /b 1 )
) else (
    echo   ARM GCC: OK
)

:: CMake
if not exist "%CMAKE_EXE%" (
    echo Installing CMake...
    winget install --id Kitware.CMake --silent --accept-package-agreements --accept-source-agreements
    if errorlevel 1 ( echo ERROR: CMake install failed. & exit /b 1 )
    :: refresh PATH
    for /f "delims=" %%i in ('where cmake 2^>nul') do set "CMAKE_EXE=%%i"
) else (
    echo   CMake: OK
)

:: Ninja
where ninja >nul 2>&1
if errorlevel 1 (
    echo Installing Ninja...
    winget install --id Ninja-build.Ninja --silent --accept-package-agreements --accept-source-agreements
    if errorlevel 1 ( echo ERROR: Ninja install failed. & exit /b 1 )
)
for /f "delims=" %%i in ('where ninja 2^>nul') do set "NINJA_EXE=%%i"
echo   Ninja: %NINJA_EXE%

:: Visual Studio (needed for host tools: pioasm, picotool)
if not exist "%VCVARSALL%" (
    echo ERROR: Visual Studio 2022 not found at expected path.
    echo Please install VS 2022 Community with C++ Desktop workload.
    echo https://visualstudio.microsoft.com/downloads/
    exit /b 1
) else (
    echo   VS 2022: OK
)

:: ============================================================
:: 2. Clone / update Pico SDK 2.3.1
:: ============================================================
echo [2/5] Checking Pico SDK %PICO_SDK_VERSION%...
if not exist "%SDK_ROOT%\CMakeLists.txt" (
    echo Cloning Pico SDK %PICO_SDK_VERSION%...
    git clone --depth 1 --branch %PICO_SDK_VERSION% https://github.com/raspberrypi/pico-sdk.git "%SDK_ROOT%"
    if errorlevel 1 ( echo ERROR: SDK clone failed. & exit /b 1 )
) else (
    echo   SDK: OK
)

:: Initialize required submodules
echo Checking SDK submodules...
cd /d "%SDK_ROOT%"
if not exist "lib\tinyusb\src" (
    echo   Initializing TinyUSB submodule...
    git submodule update --init lib/tinyusb
    if errorlevel 1 ( echo ERROR: TinyUSB submodule init failed. & exit /b 1 )
)
if not exist "lib\lwip\src" (
    echo   Initializing lwIP submodule...
    git submodule update --init lib/lwip
    if errorlevel 1 ( echo ERROR: lwIP submodule init failed. & exit /b 1 )
)
cd /d "%SCRIPT_DIR%"

:: ============================================================
:: 3. Apply TinyUSB KB5124008 NCM patch (PR #3914)
::    wNdbInDivisor and wNdbOutDivisor: 1 -> 4
:: ============================================================
echo [3/5] Applying TinyUSB KB5124008 NCM patch...
set "NCM_FILE=%SDK_ROOT%\lib\tinyusb\src\class\net\ncm_device.c"
python -c "
import sys
with open(r'%NCM_FILE%', 'r') as f: src = f.read()
patched = src.replace('.wNdbInDivisor            = 1,', '.wNdbInDivisor            = 4,')
patched = patched.replace('.wNdbOutDivisor           = 1,', '.wNdbOutDivisor           = 4,')
if patched == src:
    print('  Patch already applied or file changed - skipping')
else:
    with open(r'%NCM_FILE%', 'w') as f: f.write(patched)
    print('  Patch applied: wNdbInDivisor and wNdbOutDivisor set to 4')
" 2>&1
if errorlevel 1 ( echo WARNING: NCM patch script failed. Continuing... )

:: ============================================================
:: 4. CMake configure (inside VS dev environment for host tools)
:: ============================================================
echo [4/5] Configuring build...

if "%1"=="clean" (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

call "%VCVARSALL%" amd64 >nul 2>&1
set "PATH=%ARM_BIN%;%PATH%"

"%CMAKE_EXE%" -S "%SCRIPT_DIR%" -B "%BUILD_DIR%" ^
    -G Ninja ^
    -DCMAKE_MAKE_PROGRAM="%NINJA_EXE%" ^
    -DPICO_SDK_PATH="%SDK_ROOT%" ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 ( echo ERROR: CMake configure failed. & exit /b 1 )

:: ============================================================
:: 5. Build
:: ============================================================
echo [5/5] Building firmware...
"%CMAKE_EXE%" --build "%BUILD_DIR%" --parallel

if errorlevel 1 (
    echo.
    echo BUILD FAILED. See errors above.
    exit /b 1
)

echo.
echo ============================================================
echo BUILD SUCCESSFUL
echo Firmware: %BUILD_DIR%\USB_DEVICE_AND_KEYBOARD.uf2
echo Flash: hold BOOTSEL, plug in Pico, copy .uf2 to RPI-RP2 drive
echo ============================================================
