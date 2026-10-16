// Summoner savegame editor (format: docs/sum-exe/savegame.md). Writes a NEW file only: the output must not
// exist yet and must differ from the input, so an existing save is never overwritten. Every edit is checked
// before anything is written; one bad edit writes nothing.
//
// usage: node tools/savedit.mjs <in.sav> <out.sav> [edits...] [table options]
//   --desc "text"              description shown on the Save/Load screens (max 255 characters)
//   --gold N                   party gold (INVT +0x04)
//   --add-item "Name[=N]"      one more party inventory entry (N items, default 1)          (repeatable)
//   --remove-item "Name[=N]"   remove N of that item (default: all of it)                    (repeatable)
//   --set-item-count "Name=N"  the party holds exactly N of that item (0 removes it)          (repeatable)
//   --flag LEVEL:NAME=0|1      clear or set a story flag of a level (name or index 0-50)     (repeatable)
//   --member I                 which party member the following fields change: 0 = first (default), or a name
//                              (Joseph, Flece, Jekhar, Rosalind); may be given again for the next member
//   --level N  --xp N  --hp N  --maxhp N  --ap N  --maxap N  --skillpoints N    (once per member)
//   --skill ID=VALUE           a skill byte (ids 0-35 are saved; 10 = level 1, see the skill table in
//                              docs/sum-exe/characters.md)                                   (repeatable)
//   table options (names of items and flags): --tables FILE.vpp (default Summoner/tables.vpp), --tables-dir DIR,
//   --items-tbl FILE, --events-tbl FILE, --lang NAME (default english); see tools/savinfo.mjs.
//
// Items are found by name_hash_nocase (0x4d5f50): Crc32 (0x552130, table polynomial 0xedb88320, start 0, no final
// inversion) of the lower-cased name. A name is accepted only when its hash is one of the item types listed in the
// save's own ITEM chunk, which is the list the loader resolves against; an unknown hash would fail the load with
// "Couldn't find item type!!!". The INVT section has a fixed capacity (0x209 entries); the entries, the count
// (+0x08) and the trailing length are rewritten together.
//
// Story flags use the per-level block layout of savegame_layout_level_data (0x4d1860) with the flag lists of
// events.tbl and the automap layer counts of the <level>.tbl files. The edit is refused unless those tables
// reproduce the save's level-data size exactly. Flag i of a level is bit (i & 7) of byte i >> 3 of the level's
// block (savegame_data_offset 0x4cfbd0, savegame_set_flag 0x4cfd30). Setting a flag to 1 also does what
// quest_evaluate (0x4d0c90) does in game: it clears the journal bit of every quest whose +Invalidate: flag it is.
//
// Notes: on load the game recomputes only the skill cap from the level (xp_level_skill_cap at 0x4cf898). Raising
// --level does not grant the HP, AP or skill points of the skipped levels; set those too if wanted. The header
// checksum (0x1e4) is never read, and the section sizes are not changed, so the edited save stays loadable. Copy
// the output into Summoner/savegame under an unused slot name (summonerN.sav) to try it.
import { readFileSync, writeFileSync, existsSync } from "node:fs";
import { resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import {
  loadTables, checkSave, nameHash, hex, itemTypes, itemName, readInventory, writeInventory, chunkAt, partyMembers,
  levelBlocks, findLevel, getFlagBit, setFlagBit, setQuestBit, questBit, LEVELS,
} from "./lib/savedata.mjs";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const fail = (msg) => { console.error(msg); process.exit(1); };
const USAGE = "usage: node tools/savedit.mjs <in.sav> <out.sav> [--desc text] [--gold N] [--add-item Name[=N]] [--remove-item Name[=N]] [--set-item-count Name=N] [--flag LEVEL:NAME=0|1] [--member I|NAME] [--level N] [--xp N] [--hp N] [--maxhp N] [--ap N] [--maxap N] [--skillpoints N] [--skill ID=VALUE] [--tables FILE.vpp] [--tables-dir DIR] [--items-tbl FILE] [--events-tbl FILE] [--lang NAME]";

const args = process.argv.slice(2);
const [input, output] = args;
if (!input || !output || input.startsWith("--") || output.startsWith("--")) fail(USAGE);
if (resolve(input) === resolve(output)) fail("refusing: output must be a different file than the input");
if (existsSync(output)) fail(`refusing: ${output} already exists (savedit never overwrites files)`);

// Options are kept in command-line order. Repeatable ones may appear any number of times; the others once
// (per member for the member fields), so a value is never silently replaced by a later one.
const FIELDS = ["level", "xp", "hp", "maxhp", "ap", "maxap", "skillpoints"];
const ONCE = ["desc", "gold"], REPEAT = ["add-item", "remove-item", "set-item-count", "flag", "skill", "member"];
const TABLE = ["tables", "tables-dir", "items-tbl", "events-tbl", "lang"];
const ops = [], tableOpts = {}, seen = new Set();
for (let i = 2; i < args.length; i++) {
  if (!args[i].startsWith("--")) fail(`unexpected argument ${args[i]}`);
  const k = args[i].slice(2), v = args[++i];
  if (![...FIELDS, ...ONCE, ...REPEAT, ...TABLE].includes(k)) fail(`unknown option --${k}`);
  if (v === undefined) fail(`--${k} needs a value`);
  if (TABLE.includes(k)) { if (k in tableOpts) fail(`--${k} given twice`); tableOpts[k] = v; continue; }
  if (ONCE.includes(k)) { if (seen.has(k)) fail(`--${k} given twice`); seen.add(k); }
  ops.push({ k, v });
}
if (!ops.some((o) => o.k !== "member")) fail("no edits given");

const b = Buffer.from(readFileSync(input));
const bad = checkSave(b);
if (bad) fail(`${input}: ${bad}`);

const tables = loadTables({
  tablesVpp: tableOpts.tables ?? resolve(repoRoot, "Summoner", "tables.vpp"),
  tablesDir: tableOpts["tables-dir"], itemsTbl: tableOpts["items-tbl"], eventsTbl: tableOpts["events-tbl"], lang: tableOpts.lang,
});
const types = itemTypes(b);
if (!types) fail("no ITEM chunk");

const int = (k, v, lo = -0x80000000, hi = 0x7fffffff) => {
  if (!/^-?\d+$/.test(v) || Number(v) < lo || Number(v) > hi) fail(`--${k} needs a whole number from ${lo} to ${hi}, not "${v}"`);
  return Number(v);
};
const real = (k, v) => { if (!/^-?\d+(\.\d+)?$/.test(v)) fail(`--${k} needs a number, not "${v}"`); return Number(v); };

// "Name" or "Name=N": the item type, checked against the save's own item-type list.
function itemArg(k, v, needCount, minCount) {
  const m = /^(.*?)(?:=(-?\d+))?$/.exec(v);
  const name = m[1].trim();
  if (!name) fail(`--${k} needs an item name`);
  if (needCount && m[2] === undefined) fail(`--${k} needs Name=N`);
  const count = m[2] === undefined ? null : int(k, m[2], minCount);
  const hash = nameHash(name);
  if (!types.includes(hash)) {
    let hint = "";
    if (tables.items) {
      const near = [...tables.items.values()].filter((n) => n.toLowerCase().includes(name.toLowerCase())).slice(0, 6);
      if (near.length) hint = `; similar names: ${near.join(", ")}`;
    } else hint = " (items.tbl not loaded, so no suggestions)";
    fail(`--${k}: unknown item "${name}" (hash ${hex(hash)} is not an item type of this save)${hint}`);
  }
  return { name: itemName(tables, hash) ?? name, hash, count };
}

const changes = [];
const inv = readInventory(b);
if (!inv) fail("no INVT chunk");
let entries = inv.entries.map((e) => ({ ...e })), invChanged = false;
const held = (h) => entries.filter((e) => e.hash === h).reduce((a, e) => a + e.count, 0);

const party = partyMembers(b);
const PARTY = ["joseph", "flece", "jekhar", "rosalind"];
let member = 0;
const memberSeen = new Set();
const memberRec = () => {
  if (member >= party.length) fail(`party member ${member} not found (${party.length} in this save)`);
  return party[member];
};

let blocks = null;
const levelData = () => {
  if (!blocks) {
    const lb = levelBlocks(b, tables);
    if (lb.error) fail(`--flag: cannot place story flags safely: ${lb.error}`);
    blocks = lb.blocks;
  }
  return blocks;
};

for (const { k, v } of ops) {
  if (k === "desc") {
    if (v.length > 255) fail("--desc is longer than 255 characters");
    b.fill(0, 0xb0, 0xb0 + 0x100);
    b.write(v, 0xb0, "latin1");
    changes.push(`description = "${v}"`);
  } else if (k === "gold") {
    const o = chunkAt(b, "INVT"), old = b.readInt32LE(o + 4), g = int(k, v);
    b.writeInt32LE(g, o + 4);
    changes.push(`gold ${old} -> ${g}`);
  } else if (k === "add-item") {
    const it = itemArg(k, v, false, 1), count = it.count ?? 1;
    entries.push({ hash: it.hash, count, field: 0 }); invChanged = true;
    changes.push(`added ${count} x "${it.name}" (hash ${hex(it.hash)}) to the party inventory`);
  } else if (k === "remove-item") {
    const it = itemArg(k, v, false, 1), have = held(it.hash);
    if (!have) fail(`--remove-item: the party has no "${it.name}"`);
    let n = it.count ?? have;
    if (n > have) fail(`--remove-item: the party has only ${have} x "${it.name}"`);
    const removed = n;
    for (let i = entries.length - 1; i >= 0 && n > 0; i--) {
      if (entries[i].hash !== it.hash) continue;
      const take = Math.min(n, entries[i].count);
      entries[i].count -= take; n -= take;
      if (entries[i].count <= 0) entries.splice(i, 1);
    }
    invChanged = true;
    changes.push(`removed ${removed} x "${it.name}" (${have} -> ${have - removed})`);
  } else if (k === "set-item-count") {
    const it = itemArg(k, v, true, 0), have = held(it.hash);
    const first = entries.findIndex((e) => e.hash === it.hash);
    if (it.count === 0) entries = entries.filter((e) => e.hash !== it.hash);
    else if (first < 0) entries.push({ hash: it.hash, count: it.count, field: 0 });
    else { entries[first].count = it.count; entries = entries.filter((e, i) => e.hash !== it.hash || i === first); }
    invChanged = true;
    changes.push(`"${it.name}" count ${have} -> ${it.count}`);
  } else if (k === "flag") {
    const m = /^([^:]+):(.+)=([01])$/.exec(v);
    if (!m) fail(`--flag needs LEVEL:NAME=0 or LEVEL:NAME=1, not "${v}"`);
    const li = findLevel(m[1].trim());
    if (li < 0) fail(`--flag: unknown level "${m[1]}" (levels: ${LEVELS.join(", ")})`);
    const blk = levelData()[li], name = m[2].trim(), val = Number(m[3]), h = nameHash(name);
    const fi = blk.level.flags.findIndex((n) => nameHash(n) === h);
    if (fi < 0) {
      const other = levelData().find((x) => x.level.flags.some((n) => nameHash(n) === h));
      fail(`--flag: level ${blk.name} has no flag "${name}"` + (other ? `; it belongs to level ${other.name}` : " (not in events.tbl)"));
    }
    const old = getFlagBit(b, blk, fi);
    setFlagBit(b, blk, fi, val);
    changes.push(`flag ${blk.name}:${blk.level.flags[fi]} ${old} -> ${val}`);
    if (val && tables.quests) for (const q of tables.quests) {
      if (q.invalidate && nameHash(q.invalidate) === h && questBit(b, q.index)) {
        setQuestBit(b, q.index, 0);
        changes.push(`  journal quest "${q.name}" removed (its +Invalidate: flag, as quest_evaluate does)`);
      }
    }
  } else if (k === "member") {
    if (/^\d+$/.test(v)) member = Number(v);
    else {
      member = PARTY.includes(v.toLowerCase()) ? party.findIndex((p) => p.hash === nameHash(v)) : -1;
      if (member < 0) fail(`--member: no party member named "${v}" in this save`);
    }
    memberRec();
  } else if (k === "skill") {
    const r = memberRec().at + 4, m = /^(\d+)=(\d+)$/.exec(v);
    const id = m ? Number(m[1]) : -1, val = m ? Number(m[2]) : -1;
    if (!(id >= 0 && id < 36) || !(val >= 0 && val < 256)) fail("--skill needs ID=VALUE with ID 0-35 and VALUE 0-255");
    const key = `${member}:skill${id}`;
    if (memberSeen.has(key)) fail(`--skill ${id} given twice for member ${member}`);
    memberSeen.add(key);
    const old = b[r + 0x120 + id];
    b[r + 0x120 + id] = val;
    changes.push(`member ${member} skill 0x${id.toString(16)} ${old} -> ${val}`);
  } else {
    const rec = memberRec(), r = rec.at + 4, key = `${member}:${k}`;
    if (memberSeen.has(key)) fail(`--${k} given twice for member ${member}`);
    memberSeen.add(key);
    const set = {
      level: () => b.writeInt16LE(int(k, v, -0x8000, 0x7fff), r + 0x144), xp: () => b.writeInt32LE(int(k, v), r + 0x146),
      hp: () => b.writeFloatLE(real(k, v), r + 0x14a), maxhp: () => b.writeFloatLE(real(k, v), r + 0x14e),
      ap: () => b.writeInt16LE(int(k, v, -0x8000, 0x7fff), r + 0x152), maxap: () => b.writeInt16LE(int(k, v, -0x8000, 0x7fff), r + 0x154),
      skillpoints: () => b.writeInt32LE(int(k, v), r + 0x17c + 4),
    };
    set[k]();
    changes.push(`member ${member} ${k} = ${v}`);
  }
}
if (invChanged) { try { writeInventory(b, entries); } catch (e) { fail(e.message); } }

writeFileSync(output, b, { flag: "wx" }); // wx: fail if the file appeared meanwhile
console.log(`wrote ${output}:\n  ` + changes.join("\n  "));
for (const w of tables.warnings) console.log(`note: ${w}`);
