<#

.SYNOPSIS
This script runs the CxPlat tests.

.PARAMETER Config
    Specifies the build configuration to test.

.PARAMETER Arch
    The CPU architecture to test.

.PARAMETER ExtraArtifactDir
    Add an extra classifier to the artifact directory to allow publishing alternate builds of same base library

.PARAMETER Kernel
    Runs the Windows kernel mode tests.

.PARAMETER Filter
    A filter to include test cases from the list to execute. Multiple filters
    are separated by :. Negative filters are prefixed with -.

.PARAMETER ListTestCases
    Lists all the test cases.

.PARAMETER IsolationMode
    Controls the isolation mode when running each test case.

.PARAMETER KeepOutputOnSuccess
    Don't discard console output or logs on success.

.PARAMETER GenerateXmlResults
    Generates an xml Test report for the run.

.PARAMETER Debugger
    Attaches the debugger to each test case run.

.PARAMETER InitialBreak
    Debugger starts broken into the process to allow setting breakpoints, etc.

.PARAMETER BreakOnFailure
    Triggers a break point on a test failure.

.PARAMETER CompressOutput
    Compresses the output files generated for failed test cases.

.PARAMETER NoProgress
    Disables the progress bar.

.Parameter EnableAppVerifier
    Enables all basic Application Verifier checks on test binaries.

.Parameter GHA
    Running this script as a part of GitHub Actions.

.Parameter ErrorsAsWarnings
    Treats all errors as warnings.

.Parameter NumIterations
    Number of times to run this particular command. Catches tricky edge cases due to random nature of networks.

.EXAMPLE
    test.ps1

.EXAMPLE
    test.ps1 -ListTestCases

.EXAMPLE
    test.ps1 -ListTestCases -Filter ParameterValidation*

.EXAMPLE
    test.ps1 -Filter ParameterValidation*

.EXAMPLE
    test.ps1 -Filter ParameterValidation* -NumIterations 10
#>

param (
    [Parameter(Mandatory = $false)]
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",

    [Parameter(Mandatory = $false)]
    [ValidateSet("x86", "x64", "arm", "arm64")]
    [string]$Arch = "",

    [Parameter(Mandatory = $false)]
    [switch]$Kernel = $false,

    [Parameter(Mandatory = $false)]
    [string]$Filter = "",

    [Parameter(Mandatory = $false)]
    [switch]$ListTestCases = $false,

    [Parameter(Mandatory = $false)]
    [ValidateSet("Batch", "Isolated")]
    [string]$IsolationMode = "Isolated",

    [Parameter(Mandatory = $false)]
    [switch]$KeepOutputOnSuccess = $false,

    [Parameter(Mandatory = $false)]
    [switch]$GenerateXmlResults = $false,

    [Parameter(Mandatory = $false)]
    [switch]$Debugger = $false,

    [Parameter(Mandatory = $false)]
    [switch]$InitialBreak = $false,

    [Parameter(Mandatory = $false)]
    [switch]$BreakOnFailure = $false,

    [Parameter(Mandatory = $false)]
    [switch]$CompressOutput = $false,

    [Parameter(Mandatory = $false)]
    [switch]$NoProgress = $false,

    [Parameter(Mandatory = $false)]
    [switch]$EnableAppVerifier = $false,

    [Parameter(Mandatory = $false)]
    [switch]$EnableSystemVerifier = $false,

    [Parameter(Mandatory = $false)]
    [string]$ExtraArtifactDir = "",

    [Parameter(Mandatory = $false)]
    [switch]$GHA = $false,

    [Parameter(Mandatory = $false)]
    [switch]$ErrorsAsWarnings = $false,

    [Parameter(Mandatory = $false)]
    [string]$OsRunner = "",

    [Parameter(Mandatory = $false)]
    [int]$NumIterations = 1
)

Set-StrictMode -Version 'Latest'
$PSDefaultParameterValues['*:ErrorAction'] = 'Stop'

function Test-Administrator
{
    $user = [Security.Principal.WindowsIdentity]::GetCurrent();
    (New-Object Security.Principal.WindowsPrincipal $user).IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)
}

