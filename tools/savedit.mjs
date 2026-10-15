// Summoner savegame editor (format: docs/sum-exe/savegame.md). Writes a NEW file only: the output must not
// exist yet and must differ from the input, so an existing save is never overwritten.
//
// usage: node tools/savedit.mjs <in.sav> <out.sav> [edits...]
//   --desc "text"          description shown on the Save/Load screens (max 255 characters)
//   --gold N               party gold (INVT +0x04)
//   --member I             which party member the following fields change (0 = first, default)
//   --level N  --xp N  --hp N  --maxhp N  --ap N  --maxap N  --skillpoints N
//   --skill ID=VALUE       a skill byte (ids 0-35 are saved; 10 = level 1, see the skill table in docs/sum-exe/characters.md).
//                          The Skills panel only lists skills the character may have, so a new skill may not appear.
//
// Notes: the game does not recompute anything from these values on load. Raising --level does not grant the
// HP, AP or skill points of the skipped levels; set those too if wanted. The file has no checksum, and the
// section sizes are not changed, so the edited save stays loadable. Copy the output into Summoner/savegame
// under an unused slot name (summonerN.sav) to try it.
import { readFileSync, writeFileSync, existsSync } from "node:fs";
import { resolve } from "node:path";

const args = process.argv.slice(2);
const [input, output] = args;
if (!input || !output || input.startsWith("--") || output.startsWith("--")) {
  console.log("usage: node tools/savedit.mjs <in.sav> <out.sav> [--desc text] [--gold N] [--member I] [--level N] [--xp N] [--hp N] [--maxhp N] [--ap N] [--maxap N] [--skillpoints N] [--skill ID=VALUE]");
  process.exit(1);
}
if (resolve(input) === resolve(output)) { console.error("refusing: output must be a different file than the input"); process.exit(1); }
if (existsSync(output)) { console.error(`refusing: ${output} already exists (savedit never overwrites files)`); process.exit(1); }

const b = Buffer.from(readFileSync(input));
if (b.toString("latin1", 0, 4) !== "SAVg" || b.readInt32LE(4) !== 0x27) { console.error("not a Summoner v1.40 save (SAVg, version 39)"); process.exit(1); }

const opts = {};
for (let i = 2; i < args.length; i++) {
  if (!args[i].startsWith("--")) { console.error(`unexpected argument ${args[i]}`); process.exit(1); }
  opts[args[i].slice(2)] = args[++i];
}
const known = ["desc", "gold", "member", "level", "xp", "hp", "maxhp", "ap", "maxap", "skillpoints", "skill"];
for (const k of Object.keys(opts)) if (!known.includes(k)) { console.error(`unknown option --${k}`); process.exit(1); }
const num = (k) => { const v = Number(opts[k]); if (!Number.isFinite(v)) { console.error(`--${k} needs a number`); process.exit(1); } return v; };
const changes = [];

if (opts.desc !== undefined) {
  if (opts.desc.length > 255) { console.error("--desc is longer than 255 characters"); process.exit(1); }
  b.fill(0, 0xb0, 0xb0 + 0x100);
  b.write(opts.desc, 0xb0, "latin1");
  changes.push(`description = "${opts.desc}"`);
}
if (opts.gold !== undefined) {
  const invt = b.indexOf("INVT", 0x200, "latin1");
  if (invt < 0) { console.error("no INVT chunk"); process.exit(1); }
  const old = b.readInt32LE(invt + 4);
  b.writeInt32LE(num("gold") | 0, invt + 4);
  changes.push(`gold ${old} -> ${num("gold") | 0}`);
}

const fieldOpts = ["level", "xp", "hp", "maxhp", "ap", "maxap", "skillpoints", "skill"].filter((k) => opts[k] !== undefined);
if (fieldOpts.length) {
  const lent = b.indexOf("LENT", 0x200, "latin1");
  const count = b.readInt32LE(lent + 0x30);
  const want = opts.member !== undefined ? num("member") : 0;
  let o = lent + 0x34, seen = -1, rec = -1;
  for (let i = 0; i < count; i++) {
    const type = b.readInt32LE(o + 4);
    if (type === 7 || type === 8) { seen++; if (seen === want) { rec = o; break; } o += 4 + 0x17c + 8; }
    else o += 4 + 0x17c;
  }
  if (rec < 0) { console.error(`party member ${want} not found (${seen + 1} in this save)`); process.exit(1); }
  const r = rec + 4;
  const set = {
    level: (v) => b.writeInt16LE(v, r + 0x144), xp: (v) => b.writeInt32LE(v, r + 0x146),
    hp: (v) => b.writeFloatLE(v, r + 0x14a), maxhp: (v) => b.writeFloatLE(v, r + 0x14e),
    ap: (v) => b.writeInt16LE(v, r + 0x152), maxap: (v) => b.writeInt16LE(v, r + 0x154),
    skillpoints: (v) => b.writeInt32LE(v, rec + 4 + 0x17c + 4),
  };
  for (const k of fieldOpts) {
    if (k === "skill") {
      const [id, val] = opts.skill.split("=").map(Number);
      if (!(id >= 0 && id < 36) || !(val >= 0 && val < 256)) { console.error("--skill needs ID=VALUE with ID 0-35 and VALUE 0-255"); process.exit(1); }
      const old = b[r + 0x120 + id];
      b[r + 0x120 + id] = val;
      changes.push(`member ${want} skill 0x${id.toString(16)} ${old} -> ${val}`);
      continue;
    }
    set[k](num(k)); changes.push(`member ${want} ${k} = ${num(k)}`);
  }
}

if (!changes.length) { console.error("no edits given"); process.exit(1); }
writeFileSync(output, b, { flag: "wx" }); // wx: fail if the file appeared meanwhile
console.log(`wrote ${output}:\n  ` + changes.join("\n  "));
