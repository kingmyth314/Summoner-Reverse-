// Decompile Sum.exe functions through the REA daemon and substitute our names from symbols/.
//
// REA's Ghidra on Windows can't rename (database mutation is not admitted), so names are applied to
// the output instead: FUN_00501510 -> FrameDone, DAT_005a5740 -> reported_frametime, and addresses
// inside known structures (symbols/types.h) -> gr_screen.max_w, etc.
//
// usage: node tools/decomp.mjs <address|name> [...]      pseudocode with names applied
//        node tools/decomp.mjs --asm <address|name>       assembly with names applied
//        node tools/decomp.mjs --callees <address|name>   named/unnamed direct callees
import { readFileSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { reaCall } from "./lib/rea-queue.mjs";
import { loadSymbols } from "./symbols.mjs";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const { symbols } = loadSymbols();
const byAddr = new Map(symbols.map((s) => [s.addr, s]));
const byName = new Map(symbols.map((s) => [s.name, s]));

// Struct layouts from types.h: "typedef struct X {" ... "<type> field; // +0xNN".
const structs = new Map();
{
  let cur = null;
  for (const line of readFileSync(join(repoRoot, "symbols", "types.h"), "utf8").split(/\r?\n/)) {
    const start = line.match(/^typedef struct (\w+)/);
    if (start) { cur = { name: start[1], fields: [] }; structs.set(cur.name, cur); continue; }
    if (cur && /^}/.test(line)) { cur = null; continue; }
    const f = cur && line.match(/^\s*[\w\s*()]+?\b(\w+)\s*(?:\(\*\)\([^)]*\))?\s*;\s*\/\/\s*\+0x([0-9a-fA-F]+)/);
    if (f) cur.fields.push({ name: f[1], off: parseInt(f[2], 16) });
    const fp = cur && line.match(/\(\*(\w+)\)\([^)]*\);\s*\/\/\s*\+0x([0-9a-fA-F]+)/);
    if (fp) cur.fields.push({ name: fp[1], off: parseInt(fp[2], 16) });
  }
}
// Expand struct-typed data symbols into per-field names.
const fieldNames = new Map();
for (const s of symbols) {
  const t = s.kind.startsWith("data:") ? s.kind.slice(5) : null;
  const st = t && structs.get(t);
  if (st) for (const f of st.fields) fieldNames.set(s.addr + f.off, `${s.name}.${f.name}`);
}

function nameFor(addr) {
  return byAddr.get(addr)?.name ?? fieldNames.get(addr) ?? null;
}

// Replace Ghidra auto-names (FUN_/DAT_/_DAT_/LAB_/PTR_DAT_ + 8 hex digits) and bare 0x addresses.
export function applyNames(text) {
  const unnamed = new Set();
  let out = text.replace(/\b(_?)(FUN|DAT|LAB|PTR_DAT|PTR_FUN)_([0-9a-fA-F]{8})\b/g, (m, us, kind, hex) => {
    const n = nameFor(parseInt(hex, 16));
    if (n) return n;
    if (kind === "FUN") unnamed.add(parseInt(hex, 16));
    return m;
  });
  out = out.replace(/\b0x0*([0-9a-fA-F]{6,7})\b/g, (m, hex) => {
    const n = nameFor(parseInt(hex, 16));
    return n ? `${m}<${n}>` : m;
  });
  return { text: out, unnamed };
}

function resolveTarget(t) {
  if (/^0x[0-9a-f]+$/i.test(t)) return "0x" + parseInt(t, 16).toString(16);
  const s = byName.get(t);
  if (!s) throw new Error(`unknown symbol ${t}`);
  return s.addrText;
}

const args = process.argv.slice(2);
const mode = args[0]?.startsWith("--") ? args.shift() : "--c";
for (const target of args) {
  const addr = resolveTarget(target);
  const label = nameFor(parseInt(addr, 16));
  if (mode === "--callees") {
    const r = await reaCall("procedure_callees", { procedure: addr });
    const list = Array.isArray(r.json) ? r.json : [r.json].filter(Boolean);
    console.log(`=== callees of ${label ?? addr}`);
    for (const c of list) {
      const a = typeof c === "string" ? c : c.address ?? c;
      const n = /^0x/.test(a) ? nameFor(parseInt(a, 16)) : null;
      console.log(`  ${a}  ${n ?? "(unnamed)"}`);
    }
    continue;
  }
  const r = await reaCall(mode === "--asm" ? "procedure_assembly" : "procedure_pseudo_code", { procedure: addr });
  if (r.isError) { console.log(`=== ${addr}: ${r.text.slice(0, 300)}`); continue; }
  const raw = typeof r.json === "string" ? r.json : r.json?.value ?? JSON.stringify(r.json);
  const { text, unnamed } = applyNames(raw);
  console.log(`=== ${label ?? addr} (${addr})`);
  console.log(text.replace(/\r\n/g, "\n").trim());
  if (unnamed.size) console.log(`\n-- unnamed functions referenced: ${[...unnamed].map((a) => "0x" + a.toString(16)).join(" ")}`);
}
