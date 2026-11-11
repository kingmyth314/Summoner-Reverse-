// The repository checks of AGENTS.md ("Invariants: check before every commit") in one command, plus a cross-check of
// the setting names that tie the builds, SummonerFix, its config app and summonerfix.ini together.
//
//   node tools/check.mjs [--sweep|--no-sweep] [--quick]
//
// 1. docs: tools/docs-check.mjs (links, front matter).
// 2. tools: every tools/**/*.mjs passes node --check; every tools/*.ps1 parses (Windows PowerShell).
// 3. personal paths: no tracked text file names a folder under the Windows Users folder (either slash).
// 4. keys: the names in the `// FIXED_BUILD: <id> <Key>` / `// EXPERIMENTAL_BUILD: <id> <Key>` markers (the .fixes
//    handshake), SummonerFix's FixOn / RebuiltFixInSource keys, the config app's fix list and the keys it writes, the
//    keys the game and SummonerFix read, and src/summonerfix/summonerfix.ini. A marker key nobody reads is how
//    LevelReverb went out as "reverb" on 2026-11-01.
// 5. sweep: VC6_STRICT=1 vc6-sweep --check src/decomp/vc6-matches.txt, when src/decomp's game, engine or header
//    source differs from origin/main or the working tree has changes there (--sweep forces it, --no-sweep skips it;
//    needs VC6_DIR). The link builds (invariant 2) are not run here: they depend on which configurations changed.
// --quick skips 2's PowerShell parse and 5. Exit 1 when an error is found; warnings do not fail.
import { readFileSync, readdirSync, statSync, existsSync } from "node:fs";
import { join, resolve, dirname, relative } from "node:path";
import { fileURLToPath } from "node:url";
import { spawnSync } from "node:child_process";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const quick = args.includes("--quick");
let errors = 0, warnings = 0;
const err = (m) => { errors++; console.log(`  ERROR ${m}`); };
const warn = (m) => { warnings++; console.log(`  warn  ${m}`); };
const section = (t) => console.log(`\n${t}`);
const read = (p) => readFileSync(join(repo, p), "latin1");
const walk = (dir, re, out = []) => {
  for (const n of readdirSync(dir)) {
    const p = join(dir, n);
    if (statSync(p).isDirectory()) walk(p, re, out);
    else if (re.test(n)) out.push(p);
  }
  return out;
};
const rel = (p) => relative(repo, p).replace(/\\/g, "/");

// 1. docs
section("1. docs (tools/docs-check.mjs)");
{
  const r = spawnSync(process.execPath, [join(repo, "tools", "docs-check.mjs")], { encoding: "utf8" });
  const last = (r.stdout || "").trim().split(/\r?\n/).pop() || "";
  console.log(`  ${last}`);
  if (r.status !== 0 || !/\b0 problem/.test(last)) err("docs-check reports problems (run it for the list)");
}

// 2. tools
section("2. tools (syntax)");
{
  const mjs = walk(join(repo, "tools"), /\.mjs$/);
  let bad = 0;
  for (const f of mjs) {
    const r = spawnSync(process.execPath, ["--check", f], { encoding: "utf8" });
    if (r.status !== 0) { bad++; err(`${rel(f)}: ${(r.stderr || "").split(/\r?\n/).find((l) => /Error/.test(l)) || "node --check failed"}`); }
  }
  console.log(`  ${mjs.length} .mjs files, ${bad} fail node --check`);
  if (!quick) {
    const ps = `$n=0; Get-ChildItem '${join(repo, "tools")}\\*.ps1' | ForEach-Object { $e=$null; [void][System.Management.Automation.Language.Parser]::ParseFile($_.FullName,[ref]$null,[ref]$e); if ($e.Count) { "BAD $($_.Name): $($e[0].Message)" }; $n++ }; "COUNT $n"`;
    const r = spawnSync("powershell", ["-NoProfile", "-NonInteractive", "-Command", ps], { encoding: "utf8" });
    if (r.status !== 0 && !r.stdout) warn("PowerShell not available: .ps1 files not parsed");
    for (const l of (r.stdout || "").split(/\r?\n/)) {
      if (l.startsWith("BAD ")) err(`tools/${l.slice(4)}`);
      if (l.startsWith("COUNT ")) console.log(`  ${l.slice(6)} .ps1 files parsed`);
    }
  }
}

// 3. personal paths
section("3. personal paths (tracked files)");
{
  const r = spawnSync("git", ["grep", "-nIiE", "c:[\\\\/]+users[\\\\/]+"], { cwd: repo, encoding: "utf8" });
  const hits = (r.stdout || "").split(/\r?\n/).filter(Boolean)
    // generic placeholders in docs (C:\Users\<you>\...) are fine
    .filter((l) => !/users[\\/]+(<[^>]+>|you|yourname|name|user|%username%)[\\/]/i.test(l));
  for (const h of hits.slice(0, 20)) err(h.length > 160 ? h.slice(0, 160) + "..." : h);
  if (hits.length > 20) err(`... ${hits.length - 20} more`);
  console.log(`  ${hits.length} user-folder paths in tracked files`);
}

