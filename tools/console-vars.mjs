// Name the global variables that "Toggles X" / "Sets X" console commands control.
//
// For each such command, scan its callback for absolute memory operands (opcodes A0-A3 moffs, or a
// ModRM byte with mod=00 rm=101 -> [disp32]) that point into .data/.bss. Addresses shared by many
// callbacks (output buffers, console state) are discarded. A callback that leaves exactly one
// candidate names that variable.
//
// usage: node tools/console-vars.mjs [--symbols]
import { readFileSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { execFileSync } from "node:child_process";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const b = readFileSync(join(repoRoot, "Summoner", "Sum.exe"));
const BASE = 0x400000, DATA_LO = 0x588000, DATA_HI = 0x2fbf2f8;

// Command list from console-commands.mjs (name, callback, description).
const raw = execFileSync(process.execPath, [join(repoRoot, "tools", "console-commands.mjs")], { encoding: "utf8" });
const cmds = [];
for (const line of raw.split(/\r?\n/)) {
  const m = line.match(/^(\S+)\s+callback (0x[0-9a-f]+)\s+object (0x[0-9a-f]+)\s+(.*)$/);
  if (m) cmds.push({ name: m[1], cb: parseInt(m[2], 16), desc: m[4].trim() });
}

const isStr = (va) => {
  const o = va - BASE;
  if (o < 0 || o >= b.length) return false;
  let n = 0;
  while (n < 64 && b[o + n] >= 0x20 && b[o + n] < 0x7f) n++;
  return n >= 4 && b[o + n] === 0;
};

function memOperands(cb) {
  const out = [];
  const o0 = cb - BASE;
  for (let o = o0; o < o0 + 160 && o < b.length - 5; o++) {
    if (b[o] === 0xc3 && o > o0 + 8) break; // first ret ends a small callback
    let disp = null;
    if (b[o] >= 0xa0 && b[o] <= 0xa3) disp = b.readUInt32LE(o + 1);
    else if ((b[o + 1] & 0xc7) === 0x05 && [0x80, 0x81, 0x83, 0x88, 0x89, 0x8a, 0x8b, 0xc6, 0xc7, 0xd9, 0xdd, 0xd8, 0xf6, 0xff, 0x38, 0x39, 0x3a, 0x3b, 0x30, 0x31, 0x32, 0x33, 0x0f].includes(b[o]))
      disp = b.readUInt32LE(o + 2);
    if (disp !== null && disp >= DATA_LO && disp < DATA_HI && !isStr(disp)) out.push(disp);
  }
  return out;
}

const all = cmds.map((c) => ({ ...c, ops: [...new Set(memOperands(c.cb))] }));
const freq = new Map();
for (const c of all) for (const a of c.ops) freq.set(a, (freq.get(a) || 0) + 1);
const shared = new Set([...freq.entries()].filter(([, n]) => n > 3).map(([a]) => a));

const hex = (v) => "0x" + v.toString(16);
const results = [];
for (const c of all) {
  const m = c.desc.match(/^(Toggles|Sets)\s+([A-Za-z_][A-Za-z0-9_]*)\s*$/);
  if (!m) continue;
  const cand = c.ops.filter((a) => !shared.has(a));
  results.push({ cmd: c.name, verb: m[1], variable: m[2], cand });
}

if (process.argv.includes("--symbols")) {
  for (const r of results) {
    if (r.cand.length !== 1) continue;
    const kind = r.verb === "Toggles" ? "data:bool" : "data:int";
    console.log(`${hex(r.cand[0])} ${r.variable} ${kind} - O ; set by console command "${r.cmd}" (${r.verb.toLowerCase()}); type inferred`);
  }
} else {
  console.log(`shared addresses ignored: ${[...shared].map(hex).join(" ")}\n`);
  for (const r of results)
    console.log(`${r.cmd.padEnd(24)} ${r.verb.padEnd(7)} ${r.variable.padEnd(32)} -> ${r.cand.length === 1 ? hex(r.cand[0]) : r.cand.length ? "ambiguous " + r.cand.map(hex).join(",") : "none"}`);
  const ok = results.filter((r) => r.cand.length === 1).length;
  console.log(`\n${ok} of ${results.length} Toggles/Sets commands resolve to exactly one variable`);
}
