// Fast local disassembly of Sum.exe with llvm-objdump (from LLVM-MinGW), annotated with symbol names.
// The whole binary takes about 2 s, against minutes per function through REA (tools/disasm.mjs).
//
//   node tools/asm.mjs fn <addr|name> [max_lines]   one function, up to the next known function start
//   node tools/asm.mjs grep <regex> [max]           matching instructions in game code, with the containing function
//   node tools/asm.mjs callers <addr|name>          call sites of a function, with the containing function
//   node tools/asm.mjs strings <lo> <hi> [--unnamed] strings referenced by each function in an address range
//
// The listing is cached in build/sum.asm (git-ignored) and rebuilt when Sum.exe is newer.
import { readFileSync, existsSync, statSync, mkdirSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { loadSymbols, loadFunctions } from "./symbols.mjs";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const exePath = join(repoRoot, "Summoner", "Sum.exe");
const asmPath = join(repoRoot, "build", "sum.asm");

if (!existsSync(asmPath) || statSync(asmPath).mtimeMs < statSync(exePath).mtimeMs) {
  mkdirSync(dirname(asmPath), { recursive: true });
  const out = execFileSync("llvm-objdump", ["-d", "--no-show-raw-insn", "-M", "intel", exePath], { maxBuffer: 1 << 30 });
  (await import("node:fs")).writeFileSync(asmPath, out);
}

const exe = readFileSync(exePath);
const { symbols } = loadSymbols();
const byAddr = new Map(symbols.map((s) => [s.addr, s.name]));
const byName = new Map(symbols.map((s) => [s.name, s.addr]));
const starts = [...new Set([...(loadFunctions() ?? []).map((f) => f.addr), ...symbols.filter((s) => s.kind === "func").map((s) => s.addr)])].sort((a, b) => a - b);

// Instructions: [{va, text}] in address order.
const ins = [];
for (const line of readFileSync(asmPath, "latin1").split(/\r?\n/)) {
  const m = /^\s+([0-9a-f]+):\s+(.*)$/.exec(line);
  if (m) ins.push({ va: parseInt(m[1], 16), text: m[2].replace(/\s+<\.text\+0x[0-9a-f]+>/g, "").replace(/\s+/g, " ").trim() });
}
const indexOf = (va) => { let lo = 0, hi = ins.length - 1; while (lo < hi) { const mid = (lo + hi) >> 1; if (ins[mid].va < va) lo = mid + 1; else hi = mid; } return lo; };
const funcOf = (va) => { let lo = 0, hi = starts.length - 1, r = -1; while (lo <= hi) { const mid = (lo + hi) >> 1; if (starts[mid] <= va) { r = starts[mid]; lo = mid + 1; } else hi = mid - 1; } return r; };
const fname = (va) => byAddr.get(va) ?? `FUN_${va.toString(16).padStart(8, "0")}`;

function stringAt(va) {
  const o = va - 0x400000;
  if (o < 0x17f000 || o >= exe.length) return null; // .rdata/.data only
  const end = exe.indexOf(0, o);
  if (end < 0 || end - o < 3 || end - o > 200) return null;
  const s = exe.toString("latin1", o, end);
  return /^[\x20-\x7e\t\r\n]+$/.test(s) ? s : null;
}
// localize_strings[id] (0x25cd4dc + 4*id) -> English text from extracted/tables/strings.tbl, when extracted
// (node tools/vpp.mjs extract Summoner/tables.vpp strings.tbl).
const LOC = 0x25cd4dc, locText = new Map();
const tblPath = join(repoRoot, "extracted", "tables", "strings.tbl");
if (existsSync(tblPath)) {
  const english = readFileSync(tblPath, "latin1").split(/^#(?!english)/im)[0];
  for (const m of english.matchAll(/^(\d+),\s*\{([^}]*)\}/gm)) locText.set(+m[1], m[2].replace(/\s+/g, " "));
}
const locAt = (va) => (va > LOC && va < LOC + 4000 && (va - LOC) % 4 === 0 ? locText.get((va - LOC) / 4) ?? null : null);
function annotate(text) {
  const notes = [];
  for (const m of text.matchAll(/0x[0-9a-f]{6,8}/gi)) {
    const v = parseInt(m[0], 16), n = byAddr.get(v), s = stringAt(v), l = locAt(v);
    if (l) notes.push(`L${(v - LOC) / 4} ` + JSON.stringify(l.slice(0, 60)));
    else if (n) notes.push(n);
    else if (s) notes.push(JSON.stringify(s.length > 70 ? s.slice(0, 70) + "..." : s));
  }
  return notes.length ? `${text.padEnd(44)} ; ${notes.join(", ")}` : text;
}
const parseTarget = (t) => (byName.has(t) ? byName.get(t) : parseInt(t.replace(/^0x/, ""), 16));
const show = (i) => console.log(`${ins[i].va.toString(16)}: ${annotate(ins[i].text)}`);

const [cmd, arg, maxArg] = process.argv.slice(2);
if (cmd === "fn") {
  const va = parseTarget(arg), max = parseInt(maxArg ?? "3000", 10);
  const next = starts.find((s) => s > va) ?? Infinity;
  console.log(`; ${fname(va)}`);
  if (ins[indexOf(va)]?.va === va) {
    for (let i = indexOf(va), n = 0; i < ins.length && ins[i].va < next && n < max; i++, n++) show(i);
  } else {
    // The whole-file linear sweep ran through data (a jump table before the function) and did not land on its
    // first byte (e.g. 0x465df0's `push ecx` was swallowed): disassemble the function's own range instead.
    const stop = Number.isFinite(next) ? next : va + 0x4000;
    const out = execFileSync("llvm-objdump", ["-d", "--no-show-raw-insn", "-M", "intel", `--start-address=0x${va.toString(16)}`,
      `--stop-address=0x${stop.toString(16)}`, exePath], { maxBuffer: 1 << 26 }).toString("latin1");
    let n = 0;
    for (const line of out.split(/\r?\n/)) {
      const m = /^\s+([0-9a-f]+):\s+(.*)$/.exec(line);
      if (!m || n++ >= max) continue;
      console.log(`${m[1]}: ${annotate(m[2].replace(/\s+<\.text\+0x[0-9a-f]+>/g, "").replace(/\s+/g, " ").trim())}`);
    }
  }
} else if (cmd === "grep") {
  const re = new RegExp(arg, "i"), max = parseInt(maxArg ?? "200", 10);
  let n = 0;
  for (let i = 0; i < ins.length && n < max; i++) {
    if (ins[i].va >= 0x56c740) break; // stop at the C runtime
    if (re.test(ins[i].text)) { console.log(`${ins[i].va.toString(16)} [${fname(funcOf(ins[i].va))}]: ${annotate(ins[i].text)}`); n++; }
  }
} else if (cmd === "callers") {
  const hex = "0x" + parseTarget(arg).toString(16);
  for (let i = 0; i < ins.length; i++)
    if (ins[i].text === `call ${hex}` || ins[i].text === `jmp ${hex}`) console.log(`${ins[i].va.toString(16)} [${fname(funcOf(ins[i].va))}]: ${ins[i].text}`);
} else if (cmd === "strings") {
  // Strings referenced per function in [lo, hi); --unnamed skips functions that already have a symbol.
  const lo = parseInt(arg, 16), hi = parseInt(maxArg, 16), unnamed = process.argv.includes("--unnamed");
  const map = new Map();
  for (let i = indexOf(lo); i < ins.length && ins[i].va < hi; i++) {
    for (const m of ins[i].text.matchAll(/0x[0-9a-f]{6,7}\b/g)) {
      const v = parseInt(m[0], 16), l = locAt(v), s = l ? `L"${l}"` : stringAt(v);
      if (!s || !/[a-z]{3}/i.test(s)) continue;
      const f = funcOf(ins[i].va);
      if (unnamed && byAddr.has(f)) continue;
      if (!map.has(f)) map.set(f, new Set());
      map.get(f).add(s.replace(/\s+/g, " ").slice(0, 60));
    }
  }
  for (const [f, ss] of [...map].sort((a, b) => a[0] - b[0])) console.log(`${f.toString(16)} ${fname(f)} | ${[...ss].join(" || ")}`);
} else {
  console.log("usage: node tools/asm.mjs fn <addr|name> [max] | grep <regex> [max] | callers <addr|name> | strings <lo> <hi> [--unnamed]");
}
