// Runs tools/vc6-compare.mjs on every .cpp of a src/decomp tree and writes one line per marked function:
// "<file> <address> <match|differs|not_found>" (and "<file> ERROR <message>" for a unit that does not compile).
//
//   node tools/vc6-sweep.mjs [--root src/decomp] [--out list.txt] [--only <regex>] [--jobs 4] [--check list.txt]
//
// --root     the decomp tree (default src/decomp; a merge stage or an agent's copy also works). Its include/ is
//            passed as DECOMP_INCLUDE and its vc6-flags.txt as VC6_FLAGS_FILE.
// --out      where to write the list (default: print the summary only)
// --only     compile only the units whose path (game/x.cpp, vsdk/y.cpp) matches the regex
// --check    a saved list (src/decomp/vc6-matches.txt): report every address that matched there and does not now,
//            and exit 1 if there is one. With --only, only the listed units are checked.
// VC6_DIR must name the Service Pack 5 VC98 folder (docs/setup.md). Run from PowerShell or cmd.
import { readdirSync, readFileSync, writeFileSync, existsSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { execFile } from "node:child_process";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const opt = (name, def) => { const i = args.indexOf(name); return i >= 0 ? args[i + 1] : def; };
const root = resolve(opt("--root", join(repo, "src", "decomp")));
const out = opt("--out");
const only = opt("--only");
const jobs = +opt("--jobs", 4);
const check = opt("--check");
if (!process.env.VC6_DIR) { console.error("set VC6_DIR to the SP5 VC98 folder (docs/setup.md)"); process.exit(2); }

const files = [];
for (const d of ["game", "vsdk"]) for (const f of readdirSync(join(root, d))) if (f.endsWith(".cpp")) files.push(`${d}/${f}`);
const todo = files.filter((f) => !only || new RegExp(only).test(f)).sort();
const env = { ...process.env, DECOMP_INCLUDE: join(root, "include") };
if (existsSync(join(root, "vc6-flags.txt"))) env.VC6_FLAGS_FILE = join(root, "vc6-flags.txt");

const res = {};
let next = 0;
function worker() {
  if (next >= todo.length) return Promise.resolve();
  const f = todo[next++];
  return new Promise((done) => {
    execFile("node", [join(repo, "tools", "vc6-compare.mjs"), join(root, f)], { cwd: repo, env, maxBuffer: 1 << 28 }, (err, stdout, stderr) => {
      const lines = [];
      for (const l of (stdout || "").split(/\r?\n/)) {
        const m = /^\s+(0x[0-9a-f]+)\s+(match|target|differs|not found)/.exec(l);
        if (m) lines.push(`${f} ${m[1]} ${m[2].replace(" ", "_")}`);
      }
      if (!lines.length) lines.push(`${f} ERROR ${((stdout || "") + (stderr || "")).split(/\r?\n/).filter((x) => /error/i.test(x)).slice(0, 2).join(" | ")}`);
      res[f] = lines;
      done();
    });
  }).then(worker);
}
await Promise.all(Array.from({ length: jobs }, worker));

const all = todo.flatMap((f) => res[f]);
if (out) writeFileSync(out, all.join("\n") + "\n");
const count = (k) => all.filter((l) => l.endsWith(" " + k)).length;
const errors = all.filter((l) => / ERROR /.test(l));
console.log(`${todo.length} units: ${count("match")} match, ${count("target")} wrong target, ${count("differs")} differ, ${count("not_found")} not found, ${errors.length} errors`);
for (const e of errors) console.log("  " + e);

if (check) {
  const now = new Set(all.filter((l) => l.endsWith(" match")).map((l) => l.split(" ")[1]));
  const units = new Set(todo);
  const lost = readFileSync(check, "utf8").split(/\r?\n/).filter((l) => l.endsWith(" match"))
    .filter((l) => units.has(l.split(" ")[0])).filter((l) => !now.has(l.split(" ")[1]));
  const before = readFileSync(check, "utf8").split(/\r?\n/).filter((l) => l.endsWith(" match") && units.has(l.split(" ")[0])).length;
  console.log(`check against ${check}: ${before} matched there, ${now.size} now, ${lost.length} lost`);
  for (const l of lost) console.log("  lost " + l);
  if (lost.length || errors.length) process.exit(1);
}
