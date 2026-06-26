# Full PKO shader compile pipeline (PowerShell replacement for compile_shaders.lua).
# - Copies static/*.vsh into shaders/
# - Compiles all HLSL mappings with fxc.exe
# - Strips comments + XOR encrypts
# - Deploys to client/shader/
#
# Usage:  powershell -ExecutionPolicy Bypass -File compile_shaders.ps1

$ErrorActionPreference = 'Stop'

$Root      = $PSScriptRoot
$HlslDir   = Join-Path $Root 'hlsl'
$StaticDir = Join-Path $Root 'static'
$OutDir    = Join-Path $Root 'shaders'
$ClientDir = (Resolve-Path (Join-Path $Root '..\..\client\shader')).Path
$Key       = 'X7#m9$KpL2@v5*ZnQ8!w4&YhF3%r6^Dq'
$Target    = '/Tvs_2_a'
$Profile   = '/Dvs_2_a'

function Find-FxcPath {
    $candidates = @(
        'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\fxc.exe',
        'C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x86\fxc.exe',
        'C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x86\fxc.exe',
        'C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Utilities\bin\x86\fxc.exe'
    )
    foreach ($p in $candidates) {
        if (Test-Path -LiteralPath $p) { return $p }
    }
    $kits = 'C:\Program Files (x86)\Windows Kits\10\bin'
    if (Test-Path $kits) {
        $found = Get-ChildItem -Path $kits -Recurse -Filter 'fxc.exe' -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\x86\\fxc\.exe$' } |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($found) { return $found.FullName }
    }
    return $null
}

function Strip-Comments {
    param([string]$Path)
    $text = Get-Content -Raw -LiteralPath $Path
    $text = [regex]::Replace($text, '(?s)/\*.*?\*/', '')
    $text = [regex]::Replace($text, '//[^\r\n]*', '')
    $lines = $text -split "`r?`n" | ForEach-Object { $_.TrimEnd() } | Where-Object { $_ -ne '' }
    [System.IO.File]::WriteAllText($Path, ($lines -join "`r`n") + "`r`n")
}

function Encrypt-Xor {
    param([string]$Path, [string]$KeyStr)
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $keyB  = [System.Text.Encoding]::ASCII.GetBytes($KeyStr)
    $kLen  = $keyB.Length
    for ($i = 0; $i -lt $bytes.Length; $i++) {
        $bytes[$i] = $bytes[$i] -bxor $keyB[$i % $kLen]
    }
    [System.IO.File]::WriteAllBytes($Path, $bytes)
}

function Invoke-FxcCompile {
    param(
        [string]$Fxc,
        [string]$HlslPath,
        [string]$VshPath,
        [string]$Defs
    )
    $argList = [System.Collections.Generic.List[string]]@($Target, $Profile)
    if ($Defs -and $Defs.Trim() -ne '') {
        foreach ($d in ($Defs -split '\s+')) {
            if ($d.Trim()) { $argList.Add($d.Trim()) }
        }
    }
    $argList.Add("/Fc$VshPath")
    $argList.Add($HlslPath)

    $psi = [System.Diagnostics.ProcessStartInfo]::new($Fxc)
    $psi.UseShellExecute        = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError  = $true
    $psi.Arguments              = ($argList | ForEach-Object { '"' + $_ + '"' }) -join ' '
    $proc = [System.Diagnostics.Process]::Start($psi)
    $null = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()
    return @{ ExitCode = $proc.ExitCode; Stderr = $stderr }
}

