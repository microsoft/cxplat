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
    [switch]$Force,

    [Parameter(Mandatory = $false)]
    [switch]$ForBuild,

    [Parameter(Mandatory = $false)]
    [switch]$ForTest,

    [Parameter(Mandatory = $false)]
    [switch]$ForKernel,

    [Parameter(Mandatory = $false)]
    [switch]$InstallSigningCertificates,

    [Parameter(Mandatory = $false)]
    [switch]$InstallArm64Toolchain,

    [Parameter(Mandatory = $false)]
    [switch]$DisableTest,

    [Parameter(Mandatory = $false)]
    [switch]$InstallCoreNetCiDeps
)

# Admin is required because a lot of things are installed to the local machine
# in the script.
#Requires -RunAsAdministrator

Set-StrictMode -Version 'Latest'
$PSDefaultParameterValues['*:ErrorAction'] = 'Stop'
$ProgressPreference = 'SilentlyContinue'

if ($PSVersionTable.PSVersion.Major -lt 7) {
    # This script requires PowerShell core.
    Write-Error "PowerShell v7.x or greater is needed for this script to work."
}

$PrepConfig = & (Join-Path $PSScriptRoot get-buildconfig.ps1)

if (!$ForBuild -and !$ForTest) {
    # When no args are passed, assume we want to build and test everything
    # locally (i.e. a dev environment).
    Write-Host "No arguments passed, defaulting -ForBuild and -ForTest"
    $ForBuild = $true
    $ForTest = $true
}

if ($ForBuild) {
    # When configured for building, make sure we have all possible dependencies
    # enabled for any possible build.
    $InstallCoreNetCiDeps = $true; # For kernel signing certs
}

if ($ForTest) {
    # Since installing signing certs also checks whether test signing is enabled, which most
    # likely will fail on a devbox, do it only when we need to test kernel drivers so that
    # local testing setup won't be blocked by test signing not enabled.
    if ($ForKernel) {
        $InstallSigningCertificates = $true;
    }
}

if ($InstallSigningCertificates) {
    # Signing certs need the CoreNet-CI dependencies.
    $InstallCoreNetCiDeps = $true;
}

# Root directory of the project.
$RootDir = Split-Path $PSScriptRoot -Parent
$ArtifactsPath = Join-Path $RootDir "artifacts"
if (!(Test-Path $ArtifactsPath)) { mkdir $ArtifactsPath | Out-Null }

# Directory for the corenet CI install.
$CoreNetCiPath = Join-Path $ArtifactsPath "corenet-ci-main"
$SetupPath = Join-Path $CoreNetCiPath "vm-setup"

# Downloads and caches the latest version of the corenet-ci-main repo.
function Download-CoreNet-Deps {
    if (!$IsWindows) { return } # Windows only
    # Download and extract https://github.com/microsoft/corenet-ci.
    if ($Force) { rm -Force -Recurse $CoreNetCiPath -ErrorAction Ignore }
    if (!(Test-Path $CoreNetCiPath)) {
        Write-Host "Downloading CoreNet-CI"
        $ZipPath = Join-Path $ArtifactsPath "corenet-ci.zip"
        Invoke-WebRequest -Uri "https://github.com/microsoft/corenet-ci/archive/refs/heads/main.zip" -OutFile $ZipPath
        Expand-Archive -Path $ZipPath -DestinationPath $ArtifactsPath -Force
        Remove-Item -Path $ZipPath
    }
}

# Installs the certs downloaded via Download-CoreNet-Deps and used for signing
# our test drivers.
function Install-SigningCertificates {
    if (!$IsWindows) { return } # Windows only

    # Check to see if test signing is enabled.
    $HasTestSigning = $false
    try { $HasTestSigning = ("$(bcdedit)" | Select-String -Pattern "testsigning\s+Yes").Matches.Success } catch { }
    if (!$HasTestSigning) { Write-Error "Test Signing Not Enabled!" }

    Write-Host "Installing driver signing certificates"
    try {
        CertUtil.exe -addstore Root "$SetupPath\CoreNetSignRoot.cer" 2>&1 | Out-Null
        CertUtil.exe -addstore TrustedPublisher "$SetupPath\CoreNetSignRoot.cer" 2>&1 | Out-Null
        CertUtil.exe -addstore Root "$SetupPath\testroot-sha2.cer" 2>&1 | Out-Null # For duonic
    } catch {
        Write-Host "WARNING: Exception encountered while installing signing certs. Drivers may not start!"
    }
}

if ($ForBuild) {

    if (!$DisableTest) {
        Write-Host "Initializing googletest submodule"
        git submodule init submodules/googletest
    }

    git submodule update --jobs=8
}

if ($InstallCoreNetCiDeps) { Download-CoreNet-Deps }
if ($InstallSigningCertificates) { Install-SigningCertificates }

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

    if ($ForTest) {
        sudo apt-add-repository ppa:lttng/stable-2.13 -y
        sudo apt-get update -y
        sudo apt-get install -y lttng-tools
        sudo apt-get install -y liblttng-ust-dev
        sudo apt-get install -y gdb

        # Enable core dumps for the system.
        Write-Host "Setting core dump size limit"
        sudo sh -c "echo 'root soft core unlimited' >> /etc/security/limits.conf"
        sudo sh -c "echo 'root hard core unlimited' >> /etc/security/limits.conf"
        sudo sh -c "echo '* soft core unlimited' >> /etc/security/limits.conf"
        sudo sh -c "echo '* hard core unlimited' >> /etc/security/limits.conf"
        #sudo cat /etc/security/limits.conf

        # Set the core dump pattern.
        Write-Host "Setting core dump pattern"
        sudo sh -c "echo -n '%e.%p.%t.core' > /proc/sys/kernel/core_pattern"
        #sudo cat /proc/sys/kernel/core_pattern
    }
}

if ($IsMacOS) {
    if ($ForTest) {
        Write-Host "Setting core dump pattern"
        sudo sysctl -w kern.corefile=%N.%P.core
    }
}
