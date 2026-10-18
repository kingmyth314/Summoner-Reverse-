// Which decompiled functions a hybrid-game run executed. Reads one or more coverage files written by the agent
// command `hybridcov NAME` ([Hybrid] Coverage=1; "0xADDRESS calls" per replaced function) and joins them with the
// strict sweep list (src/decomp/vc6-matches.txt): for the functions that do not match byte for byte, the ones a
// run executed (tested in the game when the run's checks passed) and the ones none did (scenarios still to find).
// usage: node tools/hybrid-coverage.mjs <coverage.txt>... [--list ran|never] [--names]
import { readFileSync } from "node:fs";
import { join, dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { loadSymbols } from "./symbols.mjs";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const listAt = args.indexOf("--list");
const list = listAt >= 0 ? args[listAt + 1] : null;
const files = args.filter((a, i) => !a.startsWith("--") && args[i - 1] !== "--list");
if (!files.length) {
  console.error("usage: node tools/hybrid-coverage.mjs <coverage.txt>... [--list ran|never]");
  process.exit(2);
}
const calls = new Map(); // address -> calls over all files
for (const f of files)
  for (const l of readFileSync(f, "latin1").split(/\r?\n/)) {
    const m = /^(0x[0-9a-f]+)\s+(\d+)/.exec(l);
    if (m) calls.set(parseInt(m[1], 16), (calls.get(parseInt(m[1], 16)) ?? 0) + +m[2]);
  }
const status = new Map(); // address -> match | differs | not_found
for (const l of readFileSync(join(repoRoot, "src", "decomp", "vc6-matches.txt"), "latin1").split(/\r?\n/)) {
  const m = /^\S+\s+(0x[0-9a-f]+)\s+(\S+)/.exec(l);
  if (m) status.set(parseInt(m[1], 16), m[2]);
}
const names = new Map(loadSymbols().symbols.map((s) => [s.addr, s.name]));
const rows = { match: [0, 0], differs: [0, 0], other: [0, 0] };
const ran = [], never = [];
for (const [a, n] of calls) {
  const k = status.get(a) === "match" ? "match" : status.get(a) === "differs" ? "differs" : "other";
  rows[k][n ? 0 : 1]++;
  if (k === "differs") (n ? ran : never).push(a);
}
console.log(`${calls.size} replaced functions in ${files.length} run(s)`);
console.log("            ran   never");
for (const k of ["match", "differs", "other"]) console.log(`${k.padEnd(10)} ${String(rows[k][0]).padStart(5)} ${String(rows[k][1]).padStart(7)}`);
const show = list === "ran" ? ran : list === "never" ? never : null;
if (show) for (const a of show.sort((x, y) => x - y)) console.log(`0x${a.toString(16)} ${names.get(a) ?? ""}${list === "ran" ? " " + calls.get(a) : ""}`);