# Flat list of mappings (avoids PowerShell nested-array flattening bugs).
$Mappings = @(
    @{ Hlsl='pu4nt0_ld'; Out='skinmesh8_1';      Defs='/DNUM_SKIN_WEIGHTS=1' },
    @{ Hlsl='pu4nt0_ld'; Out='skinmesh8_1_tt1';  Defs='/DNUM_SKIN_WEIGHTS=1 /DUSE_UVMAT0' },
    @{ Hlsl='pu4nt0_ld'; Out='skinmesh8_1_tt2';  Defs='/DNUM_SKIN_WEIGHTS=1 /DUSE_UVMAT1' },
    @{ Hlsl='pu4nt0_ld'; Out='skinmesh8_1_tt3';  Defs='/DNUM_SKIN_WEIGHTS=1 /DUSE_UVMAT2' },

    @{ Hlsl='pb1u4nt0_ld'; Out='skinmesh8_2';      Defs='/DNUM_SKIN_WEIGHTS=2' },
    @{ Hlsl='pb1u4nt0_ld'; Out='skinmesh8_2_tt1';  Defs='/DNUM_SKIN_WEIGHTS=2 /DUSE_UVMAT0' },
    @{ Hlsl='pb1u4nt0_ld'; Out='skinmesh8_2_tt2';  Defs='/DNUM_SKIN_WEIGHTS=2 /DUSE_UVMAT1' },
    @{ Hlsl='pb1u4nt0_ld'; Out='skinmesh8_2_tt3';  Defs='/DNUM_SKIN_WEIGHTS=2 /DUSE_UVMAT2' },

    @{ Hlsl='pb2u4nt0_ld'; Out='skinmesh8_3';      Defs='/DNUM_SKIN_WEIGHTS=3' },
    @{ Hlsl='pb2u4nt0_ld'; Out='skinmesh8_3_tt1';  Defs='/DNUM_SKIN_WEIGHTS=3 /DUSE_UVMAT0' },
    @{ Hlsl='pb2u4nt0_ld'; Out='skinmesh8_3_tt2';  Defs='/DNUM_SKIN_WEIGHTS=3 /DUSE_UVMAT1' },
    @{ Hlsl='pb2u4nt0_ld'; Out='skinmesh8_3_tt3';  Defs='/DNUM_SKIN_WEIGHTS=3 /DUSE_UVMAT2' },

    @{ Hlsl='pb3u4nt0_ld'; Out='skinmesh8_4';      Defs='/DNUM_SKIN_WEIGHTS=4' },
    @{ Hlsl='pb3u4nt0_ld'; Out='skinmesh8_4_tt1';  Defs='/DNUM_SKIN_WEIGHTS=4 /DUSE_UVMAT0' },
    @{ Hlsl='pb3u4nt0_ld'; Out='skinmesh8_4_tt2';  Defs='/DNUM_SKIN_WEIGHTS=4 /DUSE_UVMAT1' },
    @{ Hlsl='pb3u4nt0_ld'; Out='skinmesh8_4_tt3';  Defs='/DNUM_SKIN_WEIGHTS=4 /DUSE_UVMAT2' },

    @{ Hlsl='pu4nt0_ld_outline';  Out='skinmesh8_1_outline'; Defs='' },
    @{ Hlsl='pb1u4nt0_ld_outline'; Out='skinmesh8_2_outline'; Defs='' },
    @{ Hlsl='pb2u4nt0_ld_outline'; Out='skinmesh8_3_outline'; Defs='' },
    @{ Hlsl='pb3u4nt0_ld_outline'; Out='skinmesh8_4_outline'; Defs='' },
    @{ Hlsl='vs_static_outline';   Out='vs_static_outline';   Defs='' },

    @{ Hlsl='vs_pndt0'; Out='vs_pndt0';           Defs='/DNO_LIGHTING' },
    @{ Hlsl='vs_pndt0'; Out='vs_pnt0';            Defs='/DNO_LIGHTING /DNO_DIFFUSE' },
    @{ Hlsl='vs_pndt0'; Out='vs_pndt0_t0uvmat';   Defs='/DNO_LIGHTING /DUSE_TEX_TRANSFORM' },
    @{ Hlsl='vs_pndt0'; Out='vs_pnt0_t0uvmat';    Defs='/DNO_LIGHTING /DUSE_TEX_TRANSFORM /DNO_DIFFUSE' },

    @{ Hlsl='vs_pndt0_ld'; Out='vs_pndt0_ld';           Defs='' },
    @{ Hlsl='vs_pndt0_ld'; Out='vs_pnt0_ld';            Defs='/DNO_DIFFUSE' },
    @{ Hlsl='vs_pndt0_ld'; Out='vs_pndt0_ld_t0uvmat';   Defs='/DUSE_TEX_TRANSFORM' },
    @{ Hlsl='vs_pndt0_ld'; Out='vs_pnt0_ld_t0uvmat';    Defs='/DUSE_TEX_TRANSFORM /DNO_DIFFUSE' },

    @{ Hlsl='vs_pndt0_ld_t0uvmat'; Out='vs_pndt0_ld_t0uvmat_alt'; Defs='' },
    @{ Hlsl='vs_pndt0_t0uvmat';    Out='vs_pndt0_t0uvmat_alt';    Defs='' },
    @{ Hlsl='vs_pnt0_ld';          Out='vs_pnt0_ld_alt';          Defs='' },
    @{ Hlsl='vs_pnt0_ld_t0uvmat';  Out='vs_pnt0_ld_t0uvmat_alt';  Defs='' },
    @{ Hlsl='vs_pnt0_t0uvmat';     Out='vs_pnt0_t0uvmat_alt';     Defs='' }
)

