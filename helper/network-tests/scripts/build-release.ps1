# Build Release|x64 targets for Phase 0 acceptance.
# Run from repo root in Visual Studio Developer PowerShell.

$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path
$Sln = Join-Path $RepoRoot "source\source.sln"
$LogDir = Join-Path $RepoRoot "helper\network-tests\data"
$LogFile = Join-Path $LogDir "build-release.log"

New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

Write-Host "Building $Sln (Release|x64) ..."
$msbuild = Get-Command msbuild -ErrorAction SilentlyContinue
if (-not $msbuild) {
    Write-Error "msbuild not found. Open 'Developer PowerShell for VS' and retry."
}

& msbuild $Sln /p:Configuration=Release /p:Platform=x64 /m /v:minimal 2>&1 | Tee-Object -FilePath $LogFile
$code = $LASTEXITCODE
if ($code -eq 0) {
    Write-Host "BUILD OK — log: $LogFile"
} else {
    Write-Host "BUILD FAILED (exit $code) — see $LogFile"
}
exit $code
