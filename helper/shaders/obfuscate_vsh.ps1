# Strip comments + XOR encrypt existing .vsh files in client/shader/
# PowerShell replacement for obfuscate_vsh.lua (no LuaJIT).

$ErrorActionPreference = 'Stop'

$Root      = $PSScriptRoot
$ClientDir = (Resolve-Path (Join-Path $Root '..\..\client\shader')).Path
$Key       = 'X7#m9$KpL2@v5*ZnQ8!w4&YhF3%r6^Dq'

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

Write-Host 'PKO VSH Obfuscator (client/shader/)'
Write-Host ''

$files = Get-ChildItem -Path $ClientDir -Filter '*.vsh' -File
if ($files.Count -eq 0) {
    Write-Host 'No .vsh files found.' -ForegroundColor Yellow
    exit 0
}

$backup = Join-Path $ClientDir ('backup_' + (Get-Date -Format 'yyyyMMdd_HHmmss'))
New-Item -ItemType Directory -Path $backup | Out-Null
Write-Host "Backup: $backup"

$count = 0
foreach ($f in $files) {
    Copy-Item -LiteralPath $f.FullName -Destination (Join-Path $backup $f.Name) -Force
    Strip-Comments -Path $f.FullName
    Encrypt-Xor -Path $f.FullName -KeyStr $Key
    Write-Host "  $($f.Name)"
    $count++
}

Write-Host ''
Write-Host "Processed $count file(s)." -ForegroundColor Cyan