$Fxc = Find-FxcPath
if (-not $Fxc) {
    throw "fxc.exe not found. Install the Windows 10 SDK (Desktop C++ workload) or DirectX SDK June 2010."
}
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir | Out-Null }

Write-Host '================================================================================'
Write-Host '  PKO Shader Compiler (PowerShell — no LuaJIT)'
Write-Host '================================================================================'
Write-Host "  fxc:    $Fxc"
Write-Host "  output: $OutDir"
Write-Host "  client: $ClientDir"
Write-Host ''

# 1. Copy static VSH files
Write-Host '[COPY] static/*.vsh -> shaders/'
$staticCount = 0
if (Test-Path $StaticDir) {
    Get-ChildItem -Path $StaticDir -Filter '*.vsh' -File | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $OutDir $_.Name) -Force
        Write-Host "  -> $($_.Name)"
        $staticCount++
    }
}
Write-Host "  Copied $staticCount static file(s)."
Write-Host ''

# 2. Compile HLSL
$ok = 0
$fail = 0
$lastHlsl = ''
foreach ($m in $Mappings) {
    $hlslName = $m.Hlsl
    $hlslPath = Join-Path $HlslDir ($hlslName + '.hlsl')
    if (-not (Test-Path -LiteralPath $hlslPath)) {
        if ($lastHlsl -ne $hlslName) {
            Write-Host "[SKIP] $hlslName.hlsl (not found)" -ForegroundColor Yellow
            $lastHlsl = $hlslName
        }
        continue
    }
    if ($lastHlsl -ne $hlslName) {
        Write-Host "[COMPILE] $hlslName.hlsl"
        $lastHlsl = $hlslName
    }
    $outName = $m.Out
    $defs    = $m.Defs
    $vshPath = Join-Path $OutDir ($outName + '.vsh')
    $result  = Invoke-FxcCompile -Fxc $Fxc -HlslPath $hlslPath -VshPath $vshPath -Defs $defs
    if ($result.ExitCode -ne 0) {
        Write-Host "  [FAIL] $outName.vsh" -ForegroundColor Red
        ($result.Stderr -split "`n" | Select-Object -First 4) | ForEach-Object { Write-Host "         $_" }
        $fail++
        continue
    }
    Write-Host "  [OK]   $outName.vsh" -ForegroundColor Green
    $ok++
}
Write-Host ''

# 3. Obfuscate + encrypt everything in shaders/
Write-Host '[OBFUSCATE] shaders/*.vsh'
$obf = 0
Get-ChildItem -Path $OutDir -Filter '*.vsh' -File | ForEach-Object {
    Strip-Comments -Path $_.FullName
    Encrypt-Xor -Path $_.FullName -KeyStr $Key
    $obf++
}
Write-Host "  Processed $obf file(s)."
Write-Host ''

# 4. Deploy to client
Write-Host '[DEPLOY] -> client\shader\'
Get-ChildItem -Path $OutDir -Filter '*.vsh' -File | ForEach-Object {
    Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $ClientDir $_.Name) -Force
}
Write-Host "  Deployed $obf file(s)."
Write-Host ''
Write-Host "Done. compiled=$ok failed=$fail deployed=$obf" -ForegroundColor Cyan

if ($fail -gt 0) { exit 1 }
exit 0
