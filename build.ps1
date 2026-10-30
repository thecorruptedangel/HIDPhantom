#Requires -Version 5.1
param([switch]$Clean, [switch]$SkipStorageRegen, [switch]$Flash)

$ErrorActionPreference = 'Stop'

$PICO_SDK_VERSION   = '2.3.1'
$LIBUSB_VERSION     = 'v1.0.30'
$LIBUSB_URL         = "https://github.com/libusb/libusb/releases/download/$LIBUSB_VERSION/libusb-1.0.30.7z"

$SDK_ROOT           = "$env:USERPROFILE\.pico-sdk\sdk\$PICO_SDK_VERSION"
$LIBUSB_ROOT        = "$env:USERPROFILE\.pico-sdk\libusb"
$PICOTOOL_INST      = "$env:USERPROFILE\.pico-sdk\picotool"
$PICOTOOL_BLD       = "$env:USERPROFILE\.pico-sdk\picotool-build"

$ARM_BIN            = 'C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\12.2 mpacbti-rel1\bin'
$CMAKE_EXE          = 'C:\Program Files\CMake\bin\cmake.exe'
$VCVARSALL          = 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat'
$SCRIPT_DIR         = $PSScriptRoot
$BUILD_DIR          = "$SCRIPT_DIR\build"

# ── Helper: load VS 2022 amd64 environment into current process ──────────────
function Import-VsEnv {
    $lines = & { $ErrorActionPreference = 'SilentlyContinue'
                 cmd /c "`"$VCVARSALL`" amd64 > nul & set" 2>&1 }
    foreach ($line in $lines) {
        $s = "$line"
        if ($s -match '^([A-Za-z_][A-Za-z0-9_]*)=(.*)$') {
            [System.Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process')
        }
    }
}

# ── Helper: run cmake, suppressing stderr NativeCommandErrors (PS 5.1) ───────
function Invoke-Cmake { param([string[]]$Args)
    & { $ErrorActionPreference = 'SilentlyContinue'; & $CMAKE_EXE @Args 2>&1 | Write-Host }
    if ($LASTEXITCODE -ne 0) { throw "cmake exited $LASTEXITCODE" }
}

# ============================================================
# 1. Prerequisites: ARM GCC, CMake, Ninja, VS 2022
# ============================================================
Write-Host '[1/7] Checking prerequisites...'

if (-not (Test-Path "$ARM_BIN\arm-none-eabi-gcc.exe")) {
    Write-Host '  Installing ARM GNU Toolchain...'
    winget install --id Arm.ArmGnuToolchain --silent --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -ne 0) { throw 'ARM GCC install failed.' }
} else { Write-Host '  ARM GCC: OK' }

if (-not (Test-Path $CMAKE_EXE)) {
    Write-Host '  Installing CMake...'
    winget install --id Kitware.CMake --silent --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -ne 0) { throw 'CMake install failed.' }
    $found = Get-Command cmake -ErrorAction SilentlyContinue
    if ($found) { $CMAKE_EXE = $found.Source }
} else { Write-Host '  CMake: OK' }

$ninjaCmd = Get-Command ninja -ErrorAction SilentlyContinue
if (-not $ninjaCmd) {
    Write-Host '  Installing Ninja...'
    winget install --id Ninja-build.Ninja --silent --accept-package-agreements --accept-source-agreements
    $env:PATH = [System.Environment]::GetEnvironmentVariable('PATH','Machine') + ';' +
                [System.Environment]::GetEnvironmentVariable('PATH','User')
    $ninjaCmd = Get-Command ninja -ErrorAction Stop
}
$NINJA_EXE = $ninjaCmd.Source
Write-Host "  Ninja: $NINJA_EXE"

if (-not (Test-Path $VCVARSALL)) { throw "VS 2022 not found at: $VCVARSALL" }
else { Write-Host '  VS 2022: OK' }

# ============================================================
# 2. Pico SDK + submodules
# ============================================================
Write-Host "[2/7] Checking Pico SDK $PICO_SDK_VERSION..."
if (-not (Test-Path "$SDK_ROOT\CMakeLists.txt")) {
    Write-Host "  Cloning Pico SDK $PICO_SDK_VERSION..."
    git clone --depth 1 --branch $PICO_SDK_VERSION https://github.com/raspberrypi/pico-sdk.git $SDK_ROOT
    if ($LASTEXITCODE -ne 0) { throw 'SDK clone failed.' }
} else { Write-Host '  SDK: OK' }

Push-Location $SDK_ROOT
try {
    Write-Host '  Checking SDK submodules...'
    if (-not (Test-Path 'lib\tinyusb\src')) {
        Write-Host '  Initializing TinyUSB...'
        git submodule update --init lib/tinyusb
        if ($LASTEXITCODE -ne 0) { throw 'TinyUSB init failed.' }
    }
    if (-not (Test-Path 'lib\lwip\src')) {
        Write-Host '  Initializing lwIP...'
        git submodule update --init lib/lwip
        if ($LASTEXITCODE -ne 0) { throw 'lwIP init failed.' }
    }
} finally { Pop-Location }

# ============================================================
# 3. TinyUSB KB5124008 NCM patch
# ============================================================
Write-Host '[3/7] Applying TinyUSB KB5124008 NCM patch...'
$NCM_FILE = "$SDK_ROOT\lib\tinyusb\src\class\net\ncm_device.c"
if (Test-Path $NCM_FILE) {
    $src     = Get-Content $NCM_FILE -Raw
    $patched = $src -replace '\.wNdbInDivisor\s*=\s*1,',  '.wNdbInDivisor            = 4,'
    $patched = $patched -replace '\.wNdbOutDivisor\s*=\s*1,', '.wNdbOutDivisor           = 4,'
    if ($patched -eq $src) { Write-Host '  Already patched — skipping.' }
    else {
        [System.IO.File]::WriteAllText($NCM_FILE, $patched, [System.Text.Encoding]::UTF8)
        Write-Host '  Patch applied.'
    }
} else { Write-Host "  WARNING: $NCM_FILE not found — skipping." }

# ============================================================
# 4. libusb + picotool with USB support
# Picotool built by the main CMake has no USB support
# (PICOTOOL_NO_LIBUSB is set by the SDK's FetchContent).
# We maintain a separately built copy with libusb.
# ============================================================
Write-Host '[4/7] Checking picotool (USB-enabled)...'

$PICOTOOL_EXE   = "$PICOTOOL_INST\picotool.exe"
$LIBUSB_INC     = "$LIBUSB_ROOT\extracted\include"
$LIBUSB_LIB     = "$LIBUSB_ROOT\extracted\VS2022\MS64\static\libusb-1.0.lib"
$LIBUSB_DLL     = "$LIBUSB_ROOT\extracted\VS2022\MS64\dll\libusb-1.0.dll"

$needPicotool = (-not (Test-Path $PICOTOOL_EXE)) -or
                ((& $PICOTOOL_EXE version 2>&1) -match 'without USB')

if ($needPicotool) {
    Write-Host '  Building picotool with USB support...'

    # -- libusb --
    if (-not (Test-Path $LIBUSB_LIB)) {
        Write-Host '  Downloading libusb...'
        New-Item -ItemType Directory -Force $LIBUSB_ROOT | Out-Null
        curl.exe -L -s -o "$LIBUSB_ROOT\libusb.7z" $LIBUSB_URL
        if ($LASTEXITCODE -ne 0) { throw 'libusb download failed.' }
        7z x "$LIBUSB_ROOT\libusb.7z" -o"$LIBUSB_ROOT\extracted" -y | Out-Null
        if (-not (Test-Path $LIBUSB_LIB)) { throw 'libusb extract failed — lib not found.' }
        Write-Host '  libusb extracted.'
    } else { Write-Host '  libusb: OK' }

    # -- picotool source: prefer the one already fetched by the main build --
    $PICOTOOL_SRC = "$BUILD_DIR\_deps\picotool-src"
    if (-not (Test-Path "$PICOTOOL_SRC\CMakeLists.txt")) {
        Write-Host '  Cloning picotool source...'
        $PICOTOOL_SRC = "$PICOTOOL_INST-src"
        git clone --depth 1 --branch $PICO_SDK_VERSION `
            https://github.com/raspberrypi/picotool.git $PICOTOOL_SRC
        if ($LASTEXITCODE -ne 0) { throw 'picotool clone failed.' }
    }

    # -- configure --
    Import-VsEnv
    New-Item -ItemType Directory -Force $PICOTOOL_BLD | Out-Null
    Invoke-Cmake @(
        '-S', $PICOTOOL_SRC,
        '-B', $PICOTOOL_BLD,
        '-G', 'NMake Makefiles',
        "-DPICO_SDK_PATH=$SDK_ROOT",
        "-DCMAKE_INSTALL_PREFIX=$PICOTOOL_INST",
        "-DLIBUSB_INCLUDE_DIR=$LIBUSB_INC",
        "-DLIBUSB_LIBRARIES=$LIBUSB_LIB",
        '-DUSE_PRECOMPILED=ON',
        '-DPICOTOOL_FLAT_INSTALL=1',
        '-DCMAKE_BUILD_TYPE=Release'
    )

    # -- build --
    Invoke-Cmake @('--build', $PICOTOOL_BLD)

    # -- install --
    New-Item -ItemType Directory -Force $PICOTOOL_INST | Out-Null
    Copy-Item "$PICOTOOL_BLD\picotool.exe" $PICOTOOL_EXE -Force
    if (Test-Path $LIBUSB_DLL) { Copy-Item $LIBUSB_DLL "$PICOTOOL_INST\libusb-1.0.dll" -Force }

    $ver = & $PICOTOOL_EXE version 2>&1
    if ($ver -match 'without USB') { throw 'picotool built without USB — check libusb paths.' }
    Write-Host "  picotool: $ver"
} else {
    Write-Host "  picotool: OK ($((& $PICOTOOL_EXE version 2>&1) -join ''))"
}

