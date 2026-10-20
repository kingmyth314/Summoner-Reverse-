// Tries source variants of one function and reports, for each, whether the function then matches Sum.exe and how
// many functions of the file match. Each variant is applied alone to the original text; the file is always restored.
//
//   node tools/vc6-try.mjs [--root src/decomp] <file under root> <address> <variants.json> [--side <name>] [--lines N]
//
// variants.json: [{"name": "...", "from": "exact text", "to": "replacement"}, ...]
// --side <mangled-name substring>: for a variant that still differs, also print the first differing lines
// (tools/vc6-side.mjs), N lines (default 12).
// Uses VC6_DIR (the SP5 VC98 folder) and the tree's include/ and vc6-flags.txt. Run from PowerShell or cmd.
import { readFileSync, writeFileSync, rmSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { tmpdir } from "node:os";
import { resolve, dirname, join, basename } from "node:path";
import { fileURLToPath } from "node:url";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const take = (name, def) => { const i = args.indexOf(name); if (i < 0) return def; const v = args[i + 1]; args.splice(i, 2); return v; };
const root = resolve(take("--root", join(repo, "src", "decomp")));
const side = take("--side");
const nlines = +take("--lines", 12);
const [rel, addr, vf] = args;
if (!vf) { console.error("usage: node tools/vc6-try.mjs [--root DIR] <file> <address> <variants.json> [--side NAME] [--lines N]"); process.exit(2); }
const file = join(root, rel);
const orig = readFileSync(file, "utf8");
const env = { ...process.env, DECOMP_INCLUDE: join(root, "include"), VC6_FLAGS_FILE: join(root, "vc6-flags.txt") };

function run(label) {
  let out;
  try { out = execFileSync("node", [join(repo, "tools", "vc6-compare.mjs"), file, "--json", ...(side ? ["--keep"] : [])], { cwd: repo, env, maxBuffer: 1 << 26 }).toString(); }
  catch (e) { out = (e.stdout || "").toString(); }
  let j; try { j = JSON.parse(out); } catch { console.log(label.padEnd(24), "ERROR", out.slice(0, 300)); return; }
  const r = Array.isArray(j) ? j[0] : j;
  if (r.error) { console.log(label.padEnd(24), "ERROR", String(r.error).slice(0, 300)); return; }
  const f = r.functions.find((x) => parseInt(x.addr) === parseInt(addr));
  const n = r.functions.filter((x) => x.status === "match").length;
  console.log(label.padEnd(24), f ? f.status : "not found", "| file matches", n);
  if (side && r.tmp && f && f.status !== "match") {
    const cod = join(r.tmp, rel.replace(/^.*[\\/]/, "").replace(/\.cpp$/, ".cod"));
    const s = execFileSync("node", [join(repo, "tools", "vc6-side.mjs"), cod, side, addr]).toString().split("\n");
    const first = s.findIndex((l) => l.startsWith("*"));
    console.log(s.slice(Math.max(0, first - 2), first + nlines).join("\n"));
  }
  // --side keeps vc6-compare's folder for the listing; remove it once read (only a vc6cmp- folder directly in the
  // temp folder: a run per variant otherwise leaves thousands behind).
  if (r.tmp && /^vc6cmp-[A-Za-z0-9]{6}$/.test(basename(r.tmp)) && resolve(dirname(r.tmp)) === resolve(tmpdir()))
    rmSync(r.tmp, { recursive: true, force: true });
}
try {
  run("base");
  for (const v of JSON.parse(readFileSync(vf, "utf8").replace(/^﻿/, ""))) {
    if (!orig.includes(v.from)) { console.log(v.name.padEnd(24), "FROM TEXT NOT FOUND"); continue; }
    writeFileSync(file, orig.replace(v.from, v.to));
    run(v.name);
  }
} finally { writeFileSync(file, orig); }