// 4. keys
section("4. setting names (.fixes markers, SummonerFix, config app, summonerfix.ini)");
{
  const sfFiles = walk(join(repo, "src", "summonerfix"), /\.c$/).map((p) => [rel(p), readFileSync(p, "latin1")]);
  const decompFiles = walk(join(repo, "src", "decomp"), /\.(cpp|h)$/).map((p) => [rel(p), readFileSync(p, "latin1")]);
  const cfg = read("src/summonerfix-config/config.c");
  const iniDoc = read("src/summonerfix/summonerfix.ini");
  const all = (files, re) => files.flatMap(([f, s]) => [...s.matchAll(re)].map((m) => ({ f, m })));

  // FixOptIn (2026-11-10): a fix that is off unless its key is 1
  const fixOn = new Set(all(sfFiles, /\bFix(?:On|OptIn)\("([A-Za-z0-9]+)"\)/g).map((x) => x.m[1]));
  const inSource = new Set(all(sfFiles, /\bRebuiltFixInSource\("([A-Za-z0-9]+)"\)/g).map((x) => x.m[1]));
  const cfgFixes = new Set([...(/k_fixes\[\]\s*=\s*\{([\s\S]*?)\n\};/.exec(cfg)?.[1] ?? "").matchAll(/\{"([A-Za-z0-9]+)",/g)].map((m) => m[1]));
  // (section, key) pairs read by SummonerFix and by the game's source (exp_ini_int, modern_ini_*, vk_from_ini)
  const pair = /\(\s*"([A-Za-z]+)"\s*,\s*"([A-Za-z0-9]+)"/;
  const reads = new Set();
  for (const { m } of all(sfFiles, /GetPrivateProfile(?:Int|String)A(\(\s*"[A-Za-z]+"\s*,\s*"[A-Za-z0-9]+")/g)) { const p = pair.exec(m[1]); reads.add(`${p[1]}.${p[2]}`); }
  for (const { m } of all(decompFiles, /\b(?:exp_ini_int|exp_ini_str|modern_ini_int|modern_ini_str)(\(\s*"[A-Za-z]+"\s*,\s*"[A-Za-z0-9]+")/g)) { const p = pair.exec(m[1]); reads.add(`${p[1]}.${p[2]}`); }
  for (const { m } of all(decompFiles, /\bexp_fix_opt_in\(\s*"([A-Za-z0-9]+)"\)/g)) reads.add(`Fixes.${m[1]}`);
  for (const k of fixOn) reads.add(`Fixes.${k}`);
  // key bindings read by key name alone, with a default key: VkFromIni (SummonerFix), vk_from_ini (the host)
  const keyOnly = new Set([...all(sfFiles, /\bVkFromIni\(\s*"([A-Za-z0-9]+)"/g), ...all(decompFiles, /\bvk_from_ini\(\s*"([A-Za-z0-9]+)"/g)].map((x) => x.m[1]));
  const readKeys = new Set([...[...reads].map((s) => s.split(".")[1]), ...keyOnly]);
  // pairs the config app writes: WritePrivateProfileStringA("S", "K", ...) and its {"S", "K", ...} option tables
  const cfgWrites = new Set([...cfg.matchAll(/WritePrivateProfileStringA\(\s*"([A-Za-z]+)"\s*,\s*"([A-Za-z0-9]+)"/g), ...cfg.matchAll(/\{\s*"([A-Z][A-Za-z]+)"\s*,\s*"([A-Za-z0-9]+)"\s*,\s*"/g)].map((m) => `${m[1]}.${m[2]}`));
  for (const k of cfgFixes) cfgWrites.add(`Fixes.${k}`);
  // option tables: only sections summonerfix.ini has (k_renderers' {"OpenGL", "Vulkan", ...} is not one)
  const iniSections = new Set([...iniDoc.matchAll(/^\s*\[([A-Za-z]+)\]/gm)].map((m) => m[1]));
  for (const p of [...cfgWrites]) if (!iniSections.has(p.split(".")[0])) cfgWrites.delete(p);
  // keys written in summonerfix.ini, active or commented out (";Key=")
  const iniKeys = new Set();
  let sec = "";
  for (const l of iniDoc.split(/\r?\n/)) {
    const s = /^\s*\[([A-Za-z]+)\]/.exec(l); if (s) { sec = s[1]; continue; }
    const k = /^\s*;?\s*([A-Za-z][A-Za-z0-9]*)\s*=/.exec(l); if (k && sec) iniKeys.add(`${sec}.${k[1]}`);
  }
  const markers = all(decompFiles, /\/\/\s*(FIXED_BUILD|EXPERIMENTAL_BUILD):\s+(\S+)\s+([A-Za-z0-9]+)/g).map(({ f, m }) => ({ f, kind: m[1], id: m[2], key: m[3] }));
  const markerKeys = new Set(markers.map((m) => m.key));

  // a. the config app's fix list and SummonerFix's FixOn keys are the same set
  for (const k of fixOn) if (!cfgFixes.has(k)) err(`[Fixes] ${k}: SummonerFix reads it (FixOn), the config app's fix list does not have it`);
  for (const k of cfgFixes) if (!fixOn.has(k)) err(`[Fixes] ${k}: in the config app's fix list, SummonerFix never reads it (FixOn)`);
  // b. every marker key is a setting someone reads (else it is a typo, or SummonerFix cannot stand down for it)
  //    a key no code reads is a fix with no switch (always on in the build: a note), or a slip when it looks like a
  //    key that is read ("reverb" for LevelReverb): an error
  const sourceOnly = [];
  for (const m of markers) {
    if (readKeys.has(m.key) || inSource.has(m.key)) continue;
    const lk = m.key.toLowerCase();
    const near = [...readKeys].find((k) => k.toLowerCase() === lk || (lk.length >= 4 && k.toLowerCase().includes(lk)) || (k.length >= 4 && lk.includes(k.toLowerCase())));
    if (near) err(`${m.f}: marker ${m.kind}: ${m.id} ${m.key}: no setting has that name; did it mean ${near}?`);
    else sourceOnly.push(m.key);
  }
  // c. every key SummonerFix stands down for has a marker that writes it into .fixes
  for (const k of new Set([...fixOn, ...inSource])) if (inSource.has(k) && !markerKeys.has(k)) err(`RebuiltFixInSource("${k}"): no // FIXED_BUILD or EXPERIMENTAL_BUILD marker has the key ${k}`);
  const standDown = [...fixOn].filter((k) => !markerKeys.has(k));
  // d. the config app writes nothing nobody reads
  for (const p of cfgWrites) if (!reads.has(p)) err(`config app writes [${p.replace(".", "] ")}, which neither SummonerFix nor the game reads`);
  // e. settings the player may set are in summonerfix.ini (Debug keys are testing switches: not required)
  for (const k of keyOnly) if (![...iniKeys].some((p) => p.endsWith("." + k))) warn(`key binding ${k} is read but not in src/summonerfix/summonerfix.ini`);
  for (const p of [...reads].sort()) if (!p.startsWith("Debug.") && !p.startsWith("Hybrid.") && !iniKeys.has(p)) warn(`[${p.replace(".", "] ")} is read but not in src/summonerfix/summonerfix.ini`);
  console.log(`  ${markers.length} markers (${markerKeys.size} keys), ${fixOn.size} [Fixes] keys (${standDown.length} patched by SummonerFix only, no source fix),`);
  if (sourceOnly.length) console.log(`  fixes with no switch (always on in the build): ${sourceOnly.join(", ")}`);
  console.log(`  ${inSource.size} other stand-down keys, ${cfgWrites.size} keys the config app writes, ${reads.size} keys read, ${iniKeys.size} in summonerfix.ini`);
}

// 5. sweep
section("5. byte-match sweep");
{
  let run = args.includes("--sweep");
  if (!run && !args.includes("--no-sweep") && !quick) {
    const paths = ["src/decomp/game", "src/decomp/vsdk", "src/decomp/include", "src/decomp/vc6-flags.txt", "src/decomp/vc6-matches.txt"];
    const a = spawnSync("git", ["diff", "--name-only", "origin/main", "--", ...paths], { cwd: repo, encoding: "utf8" }).stdout || "";
    const b = spawnSync("git", ["status", "--porcelain", "--", ...paths], { cwd: repo, encoding: "utf8" }).stdout || "";
    run = !!(a.trim() || b.trim());
    if (!run) console.log("  skipped: no game, engine or header change against origin/main (--sweep forces it)");
  } else if (!run) console.log("  skipped (--no-sweep / --quick)");
  if (run) {
    if (!process.env.VC6_DIR || !existsSync(join(process.env.VC6_DIR, "BIN", "CL.EXE"))) err("the sweep is needed but VC6_DIR is not set");
    else {
      console.log("  running (several minutes)...");
      const r = spawnSync(process.execPath, [join(repo, "tools", "vc6-sweep.mjs"), "--check", join(repo, "src", "decomp", "vc6-matches.txt")],
        { cwd: repo, encoding: "utf8", env: { ...process.env, VC6_STRICT: "1" }, maxBuffer: 1 << 28 });
      const tail = (r.stdout || "").trim().split(/\r?\n/).slice(-2);
      for (const l of tail) console.log(`  ${l}`);
      if (!tail.some((l) => /\b0 lost\b/.test(l))) err("the sweep lost matches (or did not finish)");
    }
  }
}

console.log(`\n${errors} error(s), ${warnings} warning(s)`);
process.exit(errors ? 1 : 0);