# ============================================================
# 5. Regenerate FAT12 storage image from storage_files/
# Always run unconditionally — file(GLOB) in CMake only fires
# at configure time and misses files added/removed between builds.
# ============================================================
Write-Host '[5/7] Regenerating storage image from storage_files/...'

$STORAGE_DIR    = "$SCRIPT_DIR\storage_files"
$STORAGE_DATA_C = "$BUILD_DIR\storage_data.c"

# -Clean: wipe build dir before generating so storage_data.c is the only pre-seed
if ($Clean -and (Test-Path $BUILD_DIR)) {
    Write-Host '  Cleaning build directory...'
    Remove-Item $BUILD_DIR -Recurse -Force
}
if (-not (Test-Path $BUILD_DIR)) { New-Item -ItemType Directory $BUILD_DIR | Out-Null }

if (-not $SkipStorageRegen) {
    $storageFiles = @(Get-ChildItem -Path $STORAGE_DIR -File -ErrorAction SilentlyContinue)
    Write-Host "  Files: $(($storageFiles | Select-Object -ExpandProperty Name) -join ', ')"
    python "$SCRIPT_DIR\scripts\gen_fat12_image.py" $STORAGE_DIR $STORAGE_DATA_C
    if ($LASTEXITCODE -ne 0) { throw 'Storage image generation failed.' }
    Write-Host '  Storage image regenerated.'
} else {
    Write-Host '  Skipped (-SkipStorageRegen flag set).'
}

