// Progress of the source rewrite in src/decomp/: which functions of each original file have source.
// usage: node tools/decomp-progress.mjs [--md]
// A function counts as written when its address (as in symbols/sum.symbols) appears in a "// 0xADDR" comment line
// of a .cpp under src/decomp/. The confidence of a written function is the "Confidence: exact|likely|unsure" on the
// same or the next comment line. Totals per original file come from the module column of symbols/sum.symbols.
import { readFileSync, readdirSync, statSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const md = process.argv.includes("--md");
const funcs = new Map(); // addr -> module
for (const l of readFileSync(join(root, "symbols", "sum.symbols"), "utf8").split(/\r?\n/)) {
  const m = /^(0x[0-9a-f]+)\s+\S+\s+func\s+(\S+)/.exec(l);
  if (m) funcs.set(parseInt(m[1], 16), m[2]);
}
const written = new Map(); // addr -> confidence
const walk = (d) => {
  for (const n of readdirSync(d)) {
    const p = join(d, n);
    if (statSync(p).isDirectory()) walk(p);
    else if (p.endsWith(".cpp")) {
      const L = readFileSync(p, "utf8").split(/\r?\n/);
      L.forEach((l, i) => {
        const m = /^\s*\/\/\s*(0x[0-9a-f]{6})\b/.exec(l);
        if (!m) return;
        const a = parseInt(m[1], 16);
        if (!funcs.has(a)) return;
        const c = /Confidence:\s*(exact|likely|unsure)/i.exec(l + " " + (L[i + 1] ?? ""));
        if (!written.has(a) || c) written.set(a, c ? c[1].toLowerCase() : written.get(a) ?? "-");
      });
    }
  }
};
walk(join(root, "src", "decomp"));
// The last full strict VC6 sweep (src/decomp/vc6-matches.txt, tools/vc6-sweep.mjs): which written functions the
// compiler check covers and which of them give the original bytes and targets.
const swept = new Map();
try {
  for (const l of readFileSync(join(root, "src", "decomp", "vc6-matches.txt"), "utf8").split(/\r?\n/)) {
    const [, a, s] = l.split(" ");
    if (a && a.startsWith("0x")) swept.set(parseInt(a, 16), s);
  }
} catch {}
const per = new Map();
for (const [a, mod] of funcs) {
  const r = per.get(mod) ?? per.set(mod, { total: 0, done: 0, exact: 0, likely: 0, unsure: 0, compared: 0, verified: 0 }).get(mod);
  r.total++;
  if (written.has(a)) {
    r.done++; const c = written.get(a); if (r[c] !== undefined) r[c]++;
    const s = swept.get(a);
    if (s && s !== "not_found") r.compared++;
    if (s === "match") r.verified++;
  }
}
const rows = [...per].filter(([, r]) => r.done > 0).sort((x, y) => y[1].done - x[1].done);
const sum = { total: 0, done: 0, exact: 0, likely: 0, unsure: 0, compared: 0, verified: 0 };
for (const [, r] of rows) for (const k in sum) sum[k] += r[k];
const all = funcs.size;
if (md) {
  console.log("| Original file | Written | Of | Compared by VC6 | Byte match | Exact | Likely | Unsure |\n|---|---|---|---|---|---|---|---|");
  for (const [m, r] of rows) console.log(`| \`${m}\` | ${r.done} | ${r.total} | ${r.compared} | ${r.verified} | ${r.exact} | ${r.likely} | ${r.unsure} |`);
  console.log(`| **total** | **${sum.done}** | ${sum.total} | ${sum.compared} | ${sum.verified} | ${sum.exact} | ${sum.likely} | ${sum.unsure} |`);
} else {
  for (const [m, r] of rows) console.log(`${m.padEnd(36)} ${String(r.done).padStart(4)} / ${String(r.total).padEnd(4)} compared ${r.compared} match ${r.verified}  exact ${r.exact} likely ${r.likely} unsure ${r.unsure}`);
  console.log(`total: ${sum.done} functions written (${(100 * sum.done / all).toFixed(1)}% of ${all}); ${sum.compared} compared by VC6, ${sum.verified} byte matches; ` +
    `markers: exact ${sum.exact}, likely ${sum.likely}, unsure ${sum.unsure} (on functions VC6 does not compare, "exact" is a reading of the code, not a compiler check)`);
}
