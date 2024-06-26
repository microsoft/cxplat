<#

.SYNOPSIS
This script provides a build config helper used by multiple build scripts.

.PARAMETER Config
    The debug or release configuration to build for.

.PARAMETER Arch
    The CPU architecture to build for.

.PARAMETER Platform
    Specify which platform to build for

.PARAMETER ExtraArtifactDir
    Add an extra classifier to the artifact directory to allow publishing alternate builds of same base library

#>

param (
    [Parameter(Mandatory = $false)]
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",

    [Parameter(Mandatory = $false)]
    [ValidateSet("x86", "x64", "arm", "arm64", "arm64ec", "universal", "")]
    [string]$Arch = "",

    [Parameter(Mandatory = $false)]
    [ValidateSet("gamecore_console", "uwp", "windows", "linux", "macos", "android", "ios", "winkernel", "windows-vs", "")] # For future expansion
    [string]$Platform = "",

    [Parameter(Mandatory = $false)]
    [string]$ExtraArtifactDir = ""
)

Set-StrictMode -Version 'Latest'
$PSDefaultParameterValues['*:ErrorAction'] = 'Stop'

if ($PSVersionTable.PSVersion.Major -lt 7) {
    # For convenience of locally building winkernel (which is typically done from a Developer shell for VS),
    # exclude winkernel from the PowerShell core requirement.
    if ($Platform -ne "winkernel") {
        Write-Error "PowerShell v7.x or greater is needed for this script to work."
    }

    $IsWindows = $true
    $IsLinux = $false
    $IsMacOS = $false
}

if ($Platform -eq "android") {
    if (!$IsLinux) {
        Write-Error "Can only build android on linux"
    }
    if ($Arch -eq "") {
        $Arch = "arm64"
    }
}

if ($Platform -eq "ios") {
    if (!$IsMacOS) {
        Write-Error  "Can only build ios on macOS"
    }
    if ($Arch -eq "") {
        $Arch = "arm64"
    }
}

if ("" -eq $Arch) {
    if ($IsMacOS) {
        $RunningArch = uname -m
        if ("x86_64" -eq $RunningArch) {
            $IsTranslated = sysctl -in sysctl.proc_translated
            if ($IsTranslated) {
                $Arch = "arm64"
            } else {
                $Arch = "x64"
            }
        } elseif ("arm64" -eq $RunningArch) {
            $Arch = "arm64"
        } else {
            Write-Error "Unknown architecture"
        }
    } elseif ($IsLinux) {
        $Arch = "$([System.Runtime.InteropServices.RuntimeInformation]::ProcessArchitecture)".ToLower()
    } else {
        $Arch = "x64"
    }
}

if ("" -eq $Platform) {
    if ($IsWindows) {
        $Platform = "windows"
    } elseif ($IsLinux) {
        $Platform = "linux"
    } elseif ($IsMacOS) {
        $Platform = "macos"
    } else {
        Write-Error "Unsupported platform type!"
    }
}

$RootDir = Split-Path $PSScriptRoot -Parent
$BaseArtifactsDir = Join-Path $RootDir "artifacts"
$ArtifactsDir = Join-Path $BaseArtifactsDir "bin"
$ArtifactsDir = Join-Path $ArtifactsDir $Platform
if ([string]::IsNullOrWhitespace($ExtraArtifactDir)) {
    $ArtifactsDir = Join-Path $ArtifactsDir "$($Arch)_$($Config)"
} else {
    $ArtifactsDir = Join-Path $ArtifactsDir "$($Arch)_$($Config)_$($ExtraArtifactDir)"
}

return @{
    Platform = $Platform
    Arch = $Arch
    ArtifactsDir = $ArtifactsDir
}
