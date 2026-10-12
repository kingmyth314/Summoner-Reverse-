# Build SummonerFix (dinput.dll) with LLVM-MinGW's 32-bit compiler.
# usage: .\tools\build-summonerfix.ps1 [-Install]   (-Install copies the DLL and, if missing, the ini next to Sum.exe)
param([switch]$Install)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$src = Join-Path $repo 'src\summonerfix'
$out = Join-Path $repo 'build'
New-Item -ItemType Directory -Force $out | Out-Null

$cc = Get-Command i686-w64-mingw32-gcc -ErrorAction SilentlyContinue
if (-not $cc) { throw 'i686-w64-mingw32-gcc not found (install MartinStorsjo.LLVM-MinGW.UCRT with winget).' }

$ccArgs = @('-shared', '-O2', '-Wall', '-Wextra', '-Wno-unused-parameter', '-static',
          '-o', (Join-Path $out 'dinput.dll'),
          (Join-Path $src 'main.c'), (Join-Path $src 'util.c'), (Join-Path $src 'reghook.c'), (Join-Path $src 'diag.c'), (Join-Path $src 'glfix.c'),
          (Join-Path $src 'dinput.def'),
          '-Wl,--enable-stdcall-fixup', '-ladvapi32', '-luser32')
$ErrorActionPreference = 'Continue'
& $cc.Source @ccArgs 2>&1 | ForEach-Object { "$_" }
$ErrorActionPreference = 'Stop'
if ($LASTEXITCODE -ne 0) { throw "compile failed ($LASTEXITCODE)" }
Copy-Item (Join-Path $src 'summonerfix.ini') (Join-Path $out 'summonerfix.ini') -Force
Get-Item (Join-Path $out 'dinput.dll') | ForEach-Object { "built $($_.FullName) ($($_.Length) bytes)" }

if ($Install) {
    $game = Join-Path $repo 'Summoner'
    Copy-Item (Join-Path $out 'dinput.dll') (Join-Path $game 'dinput.dll') -Force
    if (-not (Test-Path (Join-Path $game 'summonerfix.ini'))) {
        Copy-Item (Join-Path $src 'summonerfix.ini') (Join-Path $game 'summonerfix.ini')
    }
    "installed to $game"
}
