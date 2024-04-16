<#

.SYNOPSIS
This script installs all necessary dependencies on the machine, depending
on the provided configuration.

.EXAMPLE
    prepare-machine.ps1

.EXAMPLE
    prepare-machine.ps1 -ForBuild

#>

param (
    [Parameter(Mandatory = $false)]
    [switch]$ForBuild,

    [Parameter(Mandatory = $false)]
    [switch]$InstallArm64Toolchain
)

# Admin is required because a lot of things are installed to the local machine
# in the script.
#Requires -RunAsAdministrator

Set-StrictMode -Version 'Latest'
$PSDefaultParameterValues['*:ErrorAction'] = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$PrepConfig = & (Join-Path $PSScriptRoot get-buildconfig.ps1)

if ($PSVersionTable.PSVersion.Major -lt 7) {
    # This script requires PowerShell core (mostly for xplat stuff).
    Write-Error ("`nPowerShell v7.x or greater is needed for this script to work. " +
                 "Please visit https://github.com/microsoft/msquic/blob/main/docs/BUILD.md#powershell-usage")
}

if (!$ForBuild) {
    # When no args are passed, assume we want to build and test everything
    # locally (i.e. a dev environment).
    Write-Host "No arguments passed, defaulting -ForBuild"
    $ForBuild = $true
}

# Root directory of the project.
$RootDir = Split-Path $PSScriptRoot -Parent
$ArtifactsPath = Join-Path $RootDir "artifacts"
if (!(Test-Path $ArtifactsPath)) { mkdir $ArtifactsPath | Out-Null }

if ($IsLinux) {
    if ($ForBuild) {
        sudo apt-add-repository ppa:lttng/stable-2.13 -y
        sudo apt-get update -y
        sudo apt-get install -y cmake
        sudo apt-get install -y build-essential
        sudo apt-get install -y liblttng-ust-dev
        sudo apt-get install -y libssl-dev
        sudo apt-get install -y libnuma-dev
        if ($InstallArm64Toolchain) {
            sudo apt-get install -y gcc-aarch64-linux-gnu
            sudo apt-get install -y binutils-aarch64-linux-gnu
            sudo apt-get install -y g++-aarch64-linux-gnu
        }
        # only used for the codecheck CI run:
        sudo apt-get install -y cppcheck clang-tidy
        # used for packaging
        sudo apt-get install -y ruby ruby-dev rpm
        sudo gem install public_suffix -v 4.0.7
        sudo gem install fpm
    }
}
