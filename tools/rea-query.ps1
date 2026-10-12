# Submit one or more REA tool calls to tools/rea-daemon.mjs and print the results.
# usage: .\tools\rea-query.ps1 -Calls '[{"name":"...","arguments":{...}}, ...]' [-Max 6000] [-Queue .rea-queue]
param([Parameter(Mandatory)][string]$Calls, [int]$Max = 6000, [int]$TimeoutSec = 900, [string]$Queue = '.rea-queue')
$repoRoot = Split-Path $PSScriptRoot -Parent
$qdir = if ([IO.Path]::IsPathRooted($Queue)) { $Queue } else { Join-Path $repoRoot $Queue }
if (-not (Test-Path "$qdir\daemon.ready")) { "Daemon not running (no $qdir\daemon.ready). Start: node tools/rea-daemon.mjs"; exit 1 }
if (Test-Path "$qdir\daemon.exit") { "DAEMON EXITED"; exit 1 }
try { $list = $Calls | ConvertFrom-Json -ErrorAction Stop } catch { $list = $null }
if ($null -eq $list) { "Invalid -Calls JSON (quotes stripped by the calling shell?): $Calls"; exit 1 }
$ids = @()
$n = 0
foreach ($c in @($list)) {
    $id = '{0:yyyyMMddHHmmssfff}-{1:d3}' -f (Get-Date), $n++
    $json = $c | ConvertTo-Json -Depth 10 -Compress
    [IO.File]::WriteAllText("$qdir\$id.tmpreq", $json)
    Move-Item "$qdir\$id.tmpreq" "$qdir\$id.req.json"
    $ids += [pscustomobject]@{ id = $id; name = $c.name; args = ($c.arguments | ConvertTo-Json -Compress -Depth 10) }
}
$deadline = (Get-Date).AddSeconds($TimeoutSec)
foreach ($r in $ids) {
    while (-not (Test-Path "$qdir\$($r.id).res.json")) {
        if ((Get-Date) -gt $deadline) { "TIMEOUT waiting for $($r.name)"; exit 1 }
        if (Test-Path "$qdir\daemon.exit") { "DAEMON EXITED"; exit 1 }
        Start-Sleep -Milliseconds 300
    }
    $res = Get-Content "$qdir\$($r.id).res.json" -Raw | ConvertFrom-Json
    "=== $($r.name) $($r.args)  isError=$($res.isError) ($([int]($res.elapsedMs/1000))s)"
    $t = $res.text
    # Strip the repeated subject/profile boilerplate: keep evidence id, result and limitations.
    try {
        $o = $t | ConvertFrom-Json
        if ($o.evidence_id) {
            $body = if ($null -ne $o.normalized_result) { $o.normalized_result } else { $o.raw_result }
            $lim = @($o.limitations | Where-Object { $_ -notmatch '^(The session serves|The imported Program|Cancelling an active|Default-space addresses|Windows Ghidra P0|The Windows bridge|Windows sessions require|Derived by an REA|Pseudocode is Ghidra|External functions and)' })
            $t = "evidence: $($o.evidence_id)`n" + ($body | ConvertTo-Json -Depth 30 -Compress)
            if ($lim.Count) { $t += "`nlimitations: " + ($lim -join ' | ') }
        }
    } catch {}
    if ($t.Length -gt $Max) { $t.Substring(0, $Max) + "`n...[truncated $($t.Length - $Max) chars; full: $qdir\$($r.id).res.json]" } else { $t }
}
