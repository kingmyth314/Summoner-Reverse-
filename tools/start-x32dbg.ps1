# Start x32dbg (32-bit x64dbg) with its MCP server plugin, if it is not already running.
# The MCP server (127.0.0.1:9095) lives inside x32dbg, so start this before Claude Code needs it.
# usage: .\tools\start-x32dbg.ps1 [-Game]   (-Game also loads Summoner\Sum.exe, paused at the system breakpoint)
param([switch]$Game)
$dbg = Join-Path $env:LOCALAPPDATA 'Programs\x64dbg\x32'
if (-not (Test-Path "$dbg\x32dbg.exe")) { throw "x32dbg not found in $dbg (see docs/setup.md)" }
$running = Get-Process x32dbg -ErrorAction SilentlyContinue
if (-not $running) {
    $dbgArgs = @()
    if ($Game) { $dbgArgs = @('"' + (Join-Path (Split-Path $PSScriptRoot -Parent) 'Summoner\Sum.exe') + '"') }
    $p = if ($dbgArgs) { Start-Process "$dbg\x32dbg.exe" -WorkingDirectory $dbg -ArgumentList $dbgArgs -PassThru } else { Start-Process "$dbg\x32dbg.exe" -WorkingDirectory $dbg -PassThru }
    "started x32dbg (pid $($p.Id))"
} else {
    "x32dbg already running (pid $($running[0].Id))"
}
for ($i = 0; $i -lt 20; $i++) {
    if (Get-NetTCPConnection -State Listen -LocalPort 9095 -ErrorAction SilentlyContinue) { "MCP server listening on 127.0.0.1:9095"; return }
    Start-Sleep 1
}
"WARNING: MCP server not listening on port 9095 (check Plugins > x64dbg-MCP Server in x32dbg)"
