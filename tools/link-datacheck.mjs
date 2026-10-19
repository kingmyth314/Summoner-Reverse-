// Initial data of the standalone rebuilt exe against Sum.exe: for every data symbol of the address table
// (build/link/Sum-rebuilt.addr, from tools/link-build.mjs), the bytes the rebuilt exe starts with against Sum.exe's at
// the same symbol. A dword that is an address in Sum.exe counts as equal when it translates to the rebuilt exe's
// pointer through the same table, or when both point to the same string (literals are not in the table). Each object
// is compared up to the next symbol in either build. .bss (no file bytes) and the import address table are skipped.
//
//   node tools/link-datacheck.mjs [--show 40]
//
// The hybrid game cannot see a wrong initial value in a byte-matching function's data: those functions use Sum.exe's
// own data there. The standalone exe uses the decomp's definitions.
import { readFileSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const show = Number(args[args.indexOf("--show") + 1] || 40);
const out = join(repo, "build", "link");

function pe(path) {
  const b = readFileSync(path);
  const o = b.readUInt32LE(0x3c), n = b.readUInt16LE(o + 6), so = o + 24 + b.readUInt16LE(o + 20), base = b.readUInt32LE(o + 52);
  const secs = [];
  for (let i = 0; i < n; i++) {
    const s = so + i * 40;
    secs.push({ name: b.toString("latin1", s, s + 8).replace(/\0.*$/, ""), va: base + b.readUInt32LE(s + 12), vsize: b.readUInt32LE(s + 8), raw: b.readUInt32LE(s + 20), rawSize: b.readUInt32LE(s + 16) });
  }
  // File bytes of [va, va+len), or null where a byte has no file data (.bss / the uninitialised tail).
  const read = (va, len) => {
    for (const s of secs) if (va >= s.va && va < s.va + s.vsize) {
      const off = va - s.va;
      // Past the file data the section is zero (its .bss part).
      if (off >= s.rawSize) return Buffer.alloc(len);
      if (off + len > s.rawSize) return Buffer.concat([b.subarray(s.raw + off, s.raw + s.rawSize), Buffer.alloc(off + len - s.rawSize)]);
      return b.subarray(s.raw + off, s.raw + off + len);
    }
    return null;
  };
  const code = secs.find((s) => s.name === ".text");
  const iat = base + b.readUInt32LE(o + 24 + 96 + 12 * 8), iatEnd = iat + b.readUInt32LE(o + 24 + 96 + 12 * 8 + 4);
  return { read, base, iat, iatEnd, codeEnd: code.va + code.vsize, end: Math.max(...secs.map((s) => s.va + s.vsize)) };
}

const orig = pe(join(repo, "Summoner", "Sum.exe"));
const now = pe(join(out, "Sum-rebuilt.exe"));
const rows = readFileSync(join(out, "Sum-rebuilt.addr"), "latin1").trim().split(/\n/).map((l) => l.split(" ").map((x) => parseInt(x, 16)));
const translate = (va) => {
  let lo = 0, hi = rows.length - 1, best = -1;
  while (lo <= hi) { const m = (lo + hi) >> 1; if (rows[m][0] <= va) { best = m; lo = m + 1; } else hi = m - 1; }
  if (best < 0 || va - rows[best][0] >= rows[best][1]) return null;
  return rows[best][2] + (va - rows[best][0]);
};
const starts = [];
const names = new Map();
for (const l of readFileSync(join(repo, "symbols", "sum.symbols"), "latin1").split(/\r?\n/)) {
  const m = /^0x([0-9a-f]+)\s+(\S+)/i.exec(l);
  if (m) { names.set(parseInt(m[1], 16), m[2]); starts.push(parseInt(m[1], 16)); }
}
starts.sort((a, b) => a - b);
// Rebuilt objects end where the map's next symbol starts (string literals between named objects are not compared).
const mapStarts = [...new Set(readFileSync(join(out, "Sum-rebuilt.map"), "latin1").split(/\r?\n/)
  .map((l) => /^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+\S+\s+([0-9a-f]{8})\s/.exec(l)).filter(Boolean).map((m) => parseInt(m[1], 16)))].sort((a, b) => a - b);
const after = (list, a) => { let lo = 0, hi = list.length; while (lo < hi) { const m = (lo + hi) >> 1; if (list[m] <= a) lo = m + 1; else hi = m; } return list[lo] ?? a + 4; };

let checked = 0, bad = 0;
const report = [];
for (const [o, size, n] of rows) {
  if (o < orig.codeEnd) continue; // code
  if (o >= orig.iat && o < orig.iatEnd) continue; // the import address table (hint-name offsets differ)
  const len = Math.min(size, after(starts, o) - o, after(mapStarts, n) - n, 0x10000);
  const a = orig.read(o, len), b = now.read(n, len);
  if (!a) continue; // .bss in Sum.exe
  checked++;
  const diffs = [];
  if (!b) diffs.push("rebuilt has no initial bytes (.bss) where Sum.exe has data");
  else {
    for (let i = 0; i < len; ) {
      if (i + 4 <= len) {
        const x = a.readUInt32LE(i), y = b.readUInt32LE(i);
        if (x === y) { i += 4; continue; }
        if (x >= orig.base && x < orig.end && translate(x) === y) { i += 4; continue; }
        // A pointer to unnamed data (a string literal): compare what both point to.
        if (x >= orig.base && x < orig.end && y >= now.base && y < now.end) {
          const sa = orig.read(x, 64), sb = now.read(y, 64);
          if (sa && sb) { const za = sa.indexOf(0), zb = sb.indexOf(0); if (za >= 0 && za === zb && sa.subarray(0, za).equals(sb.subarray(0, zb))) { i += 4; continue; } }
        }
      }
      if (a[i] !== b[i]) { diffs.push(i); if (diffs.length > 8) break; }
      i++;
    }
  }
  if (diffs.length) {
    bad++;
    const nm = names.get(o) ?? "?";
    const where = typeof diffs[0] === "number" ? `first at +0x${diffs[0].toString(16)} of 0x${len.toString(16)}: Sum.exe ${a.subarray(diffs[0], diffs[0] + 8).toString("hex")} rebuilt ${b.subarray(diffs[0], diffs[0] + 8).toString("hex")}` : diffs[0];
    report.push(`0x${o.toString(16)} ${nm} (rebuilt 0x${n.toString(16)}): ${where}`);
  }
}
console.log(`${checked} initialised data symbols compared, ${bad} differ`);
for (const r of report.slice(0, show)) console.log("  " + r);
