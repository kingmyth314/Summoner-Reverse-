// Three-way merges a worker's copy of src/decomp into a stage (another copy, or src/decomp itself).
//
//   node tools/decomp-merge.mjs <worker tree> <base tree> <stage tree>
//
// <base tree> is the copy the worker started from (keep one snapshot per worker). For every file the worker
// changed or added relative to the base: taken as is when the stage still has the base version, merged with
// `git merge-file` when both changed it, added when new. include/DECISIONS.md conflicts are resolved by keeping
// both sides (stage first): workers only append numbered blocks. Any other conflict is left with git's markers and
// listed; resolve it by hand. Scratch files (*.tryorig, *.bak, *.orig, *.ours) are skipped. Exit 1 on conflicts.
import { readdirSync, statSync, readFileSync, writeFileSync, copyFileSync, mkdirSync, existsSync } from "node:fs";
import { join, dirname, relative } from "node:path";
import { execFileSync } from "node:child_process";

const [worker, base, stage] = process.argv.slice(2);
if (!stage) { console.error("usage: node tools/decomp-merge.mjs <worker tree> <base tree> <stage tree>"); process.exit(2); }
const walk = (d, out = []) => { for (const n of readdirSync(d)) { const p = join(d, n); if (statSync(p).isDirectory()) walk(p, out); else out.push(p); } return out; };
const same = (a, b) => readFileSync(a).equals(readFileSync(b));
const skip = /\.(tryorig|bak|orig|ours)$/;
const conflicts = [];
const count = { took: 0, merged: 0, added: 0 };

for (const wf of walk(worker)) {
  const rel = relative(worker, wf);
  if (skip.test(rel)) continue;
  const bf = join(base, rel), sf = join(stage, rel);
  if (existsSync(bf)) {
    if (same(wf, bf)) continue;
    if (!existsSync(sf)) { console.log(`gone from stage, not merged: ${rel}`); continue; }
    if (same(sf, bf)) { copyFileSync(wf, sf); count.took++; continue; }
    let rc = 0;
    try { execFileSync("git", ["merge-file", "-L", "stage", "-L", "base", "-L", "worker", sf, bf, wf]); }
    catch (e) { rc = e.status; }
    if (rc === 0) { count.merged++; console.log(`merged ${rel}`); continue; }
    if (/(^|[\\/])DECISIONS\.md$/.test(rel)) {
      const d = readFileSync(sf, "utf8").replace(/^<<<<<<< .*\r?\n([\s\S]*?)^=======\r?\n([\s\S]*?)^>>>>>>> .*\r?\n/gm,
        (m, a, b) => a.replace(/\s*$/, "\n\n") + b);
      writeFileSync(sf, d);
      count.merged++; console.log(`merged ${rel} (appended blocks kept from both sides)`);
      continue;
    }
    conflicts.push(rel); console.log(`CONFLICT ${rel}`);
  } else if (existsSync(sf)) {
    if (!same(wf, sf)) { conflicts.push(rel); console.log(`CONFLICT (new on both sides) ${rel}`); }
  } else {
    mkdirSync(dirname(sf), { recursive: true }); copyFileSync(wf, sf); count.added++; console.log(`new ${rel}`);
  }
}
console.log(`took ${count.took}, merged ${count.merged}, new ${count.added}, conflicts ${conflicts.length}`);
if (conflicts.length) process.exit(1);
