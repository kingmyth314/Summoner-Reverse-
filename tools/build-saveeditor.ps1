# Build the C savegame editor (src/saveeditor) with LLVM-MinGW's 32-bit compiler, as tools/build-summonerfix.ps1 does,
# into build\saveeditor:
#   build\saveeditor\saveedit.exe            the command line (savedit.mjs options)
#   build\saveeditor\SummonerSaveEditor.exe  the window app for players (goes in the game folder, next to Sum.exe)
#   build\saveeditor\app_test.exe            the test of the app's "changes -> edit list" logic (see APP.md)
# usage: .\tools\build-saveeditor.ps1
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$src = Join-Path $repo 'src\saveeditor'
$out = Join-Path $repo 'build\saveeditor'
New-Item -ItemType Directory -Force $out | Out-Null
$cc = Get-Command i686-w64-mingw32-gcc -ErrorAction SilentlyContinue
if (-not $cc) { throw 'i686-w64-mingw32-gcc not found (install MartinStorsjo.LLVM-MinGW.UCRT with winget).' }
$warn = @('-O2', '-Wall', '-Wextra', '-static', '-municode')
$lib = Join-Path $src 'saveedit.c'
$model = Join-Path $src 'saveeditor_model.c'
$targets = @(
    @{ exe = 'saveedit.exe'; args = @((Join-Path $src 'saveedit_cli.c'), $lib) },
    @{ exe = 'SummonerSaveEditor.exe'; args = @('-mwindows', (Join-Path $src 'saveeditor_app.c'), $model, $lib,
                                                '-lcomctl32', '-lcomdlg32', '-lshell32', '-luser32', '-lgdi32') },
    @{ exe = 'app_test.exe'; args = @((Join-Path $src 'app_test.c'), $model, $lib) }
)
foreach ($t in $targets) {
    $ccArgs = $warn + @('-o', (Join-Path $out $t.exe)) + $t.args
    $ErrorActionPreference = 'Continue'
    $msgs = & $cc.Source @ccArgs 2>&1 | ForEach-Object { "$_" }
    $ErrorActionPreference = 'Stop'
    $msgs
    if ($LASTEXITCODE -ne 0) { throw "compile of $($t.exe) failed ($LASTEXITCODE)" }
    Get-Item (Join-Path $out $t.exe) | ForEach-Object { "built $($_.Name) ($($_.Length) bytes)" }
}