# ============================================================
# 6. CMake configure + build
# ============================================================
Write-Host '[6/7] Configuring build...'

if (-not (Test-Path $BUILD_DIR)) { New-Item -ItemType Directory $BUILD_DIR | Out-Null }

Import-VsEnv
$env:PATH = "$ARM_BIN;$env:PATH"

Invoke-Cmake @(
    '-S', $SCRIPT_DIR,
    '-B', $BUILD_DIR,
    '-G', 'Ninja',
    "-DCMAKE_MAKE_PROGRAM=$NINJA_EXE",
    "-DPICO_SDK_PATH=$SDK_ROOT",
    '-DCMAKE_BUILD_TYPE=Release'
)

Write-Host '[7/7] Building firmware...'
Invoke-Cmake @('--build', $BUILD_DIR, '--parallel')

$uf2 = "$BUILD_DIR\HIDPhantom.uf2"
Write-Host ''
Write-Host '============================================================'
Write-Host 'BUILD SUCCESSFUL'
Write-Host "Firmware: $uf2"

# ============================================================
# 7. Flash (optional — pass -Flash)
# ============================================================
if ($Flash) {
    Write-Host ''
    Write-Host '============================================================'
    Write-Host 'FLASH MODE — Hold BOOTSEL and plug in (or press reset)...'
    Write-Host '============================================================'

    # Poll up to 30 s for the RPI-RP2 BOOTSEL drive
    $rpi      = $null
    $deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $deadline) {
        $rpi = Get-PSDrive -PSProvider FileSystem -ErrorAction SilentlyContinue |
               Where-Object { Test-Path "$($_.Root)INFO_UF2.TXT" } |
               Select-Object -First 1
        if ($rpi) { break }
        Start-Sleep -Milliseconds 500
    }
    if (-not $rpi) { throw 'Timed out waiting for RPI-RP2 drive. Hold BOOTSEL and try again.' }

    Write-Host "  RPI-RP2 at $($rpi.Root)"

    # Try picotool first (cleaner, no drive-eject race).
    # Falls back to UF2 drive copy if picotool can't connect — this happens when
    # the WinUSB driver hasn't been installed via Zadig for the RP2040 BOOTSEL device.
    # One-time Zadig fix: https://github.com/raspberrypi/picotool#zadig
    $usedPicotool = $false
    if (Test-Path $PICOTOOL_EXE) {
        Write-Host '  Trying picotool...'
        $env:PATH = "$PICOTOOL_INST;$env:PATH"
        & $PICOTOOL_EXE load -f $uf2 2>&1 | Write-Host
        if ($LASTEXITCODE -eq 0) {
            & $PICOTOOL_EXE reboot 2>&1 | Out-Null
            $usedPicotool = $true
        } else {
            Write-Host '  picotool failed (WinUSB driver missing? Run Zadig once to fix).'
            Write-Host '  Falling back to UF2 drive copy...'
        }
    }
    if (-not $usedPicotool) {
        Copy-Item $uf2 "$($rpi.Root)HIDPhantom.uf2"
    }

    Write-Host ''
    Write-Host '============================================================'
    Write-Host 'FLASH COMPLETE — device rebooting'
    Write-Host '============================================================'
} else {
    Write-Host 'Run with -Flash to auto-flash, or hold BOOTSEL and copy .uf2 manually.'
    Write-Host '============================================================'
}
