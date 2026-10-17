// Compiles a src/decomp .cpp with Visual C++ 6.0 and compares each function that carries a `// 0xADDR` comment
// with Sum.exe's bytes at that address (relocated fields masked).
//
//   node tools/vc6-compare.mjs <file.cpp>... [--flags "/O2"] [--json] [--keep] [-v]
// Without --flags each file uses its line in src/decomp/vc6-flags.txt (or the file VC6_FLAGS_FILE names, for a
// tree outside the repository), else the default /O2 /GX /MT.
//
// VC6_DIR must name the VC98 folder of an extracted Visual C++ 6.0 (the one with BIN\CL.EXE and INCLUDE). Sum.exe
// was built with Service Pack 5 (c2.dll 12.00.8966, shipped in vs6sp5.exe as msvcep.dll); use a VC98 folder with
// SP5's BIN files, or results differ in register and branch layout (docs/decomp-plan.md, "Toolchain"). The
// object and the /FAsc listing go to a temporary folder outside the repository (--keep prints it and leaves it).
//
// Per function: match / differs (first differing offset, sizes, instruction counts, the two instructions there) /
// not found (no compiled function for the comment). A relocated field (call / jmp rel32, absolute address) is
// masked in both; when its target is a function of the same file with an address comment, or a name that
// symbols/sum.symbols knows (`a::b` looked up as `a__b`), the original's target is checked too and a different
// one is reported as a warning only: overloads share a sum.symbols name and the linker folds identical COMDATs
// (/OPT:ICF), so a different target is a hint, not proof.
import { readFileSync, existsSync, mkdtempSync, rmSync, readdirSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { join, resolve, dirname, basename } from "node:path";
import { tmpdir } from "node:os";
import { fileURLToPath } from "node:url";
import { loadSymbols, loadFunctions } from "./symbols.mjs";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const exePath = join(repoRoot, "Summoner", "Sum.exe");
const asmPath = join(repoRoot, "build", "sum.asm");
const includeDir = process.env.DECOMP_INCLUDE || join(repoRoot, "src", "decomp", "include"); // DECOMP_INCLUDE: another header tree (a merge stage)

// ---- arguments -----------------------------------------------------------------------------------------
const args = process.argv.slice(2);
const files = [];
let flags = null, json = false, keep = false, verbose = false;
for (let i = 0; i < args.length; i++) {
  if (args[i] === "--flags") flags = args[++i];
  else if (args[i] === "--json") json = true;
  else if (args[i] === "--keep") keep = true;
  else if (args[i] === "-v") verbose = true;
  else files.push(args[i]);
}
if (!files.length) {
  console.error('usage: node tools/vc6-compare.mjs <file.cpp>... [--flags "/O2"] [--json] [--keep] [-v]');
  process.exit(2);
}
const VC = process.env.VC6_DIR;
if (!VC || !existsSync(join(VC, "BIN", "CL.EXE"))) {
  console.error("VC6_DIR is not set, or has no BIN\\CL.EXE: set it to the VC98 folder of Visual C++ 6.0, e.g.\n" +
    "  $env:VC6_DIR = 'C:\\path\\to\\VC98'   (PowerShell)");
  process.exit(2);
}

// ---- Sum.exe ----------------------------------------------------------------------------------------------
const exe = readFileSync(exePath);
const peOff = exe.readUInt32LE(0x3c);
const imageBase = exe.readUInt32LE(peOff + 24 + 28);
const peSections = [];
{
  const n = exe.readUInt16LE(peOff + 6), optSize = exe.readUInt16LE(peOff + 20);
  for (let i = 0, o = peOff + 24 + optSize; i < n; i++, o += 40)
    peSections.push({ va: imageBase + exe.readUInt32LE(o + 12), vsize: exe.readUInt32LE(o + 8), raw: exe.readUInt32LE(o + 20), rawSize: exe.readUInt32LE(o + 16) });
}
function exeBytes(va, len) {
  const s = peSections.find((x) => va >= x.va && va < x.va + Math.max(x.vsize, x.rawSize));
  if (!s) return null;
  const o = s.raw + (va - s.va);
  return exe.subarray(o, Math.min(o + len, s.raw + s.rawSize));
}
const { symbols } = loadSymbols();
const symByName = new Map(symbols.map((s) => [s.name, s.addr]));
const starts = [...new Set([...(loadFunctions() ?? []).map((f) => f.addr), ...symbols.filter((s) => s.kind === "func").map((s) => s.addr)])].sort((a, b) => a - b);
const nextStart = (va) => { let lo = 0, hi = starts.length; while (lo < hi) { const m = (lo + hi) >> 1; if (starts[m] <= va) lo = m + 1; else hi = m; } return lo < starts.length ? starts[lo] : va + 0x1000; };
// Instructions of build/sum.asm (tools/asm.mjs's llvm-objdump cache), for counts and the text at a difference.
let sumIns = null;
function loadSumAsm() {
  if (sumIns || !existsSync(asmPath)) return sumIns;
  sumIns = [];
  for (const line of readFileSync(asmPath, "latin1").split(/\r?\n/)) {
    const m = /^\s+([0-9a-f]+):\s+(.*)$/.exec(line);
    if (m) sumIns.push({ va: parseInt(m[1], 16), text: m[2].replace(/\s+<\.text\+0x[0-9a-f]+>/g, "").replace(/\s+/g, " ").trim() });
  }
  return sumIns;
}
function sumRange(lo, hi) {
  const ins = loadSumAsm();
  if (!ins) return null;
  let a = 0, b = ins.length;
  while (a < b) { const m = (a + b) >> 1; if (ins[m].va < lo) a = m + 1; else b = m; }
  const out = [];
  for (let i = a; i < ins.length && ins[i].va < hi; i++) out.push(ins[i]);
  return out;
}

// ---- COFF object ------------------------------------------------------------------------------------------
function readCoff(buf) {
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
function readCod(text) {
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
const demangledOf = (cod, mangled) => cod.names.get(mangled) ?? (mangled.startsWith("_") ? mangled.slice(1) : mangled);

// ---- the source: `// 0xADDR` comments followed by a function definition -----------------------------
function sourceFunctions(src) {
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

// ---- one file -----------------------------------------------------------------------------------------
// Per-unit flags (src/decomp/vc6-flags.txt: "<path under src/decomp> <flags>"), used when --flags is not given.
const DEFAULT_FLAGS = "/O2 /GX /MT";
const unitFlags = new Map();
try {
  const flagsFile = process.env.VC6_FLAGS_FILE || join(repoRoot, "src", "decomp", "vc6-flags.txt");
  for (const l of readFileSync(flagsFile, "utf8").split(/\r?\n/)) {
    const m = /^([^#\s]\S*)\s+(\S.*?)\s*$/.exec(l);
    if (m) unitFlags.set(m[1].toLowerCase(), m[2]);
  }
} catch {}
function flagsFor(cpp) {
  if (flags) return flags;
  const rel = resolve(cpp).replace(/\\/g, "/").toLowerCase();
  for (const [k, v] of unitFlags) if (rel.endsWith("/" + k)) return v;
  return DEFAULT_FLAGS;
}

function compile(cpp, dir, fl) {
  const env = { ...process.env, INCLUDE: `${includeDir};${join(VC, "INCLUDE")}`, PATH: `${join(VC, "BIN")};${process.env.PATH}` };
  const argv = ["/nologo", "/c", ...fl.split(/\s+/).filter(Boolean), "/FAsc", `/Fa${dir}\\`, `/Fo${dir}\\`, cpp];
  let out = "";
  try { out = execFileSync(join(VC, "BIN", "CL.EXE"), argv, { env, cwd: dir, encoding: "latin1", maxBuffer: 1 << 26 }); }
  catch (e) { return { error: ((e.stdout ?? "") + (e.stderr ?? "")).split(/\r?\n/).filter((l) => /error/.test(l)).slice(0, 10).join("\n") || String(e) }; }
  const stem = basename(cpp).replace(/\.[^.]+$/, "");
  return { obj: join(dir, stem + ".obj"), cod: join(dir, stem + ".cod"), out };
}

function compareFile(cpp) {
  const dir = mkdtempSync(join(tmpdir(), "vc6cmp-"));
  const fl = flagsFor(cpp);
  const result = { file: cpp, flags: fl, functions: [], error: null, tmp: keep ? dir : undefined };
  try {
    const c = compile(resolve(cpp), dir, fl);
    if (c.error) { result.error = c.error; return result; }
    const coff = readCoff(readFileSync(c.obj));
    const cod = readCod(readFileSync(c.cod, "latin1"));
    // Function symbols per section, with their sizes (up to the next function symbol or the section end).
    const funcs = [];
    coff.syms.forEach((s, idx) => { if (s && s.sec > 0 && (s.type & 0x20) && (s.cls === 2 || s.cls === 3)) funcs.push({ ...s, idx }); });
    for (const f of funcs) {
      const sec = coff.sections[f.sec - 1];
      const next = funcs.filter((g) => g.sec === f.sec && g.value > f.value).reduce((a, g) => Math.min(a, g.value), sec.size);
      f.size = next - f.value;
      f.demangled = demangledOf(cod, f.name);
    }
    const src = sourceFunctions(readFileSync(cpp, "latin1"));
    // The k-th source definition of a name is the k-th compiled function of that name (VC6 emits in source order).
    const byName = new Map();
    for (const f of funcs.sort((a, b) => a.sec - b.sec || a.value - b.value)) {
      const key = f.demangled.replace(/\s+/g, "");
      if (!byName.has(key)) byName.set(key, []);
      byName.get(key).push(f);
    }
    const used = new Map(), addrOfSym = new Map();
    const pairs = src.map((s) => {
      const list = byName.get(s.name) ?? byName.get(s.name.replace(/^.*::/, "")) ?? [];
      const k = used.get(s.name) ?? 0; used.set(s.name, k + 1);
      const f = list[k] ?? null;
      if (f) addrOfSym.set(f.name, s.addr);
      return { s, f };
    });
    for (const { s, f } of pairs) {
      const r = { addr: "0x" + s.addr.toString(16), name: s.name, line: s.line };
      if (!f) { r.status = "not found"; result.functions.push(r); continue; }
      const sec = coff.sections[f.sec - 1];
      // The section's alignment padding (nop / int3 after the last instruction) is not part of the function.
      const codProc = cod.procs.get(f.name);
      const ours = codProc ? codProc.ins : [];
      const lastIns = ours.length ? ours[ours.length - 1].off : 0;
      let size = f.size;
      while (size > lastIns + 1 && (sec.data[f.value + size - 1] === 0x90 || sec.data[f.value + size - 1] === 0xcc)) size--;
      const mine = Buffer.from(sec.data.subarray(f.value, f.value + size));
      const origLen = Math.max(nextStart(s.addr) - s.addr, mine.length);
      const orig0 = exeBytes(s.addr, origLen);
      if (!orig0) { r.status = "not found"; r.reason = "address not in Sum.exe"; result.functions.push(r); continue; }
      const orig = Buffer.from(orig0);
      const wrongTargets = [];
      for (const rel of sec.relocs) {
        if (rel.off < f.value || rel.off >= f.value + f.size) continue;
        const o = rel.off - f.value, sym = coff.syms[rel.sym];
        if (![6, 7, 0x14].includes(rel.type) || o + 4 > orig.length) continue;
        const addend = mine.readInt32LE(o);
        // Expected target: a function of this file with an address comment, or a sum.symbols name.
        let want = sym ? addrOfSym.get(sym.name) : undefined;
        if (want === undefined && sym) {
          const d = demangledOf(cod, sym.name);
          const key = /^[A-Za-z_][\w:]*$/.test(d) ? d.replace(/::/g, "__") : null;
          if (key && symByName.has(key)) want = symByName.get(key);
        }
        if (want !== undefined) {
          const got = rel.type === 0x14 ? (s.addr + o + 4 + orig.readInt32LE(o)) >>> 0 : orig.readUInt32LE(o);
          const exp = rel.type === 0x14 ? want : (want + addend) >>> 0;
          if (got !== exp) wrongTargets.push({ offset: o, symbol: demangledOf(cod, sym.name), expected: "0x" + exp.toString(16), found: "0x" + got.toString(16) });
        }
        mine.fill(0, o, o + 4); orig.fill(0, o, o + 4);
      }
      let diff = -1;
      for (let i = 0; i < mine.length; i++) if (i >= orig.length || mine[i] !== orig[i]) { diff = i; break; }
      // A target is only comparable while the code before it is the same.
      if (diff >= 0) for (let i = wrongTargets.length - 1; i >= 0; i--) if (wrongTargets[i].offset >= diff) wrongTargets.splice(i, 1);
      let origSize = origLen;
      while (origSize > mine.length && (orig[origSize - 1] === 0x90 || orig[origSize - 1] === 0xcc)) origSize--;
      if (diff < 0 && origSize > mine.length) {
        // Same prefix: the original's extra bytes are only real when they are not padding before the next start.
        const tail = orig.subarray(mine.length, origSize);
        if (!tail.every((b) => b === 0x90 || b === 0xcc)) r.note = `original has ${origSize - mine.length} more bytes before the next known function start`;
        origSize = mine.length;
      }
      const theirs = sumRange(s.addr, s.addr + origSize);
      r.size = mine.length; r.origSize = origSize;
      r.instructions = ours.length; r.origInstructions = theirs ? theirs.length : null;
      if (diff < 0) r.status = "match";
      else {
        r.status = "differs";
        if (diff >= 0) {
          r.offset = "0x" + diff.toString(16);
          const at = (list, key) => { let x = null; for (const e of list) if (key(e) <= diff) x = e; return x; };
          const a = at(ours, (e) => e.off), b = theirs ? at(theirs, (e) => e.va - s.addr) : null;
          r.ours = a ? a.text : null; r.orig = b ? b.text : null;
        }
      }
      if (wrongTargets.length) r.targetWarnings = wrongTargets;
      result.functions.push(r);
    }
  } finally {
    if (!keep) rmSync(dir, { recursive: true, force: true });
  }
  return result;
}

// ---- main -------------------------------------------------------------------------------------------------
const results = files.map(compareFile);
if (json) {
  console.log(JSON.stringify(results, null, 1));
} else {
  const total = { match: 0, differs: 0, "not found": 0 };
  let warned = 0;
  for (const res of results) {
    console.log(`${res.file} (${res.flags})${res.tmp ? "  [" + res.tmp + "]" : ""}`);
    if (res.error) { console.log("  compile failed:\n    " + res.error.split("\n").join("\n    ")); continue; }
    for (const f of res.functions) {
      total[f.status]++;
      let line = `  ${f.addr.padEnd(9)} ${f.status.padEnd(9)} ${f.name}`;
      if (f.status !== "not found") line += `  (${f.size}/${f.origSize} bytes, ${f.instructions}/${f.origInstructions ?? "?"} ins)`;
      if (f.offset) line += `  at +${f.offset}: ${f.ours ?? "?"}  |  orig: ${f.orig ?? "?"}`;
      if (f.targetWarnings) warned++;
      if (f.targetWarnings && verbose) line += `  target?: ` + f.targetWarnings.map((t) => `+0x${t.offset.toString(16)} ${t.symbol} ${t.expected}, orig ${t.found}`).join("; ");
      if (f.reason) line += `  (${f.reason})`;
      if (f.note && verbose) line += `  [${f.note}]`;
      console.log(line);
    }
  }
  const n = total.match + total.differs + total["not found"];
  console.log(`summary: ${total.match} match, ${total.differs} differ, ${total["not found"]} not found, of ${n} functions` +
    (n ? ` (${((100 * total.match) / n).toFixed(1)}% match)` : "") +
    (warned ? `; ${warned} with a relocation target that differs from the expected address (-v lists them)` : ""));
}
