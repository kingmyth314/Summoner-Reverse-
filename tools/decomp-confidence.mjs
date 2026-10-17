// Sets the `Confidence:` of every compared function from a VC6 sweep list (src/decomp/vc6-matches.txt):
// a function that matches is `exact`; one that differs and says `exact` becomes `likely` (one that says likely or
// unsure keeps it). Functions the sweep did not compare are not touched.
//
//   node tools/decomp-confidence.mjs [list=src/decomp/vc6-matches.txt] [--apply]
//
// Without --apply it only reports what would change. The word after "Confidence:" is replaced on the marker line
// or the line after it (where decomp-progress.mjs reads it); the explanation that follows is kept.
import { readFileSync, writeFileSync, readdirSync, statSync } from "node:fs";
import { join, resolve, dirname, relative } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const apply = args.includes("--apply");
const list = args.find((a) => !a.startsWith("--")) || join(root, "src", "decomp", "vc6-matches.txt");
const status = new Map();
for (const l of readFileSync(list, "utf8").split(/\r?\n/)) {
  const [, a, s] = l.split(" ");
  if (a && a.startsWith("0x")) status.set(parseInt(a, 16), s);
}
const changes = { raised: 0, lowered: 0, files: 0 };
const walk = (d) => {
  for (const n of readdirSync(d)) {
    const p = join(d, n);
    if (statSync(p).isDirectory()) { walk(p); continue; }
    if (!p.endsWith(".cpp")) continue;
    const text = readFileSync(p, "latin1");
    const nl = text.includes("\r\n") ? "\r\n" : "\n";
    const L = text.split(/\r?\n/);
    let touched = false;
    for (let i = 0; i < L.length; i++) {
      const m = /^\s*\/\/\s*(0x[0-9a-f]{6})\b/.exec(L[i]);
      if (!m) continue;
      const st = status.get(parseInt(m[1], 16));
      if (!st || st === "not_found") continue;
      const k = /Confidence:\s*(exact|likely|unsure)/i.test(L[i]) ? i : /Confidence:\s*(exact|likely|unsure)/i.test(L[i + 1] ?? "") && /^\s*\/\//.test(L[i + 1]) ? i + 1 : -1;
      if (k < 0) continue;
      const cur = /Confidence:\s*(exact|likely|unsure)/i.exec(L[k])[1].toLowerCase();
      let want = cur;
      if (st === "match") want = "exact";
      else if (cur === "exact") want = "likely";
      if (want === cur) continue;
      if (want === "exact") changes.raised++; else changes.lowered++;
      L[k] = L[k].replace(/Confidence:\s*(exact|likely|unsure)/i, `Confidence: ${want}`);
      touched = true;
      if (!apply) console.log(`${relative(root, p)}:${k + 1} ${m[1]} ${cur} -> ${want}`);
    }
    if (touched) { changes.files++; if (apply) writeFileSync(p, L.join(nl), "latin1"); }
  }
};
walk(join(root, "src", "decomp"));
console.log(`${apply ? "changed" : "would change"}: ${changes.raised} raised to exact, ${changes.lowered} lowered to likely, in ${changes.files} files`);
