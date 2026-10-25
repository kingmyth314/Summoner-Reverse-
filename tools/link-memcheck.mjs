// Run-time memory of the standalone rebuilt exe against the original's, symbol by symbol: two dumps written by
// SummonerFix's agent command `dumpdata OUT TABLE` at the same point of the game (for example the main menu), compared
// as tools/link-datacheck.mjs compares the files: each object up to the next symbol in either build; a dword that is
// a Sum.exe address counts as equal when the address table translates it to the rebuilt value, or when both point to
// the same string (read from the two exe files); values outside both images (heap, stack) count as equal.
//
//   node tools/link-memcheck.mjs <original dump> <rebuilt dump> [--also orig2,orig3] [--show 60] [--exe Sum-fixed.exe]
// With --also (more dumps of the original at the same point), a symbol counts only when the rebuilt value differs
// from every original dump and the original dumps agree in it; symbols that vary between original runs are counted
// apart. A byte that both processes still hold as their exe file has it is skipped (the static data is
// tools/link-datacheck.mjs's job; string literals sit in another order next to named data); --static keeps them.
import { readFileSync, existsSync, readdirSync } from "node:fs";
import { join, resolve, dirname, basename } from "node:path";
import { fileURLToPath } from "node:url";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const [dumpA, dumpB] = process.argv.slice(2);
const show = Number(process.argv[process.argv.indexOf("--show") + 1] || 60);
const alsoAt = process.argv.indexOf("--also");
const also = alsoAt > 0 ? process.argv[alsoAt + 1].split(",") : [];
// --exe: the rebuilt exe the dump came from, with its .addr next to it (default build/link/Sum-rebuilt.exe). Its .map
// is read from next to it, or else from the build folder that made it (build/*/<name>.map; test folders hold no map).
const exeAt = process.argv.indexOf("--exe");
const exePath = resolve(exeAt > 0 ? process.argv[exeAt + 1] : join(repo, "build", "link", "Sum-rebuilt.exe"));
const stem = exePath.replace(/\.exe$/i, "");
const mapPath = existsSync(stem + ".map") ? stem + ".map"
  : readdirSync(join(repo, "build")).map((d) => join(repo, "build", d, basename(stem) + ".map")).find((p) => existsSync(p));
if (!mapPath) { console.error(`no map for ${exePath}: build it with tools/link-build.mjs`); process.exit(2); }

function pe(path) {
  const b = readFileSync(path);
  const o = b.readUInt32LE(0x3c), n = b.readUInt16LE(o + 6), so = o + 24 + b.readUInt16LE(o + 20), base = b.readUInt32LE(o + 52);
  const secs = [];
  for (let i = 0; i < n; i++) {
    const s = so + i * 40;
    secs.push({ va: base + b.readUInt32LE(s + 12), vsize: b.readUInt32LE(s + 8), raw: b.readUInt32LE(s + 20), rawSize: b.readUInt32LE(s + 16) });
  }
  const read = (va, len) => {
    for (const s of secs) if (va >= s.va && va < s.va + s.vsize) {
      const off = va - s.va;
      if (off >= s.rawSize) return Buffer.alloc(len);
      if (off + len > s.rawSize) return Buffer.concat([b.subarray(s.raw + off, s.raw + s.rawSize), Buffer.alloc(off + len - s.rawSize)]);
      return b.subarray(s.raw + off, s.raw + off + len);
    }
    return null;
  };
  return { read, base, end: Math.max(...secs.map((s) => s.va + s.vsize)) };
}
const orig = pe(join(repo, "Summoner", "Sum.exe")), now = pe(exePath);
const rows = readFileSync(stem + ".addr", "latin1").trim().split(/\n/).map((l) => l.split(" ").map((x) => parseInt(x, 16)));
const rowOf = new Map(rows.map((r) => [r[0], r]));
const translate = (va) => {
  let lo = 0, hi = rows.length - 1, best = -1;
  while (lo <= hi) { const m = (lo + hi) >> 1; if (rows[m][0] <= va) { best = m; lo = m + 1; } else hi = m - 1; }
  if (best < 0 || va - rows[best][0] >= rows[best][1]) return null;
  return rows[best][2] + (va - rows[best][0]);
};
const names = new Map(), starts = [];
for (const l of readFileSync(join(repo, "symbols", "sum.symbols"), "latin1").split(/\r?\n/)) {
  const m = /^0x([0-9a-f]+)\s+(\S+)/i.exec(l);
  if (m) { names.set(parseInt(m[1], 16), m[2]); starts.push(parseInt(m[1], 16)); }
}
starts.sort((a, b) => a - b);
const mapStarts = [...new Set(readFileSync(mapPath, "latin1").split(/\r?\n/)
  .map((l) => /^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+\S+\s+([0-9a-f]{8,16})\s/.exec(l)).filter(Boolean).map((m) => parseInt(m[1], 16)))].sort((a, b) => a - b);
