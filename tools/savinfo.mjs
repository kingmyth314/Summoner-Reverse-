// Read-only Summoner savegame inspector (format: docs/sum-exe/savegame.md).
// usage: node tools/savinfo.mjs <file.sav> [more.sav ...]
// Prints the header (level, description, play time, section sizes) and the living-entity records of the
// LENT chunk (type, level, experience, HP/AP, equipment). Item names come from extracted/tables/items.tbl
// when present (node tools/vpp.mjs extract Summoner/tables.vpp items.tbl). Never writes to the save.
import { readFileSync, existsSync } from "node:fs";
import { join, resolve, dirname, basename } from "node:path";
import { fileURLToPath } from "node:url";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const SECTIONS = ["thumb", "level data", "conversation", "items", "inv", "le", "camera", "item_object", "quest_flag",
  "level_item", "two_stage", "automap_labels", "hot_buttons"];
// Chunk offsets from the size table at 0x1b0 (buffer order: thumbnail, level data, INVT, conversation, ITEM, LENT;
// savegame_read). A tag search could match the same four bytes inside the thumbnail first.
const chunkAt = (b, tag) => {
  const s = (i) => b.readInt32LE(0x1b0 + i * 4);
  const invt = 0x200 + s(0) + s(1), o = tag === "INVT" ? invt : invt + s(4) + s(2) + s(3);
  return b.toString("latin1", o, o + 4) === tag ? o : -1;
};
const TYPES = { 4: "object", 5: "creature", 6: "creature (dead)", 7: "party member", 8: "party member (dead)", 9: "npc", 10: "npc (dead)" };

let itemNames = null;
const itemsTbl = join(repoRoot, "extracted", "tables", "items.tbl");
if (existsSync(itemsTbl)) {
  itemNames = [];
  for (const m of readFileSync(itemsTbl, "latin1").matchAll(/^\s*\$Name:[ \t]*"?([^"\r\n]*)"?/gm)) itemNames.push(m[1].trim());
}
const cstr = (b, o, n) => { const e = b.indexOf(0, o); return b.toString("latin1", o, e < 0 || e > o + n ? o + n : e); };

for (const file of process.argv.slice(2)) {
  const b = readFileSync(file);
  console.log(`== ${basename(file)} (${b.length} bytes)`);
  if (b.toString("latin1", 0, 4) !== "SAVg") { console.log("   not a Summoner save (no SAVg tag)"); continue; }
  const version = b.readInt32LE(4);
  console.log(`   version ${version}${version === 0x27 ? "" : " (v1.40 expects 39)"}`);
  console.log(`   level ${cstr(b, 0x08, 0x80)}, script ${cstr(b, 0x88, 0x20)}, level time ${b.readFloatLE(0xac).toFixed(1)} s`);
  console.log(`   description: ${cstr(b, 0xb0, 0x100)}`);
  console.log(`   sections: ${SECTIONS.map((n, i) => `${n} ${b.readInt32LE(0x1b0 + i * 4)}`).join(", ")}`);

  const lent = chunkAt(b, "LENT");
  if (lent < 0) { console.log("   no LENT chunk"); continue; }
  const count = b.readInt32LE(lent + 0x30);
  console.log(`   party state 0x${b.readUInt32LE(lent + 0x0c).toString(16)}, ${count} entity records`);
  let o = lent + 0x34;
  for (let i = 0; i < count && o + 0x180 <= b.length; i++) {
    const hash = b.readUInt32LE(o), r = o + 4, type = b.readInt32LE(r);
    const line = [`#${i}`, `hash 0x${hash.toString(16).padStart(8, "0")}`, TYPES[type] ?? `type ${type}`];
    if (type === 7) {
      const equip = [];
      for (let k = 0; k < 9; k++) {
        const it = b.readInt32LE(r + 0x6c + k * 4);
        if (it >= 0) equip.push(itemNames?.[it] ?? `item ${it}`);
      }
      line.push(`level ${b.readInt16LE(r + 0x144)}`, `xp ${b.readInt32LE(r + 0x146)}`,
        `HP ${b.readFloatLE(r + 0x14a)}/${b.readFloatLE(r + 0x14e)}`, `AP ${b.readInt16LE(r + 0x152)}/${b.readInt16LE(r + 0x154)}`,
        `skill points ${b.readInt32LE(o + 4 + 0x17c + 4)}`, `equipped: ${equip.join(", ") || "-"}`);
      o += 4 + 0x17c + 8;
    } else {
      o += 4 + 0x17c;
    }
    if (type === 7 || type === 8 || process.argv.includes("--all")) console.log("   " + line.join(", "));
  }
}
