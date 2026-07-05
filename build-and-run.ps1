<#
.SYNOPSIS
    Compiles the FastAcq solution and runs the resulting executable.

.DESCRIPTION
    Locates MSBuild via vswhere, builds FastAcq.sln for the chosen
    configuration/platform, and (unless -NoRun is set) launches FastAcq.exe.

.PARAMETER Configuration
    Build configuration: Release (default) or Debug.

.PARAMETER Platform
    Build platform. Default: x64.

.PARAMETER Clean
    Perform a clean rebuild instead of an incremental build.

.PARAMETER NoRun
    Build only; do not launch the program.

.EXAMPLE
    .\build-and-run.ps1
    .\build-and-run.ps1 -Configuration Debug
    .\build-and-run.ps1 -Clean -NoRun
#>
param(
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release',

    [string]$Platform = 'x64',

    [switch]$Clean,

    [switch]$NoRun
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$solution = Join-Path $root 'FastAcq.sln'

# --- Locate MSBuild ---------------------------------------------------------
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found. Install Visual Studio (with C++ workload) or the Build Tools."
}
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) {
    throw "MSBuild.exe not found via vswhere."
}

# --- Build ------------------------------------------------------------------
$target = if ($Clean) { '/t:Rebuild' } else { '/t:Build' }
Write-Host "Building $Configuration|$Platform ..." -ForegroundColor Cyan
& $msbuild $solution $target "/p:Configuration=$Configuration" "/p:Platform=$Platform" /m /nologo /verbosity:minimal
if ($LASTEXITCODE -ne 0) {
    throw "Build FAILED (exit code $LASTEXITCODE)."
}
Write-Host "Build succeeded." -ForegroundColor Green

# --- Run --------------------------------------------------------------------
$exe = Join-Path $root "$Platform\$Configuration\FastAcq.exe"
if (-not (Test-Path $exe)) {
    throw "Executable not found: $exe"
}
if ($NoRun) {
    Write-Host "Built: $exe (skipping launch, -NoRun set)." -ForegroundColor Yellow
    return
}
Write-Host "Launching $exe ..." -ForegroundColor Cyan
Start-Process -FilePath $exe
