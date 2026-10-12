// Translation-unit map: which original source file each part of Sum.exe's .text came from.
//
// VC6 emits each .cpp file's functions contiguously. Assert/error strings embed __FILE__
// ("D:\projects\Summoner\pccode\...\x.cpp"), and code references them with `PUSH imm32`
// (68 xx xx xx xx) or `MOV r32, imm32` (B8+r). Scanning .text for those references gives, per source
// file, the code addresses that mention it. Their spans approximate the file's address range.
//
// usage: node tools/tu-map.mjs [--md docs/sum-exe/source-map.md]
import { readFileSync, writeFileSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const b = readFileSync(join(repoRoot, "Summoner", "Sum.exe"));
const pe = b.readUInt32LE(0x3c), nsec = b.readUInt16LE(pe + 6), opt = pe + 24, optSize = b.readUInt16LE(pe + 20);
const base = b.readUInt32LE(opt + 28);
const secs = [];
for (let i = 0; i < nsec; i++) {
  const s = opt + optSize + i * 40;
  secs.push({ name: b.toString("latin1", s, s + 8).replace(/\0.*$/, ""), va: base + b.readUInt32LE(s + 12),
              vsize: b.readUInt32LE(s + 8), raw: b.readUInt32LE(s + 20), rsize: b.readUInt32LE(s + 16) });
}
const off2va = (o) => { for (const s of secs) if (o >= s.raw && o < s.raw + s.rsize) return o - s.raw + s.va; return -1; };
const text = secs.find((s) => s.name === ".text");

// 1. Source-path strings (any section), keyed by VA.
const pathAt = new Map();
const re = /[A-Za-z]:\\projects\\Summoner\\pccode\\([A-Za-z0-9_\\.]+\.(?:cpp|c|h))/gi;
const str = b.toString("latin1");
let m;
while ((m = re.exec(str))) pathAt.set(off2va(m.index), m[1].toLowerCase());

// 2. References from .text: PUSH imm32 and MOV r32, imm32.
const refs = new Map(); // file -> [code VA]
for (let o = text.raw; o < text.raw + text.rsize - 5; o++) {
  const op = b[o];
  if (op !== 0x68 && (op < 0xb8 || op > 0xbf)) continue;
  const imm = b.readUInt32LE(o + 1);
  const file = pathAt.get(imm);
  if (!file) continue;
  const va = off2va(o);
  if (!refs.has(file)) refs.set(file, []);
  refs.get(file).push(va);
}

const rows = [...refs.entries()].map(([file, vas]) => ({ file, lo: Math.min(...vas), hi: Math.max(...vas), n: vas.length }))
  .sort((a, b) => a.lo - b.lo);
const hex = (v) => "0x" + v.toString(16);
console.log(`${pathAt.size} source-path strings; ${rows.length} files referenced from .text\n`);
console.log("first ref   last ref    refs  source file");
let prevHi = 0;
for (const r of rows) {
  const overlap = r.lo < prevHi ? "  (overlaps previous range)" : "";
  console.log(`${hex(r.lo).padEnd(11)} ${hex(r.hi).padEnd(11)} ${String(r.n).padStart(4)}  ${r.file}${overlap}`);
  prevHi = Math.max(prevHi, r.hi);
}
const unref = [...new Set(pathAt.values())].filter((f) => !refs.has(f));
if (unref.length) console.log(`\nstrings with no PUSH/MOV reference found: ${unref.join(", ")}`);

const mdIdx = process.argv.indexOf("--md");
if (mdIdx > 0) {
  const lines = ["| First reference | Last reference | References | Source file |", "|---|---|---|---|"];
  for (const r of rows) lines.push(`| \`${hex(r.lo)}\` | \`${hex(r.hi)}\` | ${r.n} | \`${r.file}\` |`);
  writeFileSync(join(repoRoot, process.argv[mdIdx + 1] + ".table.tmp"), lines.join("\n") + "\n");
  console.log(`\nwrote table to ${process.argv[mdIdx + 1]}.table.tmp`);
}
