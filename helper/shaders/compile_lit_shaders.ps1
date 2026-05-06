# Compile lit shaders + outline shaders (everything that depends on common.hlsli),
# strip comments, XOR-encrypt them with the PKO shader key, and write them
# directly to ..\..\client\shader\ (overwriting the existing encrypted .vsh).
#
# Usage:  powershell -ExecutionPolicy Bypass -File compile_lit_shaders.ps1
#
# Requires: fxc.exe from the Windows 10 SDK (auto-detected below).

$ErrorActionPreference = 'Stop'

$Fxc       = 'C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\fxc.exe'
$HlslDir   = Join-Path $PSScriptRoot 'hlsl'
$OutDir    = Join-Path $PSScriptRoot 'shaders'
$ClientDir = Resolve-Path (Join-Path $PSScriptRoot '..\..\client\shader')
$Key       = 'X7#m9$KpL2@v5*ZnQ8!w4&YhF3%r6^Dq'
$Target    = '/Tvs_2_a'
$Profile   = '/Dvs_2_a'

if (-not (Test-Path $Fxc))    { throw "fxc.exe not found at $Fxc" }
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory $OutDir | Out-Null }

# Only the shaders whose pixel value comes through CalcLighting() in common.hlsli.
# Static (non-lit) shaders and outline shaders are unaffected.
$Mappings = @(
    # --- Skinned, lit (cel-shaded) ---
    @{ Hlsl='pu4nt0_ld';   Out='skinmesh8_1';      Defs='/DNUM_SKIN_WEIGHTS=1' },
    @{ Hlsl='pu4nt0_ld';   Out='skinmesh8_1_tt1';  Defs='/DNUM_SKIN_WEIGHTS=1 /DUSE_UVMAT0' },
    @{ Hlsl='pu4nt0_ld';   Out='skinmesh8_1_tt2';  Defs='/DNUM_SKIN_WEIGHTS=1 /DUSE_UVMAT1' },
    @{ Hlsl='pu4nt0_ld';   Out='skinmesh8_1_tt3';  Defs='/DNUM_SKIN_WEIGHTS=1 /DUSE_UVMAT2' },

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

    # --- Static, lit (cel-shaded) ---
    @{ Hlsl='vs_pndt0_ld';          Out='vs_pndt0_ld';              Defs='' },
    @{ Hlsl='vs_pndt0_ld';          Out='vs_pnt0_ld';               Defs='/DNO_DIFFUSE' },
    @{ Hlsl='vs_pndt0_ld';          Out='vs_pndt0_ld_t0uvmat';      Defs='/DUSE_TEX_TRANSFORM' },
    @{ Hlsl='vs_pndt0_ld';          Out='vs_pnt0_ld_t0uvmat';       Defs='/DUSE_TEX_TRANSFORM /DNO_DIFFUSE' },
    @{ Hlsl='vs_pndt0_ld_t0uvmat';  Out='vs_pndt0_ld_t0uvmat_alt';  Defs='' },
    @{ Hlsl='vs_pnt0_ld';           Out='vs_pnt0_ld_alt';           Defs='' },
    @{ Hlsl='vs_pnt0_ld_t0uvmat';   Out='vs_pnt0_ld_t0uvmat_alt';   Defs='' },

    # --- Outline pass (inverted-hull, distance-scaled, tinted) ---
    @{ Hlsl='pu4nt0_ld_outline';   Out='skinmesh8_1_outline';  Defs='' },
    @{ Hlsl='pb1u4nt0_ld_outline'; Out='skinmesh8_2_outline';  Defs='' },
    @{ Hlsl='pb2u4nt0_ld_outline'; Out='skinmesh8_3_outline';  Defs='' },
    @{ Hlsl='pb3u4nt0_ld_outline'; Out='skinmesh8_4_outline';  Defs='' },
    @{ Hlsl='vs_static_outline';   Out='vs_static_outline';    Defs='' }
)

function Strip-Comments {
    param([string]$Path)
    $text = Get-Content -Raw -LiteralPath $Path
    # remove /* ... */ (DOTALL via (?s))
    $text = [regex]::Replace($text, '(?s)/\*.*?\*/', '')
    # remove // ... to end of line
    $text = [regex]::Replace($text, '//[^\r\n]*', '')
    # collapse blank/whitespace-only lines
    $lines = $text -split "`r?`n" | ForEach-Object { $_.TrimEnd() } | Where-Object { $_ -ne '' }
    [System.IO.File]::WriteAllText($Path, ($lines -join "`r`n") + "`r`n")
}

function Encrypt-Xor {
    param([string]$Path, [string]$KeyStr)
    $bytes  = [System.IO.File]::ReadAllBytes($Path)
    $keyB   = [System.Text.Encoding]::ASCII.GetBytes($KeyStr)
    $kLen   = $keyB.Length
    for ($i = 0; $i -lt $bytes.Length; $i++) {
        $bytes[$i] = $bytes[$i] -bxor $keyB[$i % $kLen]
    }
    [System.IO.File]::WriteAllBytes($Path, $bytes)
}

$ok = 0
$fail = 0
foreach ($m in $Mappings) {
    $hlsl = Join-Path $HlslDir ($m.Hlsl + '.hlsl')
    $vsh  = Join-Path $OutDir  ($m.Out + '.vsh')
    if (-not (Test-Path $hlsl)) { Write-Host "[SKIP] missing $hlsl" -ForegroundColor Yellow; continue }

    # Build arg array and call fxc directly (avoids Start-Process quoting issues)
    $argList = [System.Collections.Generic.List[string]]@($Target, $Profile)
    if ($m.Defs -and $m.Defs.Trim() -ne '') {
        foreach ($d in ($m.Defs -split '\s+')) { if ($d.Trim()) { $argList.Add($d.Trim()) } }
    }
    $argList.Add("/Fc$vsh")
    $argList.Add($hlsl)

    $psi = [System.Diagnostics.ProcessStartInfo]::new($Fxc)
    $psi.UseShellExecute        = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError  = $true
    $psi.Arguments              = ($argList | ForEach-Object { '"' + $_ + '"' }) -join ' '
    $proc = [System.Diagnostics.Process]::Start($psi)
    $stdout = $proc.StandardOutput.ReadToEnd()
    $stderr = $proc.StandardError.ReadToEnd()
    $proc.WaitForExit()
    if ($proc.ExitCode -ne 0) {
        Write-Host "[FAIL] $($m.Out): fxc exit $($proc.ExitCode)" -ForegroundColor Red
        $stderr -split "`n" | Select-Object -First 6 | ForEach-Object { Write-Host "       $_" }
        $fail++
        continue
    }

    Strip-Comments -Path $vsh
    Encrypt-Xor    -Path $vsh -KeyStr $Key

    # Deploy to client
    Copy-Item -LiteralPath $vsh -Destination (Join-Path $ClientDir ($m.Out + '.vsh')) -Force

    Write-Host "[OK]   $($m.Out).vsh -> client\shader\" -ForegroundColor Green
    $ok++
}

Write-Host ""
Write-Host "Done. ok=$ok fail=$fail" -ForegroundColor Cyan
