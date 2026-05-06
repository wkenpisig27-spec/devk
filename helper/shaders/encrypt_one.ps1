param([string]$Path)

$key = 'X7#m9$KpL2@v5*ZnQ8!w4&YhF3%r6^Dq'
$keyBytes = [System.Text.Encoding]::ASCII.GetBytes($key)
$keyLen = $keyBytes.Length

# Read text, strip comments, minimize whitespace
$lines = Get-Content -LiteralPath $Path
$out = New-Object System.Collections.Generic.List[string]
$inMl = $false
foreach ($lineRaw in $lines) {
    $line = $lineRaw
    if ($inMl) {
        $idx = $line.IndexOf('*/')
        if ($idx -ge 0) { $line = $line.Substring($idx + 2); $inMl = $false }
        else { $line = '' }
    }
    if ($line.Length -gt 0) {
        $s = $line.IndexOf('/*')
        if ($s -ge 0) {
            $e = $line.IndexOf('*/', $s)
            if ($e -ge 0) { $line = $line.Substring(0, $s) + $line.Substring($e + 2) }
            else { $line = $line.Substring(0, $s); $inMl = $true }
        }
    }
    $cs = $line.IndexOf('//')
    if ($cs -ge 0) { $line = $line.Substring(0, $cs) }
    $line = $line.TrimEnd()
    if ($line.Length -gt 0) { $out.Add($line) }
}

# Collapse consecutive empties (already removed) and join
$text = ($out -join "`n") + "`n"
$bytes = [System.Text.Encoding]::ASCII.GetBytes($text)

# XOR encrypt
for ($i = 0; $i -lt $bytes.Length; $i++) {
    $bytes[$i] = $bytes[$i] -bxor $keyBytes[$i % $keyLen]
}

[System.IO.File]::WriteAllBytes($Path, $bytes)
Write-Host "Encrypted $Path -> $($bytes.Length) bytes"