const after = (list, a) => { let lo = 0, hi = list.length; while (lo < hi) { const m = (lo + hi) >> 1; if (list[m] <= a) lo = m + 1; else hi = m; } return list[lo] ?? a + 4; };
const sameString = (p, q) => {
  const sa = orig.read(p, 256), sb = now.read(q, 256);
  if (!sa || !sb) return false;
  const za = sa.indexOf(0), zb = sb.indexOf(0);
  return za >= 0 && za === zb && sa.subarray(0, za).equals(sb.subarray(0, zb));
};
const read = (path) => {
  const buf = readFileSync(path), map = new Map();
  for (let o = 0; o + 8 <= buf.length;) {
    const va = buf.readUInt32LE(o), n = buf.readUInt32LE(o + 4);
    map.set(va, buf.subarray(o + 8, o + 8 + n));
    o += 8 + n;
  }
  return map;
};
const A = read(dumpA), B = read(dumpB), more = also.map(read);
const image = (v) => v >= 0x400000 && v < 0x3000000;
// Differing bytes of an original dump x against y over len bytes; y is the rebuilt dump (addresses translated) or,
// with raw, another original dump (compared as bytes).
const keepStatic = process.argv.includes("--static");
// Two image pointers with no table row (vtables, compiler tables): the same when the first dword they point at
// translates (a vtable's first slot).
const sameTable = (p, q) => {
  const a = orig.read(p, 4), b = now.read(q, 4);
  if (!a || !b) return false;
  const t = translate(a.readUInt32LE(0));
  return t !== null && t === b.readUInt32LE(0);
};
const diff = (x, y, len, raw, va, rva) => {
  const fx = !raw && !keepStatic ? orig.read(va, len) : null, fy = fx ? now.read(rva, len) : null;
  let first = -1, count = 0;
  for (let i = 0; i < len; ) {
    if (i + 4 <= len) {
      const p = x.readUInt32LE(i), q = y.readUInt32LE(i);
      if (p === q || (!raw && image(p) && (translate(p) === q || sameString(p, q) || (image(q) && sameTable(p, q)))) || (!image(p) && !image(q) && p > 0x10000 && q > 0x10000)) { i += 4; continue; }
    }
    if (x[i] !== y[i] && !(fx && fy && x[i] === fx[i] && y[i] === fy[i])) { if (first < 0) first = i; count++; }
    i++;
  }
  return { first, count };
};
let compared = 0, bad = 0, varies = 0;
const lines = [];
for (const [va, x] of A) {
  const y = B.get(va), r = rowOf.get(va);
  if (!y || !r) continue;
  compared++;
  const len = Math.min(x.length, y.length, after(starts, va) - va, after(mapStarts, r[2]) - r[2]);
  const { first, count } = diff(x, y, len, false, va, r[2]);
  if (!count) continue;
  const others = more.map((m) => m.get(va)).filter(Boolean);
  if (others.some((o) => !diff(o, y, Math.min(len, o.length), false, va, r[2]).count)) continue;
  if (others.some((o) => diff(x, o, Math.min(len, o.length), true).count)) { varies++; continue; }
  bad++;
  lines.push(`0x${va.toString(16)} ${names.get(va) ?? "?"}: ${count} bytes differ, first +0x${first.toString(16)} of 0x${len.toString(16)}: original ${x.subarray(first, first + 8).toString("hex")} rebuilt ${y.subarray(first, first + 8).toString("hex")}`);
}
console.log(`${compared} symbols compared, ${bad} differ${more.length ? `, ${varies} more vary between the original runs` : ""}`);
for (const l of lines.slice(0, show)) console.log("  " + l);
