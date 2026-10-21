# Drives work\SummonerSaveEditor.exe through window messages (no mouse, no keyboard) on copies of two corpus
# saves, saves once, checks the new file against tools/savedit.mjs, tries a refused save, and closes the app.
# usage (from the repository folder): powershell -File <this folder>\ui_test.ps1 [-Tables Summoner\tables.vpp]
# Everything is written into a new work\ui-<time> folder next to this script; screenshots of each page go there.
# -Corpus: a folder of COPIES of saves (never a game's savegame folder); -Out: where the ui-<time> folder goes
# (default: the system temp folder).
param([Parameter(Mandatory = $true)][string]$Corpus, [string]$Out = (Join-Path ([IO.Path]::GetTempPath()) 'saveeditor-uitest'),
      [string]$Tables = 'Summoner\tables.vpp', [string]$Save = 'summoner1.sav')
if ($Corpus -match '[\\/]savegame[\\/]?$') { throw '-Corpus must be a folder of copies, not a savegame folder' }
$ErrorActionPreference = 'Stop'
if (-not (Test-Path 'tools\savedit.mjs')) { throw 'run this from the repository folder' }
$Tables = (Resolve-Path $Tables).Path
$exe = Join-Path (Split-Path $PSScriptRoot -Parent) 'build\saveeditor\SummonerSaveEditor.exe'
$run = Join-Path $Out ("ui-" + (Get-Date -Format 'yyyyMMdd-HHmmss'))
$game = Join-Path $run 'game'
$sg = Join-Path $game 'savegame'
New-Item -ItemType Directory -Force $sg | Out-Null
Copy-Item (Join-Path $Corpus $Save) (Join-Path $sg 'summoner1.sav')
Copy-Item (Join-Path $Corpus 'summoner0.sav') (Join-Path $sg 'summoner0.sav')
(Get-Item (Join-Path $sg 'summoner0.sav')).LastWriteTime = (Get-Date).AddDays(-1)   # summoner1 is the newest: row 0

Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System; using System.Text; using System.Runtime.InteropServices; using System.Drawing; using System.Drawing.Imaging;
public static class W {
  [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr h, int id);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr SendMessage(IntPtr h, int m, IntPtr w, string l);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr SendMessage(IntPtr h, int m, IntPtr w, StringBuilder l);
  [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, int m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, int m, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, int f);
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc f, IntPtr l);
  [DllImport("kernel32.dll")] public static extern IntPtr OpenProcess(int a, bool i, uint pid);
  [DllImport("kernel32.dll")] public static extern IntPtr VirtualAllocEx(IntPtr p, IntPtr a, int n, int t, int pr);
  [DllImport("kernel32.dll")] public static extern bool WriteProcessMemory(IntPtr p, IntPtr a, byte[] b, int n, out IntPtr w);
  [DllImport("kernel32.dll")] public static extern bool ReadProcessMemory(IntPtr p, IntPtr a, byte[] b, int n, out IntPtr r);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  public static string Text(IntPtr h) { var sb = new StringBuilder(70000); SendMessage(h, 0x0D, (IntPtr)70000, sb); return sb.ToString(); }
  public static IntPtr Dialog(uint pid) {
    IntPtr found = IntPtr.Zero;
    EnumWindows((h, l) => { uint p; GetWindowThreadProcessId(h, out p); var sb = new StringBuilder(64); GetClassName(h, sb, 64);
      if (p == pid && sb.ToString() == "#32770" && IsWindowVisible(h)) { found = h; return false; } return true; }, IntPtr.Zero);
    return found;
  }
  public static void Shot(IntPtr h, string path) {
    RECT r; GetWindowRect(h, out r);
    using (var bmp = new Bitmap(r.R - r.L, r.B - r.T)) { using (var g = Graphics.FromImage(bmp)) { IntPtr dc = g.GetHdc(); PrintWindow(h, dc, 2); g.ReleaseHdc(dc); } bmp.Save(path, ImageFormat.Png); }
  }
  static IntPtr proc, mem;
  public static void Attach(uint pid) { proc = OpenProcess(0x0438, false, pid); mem = VirtualAllocEx(proc, IntPtr.Zero, 4096, 0x3000, 4); }
  // LVM_SETITEMSTATE with an LVITEM in the app's memory (32-bit layout: state at +12, stateMask at +16)
  public static void LvState(IntPtr lv, int row, int state, int mask) {
    var b = new byte[64]; BitConverter.GetBytes(state).CopyTo(b, 12); BitConverter.GetBytes(mask).CopyTo(b, 16);
    IntPtr n; WriteProcessMemory(proc, mem, b, 64, out n); SendMessage(lv, 0x102B, (IntPtr)row, mem);
  }
  public static RECT TabRect(IntPtr tab, int i) {
    SendMessage(tab, 0x130A, (IntPtr)i, mem); var b = new byte[16]; IntPtr n; ReadProcessMemory(proc, mem, b, 16, out n);
    RECT r; r.L = BitConverter.ToInt32(b, 0); r.T = BitConverter.ToInt32(b, 4); r.R = BitConverter.ToInt32(b, 8); r.B = BitConverter.ToInt32(b, 12); return r;
  }
}
'@

