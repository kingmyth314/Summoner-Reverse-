// Find the functions that reference given strings (via REA xrefs + containing procedure).
// usage: node tools/name-from-strings.mjs <string-va> [<string-va> ...]
// Prints, per string: each referencing site, its containing function, and the current symbol name.
import { reaCall } from "./lib/rea-queue.mjs";
import { loadSymbols } from "./symbols.mjs";
import { readFileSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const b = readFileSync(join(repoRoot, "Summoner", "Sum.exe"));
const { symbols } = loadSymbols();
const byAddr = new Map(symbols.map((s) => [s.addr, s.name]));
for (const va of process.argv.slice(2)) {
  const o = parseInt(va, 16) - 0x400000;
  const str = b.toString("latin1", o, b.indexOf(0, o)).replace(/\s+/g, " ");
  const x = await reaCall("xrefs", { address: va });
  const sites = Array.isArray(x.json) ? x.json : x.json ? [x.json] : [];
  const lines = [];
  for (const s of sites) {
    const p = await reaCall("resolve_containing_procedure", { address: s });
    const fa = p.json?.procedure?.address;
    lines.push(`${s} in ${fa ?? "(no function)"}${fa && byAddr.get(parseInt(fa, 16)) ? " = " + byAddr.get(parseInt(fa, 16)) : ""}`);
  }
  console.log(`${va} "${str.slice(0, 70)}"\n   ${lines.join("\n   ") || "(no references)"}`);
}
