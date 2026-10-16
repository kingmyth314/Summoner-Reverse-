// Read-only Summoner savegame inspector (format: docs/sum-exe/savegame.md). Never writes to the save.
// usage: node tools/savinfo.mjs [options] <file.sav> [more.sav ...]
//   --json              print everything as one JSON document (for a GUI or a script) instead of text
//   --all               also list every entity record of the LENT chunk
//   --flags-all         list every level and every story flag, set or not (text mode)
//   --tables FILE.vpp   game tables packfile (default Summoner/tables.vpp); read in memory, nothing is extracted
//   --tables-dir DIR    a folder of extracted .tbl files (items.tbl, events.tbl, <level>.tbl), used before the packfile
//   --items-tbl FILE    --events-tbl FILE    a single table file, used before the other two
//   --lang NAME         language section of the tables (default english)
// Prints the header (level, description, play time, section sizes), gold and the party inventory, the party
// members (level, experience, HP/AP, skill points, equipment), the story flags that are set and the non-zero
// variables of every level, and the active journal quests. Item, flag and quest names come from items.tbl and
// events.tbl; without them items are shown by hash and the flags are left out.
import { readFileSync } from "node:fs";
import { resolve, dirname, basename } from "node:path";
import { fileURLToPath } from "node:url";
import { loadTables, describeSave } from "./lib/savedata.mjs";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2), files = [], o = {};
const valued = ["tables", "tables-dir", "items-tbl", "events-tbl", "lang"], switches = ["json", "all", "flags-all"];
for (let i = 0; i < args.length; i++) {
  const a = args[i];
  if (!a.startsWith("--")) { files.push(a); continue; }
  const k = a.slice(2);
  if (switches.includes(k)) o[k] = true;
  else if (valued.includes(k)) { if (args[i + 1] === undefined) { console.error(`--${k} needs a value`); process.exit(1); } o[k] = args[++i]; }
  else { console.error(`unknown option --${k}`); process.exit(1); }
}
if (!files.length) { console.error("usage: node tools/savinfo.mjs [--json] [--all] [--flags-all] [--tables FILE.vpp] [--tables-dir DIR] [--items-tbl FILE] [--events-tbl FILE] [--lang NAME] <file.sav> [...]"); process.exit(1); }

const tables = loadTables({
  tablesVpp: o.tables ?? resolve(repoRoot, "Summoner", "tables.vpp"),
  tablesDir: o["tables-dir"],
  itemsTbl: o["items-tbl"], eventsTbl: o["events-tbl"], lang: o.lang,
});

const saves = files.map((file) => {
  let b;
  try { b = readFileSync(file); } catch (e) { return { file, ok: false, error: `cannot read: ${e.code ?? e.message}` }; }
  return { file, ...describeSave(b, tables, { all: o.all }), _buf: b };
});

if (o.json) {
  const doc = { tool: "savinfo", format: 1, tables: { lang: tables.lang, sources: tables.sources, warnings: tables.warnings }, saves: saves.map(({ _buf, ...r }) => r) };
  console.log(JSON.stringify(doc, null, 2));
  process.exit(saves.every((r) => r.ok) ? 0 : 2);
}

for (const w of tables.warnings) console.log(`note: ${w}`);
const item = (e) => e.name ?? `item ${e.hash}`;
for (const r of saves) {
  console.log(`== ${basename(r.file)}${r.size !== undefined ? ` (${r.size} bytes)` : ""}`);
  if (r.version === undefined) { console.log(`   ${r.error}`); continue; }
  console.log(`   version ${r.version}${r.version === 0x27 ? "" : " (v1.40 expects 39)"}`);
  console.log(`   level ${r.level}, script ${r.script}, level time ${r.levelTime.toFixed(1)} s`);
  console.log(`   description: ${r.description}`);
  console.log(`   sections: ${Object.entries(r.sections).map(([n, v]) => `${n} ${v}`).join(", ")}`);
  if (!r.ok) { console.log(`   ${r.error}`); continue; }
  if (r.inventory) {
    const inv = r.inventory;
    console.log(`   gold ${inv.gold}, inventory ${inv.entries.length} entries${inv.lengthOk ? "" : " (trailing length wrong: the game will refuse this save)"}`);
    for (const e of inv.entries) console.log(`     ${item(e)} x${e.count}${e.field ? ` (field ${e.field})` : ""}${e.known === false ? "  UNKNOWN ITEM TYPE: the load fails" : ""}`);
  }
  console.log(`   party state ${r.partyState}, ${r.entityCount} entity records`);
  for (const m of r.party ?? []) {
    console.log(`   #${m.record} ${m.name ?? m.hash}${m.dead ? " (dead)" : ""}: level ${m.level}, xp ${m.xp}, HP ${m.hp}/${m.maxhp}, AP ${m.ap}/${m.maxap}, skill points ${m.skillpoints}`);
    console.log(`      equipped: ${m.equipped.map((e) => e.name ?? `item ${e.index}`).join(", ") || "-"}`);
  }
  for (const d of r.dyingSummons ?? []) console.log(`   #${d.record} hash ${d.hash}, summon (dying)`);
  if (r.entities) for (const e of r.entities) console.log(`   #${e.record}, hash ${e.hash}, ${e.typeName ?? `type ${e.type}`}`);
  if (r.levelDataError) console.log(`   story flags not shown: ${r.levelDataError}`);
  else {
    console.log("   levels (visits, set flags / all flags):");
    for (const lv of r.levels) {
      const vars = [...Object.entries(lv.ints), ...Object.entries(lv.floats).map(([n, v]) => [n, +v.toFixed(3)])].filter(([, v]) => v);
      if (!o["flags-all"] && !lv.visits && !lv.flagsSet.length && !vars.length) continue;
      console.log(`     ${lv.name} (${lv.index}): ${lv.visits} visits, ${lv.flagsSet.length}/${lv.flagCount} flags${lv.flagsSet.length ? ": " + lv.flagsSet.join(", ") : ""}`);
      if (o["flags-all"]) for (const [n, v] of Object.entries(lv.flags)) if (!v) console.log(`       0 ${n}`);
      if (vars.length) console.log(`       variables: ${vars.map(([n, v]) => `${n}=${v}`).join(", ")}`);
    }
  }
  if (r.journal) console.log(`   journal quests: ${r.journal.map((q) => q.name).join("; ") || "-"}`);
  else if (r.journalBits) console.log(`   journal quest bits: ${r.journalBits.join(", ") || "-"}`);
}
