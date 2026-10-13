// Assign game functions to source files using strings.tbl.
// strings.tbl groups numbered strings under "// <file>.cpp" comments; code reads them as localize_strings[id]
// (absolute address 0x25cd4dc + 4*id). Every such reference places its function in that file.
// usage: node tools/strings-tu.mjs [path/to/strings.tbl] [--md] [--symbols]
//   (default path: extracted/tables/strings.tbl; extract it with: node tools/vpp.mjs extract Summoner/tables.vpp strings.tbl)
// --md prints a Markdown table of address ranges per file; --symbols prints unnamed functions as I-confidence stubs.
import { readFileSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { loadSymbols, loadFunctions } from "./symbols.mjs";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const md = args.includes("--md"), asSymbols = args.includes("--symbols");
const tblPath = args.find((a) => !a.startsWith("--")) ?? join(repoRoot, "extracted", "tables", "strings.tbl");
const STRINGS = 0x25cd4dc, COUNT = 1000, CRT = 0x56c740, LOCALIZE = [0x503fb0, 0x505560];

// 1. id -> source file, from the #english section
const idFile = new Map();
let file = null;
for (const raw of readFileSync(tblPath, "latin1").split(/\r?\n/)) {
  const line = raw.trim();
  if (line.startsWith("#") && line.toLowerCase() !== "#english") break; // next language
  const c = /^\/\/\s*([A-Za-z0-9_\\/.]+\.(?:cpp|h))\s*$/.exec(line);
  if (c) { file = c[1]; continue; }
  const s = /^(\d+)\s*,/.exec(line);
  if (s && file) idFile.set(+s[1], file);
}

// 2. references to localize_strings[id] in .text (as 32-bit operands)
const exe = readFileSync(join(repoRoot, "Summoner", "Sum.exe"));
const funcs = loadFunctions().map((f) => f.addr).filter((a) => a < CRT).sort((a, b) => a - b);
const containing = (va) => {
  let lo = 0, hi = funcs.length - 1, best = -1;
  while (lo <= hi) { const m = (lo + hi) >> 1; if (funcs[m] <= va) { best = funcs[m]; lo = m + 1; } else hi = m - 1; }
  return best;
};
const byFunc = new Map();
for (let o = 0x1000; o < CRT - 0x400000; o++) {
  const v = exe.readUInt32LE(o);
  if (v < STRINGS || v >= STRINGS + COUNT * 4 || (v - STRINGS) % 4) continue;
  const id = (v - STRINGS) / 4, f = idFile.get(id);
  if (!f || id === 0) continue; // id 0 is also the array base (loops, memset)
  const fn = containing(0x400000 + o);
  if (fn < 0 || (fn >= LOCALIZE[0] && fn < LOCALIZE[1])) continue; // the localization module itself

  if (!byFunc.has(fn)) byFunc.set(fn, new Map());
  const m = byFunc.get(fn); m.set(f, (m.get(f) ?? 0) + 1);
}

const { symbols } = loadSymbols();
const names = new Map(symbols.map((s) => [s.addr, s.name]));
const hex = (a) => "0x" + a.toString(16);
const rows = [...byFunc.entries()].sort((a, b) => a[0] - b[0]).map(([fn, m]) => {
  const files = [...m.entries()].sort((a, b) => b[1] - a[1]);
  return { fn, file: files[0][0], files };
});

if (md) {
  // collapse consecutive functions with the same file into ranges
  const ranges = [];
  for (const r of rows) {
    const last = ranges.at(-1);
    if (last && last.file === r.file) { last.hi = r.fn; last.n++; } else ranges.push({ file: r.file, lo: r.fn, hi: r.fn, n: 1 });
  }
  console.log("| Functions | Count | File |\n|---|---|---|");
  for (const g of ranges) console.log(`| \`${hex(g.lo)}\`${g.hi !== g.lo ? `-\`${hex(g.hi)}\`` : ""} | ${g.n} | \`${g.file}\` |`);
} else if (asSymbols) {
  for (const r of rows) if (!names.has(r.fn)) console.log(`${hex(r.fn)} ${r.file.replace(/\.cpp$/, "")}_${r.fn.toString(16)} func ${r.file} I ; uses strings.tbl ids of ${r.file}`);
} else {
  for (const r of rows) console.log(`${hex(r.fn)}  ${(names.get(r.fn) ?? "").padEnd(28)} ${r.files.map(([f, n]) => `${f}(${n})`).join(" ")}`);
  console.log(`${rows.length} functions placed, ${idFile.size} string ids mapped`);
}
