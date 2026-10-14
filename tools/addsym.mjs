// Merge proposed symbols into symbols/sum.symbols, keeping address order.
// usage: node tools/addsym.mjs <proposals.txt> [--dry-run]
// Each proposal line: 0xADDR|name|kind|module|conf|comment   (kind: func or data:<type>; conf: V, O or I)
// Lines whose address or name already exists, or that are malformed, are skipped and reported.
import { readFileSync, writeFileSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const path = join(repoRoot, "symbols", "sum.symbols");
const [input, flag] = process.argv.slice(2);
const dry = flag === "--dry-run";

const lines = readFileSync(path, "utf8").split(/\r?\n/);
const addrOf = (l) => (/^0x[0-9a-f]+\s/.test(l) ? parseInt(l, 16) : null);
const names = new Set(), addrs = new Set();
for (const l of lines) {
  const m = /^(0x[0-9a-f]+)\s+(\S+)/.exec(l);
  if (m) { addrs.add(parseInt(m[1], 16)); names.add(m[2]); }
}

let added = 0;
const skipped = [];
for (const raw of readFileSync(input, "utf8").split(/\r?\n/)) {
  if (!raw.trim() || raw.startsWith("#")) continue;
  const [addr, name, kind, mod, conf, ...c] = raw.split("|").map((s) => s.trim());
  const why =
    !/^0x[0-9a-f]+$/.test(addr ?? "") ? "bad address" :
    !/^[A-Za-z_][A-Za-z0-9_]*$/.test(name ?? "") ? "bad name" :
    !/^(func|data:[^\s;|]+)$/.test(kind ?? "") ? "bad kind" :
    !/^[VOI]$/.test(conf ?? "") ? "bad confidence" :
    !mod || /\s/.test(mod) ? "bad module" :
    addrs.has(parseInt(addr, 16)) ? "address exists" :
    names.has(name) ? "name exists" : null;
  if (why) { skipped.push(`${why}: ${raw.slice(0, 100)}`); continue; }
  const comment = c.join("|").replace(/;/g, ",");
  const line = `${addr}  ${name.padEnd(24)} ${kind.padEnd(5)} ${mod.padEnd(20)} ${conf}${comment ? " ; " + comment : ""}`;
  const v = parseInt(addr, 16), isData = kind.startsWith("data");
  let idx = -1;
  for (let i = 0; i < lines.length; i++) {
    const x = addrOf(lines[i]);
    if (x === null || /\sdata:/.test(lines[i]) !== isData) continue;
    if (x > v) { idx = i; break; }
    idx = i + 1;
  }
  lines.splice(idx, 0, line);
  addrs.add(v); names.add(name); added++;
}
if (!dry) writeFileSync(path, lines.join("\n"));
console.log(`${dry ? "would add" : "added"} ${added}, skipped ${skipped.length}`);
for (const s of skipped) console.log("  " + s);
