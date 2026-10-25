# standalone-probe.ps1: run a rebuilt exe WITHOUT SummonerFix and watch it from outside (docs/tools.md,
# docs/future/modern-port.md rung L3). The test harness for a 64-bit exe until agent mode is in the source: SummonerFix
# is a 32-bit DLL, and screen captures of the OpenGL window come back black, so the game's state is read with
# ReadProcessMemory at its own addresses: a 64-bit exe's from its link map (plus the module's load address), a 32-bit
# rebuilt exe's from its .addr table (Sum.exe address -> its own; loaded at 0x400000).
#
#   tools/standalone-probe.ps1 -Exe Sum-fixed-clang-x64.exe -Map build/link-fixed-clang-x64/Sum-fixed-clang-x64.map
#   tools/standalone-probe.ps1 -Exe Sum-fixed-clang.exe -Map <game>/Sum-fixed-clang.addr -Load
#   tools/standalone-probe.ps1 -Exe Sum-fixed-clang-x64.exe -Map ... -Load -Lldb out.txt
#
# -Game: the folder (default SummonerToolchain/rebuilt-test). Its dinput.dll (SummonerFix) is moved to queue/ for the run
# and always put back. -Seconds: how long to watch (default 24). -Load: click Load Saved Game and the newest save with
# real mouse input (game coordinates 623,418 and 680,392 of the 1024x768 client, the spots tools/link-savecheck.mjs
# clicks); this takes the foreground for a moment. -Lldb FILE: attach llvm-mingw's lldb (CLANG_DIR) and write its
# backtrace and registers there if the game crashes. The game is killed at the end; nothing is saved.
param([Parameter(Mandatory)][string]$Exe, [Parameter(Mandatory)][string]$Map,
      [string]$Game = (Join-Path $PSScriptRoot '..\..\..\SummonerToolchain\rebuilt-test'),
      [int]$Seconds = 24, [switch]$Load, [string]$Lldb)
$ErrorActionPreference = 'Stop'
$Game = (Resolve-Path $Game).Path
$Map = (Resolve-Path $Map).Path
$dll = Join-Path $Game 'dinput.dll'
$aside = Join-Path $Game 'queue\dinput.dll.standalone-probe'

# The watched variables: Sum.exe address (for an .addr table) and source name (for a map).
$vars = [ordered]@{
    gameseq_state_cur = '5b27e0'; interface_cur_screen = '5bd814'; fps = '5a5744'; Timestamp_realtime_ticker = '5a575c'
    level_time = '60a654'; mouse_x = '257efe0'; mouse_y = '257efdc'
}
$addr = @{}
$fixedBase = $Map -like '*.addr'
if ($fixedBase) {
    $table = @{}
    foreach ($l in Get-Content $Map) { $f = $l -split ' '; $table[$f[0]] = [Convert]::ToInt64($f[2], 16) }
    foreach ($n in $vars.Keys) { $addr[$n] = $table[$vars[$n]] }
} else {
    foreach ($l in Get-Content $Map) {
        if ($l -match '^([0-9a-f]{8}) 00000000 +0 +(\S+)$' -and $vars.Contains($Matches[2]) -and -not $addr.ContainsKey($Matches[2])) {
            $addr[$Matches[2]] = [Convert]::ToInt64($Matches[1], 16)
        }
    }
}
foreach ($n in $vars.Keys) { if (-not $addr.ContainsKey($n) -or $null -eq $addr[$n]) { throw "no address for $n in $Map" } }

Add-Type @"
using System; using System.Runtime.InteropServices;
public static class StandaloneProbe {
  [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(int a, bool i, int pid);
  [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
  [DllImport("kernel32.dll")] static extern bool ReadProcessMemory(IntPtr h, IntPtr addr, byte[] buf, int n, out int read);
  public static byte[] Read(int pid, long addr, int n) {
    var h = OpenProcess(0x10, false, pid); var b = new byte[n]; int r; ReadProcessMemory(h, new IntPtr(addr), b, n, out r); CloseHandle(h); return b; }
  public struct PT { public int X, Y; }
  [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr h, ref PT p);
  [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] static extern bool SetCursorPos(int x, int y);
  [DllImport("user32.dll")] static extern void mouse_event(uint f, uint dx, uint dy, uint d, IntPtr e);
  [DllImport("user32.dll")] static extern void keybd_event(byte vk, byte scan, uint f, IntPtr e);
  public static void Click(IntPtr h, int x, int y) {
    keybd_event(0x12, 0, 0, IntPtr.Zero); keybd_event(0x12, 0, 2, IntPtr.Zero);   // Alt lets the foreground change through
    SetForegroundWindow(h); System.Threading.Thread.Sleep(400);
    var p = new PT { X = x, Y = y }; ClientToScreen(h, ref p); SetCursorPos(p.X, p.Y); System.Threading.Thread.Sleep(150);
    mouse_event(2, 0, 0, 0, IntPtr.Zero); System.Threading.Thread.Sleep(120); mouse_event(4, 0, 0, 0, IntPtr.Zero); } }
"@

function Get-State($p, $base) {
    ($vars.Keys | ForEach-Object {
        $b = [StandaloneProbe]::Read($p.Id, $base + $addr[$_], 4)
        $v = if ($_ -in 'fps', 'level_time') { [BitConverter]::ToSingle($b, 0).ToString('0.0') } else { [BitConverter]::ToInt32($b, 0) }
        "$($_ -replace '^(gameseq_|interface_cur_|Timestamp_realtime_)', '')=$v"
    }) -join ' '
}

if (Test-Path $dll) { Move-Item $dll $aside -Force }
$p = $null; $dbg = $null
try {
    $p = Start-Process -FilePath (Join-Path $Game $Exe) -WorkingDirectory $Game -PassThru
    Start-Sleep -Seconds 5
    $base = if ($fixedBase) { 0 } else { $p.MainModule.BaseAddress.ToInt64() }
    if ($Lldb) {
        if (-not $env:CLANG_DIR) { throw '-Lldb needs CLANG_DIR (llvm-mingw''s bin folder, for lldb.exe)' }
        $lldbArgs = "--batch -p $($p.Id) -o continue -k `"thread backtrace -c 30`" -k `"register read`" -k `"process kill`" -k quit"
        $dbg = Start-Process -FilePath (Join-Path $env:CLANG_DIR 'lldb.exe') -ArgumentList $lldbArgs -RedirectStandardOutput $Lldb `
            -RedirectStandardError "$Lldb.err" -PassThru -WindowStyle Hidden
        Start-Sleep -Seconds 4
    }
    "start   " + (Get-State $p $base)
    if ($Load) {
        [StandaloneProbe]::Click($p.MainWindowHandle, 623, 418)
        Start-Sleep -Milliseconds 2500
        "menu    " + (Get-State $p $base)
        [StandaloneProbe]::Click($p.MainWindowHandle, 680, 392)
    }
    for ($t = 3; $t -le $Seconds; $t += 3) {
        Start-Sleep -Seconds 3
        $p.Refresh()
        if ($p.HasExited) { "t=${t}s exited, code 0x{0:x8}" -f $p.ExitCode; break }
        "t=${t}s".PadRight(8) + (Get-State $p $base)
    }
} finally {
    if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force }
    if ($dbg) { Start-Sleep -Seconds 2; if (-not $dbg.HasExited) { Stop-Process -Id $dbg.Id -Force } }
    Start-Sleep -Milliseconds 500
    if (Test-Path $aside) { Move-Item $aside $dll -Force }
}
