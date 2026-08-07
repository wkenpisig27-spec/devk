# Rebuild FreeType + RmlUi static libraries for the game client.
param(
    [ValidateSet("Debug", "Release", "Both")]
    [string]$Config = "Both"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$cmake = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if (-not (Test-Path $cmake)) {
    $cmake = (Get-ChildItem "C:\Program Files\Microsoft Visual Studio" -Recurse -Filter cmake.exe -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName)
}
if (-not $cmake) { throw "cmake.exe not found" }

function Build-Config([string]$name) {
    Write-Host "=== Building $name ==="
    & $cmake --build "$root\third_party\freetype\build" --config $name --parallel
    & $cmake --build "$root\third_party\RmlUi\build" --config $name --parallel

    $libDir = "$root\source\lib\$name"
    New-Item -ItemType Directory -Force -Path $libDir | Out-Null
    Copy-Item "$root\third_party\RmlUi\build\$name\rmlui.lib" "$libDir\rmlui.lib" -Force
    Copy-Item "$root\third_party\RmlUi\build\$name\rmlui_debugger.lib" "$libDir\rmlui_debugger.lib" -Force
    if ($name -eq "Debug") {
        Copy-Item "$root\third_party\freetype\build\Debug\freetyped.lib" "$libDir\freetyped.lib" -Force
    } else {
        Copy-Item "$root\third_party\freetype\build\Release\freetype.lib" "$libDir\freetype.lib" -Force
    }
}

$ftInc = "$root\third_party\freetype\include;$root\third_party\freetype\build\include"
$ftLib = "$root\third_party\freetype\build\Release\freetype.lib"

if (-not (Test-Path "$root\third_party\freetype\build")) {
    & $cmake -S "$root\third_party\freetype" -B "$root\third_party\freetype\build" -G "Visual Studio 18 2026" -A x64 `
        -DFT_DISABLE_ZLIB=TRUE -DFT_DISABLE_BZIP2=TRUE -DFT_DISABLE_PNG=TRUE -DFT_DISABLE_HARFBUZZ=TRUE -DFT_DISABLE_BROTLI=TRUE `
        -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded`$<`$<CONFIG:Debug>:Debug>DLL"
}

if (-not (Test-Path "$root\third_party\RmlUi\build")) {
    & $cmake -S "$root\third_party\RmlUi" -B "$root\third_party\RmlUi\build" -G "Visual Studio 18 2026" -A x64 `
        -DBUILD_SHARED_LIBS=OFF -DRMLUI_SAMPLES=OFF -DRMLUI_LOTTIE_PLUGIN=OFF -DRMLUI_SVG_PLUGIN=OFF -DRMLUI_LUA_BINDINGS=OFF `
        -DFREETYPE_INCLUDE_DIRS="$ftInc" -DFREETYPE_LIBRARY="$ftLib" `
        -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded`$<`$<CONFIG:Debug>:Debug>DLL"
}

if ($Config -eq "Both" -or $Config -eq "Release") { Build-Config "Release" }
if ($Config -eq "Both" -or $Config -eq "Debug") { Build-Config "Debug" }

Write-Host "Done. Libraries copied to source/lib/{Debug,Release}"
