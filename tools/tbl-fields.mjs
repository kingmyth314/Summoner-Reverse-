// Pair each $Field:/+Field: of a .tbl parser with the offset its value is stored at.
// usage: node tools/asm.mjs fn <parser> 6000 > parser.txt; node tools/tbl-fields.mjs parser.txt
// See docs/sum-exe/game-tables.md for the method and its limits.
import { readFileSync } from "node:fs";
const lines = readFileSync(process.argv[2], "utf8").split("\n");
let pending = null, parsed = null; const out = [];
const flush = (why) => { if (parsed) out.push(`${parsed.field.padEnd(26)} ${parsed.how.padEnd(16)} ${why}`); parsed = null; };
for (const l of lines) {
  const f = /; "([$+][^"]*)"/.exec(l);
  if (f && /push 0x/.test(l)) { if (pending) { pending = f[1]; continue; } pending = f[1]; continue; }
  const c = /call .*TextFile__(Parse\w+)/.exec(l);
  if (c && pending) { flush("(no store)"); parsed = { field: pending, how: c[1] }; pending = null; continue; }
  const s = /(mov|fstp|fst) (dword|word|byte) ptr \[(e(?:bp|si|di|bx|ax|cx|dx)) ([+-] 0x[0-9a-f]+)\](?:, (\w+))?$/.exec(l.replace(/\s+;.*$/, ""));
  if (s && parsed && !(s[1] === "mov" && /^0x|^-/.test(s[5] ?? "0x"))) flush(`[${s[3]} ${s[4]}] ${s[2]}`);
  const s0 = /(mov|fstp) (dword|word|byte) ptr \[(e(?:di|si|bx))\](?:, (\w+))?$/.exec(l.replace(/\s+;.*$/, ""));
  if (s0 && parsed && !(s0[1] === "mov" && /^0x/.test(s0[4] ?? "0x"))) flush(`[${s0[3]}] ${s0[2]}`);
}
flush("(no store)");
console.log(out.join("\n"));
