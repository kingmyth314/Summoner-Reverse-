// Extract every console command registered by Sum.exe's static initializers.
//
// Pattern (observed at 0x4323f0 for show_framerate):
//   68 <callback>   push callback
//   68 <desc>       push description string
//   68 <name>       push command name string
//   B9 <object>     mov ecx, console_command object
//   E8 <rel32>      call console_command::console_command
// The constructor is the call target shared by these sites.
//
// usage: node tools/console-commands.mjs [--md <file>] [--symbols]
//   --symbols  print sum.symbols lines for the callbacks, objects and constructor
import { readFileSync, writeFileSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const b = readFileSync(join(repoRoot, "Summoner", "Sum.exe"));
const BASE = 0x400000, TEXT_LO = 0x401000, TEXT_HI = 0x57e000, IMG_HI = 0x2fc1000;
const off = (va) => va - BASE; // Sum.exe: file offset == RVA for all initialised sections
const u32 = (va) => b.readUInt32LE(off(va));

function cstr(va) {
  if (va < 0x57e000 || va >= 0x2fbf2f8) return null;
  const o = off(va);
  if (o < 0 || o >= b.length) return null;
  const end = b.indexOf(0, o);
  if (end < 0 || end - o > 300) return null;
  const s = b.toString("latin1", o, end);
  return /^[\x20-\x7e\t\r\n]*$/.test(s) ? s : null;
}

// Find candidate sites and the most common call target.
const sites = [];
for (let o = off(TEXT_LO); o < off(TEXT_HI) - 25; o++) {
  if (b[o] !== 0x68 || b[o + 5] !== 0x68 || b[o + 10] !== 0x68 || b[o + 15] !== 0xb9 || b[o + 20] !== 0xe8) continue;
  const va = o + BASE;
  const cb = b.readUInt32LE(o + 1), desc = b.readUInt32LE(o + 6), name = b.readUInt32LE(o + 11);
  const obj = b.readUInt32LE(o + 16), target = (va + 25 + b.readInt32LE(o + 21)) >>> 0;
  const n = cstr(name), d = cstr(desc);
  if (n === null || d === null) continue;
  sites.push({ va, cb, desc: d, name: n, obj, target });
}
const counts = new Map();
for (const s of sites) counts.set(s.target, (counts.get(s.target) || 0) + 1);
const [ctor] = [...counts.entries()].sort((a, b) => b[1] - a[1])[0] || [];
const cmds = sites.filter((s) => s.target === ctor).sort((a, b) => a.name.localeCompare(b.name));
const hex = (v) => "0x" + v.toString(16);

const args = process.argv.slice(2);
if (args.includes("--symbols")) {
  const ident = (s) => s.replace(/[^A-Za-z0-9_]/g, "_");
  console.log(`${hex(ctor)} console_command__console_command func  -  O ; ctor(name, description, callback); ${cmds.length} static instances`);
  for (const c of cmds) {
    const inText = c.cb >= TEXT_LO && c.cb < TEXT_HI;
    if (inText) console.log(`${hex(c.cb)} cmd_${ident(c.name)} func - O ; console command "${c.name}": ${c.desc.replace(/;/g, ",").replace(/\s+/g, " ").trim()}`);
    console.log(`${hex(c.obj)} ccmd_${ident(c.name)} data:console_command - O ; console command object "${c.name}"`);
  }
} else {
  console.log(`constructor ${hex(ctor)}: ${cmds.length} commands (${sites.length} pattern sites, ${counts.size} distinct targets)\n`);
  for (const c of cmds) console.log(`${c.name.padEnd(28)} callback ${hex(c.cb).padEnd(9)} object ${hex(c.obj).padEnd(9)} ${c.desc}`);
}
const mi = args.indexOf("--md");
if (mi >= 0) {
  const esc = (s) => s.replace(/\s+/g, " ").trim().replace(/\|/g, "\\|").replace(/</g, "&lt;").replace(/>/g, "&gt;");
  const lines = ["| Command | Callback | Object | Description |", "|---|---|---|---|"];
  for (const c of cmds) lines.push(`| \`${esc(c.name)}\` | \`${hex(c.cb)}\` | \`${hex(c.obj)}\` | ${esc(c.desc)} |`);
  writeFileSync(join(repoRoot, args[mi + 1]), lines.join("\n") + "\n");
}
