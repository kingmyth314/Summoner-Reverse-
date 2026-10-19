// The save comparison shared by tools/hybrid-test.mjs (the hybrid game) and tools/link-savecheck.mjs (the standalone
// rebuilt exe): one save against several saves of the original made the same way, dword by dword after the
// thumbnail. A dword that varies between the original runs may vary as much in the tested save; one that never varies
// must be equal. LENT records are walked and named. Save layout: docs/sum-exe/savegame.md.
import { readFileSync } from "node:fs";

const SECTIONS = [["level data", 25035], ["INVT", 8352], ["conversation", 64624], ["ITEM", 2158], ["LENT", 67660],
  ["CMRA", 40], ["CONT", 6324], ["quest bits", 25], ["level items", 2052], ["two-stage", 26431],
  ["automap labels", 6660], ["hot buttons", 180]];
const START = 0x200 + 60702; // after the header and the thumbnail

function cells(A) {
  let o = START;
  const sec = SECTIONS.map(([n, s]) => { const r = { n, a: o, s }; o += s; return r; });
  const le0 = sec.find((s) => s.n === "LENT").a, out = [];
  for (const s of sec) if (s.n !== "LENT") for (let i = s.a; i + 4 <= s.a + s.s; i += 4) out.push([i, `${s.n}+0x${(i - s.a).toString(16)}`]);
  for (let i = le0; i < le0 + 0x34; i += 4) if (i < le0 + 0x10 || i >= le0 + 0x30) out.push([i, `LENT+0x${(i - le0).toString(16)}`]); // +0x10..0x2f: the summon name, stack leftovers
  let r = le0 + 0x34;
  for (let k = 0, n = A.readInt32LE(le0 + 0x30); k < n; k++) {
    const t = A.readInt32LE(r + 4), size = 4 + 0x17c + (t === 7 ? 8 : 0); // type 7 (party member) has 8 bytes more
    for (let i = r + 4; i + 4 <= r + size; i += 4) out.push([i, `LENT rec ${k} (type ${t}, hash ${A.readUInt32LE(r).toString(16)}) +0x${(i - r - 4).toString(16)}`]);
    r += size;
  }
  return out;
}

// origs: the original saves' paths; tested: the tested save's path. Returns [[offset, label, line], ...].
export function compareSaves(origPaths, testedPath) {
  const origs = origPaths.map((p) => readFileSync(p));
  const H = readFileSync(testedPath), A = origs[0];
  const within = (vals, h) => { const lo = Math.min(...vals), hi = Math.max(...vals), w = hi - lo; return h >= lo - w - 1e-6 && h <= hi + w + 1e-6; };
  const bad = [];
  for (const [i, label] of cells(A)) {
    const hb = H.subarray(i, i + 4);
    if (origs.every((x) => x.subarray(i, i + 4).equals(hb))) continue;
    if (!origs.every((x) => x.subarray(i, i + 4).equals(A.subarray(i, i + 4)))) {
      if (within(origs.map((x) => x.readInt32LE(i)), H.readInt32LE(i))) continue;
      const f = origs.map((x) => x.readFloatLE(i));
      if (f.every(Number.isFinite) && within(f, H.readFloatLE(i))) continue;
    }
    bad.push([i, label, `${label} [0x${i.toString(16)}]: original ${origs.map((x) => x.subarray(i, i + 4).toString("hex")).join("/")} tested ${hb.toString("hex")}`]);
  }
  return bad;
}
