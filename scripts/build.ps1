<#

.SYNOPSIS
This script provides helpers for building cxplat.

.PARAMETER Config
    The debug or release configuration to build for.

.PARAMETER Arch
    The CPU architecture to build for.

.PARAMETER Platform
    Specify which platform to build for.

.PARAMETER Clean
    Deletes all previous build and configuration.

.PARAMETER Parallel
    Enables CMake to build in parallel, where possible.

.PARAMETER Generator
    Specifies a specific cmake generator (Only supported on unix)

.PARAMETER SkipPdbAltPath
    Skip setting PDBALTPATH into built binaries on Windows. Without this flag, the PDB must be in the same directory as the DLL or EXE.

.PARAMETER Clang
    Build with Clang if available

.PARAMETER ConfigureOnly
    Run configuration only.

.PARAMETER CI
    Build is occuring from CI

.PARAMETER OfficialRelease
    Build is for an official (tag) release.

.PARAMETER ExtraArtifactDir
    Add an extra classifier to the artifact directory to allow publishing alternate builds of same base library

.PARAMETER LibraryName
    Renames the library to whatever is passed in

.PARAMETER SysRoot
    Directory with cross-compilation tools

.PARAMETER OneBranch
    Build is occuring from Onebranch pipeline.

.EXAMPLE
    build.ps1

.EXAMPLE
    build.ps1 -Config Release

#>

param (
    [Parameter(Mandatory = $false)]
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",

    [Parameter(Mandatory = $false)]
    [ValidateSet("x86", "x64", "arm", "arm64", "arm64ec")]
    [string]$Arch = "",

    [Parameter(Mandatory = $false)]
    [ValidateSet("gamecore_console", "uwp", "windows", "linux", "macos", "android", "ios", "winkernel")] # For future expansion
    [string]$Platform = "",

    [Parameter(Mandatory = $false)]
    [switch]$Clean = $false,

    [Parameter(Mandatory = $false)]
    [int32]$Parallel = -2,

    [Parameter(Mandatory = $false)]
    [string]$Generator = "",

    [Parameter(Mandatory = $false)]
    [switch]$SkipPdbAltPath = $false,

    [Parameter(Mandatory = $false)]
    [switch]$Clang = $false,

    [Parameter(Mandatory = $false)]
    [switch]$ConfigureOnly = $false,

    [Parameter(Mandatory = $false)]
    [switch]$CI = $false,

    [Parameter(Mandatory = $false)]
    [switch]$OfficialRelease = $false,

    [Parameter(Mandatory = $false)]
    [string]$ExtraArtifactDir = "",

    [Parameter(Mandatory = $false)]
    [string]$LibraryName = "cxplat",

    [Parameter(Mandatory = $false)]
    [string]$SysRoot = "/",

    [Parameter(Mandatory = $false)]
    [switch]$OneBranch = $false,

    [Parameter(Mandatory = $false)]
    [string]$ToolchainFile = ""
)

Set-StrictMode -Version 'Latest'
$PSDefaultParameterValues['*:ErrorAction'] = 'Stop'

if ($PSVersionTable.PSVersion.Major -lt 7) {
    # For convenience of locally building winkernel (which is typically done from a Developer shell for VS),
    # exclude winkernel from the PowerShell core requirement.
    if ($Platform -ne "winkernel") {
        Write-Error "[$(Get-Date)] PowerShell v7.x or greater is needed for this script to work."
    }

    $IsWindows = $true
    $IsLinux = $false
    $IsMacOS = $false
}

if ($Parallel -lt -1) {
    if ($IsWindows) {
        $Parallel = -1
    } else {
        $Parallel = 0
    }
}

$BuildConfig = & (Join-Path $PSScriptRoot get-buildconfig.ps1) -Platform $Platform -Arch $Arch -ExtraArtifactDir $ExtraArtifactDir -Config $Config

$Platform = $BuildConfig.Platform
$Arch = $BuildConfig.Arch
$ArtifactsDir = $BuildConfig.ArtifactsDir

if ($Generator -eq "") {
    if (!$IsWindows) {
        $Generator = "Unix Makefiles"
    } else {
        $Generator = "Visual Studio 17 2022"
    }
}

if (!$IsWindows -And $Platform -eq "uwp") {
    Write-Error "[$(Get-Date)] Cannot build uwp on non windows platforms"
}

if (!$IsWindows -And ($Platform -eq "gamecore_console")) {
    Write-Error "[$(Get-Date)] Cannot build gamecore on non windows platforms"
}

if ($Arch -ne "x64" -And ($Platform -eq "gamecore_console")) {
    Write-Error "[$(Get-Date)] Cannot build gamecore for non-x64 platforms"
}

if ($Arch -eq "arm64ec") {
    if (!$IsWindows) {
        Write-Error "[$(Get-Date)] Arm64EC is only supported on Windows"
    }
}