# control ids (the enum in saveeditor_app.c)
$ID = @{ TAB=100; SAVEDIR=101; SAVELIST=102; OPEN=103; TABLESINFO=106; FILEINFO=107; DESC=108; GOLD=109; FIELD0=111;
         SKILLS=118; SKILLVAL=119; SKILLSET=120; INV=122; INVCOUNT=123; INVSET=124; ADDITEM=126; ADDCOUNT=127; ADD=128;
         LEVELS=130; FLAGS=131; FLAGINFO=132; NOTICE=133; STATUS=134; SAVESLOT=135 }
$fails = 0
function Check($ok, $what) { if ($ok) { "pass  $what" } else { "FAIL  $what"; $script:fails++ } }
function C($name) { [W]::GetDlgItem($script:hwnd, $ID[$name]) }
function T($name) { [W]::Text((C $name)) }
function SetT($name, $text) { [void][W]::SendMessage((C $name), 0x0C, [IntPtr]::Zero, [string]$text) }
function Click($name) { [void][W]::SendMessage((C $name), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero) }
function Page($i, $shot) {
  $r = [W]::TabRect((C 'TAB'), $i)
  $x = [int][math]::Floor(($r.L + $r.R) / 2); $y = [int][math]::Floor(($r.T + $r.B) / 2)
  $lp = [IntPtr](($y -shl 16) -bor $x)
  [void][W]::PostMessage((C 'TAB'), 0x201, [IntPtr]1, $lp); [void][W]::PostMessage((C 'TAB'), 0x202, [IntPtr]::Zero, $lp)
  Start-Sleep -Milliseconds 600
  [W]::Shot($script:hwnd, (Join-Path $run $shot))
}
function WaitBox() { for ($i = 0; $i -lt 50; $i++) { $b = [W]::Dialog([uint32]$script:p.Id); if ($b -ne [IntPtr]::Zero) { return $b }; Start-Sleep -Milliseconds 100 }; return [IntPtr]::Zero }

