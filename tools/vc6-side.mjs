// Prints one function's VC6 output next to Sum.exe's disassembly, one instruction per line, marking with * the
// lines whose mnemonics differ. Addresses and jump targets are not compared (they differ by design).
//
//   node tools/vc6-side.mjs <listing.cod> <mangled-name-substring> <address>
//
// The .cod listing comes from `node tools/vc6-compare.mjs <file.cpp> --keep` (the kept folder is printed, or in
// --json as "tmp"). The name substring picks the PROC line, e.g. "script_action_die@" or "?len@vector3@@".
import { readFileSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { resolve, dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const [cod, name, addr] = process.argv.slice(2);
if (!addr) { console.error("usage: node tools/vc6-side.mjs <listing.cod> <mangled-name-substring> <address>"); process.exit(2); }
const lines = readFileSync(cod, "latin1").split(/\r?\n/);
let on = false;
const mine = [];
for (const l of lines) {
  if (!on && l.includes(name) && /PROC/.test(l)) { on = true; continue; }
  if (on && /ENDP/.test(l)) break;
  if (!on) continue;
  // An instruction line, or the second line of an instruction whose bytes wrapped.
  const m = /^(?:\s+[0-9a-f]{5})?\t[0-9a-f ]+\t\s*(\S.*)$/.exec(l);
  if (m) mine.push(m[1].replace(/\s+/g, " ").replace(/;.*$/, "").trim());
}
const dis = execFileSync("node", [join(repo, "tools", "asm.mjs"), "fn", addr, "5000"], { cwd: repo }).toString().split(/\r?\n/)
  .filter((l) => /^[0-9a-f]+: /.test(l)).map((l) => l.replace(/^[0-9a-f]+: /, "").replace(/;.*$/, "").replace(/\s+/g, " ").trim());
const n = Math.max(mine.length, dis.length);
for (let i = 0; i < n; i++) {
  const a = (mine[i] || "").slice(0, 60), b = (dis[i] || "").slice(0, 60);
  const same = a.split(" ")[0] === b.split(" ")[0];
  console.log(`${same ? " " : "*"} ${String(i).padStart(3)} ${a.padEnd(60)} | ${b}`);
}
