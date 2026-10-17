// Functions counted as written (an address of a sum.symbols function in a `// 0xADDR` comment, as
// decomp-progress.mjs counts) that a VC6 sweep never compared, grouped by why vc6-compare does not pick them up.
//
//   node tools/decomp-uncompared.mjs <sweep list> [--list]
//
// Reasons: "inside comment" (the address is in a comment block whose first line names another address, e.g. a
// note that mentions it); "note, blank line" (the comment block ends in a blank line, so no definition follows);
// "directive" (a #if / #define follows); "not a definition" (what follows has no `(...) {`, e.g. a macro that
// defines the function, a declaration or data); "compared under another marker" is not possible: one address,
// one line.
import { readFileSync, readdirSync, statSync } from "node:fs";
import { join, resolve, dirname, relative } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const [list] = process.argv.slice(2);
const showList = process.argv.includes("--list");
const funcs = new Set();
for (const l of readFileSync(join(root, "symbols", "sum.symbols"), "utf8").split(/\r?\n/)) {
  const m = /^(0x[0-9a-f]+)\s+\S+\s+func\s/.exec(l);
  if (m) funcs.add(parseInt(m[1], 16));
}
const compared = new Set(readFileSync(list, "utf8").split(/\r?\n/).map((l) => l.split(" ")[1]).filter((a) => a && a.startsWith("0x")).map((a) => parseInt(a, 16)));
const out = new Map();
const walk = (d) => {
  for (const n of readdirSync(d)) {
    const p = join(d, n);
    if (statSync(p).isDirectory()) { walk(p); continue; }
    if (!p.endsWith(".cpp")) continue;
    const L = readFileSync(p, "latin1").split(/\r?\n/);
    L.forEach((l, i) => {
      const m = /^\s*\/\/\s*(0x[0-9a-f]{6})\b/.exec(l);
      if (!m) return;
      const a = parseInt(m[1], 16);
      if (!funcs.has(a) || compared.has(a) || out.has(a)) return;
      let k = i - 1;
      while (k >= 0 && /^\s*\/\//.test(L[k]) && !/^\s*\/\/\s*0x[0-9a-f]{6}\b/.test(L[k])) k--;
      let reason;
      if (k >= 0 && /^\s*\/\/\s*0x[0-9a-f]{6}\b/.test(L[k])) reason = "inside comment";
      else {
        let j = i + 1;
        while (j < L.length && /^\s*\/\//.test(L[j])) j++;
        if (j >= L.length || /^\s*$/.test(L[j])) reason = "note, blank line";
        else if (/^\s*#/.test(L[j])) reason = "directive";
        else reason = "not a definition: " + L[j].trim().slice(0, 60);
      }
      out.set(a, { file: relative(join(root, "src", "decomp"), p).replace(/\\/g, "/"), line: i + 1, reason });
    });
  }
};
walk(join(root, "src", "decomp"));
const groups = new Map();
for (const [, v] of out) { const g = v.reason.startsWith("not a definition") ? "not a definition" : v.reason; groups.set(g, (groups.get(g) || 0) + 1); }
console.log(`${out.size} written functions never compared`);
for (const [g, n] of [...groups].sort((a, b) => b[1] - a[1])) console.log(`  ${String(n).padStart(5)}  ${g}`);
if (showList) for (const [a, v] of [...out].sort((x, y) => x[0] - y[0])) console.log(`0x${a.toString(16)} ${v.file}:${v.line} ${v.reason}`);