$p = Start-Process $exe -ArgumentList @('--game-dir', "`"$game`"", '--tables', "`"$Tables`"") -PassThru
try {
  for ($i = 0; $i -lt 50 -and $p.MainWindowHandle -eq 0; $i++) { Start-Sleep -Milliseconds 100; $p.Refresh() }
  $hwnd = $p.MainWindowHandle
  Check ($hwnd -ne 0) 'main window opened'
  [W]::Attach([uint32]$p.Id)
  Start-Sleep -Milliseconds 600
  [W]::Shot($hwnd, (Join-Path $run '1-saves.png'))
  Check ((T 'NOTICE') -eq 'Changes are written to a new save; your saves are never overwritten.') 'notice line'
  Check ([int][W]::SendMessage((C 'SAVELIST'), 0x1004, [IntPtr]::Zero, [IntPtr]::Zero) -eq 2) 'saves list shows the 2 saves'
  Check ((T 'TABLESINFO') -like 'Names of items and story flags:*') 'tables.vpp loaded'

  # open the newest save (row 0) with the Open button
  [W]::LvState((C 'SAVELIST'), 0, 3, 3)
  Click 'OPEN'
  Check ((T 'FILEINFO') -like '*summoner1.sav*') 'Open opened summoner1.sav'
  [W]::Shot($hwnd, (Join-Path $run '2-general.png'))
  $gold0 = T 'GOLD'
  SetT 'DESC' 'ui test'; SetT 'GOLD' '4321'
  Page 2 '3-party.png'
  SetT 'FIELD0' '9'
  [W]::LvState((C 'SKILLS'), 3, 3, 3)
  $sk = T 'SKILLVAL'
  Check ($sk -match '^\d+$') "skill row 3 selected shows its value ($sk)"
  $newSkill = if ($sk -eq '77') { '78' } else { '77' }
  SetT 'SKILLVAL' $newSkill; Click 'SKILLSET'
  Page 3 '4-inventory.png'
  [W]::LvState((C 'INV'), 0, 3, 3)
  $cnt = T 'INVCOUNT'
  SetT 'INVCOUNT' ([string]([int]$cnt + 4)); Click 'INVSET'
  [void][W]::SendMessage((C 'ADDITEM'), 0x14E, [IntPtr]5, [IntPtr]::Zero)
  $addName = T 'ADDITEM'
  SetT 'ADDCOUNT' '2'; Click 'ADD'
  Page 4 '5-flags.png'
  $st = [int][W]::SendMessage((C 'FLAGS'), 0x102C, [IntPtr]0, [IntPtr]0xF000)
  $newState = if ($st -eq 0x2000) { 0x1000 } else { 0x2000 }
  [W]::LvState((C 'FLAGS'), 0, $newState, 0xF000)
  [W]::Shot($hwnd, (Join-Path $run '6-flag-toggled.png'))
  $status = T 'STATUS'
  "status: $status"
  Check ($status -match '(6|7) changes not saved yet') 'status counts the changes'
  $slot = T 'SAVESLOT'
  Check ($slot -eq 'Save as summoner2.sav') "slot button offers the next free slot ($slot)"

  # save
  [void][W]::PostMessage((C 'SAVESLOT'), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
  $box = WaitBox
  Check ($box -ne [IntPtr]::Zero) 'a message box reports the save'
  $boxText = [W]::Text([W]::GetDlgItem($box, 0xFFFF))
  [W]::Shot($box, (Join-Path $run '7-saved.png'))
  "message box:`n$boxText"
  [void][W]::PostMessage($box, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
  Start-Sleep -Milliseconds 500
  $new = Join-Path $sg 'summoner2.sav'
  Check (Test-Path $new) 'summoner2.sav written'
  Check ((Get-FileHash (Join-Path $sg 'summoner1.sav')).Hash -eq (Get-FileHash (Join-Path $Corpus $Save)).Hash) 'the opened save is unchanged'
  Check ((T 'FILEINFO') -like '*summoner2.sav*') 'the editor now shows the new save'
  Check ((T 'SAVESLOT') -eq 'Save as summoner3.sav') 'next slot is now summoner3.sav'

  # the same edits with savedit.mjs, names taken from the change log
  $a = @('--desc', 'ui test', '--gold', '4321', '--level', '9', '--skill', "3=$newSkill")
  foreach ($line in ($boxText -split "`r?`n")) {
    if ($line -match '^"(.+)" count (-?\d+) -> (\d+)$') { $a += @('--set-item-count', "$($Matches[1])=$($Matches[3])") }
    elseif ($line -match '^flag ([^:]+):(.+) (\d) -> (\d)$') { $a += @('--flag', "$($Matches[1]):$($Matches[2])=$($Matches[4])") }
  }
  "node args: $($a -join ' | ')"
  $nodeOut = Join-Path $run 'node.sav'
  & node tools\savedit.mjs (Join-Path $sg 'summoner1.sav') $nodeOut @a --tables $Tables | Out-Null
  Check ((Test-Path $nodeOut) -and (Get-FileHash $nodeOut).Hash -eq (Get-FileHash $new).Hash) 'app output is byte-identical to savedit.mjs'
  Check ($boxText -like "*$addName*") "the added item ($addName) is in the change log"

  # a refused save writes nothing
  SetT 'GOLD' '12x'
  [void][W]::PostMessage((C 'SAVESLOT'), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
  $box = WaitBox
  $boxText = if ($box -ne [IntPtr]::Zero) { [W]::Text([W]::GetDlgItem($box, 0xFFFF)) } else { '' }
  [W]::Shot($box, (Join-Path $run '8-refused.png'))
  "message box:`n$boxText"
  [void][W]::PostMessage($box, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
  Start-Sleep -Milliseconds 600
  Check ($boxText -like '*refused*Nothing was written*') 'refusal shows the library message'
  Check (-not (Test-Path (Join-Path $sg 'summoner3.sav'))) 'nothing written on refusal'

  # back to no changes, then close the window normally
  SetT 'GOLD' '4321'
  Check ((T 'STATUS') -like '*no changes*') 'status: no changes'
  [void]$p.CloseMainWindow()
  Check ($p.WaitForExit(5000)) 'closed on WM_CLOSE'
  if ($p.HasExited) { Check ($p.ExitCode -eq 0) "exit code $($p.ExitCode)" }
} finally {
  if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; 'stopped the app' }
}

# without tables.vpp: the app says so, opens a save, still edits gold; story flags are not offered
$p = Start-Process $exe -ArgumentList @('--game-dir', "`"$game`"", '--open', "`"$(Join-Path $sg 'summoner1.sav')`"") -PassThru
try {
  for ($i = 0; $i -lt 50 -and $p.MainWindowHandle -eq 0; $i++) { Start-Sleep -Milliseconds 100; $p.Refresh() }
  $hwnd = $p.MainWindowHandle
  [W]::Attach([uint32]$p.Id)
  Start-Sleep -Milliseconds 500
  Check ((T 'TABLESINFO') -like 'tables.vpp was not found*') 'no tables: the Saves page says tables.vpp is missing'
  Check ((T 'FILEINFO') -like '*summoner1.sav') 'no tables: --open opened the save'
  Check ((T 'FLAGINFO') -like 'Story flags are not available*') 'no tables: story flags not offered'
  SetT 'GOLD' '77'
  Check ((T 'STATUS') -like '*1 change not saved yet*') 'no tables: gold edit counted'
  Page 4 '9-notables-flags.png'
  # leave a pending change: closing must ask first; answer No (keep the window), then undo by typing the old gold back
  [void][W]::PostMessage($hwnd, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
  $box = WaitBox
  Check ($box -ne [IntPtr]::Zero) 'closing with unsaved changes asks first'
  if ($box -ne [IntPtr]::Zero) { [void][W]::PostMessage($box, 0x111, [IntPtr]7, [IntPtr]::Zero) }   # IDNO
  Start-Sleep -Milliseconds 600
  Check (-not $p.HasExited) 'No keeps the window open'
  SetT 'GOLD' $gold0
  [void]$p.CloseMainWindow()
  if (-not $p.WaitForExit(3000)) {
    $box = WaitBox; if ($box -ne [IntPtr]::Zero) { [void][W]::PostMessage($box, 0x111, [IntPtr]6, [IntPtr]::Zero) }   # IDYES
    [void]$p.WaitForExit(3000)
  }
  Check ($p.HasExited -and $p.ExitCode -eq 0) 'no tables: closed cleanly'
} finally {
  if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; 'stopped the app' }
}
"ui_test: $fails fail; screenshots in $run"
exit $fails
