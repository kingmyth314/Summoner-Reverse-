// Shared helpers for tools that work with VC6 objects compiled from src/decomp: the COFF object reader, the .cod
// listing reader (mangled -> demangled names, instructions), the `// 0xADDR` function markers of a source file, and
// the addresses the decomp tree gives its functions and globals. Used by tools/vc6-compare.mjs and
// tools/hybrid-build.mjs.
import { readFileSync, readdirSync } from "node:fs";
import { join } from "node:path";

// ---- COFF object ------------------------------------------------------------------------------------------
export function readCoff(buf) {
  const nSec = buf.readUInt16LE(2), symPtr = buf.readUInt32LE(8), nSym = buf.readUInt32LE(12), optSize = buf.readUInt16LE(16);
  const strTab = symPtr + nSym * 18;
  const name8 = (o) => {
    if (buf.readUInt32LE(o) === 0) { const s = strTab + buf.readUInt32LE(o + 4); return buf.toString("latin1", s, buf.indexOf(0, s)); }
    const e = buf.subarray(o, o + 8).indexOf(0);
    return buf.toString("latin1", o, e < 0 ? o + 8 : o + e);
  };
  const sections = [];
  for (let i = 0, o = 20 + optSize; i < nSec; i++, o += 40) {
    const sec = { name: name8(o), size: buf.readUInt32LE(o + 16), raw: buf.readUInt32LE(o + 20), relocs: [] };
    const rp = buf.readUInt32LE(o + 24), nr = buf.readUInt16LE(o + 32);
    for (let r = 0; r < nr; r++) sec.relocs.push({ off: buf.readUInt32LE(rp + r * 10), sym: buf.readUInt32LE(rp + r * 10 + 4), type: buf.readUInt16LE(rp + r * 10 + 8) });
    sec.data = sec.raw ? buf.subarray(sec.raw, sec.raw + sec.size) : Buffer.alloc(sec.size);
    sections.push(sec);
  }
  const syms = [];
  for (let i = 0; i < nSym; i++) {
    const o = symPtr + i * 18;
    const s = { name: name8(o), value: buf.readUInt32LE(o + 8), sec: buf.readInt16LE(o + 12), type: buf.readUInt16LE(o + 14), cls: buf.readUInt8(o + 16) };
    syms[i] = s;
    i += buf.readUInt8(o + 17);
  }
  return { sections, syms };
}

// ---- the .cod listing: mangled -> demangled names, and the instructions of each function ---------------
export function readCod(text) {
  const names = new Map(), procs = new Map();
  let cur = null;
  for (const line of text.split(/\r?\n/)) {
    let m;
    if ((m = /^(?:PUBLIC|EXTRN)\s+(\S+?)(?::\w+)?\s*;\s*(.+?)\s*$/.exec(line))) { names.set(m[1], m[2]); continue; }
    if ((m = /^(\S+)\s+PROC NEAR(?:\s*;\s*([^,]+))?/.exec(line))) {
      if (m[2] && m[2].trim() !== "COMDAT") names.set(m[1], m[2].trim());
      cur = { ins: [] }; procs.set(m[1], cur); continue;
    }
    if (/^\S+\s+ENDP\b/.test(line)) { cur = null; continue; }
    // An instruction longer than 5 bytes is listed over two lines: offset and bytes, then the rest and the text.
    if (cur && (m = /^\s+([0-9a-f]{5,8})\t([0-9a-f]{2}(?: [0-9a-f]{2})*)\s*(?:\t\s*(.*))?$/.exec(line))) cur.ins.push({ off: parseInt(m[1], 16), text: (m[3] ?? "").replace(/\s+/g, " ").trim() });
    else if (cur && cur.ins.length && (m = /^\t([0-9a-f ]+)\t\s*(\S.*)$/.exec(line))) cur.ins[cur.ins.length - 1].text = m[2].replace(/\s+/g, " ").trim();
  }
  for (const p of procs.values()) { const base = p.ins.length ? p.ins[0].off : 0; for (const x of p.ins) x.off -= base; }
  return { names, procs };
}
export const demangledOf = (cod, mangled) => cod.names.get(mangled) ?? (mangled.startsWith("_") ? mangled.slice(1) : mangled);

