// Ranks the functions of a src/decomp tree that still differ from Sum.exe by how close they are to a match
// (tools/vc6-compare.mjs --score: instructions outside the longest common subsequence, branch distances masked).
//
//   node tools/vc6-nearmiss.mjs [--root src/decomp] [--only <regex>] [--jobs 4] [--top 60] [--max N] [--out list.txt]
//
// Prints "<distance> <file> <address> <name> (<ours>/<orig> instructions)" smallest first; a distance of 0 means
// only branch distances differ (usually a block placed elsewhere). --max keeps distances up to N, --out writes the
// full ranked list. Compiles like vc6-sweep: VC6_DIR must name the SP5 VC98 folder. Run from PowerShell or cmd.
import { readdirSync, writeFileSync, existsSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { execFile } from "node:child_process";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const opt = (name, def) => { const i = args.indexOf(name); return i >= 0 ? args[i + 1] : def; };
const root = resolve(opt("--root", join(repo, "src", "decomp")));
const only = opt("--only");
const jobs = +opt("--jobs", 4);
const top = +opt("--top", 60);
const max = opt("--max") !== undefined ? +opt("--max") : Infinity;
const out = opt("--out");
if (!process.env.VC6_DIR) { console.error("set VC6_DIR to the SP5 VC98 folder (docs/setup.md)"); process.exit(2); }

const files = [];
for (const d of ["game", "vsdk"]) for (const f of readdirSync(join(root, d))) if (f.endsWith(".cpp")) files.push(`${d}/${f}`);
const todo = files.filter((f) => !only || new RegExp(only).test(f)).sort();
const env = { ...process.env, DECOMP_INCLUDE: join(root, "include") };
if (existsSync(join(root, "vc6-flags.txt"))) env.VC6_FLAGS_FILE = join(root, "vc6-flags.txt");

const rows = [], errors = [];
let next = 0;
function worker() {
  if (next >= todo.length) return Promise.resolve();
  const f = todo[next++];
  return new Promise((done) => {
    execFile("node", [join(repo, "tools", "vc6-compare.mjs"), join(root, f), "--json", "--score"], { cwd: repo, env, maxBuffer: 1 << 28 }, (err, stdout) => {
      try {
        for (const res of JSON.parse(stdout)) {
          if (res.error) { errors.push(`${f} ERROR ${res.error.split("\n")[0]}`); continue; }
          for (const fn of res.functions) if (fn.status === "differs")
            rows.push({ d: fn.distance ?? Infinity, f, addr: fn.addr, name: fn.name, ins: `${fn.instructions}/${fn.origInstructions ?? "?"}` });
        }
      } catch { errors.push(`${f} ERROR ${(err && err.message.split("\n")[0]) || "no output"}`); }
      done();
    });
  }).then(worker);
}
await Promise.all(Array.from({ length: jobs }, worker));

rows.sort((a, b) => a.d - b.d || a.f.localeCompare(b.f) || a.addr.localeCompare(b.addr));
const line = (r) => `${String(r.d === Infinity ? "large" : r.d).padStart(5)} ${r.f} ${r.addr} ${r.name} (${r.ins} instructions)`;
const kept = rows.filter((r) => r.d <= max);
if (out) writeFileSync(out, kept.map(line).join("\n") + "\n");
for (const r of kept.slice(0, top)) console.log(line(r));
for (const e of errors) console.log(e);
const band = (lo, hi) => rows.filter((r) => r.d >= lo && r.d <= hi).length;
console.log(`summary: ${rows.length} differ in ${todo.length} units; distance 0: ${band(0, 0)}, 1-4: ${band(1, 4)}, 5-10: ${band(5, 10)}, 11-30: ${band(11, 30)}, over 30: ${rows.length - band(0, 30)}`);