if ($OfficialRelease) {
    # We only actually try to do official release if there is a matching git tag.
    # Clear the flag and then only set it if we find a tag.
    $OfficialRelease = $false
    try {
        $env:GIT_REDIRECT_STDERR = '2>&1'
        # Thanks to https://stackoverflow.com/questions/3404936/show-which-git-tag-you-are-on
        # for this magic git command!
        $Output = git describe --exact-match --tags $(git log -n1 --pretty='%h')
        if (!$Output.Contains("fatal: no tag exactly matches")) {
            Log "Configuring OfficialRelease for tag build"
            $OfficialRelease = $true
        }
    } catch { }
    $global:LASTEXITCODE = 0
}

# Root directory of the project.
$RootDir = Split-Path $PSScriptRoot -Parent

# Important directory paths.
$BaseArtifactsDir = Join-Path $RootDir "artifacts"
$BaseBuildDir = Join-Path $RootDir "build"

$BuildDir = Join-Path $BaseBuildDir $Platform
$BuildDir = Join-Path $BuildDir "$($Arch)"

if ($Clean) {
    # Delete old build/config directories.
    if (Test-Path $ArtifactsDir) { Remove-Item $ArtifactsDir -Recurse -Force | Out-Null }
    if (Test-Path $BuildDir) { Remove-Item $BuildDir -Recurse -Force | Out-Null }
}

# Initialize directories needed for building.
if (!(Test-Path $BaseArtifactsDir)) {
    New-Item -Path $BaseArtifactsDir -ItemType Directory -Force | Out-Null
}
if (!(Test-Path $BuildDir)) { New-Item -Path $BuildDir -ItemType Directory -Force | Out-Null }

if ($Clang) {
    if ($IsWindows) {
        Write-Error "[$(Get-Date)] Clang is not supported on windows currently"
    }
    $env:CC = 'clang'
    $env:CXX = 'clang++'
}

function Log($msg) {
    Write-Host "[$(Get-Date)] $msg"
}

# Executes cmake with the given arguments.
function CMake-Execute([String]$Arguments) {
    Log "cmake $($Arguments)"
    $process = Start-Process cmake $Arguments -PassThru -NoNewWindow -WorkingDirectory $BuildDir
    $handle = $process.Handle # Magic work around. Don't remove this line.
    $process.WaitForExit();

    if ($process.ExitCode -ne 0) {
        Write-Error "[$(Get-Date)] CMake exited with status code $($process.ExitCode)"
    }
}