// ---- the source: `// 0xADDR` comments followed by a function definition -----------------------------
export function sourceFunctions(src) {
  const L = src.split(/\r?\n/), out = [];
  for (let i = 0; i < L.length; i++) {
    const m = /^\s*\/\/\s*(0x[0-9a-f]{6})\b/.exec(L[i]);
    if (!m) continue;
    // Only the first address of a comment block names the function; later ones are data in the text.
    let k = i - 1;
    while (k >= 0 && /^\s*\/\//.test(L[k]) && !/^\s*\/\/\s*0x[0-9a-f]{6}\b/.test(L[k])) k--;
    if (k >= 0 && /^\s*\/\/\s*0x[0-9a-f]{6}\b/.test(L[k])) continue;
    let j = i + 1;
    while (j < L.length && /^\s*\/\//.test(L[j])) j++;
    // The definition follows the comment block directly (a blank line ends it: a note, not a function).
    if (j >= L.length || /^\s*$/.test(L[j]) || /^\s*#/.test(L[j])) continue;
    let head = "";
    for (let k = j; k < Math.min(L.length, j + 8); k++) { head += " " + L[k].replace(/\/\/.*$/, ""); if (/[{;]/.test(L[k].replace(/\/\/.*$/, ""))) break; }
    const paren = head.indexOf("(");
    const brace = head.indexOf("{"), semi = head.indexOf(";"), eq = head.indexOf("=");
    if (paren < 0 || brace < 0 || (semi >= 0 && semi < brace) || (eq >= 0 && eq < paren && !/operator\s*[^\s(]*=/.test(head))) continue;
    const nm = /((?:[A-Za-z_]\w*\s*::\s*)*(?:~?[A-Za-z_]\w*|operator\s*(?:\(\)|[^\s(]+)))\s*$/.exec(head.slice(0, paren));
    if (!nm) continue;
    out.push({ addr: parseInt(m[1], 16), name: nm[1].replace(/\s+/g, ""), line: j + 1 });
  }
  return out;
}

// ---- addresses the decomp tree itself gives names: function markers and globals with a trailing address comment
// root: the src/decomp folder (it holds game/ and vsdk/). Returns { func, data }: Map name -> Set of addresses.
const decompMaps = new Map();
export function decompAddrs(root) {
  if (decompMaps.has(root)) return decompMaps.get(root);
  const map = { func: new Map(), data: new Map() };
  decompMaps.set(root, map);
  const add = (m, name, addr) => { if (!m.has(name)) m.set(name, new Set()); m.get(name).add(addr); };
  for (const d of ["game", "vsdk"]) {
    let list = [];
    try { list = readdirSync(join(root, d)); } catch {}
    for (const f of list) {
      if (!f.endsWith(".cpp")) continue;
      const text = readFileSync(join(root, d, f), "latin1");
      for (const s of sourceFunctions(text)) add(map.func, s.name, s.addr);
      // The address comment must be on the definition's own line.
      const re = /^(?!\s)(?!return\b|typedef\b|#|\/\/)[A-Za-z_][^(;=\n]*?\b((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)[ \t]*(?:\[[^\]\n]*\])*[ \t]*(?:=[^;\n]*)?;[ \t]*\/\/[ \t]*(0x[0-9a-f]{6,7})\b[^\n]*/gm;
      for (const m of text.matchAll(re)) {
        // `T a, b;   // 0xA, 0xB`: several declarators, their addresses in the same order.
        const decl = m[0].slice(0, m[0].indexOf(";"));
        const addrs = [...m[0].slice(m[0].indexOf("//")).matchAll(/^[^a-z(]*?((?:0x[0-9a-f]{6,7}\b[ ,]*)+)/g)][0]?.[1].match(/0x[0-9a-f]{6,7}/g).map((x) => parseInt(x, 16)) ?? [];
        const names = !decl.includes("=") && decl.includes(",") ? decl.split(",").map((p) => (/((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*$/.exec(p) || [])[1]) : null;
        if (names && names.every(Boolean) && names.length === addrs.length) names.forEach((n, i) => add(map.data, n, addrs[i]));
        else add(map.data, m[1], parseInt(m[2], 16));
      }
    }
  }
  return map;
}