if ($IsWindows -and !(Test-Administrator)) {
    Write-Warning "We recommend running this test as administrator. Crash dumps will not work"
}

# Validate the kernel switch.
if ($Kernel -and !$IsWindows) {
    Write-Error "-Kernel switch only supported on Windows"
}

$BuildConfig = & (Join-Path $PSScriptRoot get-buildconfig.ps1) -Arch $Arch -ExtraArtifactDir $ExtraArtifactDir -Config $Config

$Arch = $BuildConfig.Arch
$RootArtifactDir = $BuildConfig.ArtifactsDir

# Root directory of the project.
$RootDir = Split-Path $PSScriptRoot -Parent

# Path to the run-gtest Powershell script.
$RunTest = Join-Path $RootDir "scripts/run-gtest.ps1"

if ("" -ne $ExtraArtifactDir -and $Kernel) {
    Write-Error "Kernel not supported with extra artifact dir"
}

# Path to the cxplattest executable.
$CxPlatTest = $null
$KernelPath = $null;
if ($IsWindows) {
    $CxPlatTest = Join-Path $RootArtifactDir  "cxplattest.exe"
    $KernelPath = Join-Path $RootDir "\artifacts\bin\winkernel\$($Arch)_$($Config)"
}  elseif ($IsLinux -or $IsMacOS) {
    $CxPlatTest = Join-Path $RootArtifactDir "cxplattest"
} else {
    Write-Error "Unsupported platform type!"
}

# Make sure the build is present.
if (!(Test-Path $CxPlatTest)) {
    $BuildScriptPath = Join-Path $RootDir "scripts"
    $BuildScriptPath = Join-Path $BuildScriptPath "build.ps1"
    Write-Error "Build does not exist!`n `nRun the following to generate it:`n `n    $BuildScriptPath -Config $Config -Arch $Arch`n"
}
if ($Kernel) {
    if (!(Test-Path (Join-Path $KernelPath "cxplattest.sys"))) {
        Write-Error "Kernel binaries do not exist!"
    }
}

# Build up all the arguments to pass to the Powershell script.
$TestArguments =  "-IsolationMode $IsolationMode"

if ($Kernel) {
    $TestArguments += " -Kernel $KernelPath"
}
if ("" -ne $Filter) {
    $TestArguments += " -Filter $Filter"
}
if ($ListTestCases) {
    $TestArguments += " -ListTestCases"
}
if ($KeepOutputOnSuccess) {
    $TestArguments += " -KeepOutputOnSuccess"
}
if ($GenerateXmlResults) {
    $TestArguments += " -GenerateXmlResults"
}
if ($Debugger) {
    $TestArguments += " -Debugger"
}
if ($InitialBreak) {
    $TestArguments += " -InitialBreak"
}
if ($BreakOnFailure) {
    $TestArguments += " -BreakOnFailure"
}
if ($CompressOutput) {
    $TestArguments += " -CompressOutput"
}
if ($NoProgress) {
    $TestArguments += " -NoProgress"
}
if ($EnableAppVerifier) {
    $TestArguments += " -EnableAppVerifier"
}
if ($EnableSystemVerifier) {
    $TestArguments += " -EnableSystemVerifier"
}
if ($GHA) {
    $TestArguments += " -GHA"
}
if ($ErrorsAsWarnings) {
    $TestArguments += " -ErrorsAsWarnings"
}
if ("" -ne $OsRunner) {
    $TestArguments += " -OsRunner $OsRunner"
}

if (![string]::IsNullOrWhiteSpace($ExtraArtifactDir)) {
    $TestArguments += " -ExtraArtifactDir $ExtraArtifactDir"
}

for ($iteration = 1; $iteration -le $NumIterations; $iteration++) {
    if ($NumIterations -gt 1) {
        Write-Host "------- Iteration $iteration -------"
    }
    # Run the script.
    Invoke-Expression ($RunTest + " -Path $CxPlatTest " + $TestArguments)
}
