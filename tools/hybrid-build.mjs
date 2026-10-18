// Hybrid build: recompile src/decomp files with Visual C++ 6.0 and link the result into a DLL whose functions replace
// the original ones inside the running Sum.exe (SummonerFix [Hybrid] Dll=..., src/summonerfix/hybrid.c). The rest of
// the game stays the original, so a decompiled function can be run in the game one file, or one function, at a time.
//
// usage: node tools/hybrid-build.mjs <file.cpp>... [--only 0xADDR,0xADDR] [--out build/hybrid] [--force] [-v]
//   VC6_DIR must name the VC98 folder of Visual C++ 6.0 (as for tools/vc6-compare.mjs).
//   --only   inject just these functions; the others in the files stay the original
//   --force  link even with names that have no address (a call to one jumps to 0: only for trying things)
//
// How: each file is compiled with its flags from src/decomp/vc6-flags.txt. In the objects, every symbol is made to
// mean the original program unless it is a function we inject:
//   - an undefined symbol (a function or global of a file not in this build, a C runtime function, an import) becomes
//     an absolute symbol with its Sum.exe address: from the decomp tree's address comments (sources and headers,
//     `// folded (0xADDR` too), else symbols/sum.symbols, else Sum.exe's import table (__imp__Name@N);
//   - a defined global with an address comment becomes absolute too, so the injected code reads and writes the
//     game's own variable, not a copy in the DLL;
//   - a defined function that is not injected becomes absolute, so calls go to the original function;
//   - a name another object of the build defines is left alone: the linker joins them;
//   - constants (strings, floats, jump tables) stay in the DLL.
// The objects are linked with /DLL /FIXED /NOENTRY /NODEFAULTLIB (no C runtime of its own: the game's is used),
// exporting each injected function as f_<address>_<size>; the loader writes a jmp at each address.
// Writes <out>/hybrid.dll, hybrid.map and hybrid.txt (the injected functions).
import { readFileSync, writeFileSync, mkdirSync, existsSync, readdirSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { join, resolve, dirname, basename } from "node:path";
import { fileURLToPath } from "node:url";
import { loadSymbols } from "./symbols.mjs";
import { readCoff, readCod, demangledOf, sourceFunctions, decompAddrs } from "./lib/vc6obj.mjs";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const decompRoot = join(repoRoot, "src", "decomp");
const includeDir = join(decompRoot, "include");
const exePath = join(repoRoot, "Summoner", "Sum.exe");
const BASE = 0x50000000; // free in Sum.exe: binkw32.dll takes 0x30000000

const args = process.argv.slice(2);
let outDir = join(repoRoot, "build", "hybrid"), only = null, verbose = false, force = false;
const files = [];
for (let i = 0; i < args.length; i++) {
  if (args[i] === "--out") outDir = resolve(args[++i]);
  else if (args[i] === "--only") only = new Set(args[++i].split(",").map((x) => parseInt(x, 16)));
  else if (args[i] === "--force") force = true;
  else if (args[i] === "-v") verbose = true;
  else files.push(resolve(args[i]));
}
if (!files.length) {
  console.error("usage: node tools/hybrid-build.mjs <file.cpp>... [--only 0xADDR,...] [--out DIR] [--force] [-v]");
  process.exit(2);
}
const VC = process.env.VC6_DIR;
if (!VC || !existsSync(join(VC, "BIN", "CL.EXE"))) {
  console.error("VC6_DIR is not set, or has no BIN\\CL.EXE: set it to the VC98 folder of Visual C++ 6.0.");
  process.exit(2);
}
mkdirSync(outDir, { recursive: true });

// ---- names -> Sum.exe addresses --------------------------------------------------------------------------
const { symbols } = loadSymbols();
const symByName = new Map(symbols.map((s) => [s.name, s.addr]));
const tree = decompAddrs(decompRoot);
// The room each original function has for the loader's 5-byte jmp: up to the next function in sum.symbols.
const funcStarts = [...new Set(symbols.filter((s) => s.kind === "func").map((s) => s.addr))].sort((a, b) => a - b);
// The export name: f_<address>_<size>, the original's size (up to the next function), so the loader can leave
// original a function SummonerFix patches anywhere inside.
const exportName = (addr) => `f_${addr.toString(16)}_${Math.min(roomAt(addr), 0xffff).toString(16)}`;
const roomAt = (addr) => {
  let lo = 0, hi = funcStarts.length;
  while (lo < hi) { const mid = (lo + hi) >> 1; if (funcStarts[mid] <= addr) lo = mid + 1; else hi = mid; }
  return lo < funcStarts.length ? funcStarts[lo] - addr : Infinity;
};
const add = (m, name, addr) => { if (!m.has(name)) m.set(name, new Set()); m.get(name).add(addr); };
// Folded definitions (`// folded (0xADDR, ...)` above the body: the original shares one function) and declarations
// in the headers with an address comment, functions and globals.
for (const d of ["game", "vsdk"]) {
  for (const f of readdirSync(join(decompRoot, d))) {
    if (!f.endsWith(".cpp")) continue;
    const L = readFileSync(join(decompRoot, d, f), "latin1").split(/\r?\n/);
    for (let i = 0; i < L.length - 1; i++) {
      // A comment block that says the linker folded the functions declared below it into one, with one code
      // address in it ("... folded ... into empty_function (0x4f13e0)"), then the declarations.
      if (/^\/\//.test(L[i]) && !/^\/\//.test(L[i - 1] ?? "")) {
        let j = i;
        while (j < L.length && /^\/\//.test(L[j])) j++;
        const text = L.slice(i, j).join(" ");
        const code = [...new Set((text.match(/0x[0-9a-f]{6}\b/g) ?? []).map((x) => parseInt(x, 16)))].filter((a) => a >= 0x401000 && a < 0x57db40);
        if (/\bfold/.test(text) && code.length === 1) {
          for (let k = j; k < L.length; k++) {
            const nm = /^[A-Za-z_][\w\s*&:]*?\b((?:[A-Za-z_]\w*::)*~?[A-Za-z_]\w*)\s*\([^;{]*\)\s*;/.exec(L[k]);
            if (!nm) break;
            add(tree.func, nm[1], code[0]);
          }
        }
      }
      const m = /^\/\/\s*folded \((0x[0-9a-f]{6})\b/.exec(L[i]);
      if (!m) continue;
      let j = i + 1;
      while (j < L.length && /^\s*\/\//.test(L[j])) j++;
      const nm = /((?:[A-Za-z_]\w*::)*~?[A-Za-z_]\w*)\s*\(/.exec(L[j] ?? "");
      if (nm) add(tree.func, nm[1], parseInt(m[1], 16));
    }
  }
}
(function headers(dir) {
  for (const e of readdirSync(dir, { withFileTypes: true })) {
    if (e.isDirectory()) { headers(join(dir, e.name)); continue; }
    if (!e.name.endsWith(".h")) continue;
    for (const l of readFileSync(join(dir, e.name), "latin1").split(/\r?\n/)) {
      let m;
      if ((m = /^\s*(?:extern\s+)?[A-Za-z_][^;()=]*?\b([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*;\s*\/\/\s*(0x[0-9a-f]{6,7})\b/.exec(l)) && !/^\s*(typedef|return)\b/.test(l))
        add(tree.data, m[1], parseInt(m[2], 16));
      else if ((m = /^\s*(?:static\s+|virtual\s+|inline\s+)*(?:[A-Za-z_][\w\s*&:<>,]*?\b)?((?:[A-Za-z_]\w*::)*~?[A-Za-z_]\w*)\s*\([^;{]*\)\s*(?:const\s*)?;\s*\/\/\s*(?:folded \()?(0x[0-9a-f]{6})\b/.exec(l)))
        add(tree.func, m[1], parseInt(m[2], 16));
    }
  }
})(includeDir);
// Sum.exe's import table: "Name" -> the address of its IAT slot (what `call [__imp__Name@N]` reads).
const iat = new Map();
const pe = readFileSync(exePath);
const peNt = pe.readUInt32LE(0x3c), imageBase = pe.readUInt32LE(peNt + 24 + 28);
const peSecs = [];
for (let i = 0, o = peNt + 24 + pe.readUInt16LE(peNt + 20); i < pe.readUInt16LE(peNt + 6); i++, o += 40)
  peSecs.push({ va: pe.readUInt32LE(o + 12), size: Math.max(pe.readUInt32LE(o + 8), pe.readUInt32LE(o + 16)), raw: pe.readUInt32LE(o + 20), rawSize: pe.readUInt32LE(o + 16) });
const peOff = (rva) => { const s = peSecs.find((s) => rva >= s.va && rva < s.va + s.rawSize); return s ? rva - s.va + s.raw : -1; };
// The dword at a Sum.exe address (from the file), or null.
const origDword = (va) => { const o = peOff(va - imageBase); return o < 0 ? null : pe.readUInt32LE(o); };
{
  const impRva = pe.readUInt32LE(peNt + 24 + 104);
  const off = peOff;
  for (let d = off(impRva); d >= 0 && pe.readUInt32LE(d + 12); d += 20) {
    const ilt = pe.readUInt32LE(d) || pe.readUInt32LE(d + 16), first = pe.readUInt32LE(d + 16);
    for (let k = 0; ; k++) {
      const e = pe.readUInt32LE(off(ilt) + k * 4);
      if (!e) break;
      if (e & 0x80000000) continue; // by ordinal: no name
      const p = off(e) + 2;
      iat.set(pe.toString("latin1", p, pe.indexOf(0, p)), imageBase + first + k * 4);
    }
  }
}
// Compiler markers the C runtime defines as absolute values: __except_list is fs:[0] (the SEH chain), __fltused only
// pulls in floating-point support. And C runtime names the game's copy goes by in sum.symbols.
const absoluteMarkers = new Map([["__except_list", 0], ["__fltused", 0],
  // Named only in prose so far (the comment or decision that gives the address):
  ["__purecall", 0x56e903],                     // DECISIONS.md: the vtables' pure-virtual slot
  ["__except_handler3", 0x56de44],              // the handler RecordExceptionInfo (0x5419a0) pushes
  ["_GUID_SysMouse", 0x583310], ["_c_dfDIMouse", 0x5832c8], // unnamed_mouse_0x500a30.cpp, from dxguid / dinput.lib
  ["?equip_page_slots@@3QBHB", 0x5819d4],       // DECISIONS.md D5208
  ["?pf_debug_search_step@@YAXMMMMPAUpf_vis_node@@0HM@Z", 0x4f13e0], // pathfinding.h: folded into empty_function
  ["__imp__EAXDirectSoundCreate@12", 0x57e03c], // EAX.DLL ordinal 5, called by ds_init at 0x55c212
  // boss_ai.h: folded; boss_script_hook_stub (0x4a2360) calls empty_function in all four cases
  ["?serpent_script_hook@@YAXPAVCharacter@@@Z", 0x4f13e0], ["?tiger_script_hook@@YAXPAVCharacter@@@Z", 0x4f13e0],
  ["?phoenix_script_hook@@YAXPAVCharacter@@@Z", 0x4f13e0],
]);
const crtAliases = new Map([["__alloca_probe", "_chkstk"], ["??_L@YGXPAXIHP6EX0@Z1@Z", "eh_vector_ctor_iterator"],
  ["??_M@YGXPAXIHP6EX0@Z@Z", "eh_vector_dtor_iterator"]]);
// One address for a name, or null.
function addressOf(mangled, demangled, isFunc) {
  if (absoluteMarkers.has(mangled)) return absoluteMarkers.get(mangled);
  if (crtAliases.has(mangled) && symByName.has(crtAliases.get(mangled))) return symByName.get(crtAliases.get(mangled));
  const imp = /^__imp__(\w+?)(?:@\d+)?$/.exec(mangled);
  if (imp) return iat.get(mangled.slice(6)) ?? iat.get(imp[1]) ?? null; // Bink keeps its decoration: _BinkOpen@8
  // "operator new" -> operator_new in sum.symbols.
  const op = /^operator\s+(\w+)$/.exec(demangled.trim());
  if (op && symByName.has("operator_" + op[1])) return symByName.get("operator_" + op[1]);
  const d = demangled.replace(/\s+/g, "");
  for (const k of isFunc ? ["func", "data"] : ["data", "func"]) {
    const set = tree[k].get(d);
    if (set && set.size === 1) return [...set][0];
  }
  // A constructor or destructor declared in its class with an address comment (`~ai_state();  // 0x4f13e0`).
  const cd = /^(\w+)::(~?)\1$/.exec(d);
  if (cd && isFunc && tree.func.get(cd[2] + cd[1])?.size === 1) return [...tree.func.get(cd[2] + cd[1])][0];
  // sum.symbols: "a::b" -> "a__b"; C names without the underscore of their mangling, stdcall without the @N.
  const c = mangled.replace(/^_/, "").replace(/@\d+$/, "");
  for (const key of [d.replace(/::/g, "__"), c, mangled])
    if (/^[A-Za-z_]\w*$/.test(key) && symByName.has(key)) return symByName.get(key);
  return null;
}

// ---- per-file flags --------------------------------------------------------------------------------------
const unitFlags = new Map();
for (const l of readFileSync(join(decompRoot, "vc6-flags.txt"), "utf8").split(/\r?\n/)) {
  const m = /^([^#\s]\S*)\s+(\S.*?)\s*$/.exec(l);
  if (m) unitFlags.set(m[1].toLowerCase(), m[2]);
}
const flagsFor = (cpp) => {
  const rel = cpp.replace(/\\/g, "/").toLowerCase();
  for (const [k, v] of unitFlags) if (rel.endsWith("/" + k)) return v;
  return "/O2 /GX /MT";
};

// ---- pass 1: compile, and find each object's decompiled functions ------------------------------------------
const units = [];
for (const cpp of files) {
  const stem = basename(cpp).replace(/\.[^.]+$/, "");
  const env = { ...process.env, INCLUDE: `${includeDir};${join(VC, "INCLUDE")}`, PATH: `${join(VC, "BIN")};${process.env.PATH}` };
  try {
    execFileSync(join(VC, "BIN", "CL.EXE"), ["/nologo", "/c", ...flagsFor(cpp).split(/\s+/), "/FAsc", `/Fa${outDir}\\`, `/Fo${outDir}\\`, cpp],
      { env, cwd: outDir, encoding: "latin1", maxBuffer: 1 << 26 });
  } catch (e) {
    console.error(`${stem}: compile failed\n` + ((e.stdout ?? "") + (e.stderr ?? "")).split(/\r?\n/).filter((l) => /error/.test(l)).slice(0, 10).join("\n"));
    process.exit(1);
  }
  const objPath = join(outDir, stem + ".obj");
  const buf = readFileSync(objPath);
  const coff = readCoff(buf);
  const cod = readCod(readFileSync(join(outDir, stem + ".cod"), "latin1"));
  // Pair the source's `// 0xADDR` functions with the object's function symbols by name, in order (overloads).
  const byName = new Map();
  coff.syms.forEach((s, idx) => {
    if (!s || s.sec <= 0 || !(s.type & 0x20) || !/^\.text/.test(coff.sections[s.sec - 1].name)) return;
    const key = demangledOf(cod, s.name).replace(/\s+/g, "");
    if (!byName.has(key)) byName.set(key, []);
    byName.get(key).push(idx);
  });
  const text = readFileSync(cpp, "latin1");
  const used = new Map(), funcAddr = new Map(); // symbol index -> original address
  for (const f of sourceFunctions(text)) {
    const list = byName.get(f.name) ?? byName.get(f.name.replace(/^.*::/, "")) ?? [];
    const k = used.get(f.name) ?? 0; used.set(f.name, k + 1);
    if (list[k] != null) funcAddr.set(list[k], f.addr);
  }
  // Function-local statics with an address comment, and the guard of the ones built on first use
  // (`static GameTimer timer;  // 0x257e9e8 ... (guard 0x257e9e4)`), keyed by the enclosing function's name.
  // And tables whose comment block starts with their address (`// 0x58c080  45 x 0x40 ...` above the definition).
  const L = text.split(/\r?\n/), locals = [], tables = new Map();
  for (let i = 0; i < L.length; i++) {
    let m;
    if ((m = /^\s+static\s[^(;]*?\b([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*(?:=[^;]*|\([^;]*\))?;\s*\/\/\s*(0x[0-9a-f]{6,7})\b(.*)$/.exec(L[i]))) {
      let fn = null;
      for (let k = i - 1; k >= 0 && !fn; k--) {
        const f = /^(?![\s/#}{])[^;]*?\b(~?[A-Za-z_]\w*)\s*\(/.exec(L[k]);
        if (f) fn = f[1];
      }
      const g = /guard(?: byte)? (0x[0-9a-f]{6,7})/.exec(m[3]);
      locals.push({ fn, name: m[1], addr: parseInt(m[2], 16), guard: g ? parseInt(g[1], 16) : null });
    }
    // An object built with constructor arguments, its address on the line (`StringPool p(buf, 0x2000);  // 0x26b4268`):
    // a data address, not a function declaration with its address.
    if ((m = /^(?!return\b|typedef\b|extern\b)[A-Za-z_][\w\s:*&<>]*?\b([A-Za-z_]\w*)\s*\([^;]*\)\s*;\s*\/\/\s*(0x[0-9a-f]{6,7})\b/.exec(L[i])) && parseInt(m[2], 16) >= 0x57e000)
      tables.set(m[1], parseInt(m[2], 16));
    // A definition at the top level whose comment block has a line starting with an address (the nearest one).
    const d = /^(?!return\b|typedef\b|extern\b)[A-Za-z_][^(;=]*?\b([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*(?:=|;)/.exec(L[i]);
    // (Only without an address on the definition's own line; a data address, followed by a space: not a
    // continuation line such as `//    0x505530);`.)
    if (d && !/\(/.test(L[i].split("=")[0]) && /^\/\//.test(L[i - 1] ?? "") && !/\/\/\s*0x[0-9a-f]{6,7}\b/.test(L[i])) {
      for (let k = i - 1; k >= 0 && /^\/\//.test(L[k]); k--)
        if ((m = /^\/\/\s*(0x[0-9a-f]{6,7})(?:[\s:]|$)/.exec(L[k])) && parseInt(m[1], 16) >= 0x57e000) { tables.set(d[1], parseInt(m[1], 16)); break; }
    }
    // A table with an initialiser and its address on the line (`vector3 pts[10] = {  // 0x25ed270`).
    if (d && /=\s*\{/.test(L[i]) && (m = /\/\/\s*(0x[0-9a-f]{6,7})\b/.exec(L[i])) && parseInt(m[1], 16) >= 0x57e000)
      tables.set(d[1], parseInt(m[1], 16));
    // A global named after its address (`ca_character_2ee22f8`, a name of ours).
    if (d && (m = /_([0-9a-f]{6,7})$/.exec(d[1])) && parseInt(m[1], 16) >= 0x57e000 && !tables.has(d[1]))
      tables.set(d[1], parseInt(m[1], 16));
  }
  units.push({ stem, objPath, buf, coff, cod, funcAddr, locals, tables });
}
// External names some object of the build defines.
const definedHere = new Set();
for (const u of units) for (const s of u.coff.syms) if (s && s.sec > 0 && s.cls === 2) definedHere.add(s.name);

// ---- pass 2: make every symbol mean the original program unless injected -----------------------------------
const exports = [], report = [], unresolved = new Map(), exported = new Set(), kept = [], shortOnes = [], suspicious = [], stale = [];
// Which functions are injected. Every reference to one (calls from other injected functions too) uses its original
// address and so goes through the loader's jmp: [Hybrid] Skip / Only then decide for each function on its own. Only
// the export alias names the compiled code. (The function's own symbol stays: an inline function's COMDAT section
// needs it.)
// Functions whose compiled bytes equal the original's (the last strict sweep, src/decomp/vc6-matches.txt). In these
// the original's code holds, at the same offset, the address of each string, constant and global it uses: their
// references are pointed there, so the game's own data is used (one SummonerFix may have patched, such as
// TestSaves' "savegame" -> "savetest") and not a copy in the DLL.
const matched = new Set();
for (const l of readFileSync(join(decompRoot, "vc6-matches.txt"), "latin1").split(/\r?\n/)) {
  const m = /^\S+\s+(0x[0-9a-f]+)\s+match\b/.exec(l);
  if (m) matched.add(parseInt(m[1], 16));
}
// Functions the shipped Sum.exe has edited in the file, which the decomp writes unpatched: they stay original.
// The no-CD edit (summoner_cd.cpp, DECISIONS.md D2600): the decompiled ones would ask for the disc.
const shippedPatched = new Set([0x43e660 /* cd_find_drive */, 0x43e8e0 /* cd_set_root */]);
const decompNames = new Map(); // external name of every decompiled function -> its original address
for (const u of units) {
  u.inject = new Map(); // symbol index -> original address
  for (const [idx, addr] of u.funcAddr) {
    const s = u.coff.syms[idx];
    // (An original shorter than the jmp stays: the jmp would overwrite the next function.)
    if (s.cls === 2) decompNames.set(s.name, addr);
    if (roomAt(addr) < 5) shortOnes.push(`0x${addr.toString(16)} ${demangledOf(u.cod, s.name)}`);
    if ((only && !only.has(addr)) || exported.has(addr) || roomAt(addr) < 5 || shippedPatched.has(addr)) continue;
    exported.add(addr);
    u.inject.set(idx, addr);
    exports.push({ addr, mangled: s.name, name: demangledOf(u.cod, s.name), sym: exportName(addr) });
  }
}
for (const { stem, objPath, buf, coff, cod, funcAddr, locals, tables, inject } of units) {
  const symPtr = buf.readUInt32LE(8);
  // This file's own addresses for a data symbol: a function-local static (?name@?N??function@...) or its guard
  // (?$S<n>@?N??function@...), or a table.
  const ownAddress = (mangled, demangled) => {
    // VC6 writes _?name@?<scope>??function@...; a scope above 9 is letters and @ (?L@).
    const m = /^_?\?(\$S\d+|[A-Za-z_]\w*)@\?(?:\d|[A-P]+@)\?\?([A-Za-z_]\w*)@/.exec(mangled);
    if (m && m[1].startsWith("$S")) return locals.find((l) => l.fn === m[2] && l.guard != null)?.guard ?? null;
    if (m) return locals.find((l) => l.fn === m[2] && l.name === m[1])?.addr ?? null;
    return tables.get(demangled) ?? null;
  };
  // New names go at the end of the string table (new symbols go after the old ones, before the strings).
  const nSym = buf.readUInt32LE(12), strTab = symPtr + nSym * 18;
  let strings = buf.subarray(strTab);
  const addString = (s) => {
    const off = strings.length;
    strings = Buffer.concat([strings, Buffer.from(s + "\0", "latin1")]);
    strings.writeUInt32LE(strings.length, 0);
    return off;
  };
  // LINK reads an absolute symbol that is static as 0, so a static one becomes external, under a name of its own
  // (two files may have statics of the same name).
  // rename: also an external one (a reference to an injected function: its own name is the DLL's code).
  const makeAbsolute = (idx, addr, rename = false) => {
    const o = symPtr + idx * 18;
    // A function that lands outside the code, or data inside it, means a wrong address: report it.
    const fn = (buf.readUInt16LE(o + 14) & 0x20) !== 0, inCode = addr >= 0x401000 && addr < 0x57db40;
    if (addr !== 0 && fn !== inCode && !/^__imp_/.test(coff.syms[idx].name)) suspicious.push(`${stem}: ${demangledOf(cod, coff.syms[idx].name)} -> 0x${addr.toString(16)}`);
    buf.writeUInt32LE(addr >>> 0, o + 8);
    buf.writeInt16LE(-1, o + 12);
    if (buf.readUInt8(o + 16) === 3 || rename) {
      buf.writeUInt32LE(0, o);
      buf.writeUInt32LE(addString(`$hybrid_${stem}_s${idx}`), o + 4);
    }
    buf.writeUInt8(2, o + 16);
  };
  let injected = 0, redirected = 0;
  const aliases = []; // [symbol index, address]: each injected function gets an external name f_<address>_<size>
  coff.syms.forEach((s, idx) => {
    if (!s) return;
    const isFunc = (s.type & 0x20) !== 0;
    const sec = s.sec > 0 ? coff.sections[s.sec - 1] : null;
    if (funcAddr.has(idx)) {
      const addr = funcAddr.get(idx);
      if (inject.has(idx)) {
        aliases.push([idx, addr, s.sec, s.value]);
        injected++;
      } else redirected++; // kept (an inline function's COMDAT needs its symbol), never referenced
      return;
    }
    if (s.sec === 0 && s.cls === 2 && s.value === 0) {
      // Undefined: an injected function of another object (its original address), another object of the build,
      // else the original program.
      if (decompNames.has(s.name)) { makeAbsolute(idx, decompNames.get(s.name), true); redirected++; return; }
      if (definedHere.has(s.name)) return;
      const addr = addressOf(s.name, demangledOf(cod, s.name), isFunc);
      if (addr == null) { unresolved.set(s.name, demangledOf(cod, s.name)); return; }
      makeAbsolute(idx, addr);
      redirected++;
      return;
    }
    // A global defined here with an address comment: the game's own variable.
    if (sec && /^\.(data|bss)/.test(sec.name) && (s.cls === 2 || s.cls === 3) && !/^\./.test(s.name) && !/^\?\?_C@/.test(s.name)) {
      if (/^\$SG\d+$/.test(s.name)) return; // a string literal
      const addr = ownAddress(s.name, demangledOf(cod, s.name)) ?? addressOf(s.name, demangledOf(cod, s.name), false);
      if (addr != null) { makeAbsolute(idx, addr); redirected++; }
      else {
        // Its size: up to the next symbol of the section, or the section's end.
        let end = sec.size;
        for (const t of coff.syms) if (t && t.sec === s.sec && t.value > s.value && t.value < end) end = t.value;
        kept.push({ stem, name: demangledOf(cod, s.name), size: end - s.value, bss: /^\.bss/.test(sec.name) });
      }
    }
  });
  // Relocations through a .data / .bss section symbol (statics without a symbol of their own) would reach the DLL's
  // copy: report them.
  for (const sec of coff.sections) {
    if (!/^\.text/.test(sec.name)) continue;
    for (const r of sec.relocs) {
      const t = coff.syms[r.sym];
      if (t && /^\.(data|bss)$/.test(t.name) && t.cls === 3) report.push(`${stem}: a reference through ${t.name}+${sec.data.readUInt32LE(r.off).toString(16)} at ${sec.name}+0x${r.off.toString(16)} uses the DLL's copy`);
    }
  }
  // LINK treats an absolute symbol as an RVA in a rel32 relocation (a call or jmp) and adds the image base, while a
  // dir32 one gets the value as it is. So each absolute symbol reached by a rel32 gets a shadow symbol of value
  // (address - BASE), and those relocations are pointed at it. The symbols go after the old ones, before the strings.
  // (A shadow must be external: LINK reads a static absolute symbol in a rel32 as 0.) Their names, unique per object,
  // go at the end of the string table.
  const shadows = new Map(), extra = [];
  // The byte-matching functions of each section: [start, end, original address].
  const matchedRanges = new Map();
  coff.syms.forEach((s, idx) => {
    if (!s || !funcAddr.has(idx) || !matched.has(funcAddr.get(idx))) return;
    const sec = coff.sections[s.sec - 1], addr = funcAddr.get(idx);
    let end = sec.size;
    for (const t of coff.syms) if (t && t.sec === s.sec && (t.type & 0x20) && t.value > s.value && t.value < end) end = t.value;
    // The list may be older than the source: check the bytes now, relocated fields masked. Trailing padding of the
    // compiled function is not compared.
    const masked = new Set();
    for (const r of sec.relocs) if (r.off >= s.value && r.off < end) for (let k = 0; k < 4; k++) masked.add(r.off + k);
    let last = end;
    while (last > s.value && (sec.data[last - 1] === 0x90 || sec.data[last - 1] === 0xcc) && !masked.has(last - 1)) last--;
    for (let o = s.value; o < last; o++) {
      if (masked.has(o)) continue;
      const fo = peOff(addr + (o - s.value) - imageBase);
      if (fo < 0 || pe[fo] !== sec.data[o]) { stale.push(`0x${addr.toString(16)} ${demangledOf(cod, s.name)}`); return; }
    }
    if (!matchedRanges.has(s.sec)) matchedRanges.set(s.sec, []);
    matchedRanges.get(s.sec).push([s.value, end, addr]);
  });
  let constantsMoved = 0;
  // What the byte-matching functions teach: the original address of each named data symbol they use (a vtable, a
  // string, a constant). Every other reference to that symbol in this object, in a function that does not match,
  // is pointed there too (the address plus that reference's own addend), so replaced and original code share one
  // copy (a constructor that does not match then installs the game's vtable, not the DLL's). Section symbols
  // (.data + offset) are left alone: their layout in the DLL need not be the original's.
  const learned = new Map(); // symbol index -> original address (null when two references disagree)
  for (let i = 0, o = 20 + buf.readUInt16LE(16); i < buf.readUInt16LE(2); i++, o += 40) {
    const rp = buf.readUInt32LE(o + 24), nr = buf.readUInt16LE(o + 32), raw = buf.readUInt32LE(o + 20);
    for (let r = 0; r < nr; r++) {
      const ro = rp + r * 10, sym = buf.readUInt32LE(ro + 4), type = buf.readUInt16LE(ro + 8);
      const tsec = buf.readInt16LE(symPtr + sym * 18 + 12), tname = coff.syms[sym]?.name ?? ".";
      if (type !== 0x06 || tsec <= 0 || /^\.text/.test(coff.sections[tsec - 1].name) || tname.startsWith(".")) continue;
      const roff = buf.readUInt32LE(ro);
      const f = (matchedRanges.get(i + 1) ?? []).find(([a, b]) => roff >= a && roff < b);
      const va = f ? origDword(f[2] + roff - f[0]) : null;
      if (va == null || va < 0x57e000 || va >= 0x3000000) continue;
      const base = (va - buf.readUInt32LE(raw + roff)) >>> 0;
      learned.set(sym, learned.has(sym) && learned.get(sym) !== base ? null : base);
    }
  }
  for (let i = 0, o = 20 + buf.readUInt16LE(16); i < buf.readUInt16LE(2); i++, o += 40) {
    const rp = buf.readUInt32LE(o + 24), nr = buf.readUInt16LE(o + 32);
    for (let r = 0; r < nr; r++) {
      const ro = rp + r * 10, sym = buf.readUInt32LE(ro + 4), type = buf.readUInt16LE(ro + 8);
      // A reference outside the matching functions to a symbol they taught: its original address (addend kept).
      if (type === 0x06 && learned.get(sym) != null) {
        const roff = buf.readUInt32LE(ro);
        if (!(matchedRanges.get(i + 1) ?? []).some(([a, b]) => roff >= a && roff < b)) {
          const base = learned.get(sym), key = `l${base}`;
          if (!shadows.has(key)) {
            const e = Buffer.alloc(18);
            e.writeUInt32LE(addString(`$hybrid_${stem}_l${shadows.size}`), 4);
            e.writeUInt32LE(base, 8);
            e.writeInt16LE(-1, 12);
            e.writeUInt8(2, 16);
            shadows.set(key, nSym + extra.length);
            extra.push(e);
          }
          buf.writeUInt32LE(shadows.get(key), ro + 4);
          constantsMoved++;
          continue;
        }
      }
      // In a byte-matching function, a dir32 reference to data in this object (a string, a float, a global with no
      // address): the address the original has at the same place.
      const tsec = buf.readInt16LE(symPtr + sym * 18 + 12);
      if (type === 0x06 && tsec > 0 && !/^\.text/.test(coff.sections[tsec - 1].name)) {
        const roff = buf.readUInt32LE(ro);
        const f = (matchedRanges.get(i + 1) ?? []).find(([a, b]) => roff >= a && roff < b);
        const va = f ? origDword(f[2] + roff - f[0]) : null;
        if (va != null && va >= 0x57e000 && va < 0x3000000) {
          const key = `c${va}`;
          if (!shadows.has(key)) {
            const e = Buffer.alloc(18);
            e.writeUInt32LE(addString(`$hybrid_${stem}_c${shadows.size}`), 4);
            e.writeUInt32LE(va, 8);
            e.writeInt16LE(-1, 12);
            e.writeUInt8(2, 16);
            shadows.set(key, nSym + extra.length);
            extra.push(e);
          }
          buf.writeUInt32LE(shadows.get(key), ro + 4);
          // The addend in the code: the original's value is the whole address.
          const sec = coff.sections[i];
          buf.writeUInt32LE(0, sec.raw + roff);
          constantsMoved++;
          continue;
        }
      }
      // A reference (dir32 or rel32) to a function decompiled in this object, injected or not: its original address.
      if (funcAddr.has(sym) && (type === 0x06 || type === 0x14)) {
        const key = `${sym}:${type}`;
        if (!shadows.has(key)) {
          const e = Buffer.alloc(18);
          e.writeUInt32LE(addString(`$hybrid_${stem}_o${shadows.size}`), 4);
          e.writeUInt32LE(((type === 0x14 ? funcAddr.get(sym) - BASE : funcAddr.get(sym))) >>> 0, 8);
          e.writeInt16LE(-1, 12);
          e.writeUInt8(2, 16);
          shadows.set(key, nSym + extra.length);
          extra.push(e);
        }
        buf.writeUInt32LE(shadows.get(key), ro + 4);
        continue;
      }
      if (type !== 0x14 || buf.readInt16LE(symPtr + sym * 18 + 12) !== -1) continue;
      if (!shadows.has(sym)) {
        const e = Buffer.alloc(18);
        e.writeUInt32LE(addString(`$hybrid_${stem}_${shadows.size}`), 4);
        e.writeUInt32LE((buf.readUInt32LE(symPtr + sym * 18 + 8) - BASE) >>> 0, 8);
        e.writeInt16LE(-1, 12);
        e.writeUInt8(2, 16);
        shadows.set(sym, nSym + extra.length);
        extra.push(e);
      }
      buf.writeUInt32LE(shadows.get(sym), ro + 4);
    }
  }
  // The export names: a static function (one file's helper) cannot be exported by its own name, and two files may
  // have statics of the same name, so each injected function is exported through an alias, _f_<address>_<size>.
  for (const [, addr, sec, value] of aliases) {
    const e = Buffer.alloc(18);
    e.writeUInt32LE(addString(`_${exportName(addr)}`), 4);
    e.writeUInt32LE(value, 8);
    e.writeInt16LE(sec, 12);
    e.writeUInt16LE(0x20, 14);
    e.writeUInt8(2, 16);
    extra.push(e);
  }
  buf.writeUInt32LE(nSym + extra.length, 12);
  writeFileSync(objPath, Buffer.concat([buf.subarray(0, strTab), ...extra, strings]));
  if (verbose || units.length <= 10) console.log(`${stem}: ${injected} functions injected, ${redirected} symbols pointed at Sum.exe, ${constantsMoved} data references of matching functions moved to the original`);
}

if (unresolved.size) {
  console.log(`\n${unresolved.size} names have no Sum.exe address${force ? " (--force: calls to them go to 0)" : ""}:`);
  for (const [m, d] of unresolved) console.log(`  ${d}   (${m})`);
}
for (const r of report) console.log(r);
if (stale.length)
  console.log(`\n${stale.length} functions vc6-matches.txt lists as matching differ now; their data stays in the DLL` +
    ` (run a strict sweep):\n  ` + stale.join("\n  "));
// A variable the game also has (not a function-local static: `?name@?N??function`, nor a guard) that stays in the
// DLL splits the state between replaced and original functions: give it its address.
const keptVars = kept.filter((k) => k.bss && !/^\?/.test(k.name));
if (keptVars.length)
  console.log(`\n${keptVars.length} global variables keep a copy in the DLL (no address in the source); replaced and original` +
    ` functions would each see their own copy:\n  ` + keptVars.map((k) => `${k.stem}: ${k.name}`).join("\n  "));
if (suspicious.length) console.log(`\n${suspicious.length} names got an address of the wrong kind (a function outside the code, data inside it):\n  ` + suspicious.join("\n  "));
if (shortOnes.length)
  console.log(`\n${shortOnes.length} originals are shorter than a jmp and stay original` + (verbose ? ":\n  " + shortOnes.join("\n  ") : ""));
// Globals with no address keep a copy in the DLL: harmless for a constant table, a divergence for a variable the
// original also uses (the injected code would read and write its own copy). Listed in hybrid-globals.txt.
if (kept.length) {
  kept.sort((a, b) => b.size - a.size);
  writeFileSync(join(outDir, "hybrid-globals.txt"), kept.map((k) => `${k.size}\t${k.bss ? "bss " : "data"}\t${k.stem}\t${k.name}`).join("\n") + "\n");
  const total = kept.reduce((n, k) => n + k.size, 0);
  console.log(`\n${kept.length} globals have no address and keep a copy in the DLL (${total} bytes; hybrid-globals.txt)` +
    (verbose ? ":\n" + kept.map((k) => `  ${k.stem}: ${k.name} (${k.size} bytes)`).join("\n") : ""));
}

// ---- link ---------------------------------------------------------------------------------------------------
// The exports go in a .def file: thousands of /EXPORT options overflow the command line.
writeFileSync(join(outDir, "hybrid.def"), "EXPORTS\r\n" + exports.map((e) => `  ${e.sym}\r\n`).join(""));
writeFileSync(join(outDir, "hybrid.rsp"), units.map((u) => `"${u.objPath}"`).join("\r\n") + "\r\n");
const linkArgs = ["/nologo", "/DLL", "/NOENTRY", "/NODEFAULTLIB", `/BASE:0x${BASE.toString(16)}`, "/FIXED", "/MAP:hybrid.map",
  "/DEF:hybrid.def", "/OUT:hybrid.dll", ...(force ? ["/FORCE:UNRESOLVED"] : []), "@hybrid.rsp"];
try {
  const out = execFileSync(join(VC, "BIN", "LINK.EXE"), linkArgs, { cwd: outDir, env: { ...process.env, PATH: `${join(VC, "BIN")};${process.env.PATH}` }, encoding: "latin1", maxBuffer: 1 << 26 });
  if (out.trim()) console.log(out.trim().split(/\r?\n/).filter((l) => !/LNK4088|unresolved external/.test(l) || verbose).join("\n"));
} catch (e) {
  const text = ((e.stdout ?? "") + (e.stderr ?? "")).trim();
  if (!(force && existsSync(join(outDir, "hybrid.dll")) && !/fatal error (?!LNK1120)/.test(text))) {
    console.error(`link failed (exit ${e.status}):\n` + text.split(/\r?\n/).slice(0, 40).join("\n"));
    process.exit(1);
  }
}
// What a function returns, from its mangled name (for dual calls, src/summonerfix/dualcall.c): v nothing (void, a
// constructor or destructor), f a float in st0, b a bool or char in al, s a short in ax, i a value in eax; C names
// (no mangling): i.
const returnKind = (m) => {
  const r = /@@(?:Y[AGI]|S[AGI]|[A-Z][AB][AEGI])(.*)$/.exec(m);
  if (!r) return "i";
  const t = r[1];
  if (t.startsWith("X") || t.startsWith("@")) return "v";
  if (/^[MNO]/.test(t)) return "f";
  if (t.startsWith("_N") || /^[DE]/.test(t)) return "b"; // bool, char: al
  if (/^[FG]/.test(t)) return "s"; // short: ax
  return "i";
};
// hybrid.txt: address, return kind, name.
writeFileSync(join(outDir, "hybrid.txt"), exports.map((e) => `0x${e.addr.toString(16)} ${returnKind(e.mangled)} ${e.name}`).join("\n") + "\n");
console.log(`\n${join(outDir, "hybrid.dll")}: ${exports.length} functions from ${units.length} files (list in hybrid.txt)`);
