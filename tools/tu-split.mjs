// Propose translation-unit (source file) boundaries from data locality.
// usage: node tools/tu-split.mjs [--lo 0x401000] [--hi 0x56a000] [--window 60] [--gap 0x40] [--out file] [--score]
//
// MSVC 6 lays out each .cpp file's functions, its initialised data and its .bss globals as contiguous blocks,
// in the same link order. Inside one file the .bss globals are in declaration order, so the functions of a file
// use .bss addresses that interleave; two files use disjoint, increasing .bss ranges.
//  1. A global is "private" when every function that references it lies within --window functions of the others.
//     Entries of localize_strings (one big shared table) never count.
//  2. A cut between two adjacent functions is possible only if no private global is used on both sides,
//     and never right after an initialiser thunk (the thunk and its initialiser belong together).
//  3. Module tags that name a .cpp file force a cut between two differently tagged functions, at the
//     least-crossed gap.
//  4. Pieces are joined greedily: a piece joins the current unit unless a cut is forced, or the unit and the
//     piece both use .bss and the piece's .bss lies entirely above the unit's (a new file's block).
// --score prints how many units mix two .cpp tags and how many tags are split over several units.
// Needs build/sum.asm (node tools/asm.mjs builds it) and symbols/sum.symbols.
import { readFileSync, writeFileSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const arg = (k, d) => { const i = process.argv.indexOf(k); return i > 0 ? process.argv[i + 1] : d; };
const LO = parseInt(arg("--lo", "0x401000"), 16), HI = parseInt(arg("--hi", "0x56a000"), 16);
const WINDOW = +arg("--window", "60");
const GAP = parseInt(arg("--gap", "0x40"), 16); // a new file starts only this far above the unit's .bss
const out = arg("--out", null);
const score = process.argv.includes("--score");
const DATA_LO = 0x588000, BSS = 0x5b3000, DATA_HI = 0x2fc0000;
const SHARED = [[0x25cd4dc, 0x25cd4dc + 4 * 1000]]; // localize_strings

const fns = [];
for (const l of readFileSync(join(root, "symbols", "sum.symbols"), "utf8").split(/\r?\n/)) {
  const m = /^0x([0-9a-f]+)\s+(\S+)\s+func\s+(\S+)/.exec(l);
  if (m) { const a = parseInt(m[1], 16); if (a >= LO && a < HI) fns.push({ a, name: m[2], mod: m[3] }); }
}
fns.sort((x, y) => x.a - y.a);
const isThunk = (i) => /_thunk$/.test(fns[i]?.name ?? "");
const isFile = (m) => /\.cpp$/i.test(m);

// global address -> [first user, last user]; per-function list of globals
const users = new Map();
const fnData = fns.map(() => []);
let fi = -1;
for (const l of readFileSync(join(root, "build", "sum.asm"), "utf8").split("\n")) {
  const at = /^\s*([0-9a-f]+):/.exec(l);
  if (!at) continue;
  const a = parseInt(at[1], 16);
  if (a < LO || a >= HI) continue;
  while (fi + 1 < fns.length && fns[fi + 1].a <= a) fi++;
  if (fi < 0) continue;
  for (const m of l.slice(l.indexOf("\t")).matchAll(/\b0x([0-9a-f]{6,7})\b/g)) {
    const d = parseInt(m[1], 16);
    if (d < DATA_LO || d >= DATA_HI || SHARED.some(([s, e]) => d >= s && d < e)) continue;
    const u = users.get(d);
    if (!u) users.set(d, [fi, fi]); else { u[0] = Math.min(u[0], fi); u[1] = Math.max(u[1], fi); }
    fnData[fi].push(d);
  }
}
const isPrivate = (d) => { const u = users.get(d); return u[1] - u[0] <= WINDOW; };

const crossed = new Uint32Array(fns.length);
for (const [, [a, b]] of users) if (b - a <= WINDOW) for (let i = a; i < b; i++) crossed[i]++;

const forced = new Uint8Array(fns.length);
let lastTagged = -1;
for (let j = 0; j < fns.length; j++) {
  if (!isFile(fns[j].mod)) continue;
  if (lastTagged >= 0 && fns[lastTagged].mod !== fns[j].mod) {
    const cost = (i) => crossed[i] * 4 + (isThunk(i + 1) ? 0 : 1); // prefer the gap just before a thunk
    let best = -1;
    for (let i = lastTagged; i < j; i++) if (!isThunk(i) && (best < 0 || cost(i) <= cost(best))) best = i;
    if (best >= 0) forced[best] = 1;
  }
  lastTagged = j;
}

// pieces between possible cuts, each with its private .bss addresses
const pieces = [];
for (let i = 0, s = 0; i < fns.length; i++) {
  if ((!crossed[i] && !isThunk(i)) || forced[i] || i === fns.length - 1) {
    const bss = [];
    for (let j = s; j <= i; j++) for (const d of fnData[j]) if (d >= BSS && isPrivate(d)) bss.push(d);
    pieces.push({ s, e: i, bss });
    s = i + 1;
  }
}

// greedy join
const units = [];
for (const p of pieces) {
  const u = units[units.length - 1];
  const lo = p.bss.length ? Math.min(...p.bss) : null;
  const newFile = u && u.bss.length && p.bss.length && lo > u.hi + GAP;
  if (!u || forced[u.e] || newFile) {
    units.push({ s: p.s, e: p.e, bss: [...p.bss], hi: p.bss.length ? Math.max(...p.bss) : -1 });
  } else {
    u.e = p.e; u.bss.push(...p.bss); if (p.bss.length) u.hi = Math.max(u.hi, ...p.bss);
  }
}

const hex = (v) => "0x" + v.toString(16);
const lines = [];
let mixed = 0;
const tagUnits = new Map();
for (const [k, u] of units.entries()) {
  const mods = new Map();
  for (let j = u.s; j <= u.e; j++) if (fns[j].mod !== "-") mods.set(fns[j].mod, (mods.get(fns[j].mod) ?? 0) + 1);
  const files = [...mods.keys()].filter(isFile);
  if (files.length > 1) mixed++;
  for (const f of files) (tagUnits.get(f) ?? tagUnits.set(f, new Set()).get(f)).add(k);
  const tag = [...mods].sort((x, y) => y[1] - x[1]).map(([m, c]) => `${m}:${c}`).join(",") || "-";
  const b = u.bss.length ? `${hex(Math.min(...u.bss))}..${hex(Math.max(...u.bss))}` : "-";
  lines.push(`${hex(fns[u.s].a)}-${hex(fns[u.e].a)} n=${u.e - u.s + 1} bss=${b} ${tag} ${fns[u.s].name}..${fns[u.e].name}`);
}
const split = [...tagUnits].filter(([, s]) => s.size > 1);
const summary = `# ${units.length} units; ${mixed} mix two .cpp tags; ${split.length} of ${tagUnits.size} tagged files split over several units`;
if (score) {
  console.log(summary);
  for (const [f, s] of split) console.log(`#   split: ${f} x${s.size}`);
  // A split file usually means a stray tag: list tagged functions in a unit whose majority file tag is another
  // file (or that hold under a quarter of their file's tagged functions), for review.
  const total = new Map();
  for (const f of fns) if (isFile(f.mod)) total.set(f.mod, (total.get(f.mod) ?? 0) + 1);
  for (const u of units) {
    const c = new Map();
    for (let j = u.s; j <= u.e; j++) if (isFile(fns[j].mod)) c.set(fns[j].mod, (c.get(fns[j].mod) ?? 0) + 1);
    for (const [m, n] of c) {
      if (!split.some(([f]) => f === m) || n * 4 >= total.get(m)) continue;
      for (let j = u.s; j <= u.e; j++) if (fns[j].mod === m) console.log(`#   stray? ${hex(fns[j].a)} ${fns[j].name} ${m} (${n} of ${total.get(m)} in unit ${hex(fns[u.s].a)}-${hex(fns[u.e].a)})`);
    }
  }
}
const text = lines.join("\n") + "\n" + summary + "\n";
if (out) writeFileSync(out, text); else if (!score) process.stdout.write(text);
