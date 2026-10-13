// Linear disassembly through REA, for code Ghidra did not make into a function (pointer-only targets
// such as game-state handlers and console callbacks), where tools/decomp.mjs fails.
// usage: node tools/disasm.mjs <addr|name> [max_instructions=200] [--follow-jumps]
// Stops at the first RET unless a later conditional jump still targets code past it.
// Call/jump targets are shown with their symbol names; immediates that point at strings are printed.
import { readFileSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { reaCall } from "./lib/rea-queue.mjs";
import { loadSymbols } from "./symbols.mjs";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const exe = readFileSync(join(repoRoot, "Summoner", "Sum.exe"));
const { symbols } = loadSymbols();
const byAddr = new Map(symbols.map((s) => [s.addr, s.name]));
const byName = new Map(symbols.map((s) => [s.name, s.addr]));

const [target, maxArg] = process.argv.slice(2);
let addr = byName.has(target) ? byName.get(target) : parseInt(target, 16);
const max = parseInt(maxArg ?? "200", 10);

function stringAt(va) {
  const o = va - 0x400000;
  if (o < 0x17f000 || o >= exe.length) return null; // .rdata/.data only
  const end = exe.indexOf(0, o);
  if (end < 0 || end - o < 3 || end - o > 200) return null;
  const s = exe.toString("latin1", o, end);
  return /^[\x20-\x7e\t\r\n]+$/.test(s) ? s : null;
}
const label = (va) => byAddr.get(va) ?? null;

let furthest = addr;
for (let i = 0; i < max; i++) {
  const r = await reaCall("inspect_native_instruction", { address: "0x" + addr.toString(16) });
  const ins = r.json;
  if (!ins || ins.status !== "decoded") { console.log(`0x${addr.toString(16)}: (could not decode)`); break; }
  let text = ins.raw_disassembly;
  const notes = [];
  for (const m of text.matchAll(/0x[0-9a-f]{6,8}/gi)) {
    const v = parseInt(m[0], 16);
    const n = label(v), s = stringAt(v);
    if (n) notes.push(n);
    else if (s) notes.push(JSON.stringify(s.length > 70 ? s.slice(0, 70) + "..." : s));
  }
  console.log(`0x${addr.toString(16)}: ${text.padEnd(40)}${notes.length ? " ; " + notes.join(", ") : ""}`);
  for (const d of ins.flow?.direct_destinations ?? []) {
    const v = parseInt(d, 16);
    if (ins.flow.kind !== "call" && v > furthest) furthest = v;
  }
  const next = addr + ins.length;
  if (ins.mnemonic === "RET" && next > furthest) break;
  if (ins.mnemonic === "JMP" && !ins.flow?.conditional && next > furthest) break;
  addr = next;
}
