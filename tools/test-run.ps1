# Start Sum.exe, wait, screenshot the primary screen, report state and the SummonerFix log, then close it.
# usage: .\tools\test-run.ps1 [-Seconds 20] [-Shots 2] [-OutDir <folder>] [-KeepRunning] [-GameArgs '...']
param([int]$Seconds = 20, [int]$Shots = 2, [string]$OutDir = $env:TEMP, [switch]$KeepRunning, [string]$GameArgs = '')
$repo = Split-Path $PSScriptRoot -Parent
$game = Join-Path $repo 'Summoner'
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
if (-not ('WinText' -as [type])) {
    Add-Type @'
using System; using System.Text; using System.Collections.Generic; using System.Runtime.InteropServices;
public static class WinText {
    delegate bool EnumProc(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc f, IntPtr l);
    [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr p, EnumProc f, IntPtr l);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll", CharSet = CharSet.Auto)] static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
    static string Text(IntPtr h) { var s = new StringBuilder(4096); GetWindowText(h, s, s.Capacity); return s.ToString(); }
    // Visible top-level windows of a process, each with the text of its child controls (dialog message text).
    public static List<string> Dump(uint pid) {
        var r = new List<string>();
        EnumWindows((h, l) => {
            uint p; GetWindowThreadProcessId(h, out p);
            if (p == pid && IsWindowVisible(h)) {
                var parts = new List<string>();
                EnumChildWindows(h, (c, l2) => { var t = Text(c); if (t.Length > 0) parts.Add(t.Replace("\r", " ").Replace("\n", " ")); return true; }, IntPtr.Zero);
                r.Add("'" + Text(h) + "': " + string.Join(" | ", parts));
            }
            return true;
        }, IntPtr.Zero);
        return r;
    }
}
'@
}

function Save-Screen([string]$path) {
    $b = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bmp = New-Object System.Drawing.Bitmap $b.Width, $b.Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($b.Location, [System.Drawing.Point]::Empty, $b.Size)
    # Downscale so the image is quick to inspect.
    $w = [Math]::Min(1280, $b.Width); $h = [int]($b.Height * $w / $b.Width)
    $small = New-Object System.Drawing.Bitmap $bmp, $w, $h
    $small.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose(); $small.Dispose()
    $path
}

Get-Process Sum -ErrorAction SilentlyContinue | Stop-Process -Force
$psi = @{ FilePath = (Join-Path $game 'Sum.exe'); WorkingDirectory = $game; PassThru = $true }
if ($GameArgs) { $psi.ArgumentList = $GameArgs }
$p = Start-Process @psi
"started Sum.exe pid $($p.Id) $GameArgs"
$step = [Math]::Max(1, [int]($Seconds / [Math]::Max(1, $Shots)))
for ($i = 1; $i -le $Shots; $i++) {
    Start-Sleep -Seconds $step
    $p.Refresh()
    $state = if ($p.HasExited) { "EXITED code $($p.ExitCode)" } else { "running, window '$($p.MainWindowTitle)', $([int]($p.WorkingSet64/1MB)) MB" }
    $shot = Save-Screen (Join-Path $OutDir ("sum-shot-$i.png"))
    "t+$($i*$step)s: $state  screenshot: $shot"
    if (-not $p.HasExited) { [WinText]::Dump([uint32]$p.Id) | ForEach-Object { "   window $_" } }
    if ($p.HasExited) { break }
}
$others = Get-Process | Where-Object { $_.MainWindowTitle -and $_.Id -ne $p.Id -and $_.StartTime -gt (Get-Date).AddSeconds(-($Seconds + 5)) } | ForEach-Object { "$($_.ProcessName): '$($_.MainWindowTitle)'" }
if ($others) { "new windows from other processes: $($others -join '; ')" }
if (-not $KeepRunning -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force; "closed Sum.exe" }
"--- summonerfix.log"
Get-Content (Join-Path $game 'summonerfix.log') -ErrorAction SilentlyContinue