# Uses cmake to generate the build configuration files.
function CMake-Generate {
    $Arguments = ""

    if ($Generator.Contains(" ")) {
        $Generator = """$Generator"""
    }

    if ($IsWindows) {
        if ($Generator.Contains("Visual Studio") -or [string]::IsNullOrWhiteSpace($Generator)) {
            if ($Generator.Contains("Visual Studio")) {
                $Arguments += " -G $Generator"
            }
            $Arguments += " -A "
            switch ($Arch) {
                "x86"   { $Arguments += "Win32" }
                "x64"   { $Arguments += "x64" }
                "arm"   { $Arguments += "arm" }
                "arm64" { $Arguments += "arm64" }
                "arm64ec" { $Arguments += "arm64ec" }
            }
        } else {
            Log "Non VS based generators must be run from a Visual Studio Developer Powershell Prompt matching the passed in architecture"
            $Arguments += " -G $Generator"
        }
    } else {
        $Arguments += "-G $Generator"
    }
    if ($Platform -eq "ios") {
        $IosTCFile = Join-Path $RootDir cmake toolchains ios.cmake
        $Arguments +=  " -DCMAKE_TOOLCHAIN_FILE=""$IosTCFile"" -DDEPLOYMENT_TARGET=""13.0"" -DENABLE_ARC=0 -DCMAKE_OSX_DEPLOYMENT_TARGET=""13.0"""
        switch ($Arch) {
            "x64"   { $Arguments += " -DPLATFORM=SIMULATOR64"}
            "arm64" { $Arguments += " -DPLATFORM=OS64"}
        }
    }
    if ($Platform -eq "macos") {
        switch ($Arch) {
            "x64"   { $Arguments += " -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_OSX_DEPLOYMENT_TARGET=""12"""}
            "arm64" { $Arguments += " -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=""11.0"""}
        }
    }
    if ($Platform -eq "linux") {
        $Arguments += " $Generator"
        $HostArch = "$([System.Runtime.InteropServices.RuntimeInformation]::ProcessArchitecture)".ToLower()
        if ($HostArch -ne $Arch) {
            if ($OneBranch) {
                $Arguments += " -DONEBRANCH=1"
                if ($ToolchainFile -eq "") {
                    switch ($Arch) {
                        "arm"   { $ToolchainFile = "cmake/toolchains/arm-linux.cmake" }
                        "arm64" { $ToolchainFile = "cmake/toolchains/aarch64-linux.cmake" }
                    }
                }
            }
            $Arguments += " -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER -DCMAKE_CROSSCOMPILING=1 -DCMAKE_SYSROOT=$SysRoot"
            switch ($Arch) {
                "arm64" { $Arguments += " -DCMAKE_CXX_COMPILER_TARGET=aarch64-linux-gnu -DCMAKE_C_COMPILER_TARGET=aarch64-linux-gnu -DCMAKE_TARGET_ARCHITECTURE=arm64" }
                "arm" { $Arguments += " -DCMAKE_CXX_COMPILER_TARGET=arm-linux-gnueabihf  -DCMAKE_C_COMPILER_TARGET=arm-linux-gnueabihf -DCMAKE_TARGET_ARCHITECTURE=arm" }
            }

            # to build with pkg-config
            switch ($Arch) {
                "arm"   { $env:PKG_CONFIG_PATH="$SysRoot/usr/lib/arm-linux-gnueabihf/pkgconfig" }
                "arm64" { $env:PKG_CONFIG_PATH="$SysRoot/usr/lib/aarch64-linux-gnu/pkgconfig" }
            }
       }
    }
    if ($ToolchainFile -ne "") {
        $Arguments += " -DCMAKE_TOOLCHAIN_FILE=""$ToolchainFile"""
    }

    $Arguments += " -DCXPLAT_OUTPUT_DIR=""$ArtifactsDir"""

    if (!$IsWindows) {
        $ConfigToBuild = $Config;
        if ($Config -eq "Release") {
            $ConfigToBuild = "RelWithDebInfo"
        }
        $Arguments += " -DCMAKE_BUILD_TYPE=" + $ConfigToBuild
    }
    if ($Platform -eq "uwp") {
        $Arguments += " -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0 -DCXPLAT_UWP_BUILD=on"
    }
    if ($Platform -eq "gamecore_console") {
        $Arguments += " -DCMAKE_SYSTEM_VERSION=10.0 -DCXPLAT_GAMECORE_BUILD=on"
    }
    if ($SkipPdbAltPath) {
        $Arguments += " -DCXPLAT_PDBALTPATH=OFF"
    }
    if ($CI) {
        $Arguments += " -DCXPLAT_CI=ON"
        if ($Platform -eq "android" -or $ToolchainFile -ne "") {
            $Arguments += " -DCXPLAT_SKIP_CI_CHECKS=ON"
        }
        $Arguments += " -DCXPLAT_VER_BUILD_ID=$env:BUILD_BUILDID"
        $Arguments += " -DCXPLAT_VER_SUFFIX=-official"
    }
    if ($OfficialRelease) {
        $Arguments += " -DCXPLAT_OFFICIAL_RELEASE=ON"
    }
    if ($Platform -eq "android") {
        $NDK = $env:ANDROID_NDK_LATEST_HOME -replace '26\.\d+\.\d+', '25.2.9519653' # Temporary work around. Use RegEx to replace newer version.
        $env:PATH = "$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin:$env:PATH"
        switch ($Arch) {
            "x86"   { $Arguments += " -DANDROID_ABI=x86"}
            "x64"   { $Arguments += " -DANDROID_ABI=x86_64" }
            "arm"   { $Arguments += " -DANDROID_ABI=armeabi-v7a" }
            "arm64" { $Arguments += " -DANDROID_ABI=arm64-v8a" }
        }
        $Arguments += " -DANDROID_PLATFORM=android-29"
        $env:ANDROID_NDK_HOME = $NDK
        $NdkToolchainFile = "$NDK/build/cmake/android.toolchain.cmake"
        $Arguments += " -DANDROID_NDK=""$NDK"""
        $Arguments += " -DCMAKE_TOOLCHAIN_FILE=""$NdkToolchainFile"""
    }

    $Arguments += " -DCXPLAT_LIBRARY_NAME=$LibraryName"
    $Arguments += " ../../.."

    Log "Executing: $Arguments"
    CMake-Execute $Arguments
}


# Uses cmake to generate the build configuration files.
function CMake-Build {
    $Arguments = "--build ."
    if ($Parallel -gt 0) {
        $Arguments += " --parallel $($Parallel)"
    } elseif ($Parallel -eq 0) {
        $Arguments += " --parallel"
    }
    if ($IsWindows) {
        $Arguments += " --config " + $Config
    } else {
        $Arguments += " -- VERBOSE=1"
    }

    Log "Running: $Arguments"
    CMake-Execute $Arguments
}

##############################################################
#                     Main Execution                         #
##############################################################

if ($Platform -eq "winkernel") {
    # Restore Nuget packages.
    Log "Restoring packages..."
    msbuild cxplat.kernel.sln -t:restore /p:RestorePackagesConfig=true /p:Configuration=$Config /p:Platform=$Arch

    if (!$ConfigureOnly) {
        # Build the code.
        Log "Building..."
        msbuild cxplat.kernel.sln /m /p:Configuration=$Config /p:Platform=$Arch
    }
} else {
    # Generate the build files.
    Log "Generating files..."
    CMake-Generate

    if (!$ConfigureOnly) {
        # Build the code.
        Log "Building..."
        CMake-Build
    }
}

Log "Done."
