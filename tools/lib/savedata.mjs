// Shared reader for Summoner v1.40 savegames (format: docs/sum-exe/savegame.md) and the game tables that give
// the save its names. Used by tools/savinfo.mjs and tools/savedit.mjs, and meant as the engine of a future GUI:
// every function here works on a Buffer and plain objects, prints nothing and never writes a file.
import { readFileSync, existsSync, readdirSync, openSync, readSync, closeSync } from "node:fs";
import { join } from "node:path";
import { readDirectory } from "../vpp.mjs";

// level_script_table (0x59f128): 51 records, in this order; the index is the save's level index.
export const LEVELS = ["Wolong", "Catacombs", "test", "masad", "worldmap1", "lenele1b", "lenele1c", "lenele1d", "lenele1e",
  "sewer", "rand-hills01", "IkaemosBottomInt", "IkaemosTopInt", "IkaemosExt", "KhosaniLab", "IonaExt", "WolongCaverns",
  "TempleInt", "rand-forest01", "rand-forestnite1", "Liangshan", "Rand-Desert", "IonaExt02", "KhosaniStrng", "LPalaceInt",
  "tancredhouse", "LPalaceInt02", "KhosaniLab2", "Wolong2", "eleh", "lenele2aa", "lenele2ab", "lenele3a", "lenele3d",
  "jadetemple", "lenele1aa", "lenele1ab", "TempleInt2", "IkaemosExt2", "IkaemosBottomInt2", "Rand-HillsNite01",
  "Rand-Iceland01", "Rand-Grassland01", "Rand-Orenia01", "Rand-OreniaNite01", "Rand-DesertNite01", "Rand-GrasslandNite01",
  "Rand-IcelandNite01", "endgame", "lenele2d", "sewerboss"];
export const SECTIONS = ["thumb", "level data", "conversation", "items", "inv", "le", "camera", "item_object", "quest_flag",
  "level_item", "two_stage", "automap_labels", "hot_buttons"];
export const ENTITY_TYPES = { 4: "object", 5: "creature", 6: "creature (dead)", 7: "party member", 8: "summon (dying)", 9: "npc", 10: "npc (dead)" };
export const SAVE_SIZE = 270755, INVT_MAX_ENTRIES = 0x209;

// name_hash_nocase (0x4d5f50): Crc32 (0x552130) of the lower-cased name; table polynomial 0xedb88320, start 0,
// no final inversion.
const crcTable = Array.from({ length: 256 }, (_, i) => { let v = i; for (let k = 0; k < 8; k++) v = v & 1 ? (v >>> 1) ^ 0xedb88320 : v >>> 1; return v >>> 0; });
export const nameHash = (s) => { let v = 0; for (const ch of Buffer.from(s.toLowerCase(), "latin1")) v = (crcTable[(v ^ ch) & 0xff] ^ (v >>> 8)) >>> 0; return v; };
const PARTY_NAMES = new Map(["Joseph", "Flece", "Jekhar", "Rosalind"].map((n) => [nameHash(n), n]));
export const hex = (v) => "0x" + (v >>> 0).toString(16).padStart(8, "0");

// ---------------------------------------------------------------------------------------------------------------
// Game tables. Sources, first match wins per file: an explicit file (itemsTbl, eventsTbl), a folder of extracted
// .tbl files (tablesDir), the packfile (tablesVpp, read in memory; nothing is extracted).

function tableSource({ tablesVpp, tablesDir, itemsTbl, eventsTbl }) {
  const explicit = { "items.tbl": itemsTbl, "events.tbl": eventsTbl };
  let dirIndex = null, vppDir = null;
  if (tablesDir && existsSync(tablesDir)) dirIndex = new Map(readdirSync(tablesDir).map((n) => [n.toLowerCase(), join(tablesDir, n)]));
  if (tablesVpp && existsSync(tablesVpp)) { try { vppDir = readDirectory(tablesVpp); } catch { vppDir = null; } }
  const vppIndex = vppDir ? new Map(vppDir.files.map((f) => [f.name.toLowerCase(), f])) : null;
  return (name) => {
    const ex = explicit[name.toLowerCase()];
    if (ex) return existsSync(ex) ? { text: readFileSync(ex, "latin1"), from: ex } : null;
    const d = dirIndex?.get(name.toLowerCase());
    if (d) return { text: readFileSync(d, "latin1"), from: d };
    const f = vppIndex?.get(name.toLowerCase());
    if (f) {
      const b = Buffer.alloc(f.size), fd = openSync(tablesVpp, "r");
      try { readSync(fd, b, 0, f.size, f.offset); } finally { closeSync(fd); }
      return { text: b.toString("latin1"), from: `${tablesVpp}:${f.name}` };
    }
    return null;
  };
}

// TextFile-style tokens: // comments removed, quoted strings kept whole.
function tokens(text) {
  const out = [];
  for (const line of text.split(/\r?\n/)) {
    const re = /"([^"]*)"|(\S+)/g;
    let m;
    while ((m = re.exec(line))) {
      if (m[2]?.startsWith("//")) break;
      out.push(m[1] !== undefined ? { s: m[1] } : { w: m[2] });
    }
  }
  return out;
}
// The language section ("#english", localize_get_language_name lower-cased; events_parse_table 0x4d3a58).
function languageSection(toks, lang) {
  const i = toks.findIndex((t) => t.w?.toLowerCase() === "#" + lang.toLowerCase());
  return i < 0 ? null : toks.slice(i + 1);
}

// items.tbl: the $Name: of every item type of the language section.
function parseItems(text, lang) {
  const t = languageSection(tokens(text), lang);
  if (!t) return null;
  const names = [];
  for (let i = 0; i < t.length; i++) {
    if (t[i].w === "#End") break;
    if (t[i].w === "$Name:" && t[i + 1]?.s !== undefined) names.push(t[i + 1].s.trim());
  }
  return names;
}

// events.tbl as events_parse_table (0x4d39c0) reads it: $Level: blocks up to #End, then $Quest: blocks up to #End.
// A flag name already known in ANY level (level_script_flag_hash_known 0x4d5ed0) is not added again; ints and floats
// are both checked against the int lists only (level_script_int_hash_known 0x4d5f10). An unknown $Level: name ends
// the parse (0x4d3b2b), as in the game.
function parseEvents(text, lang) {
  const t = languageSection(tokens(text), lang);
  if (!t) return null;
  const levels = LEVELS.map((name) => ({ name, flags: [], ints: [], floats: [] }));
  const flagKnown = new Set(), intKnown = new Set(), quests = [];
  let i = 0, stopped = null;
  while (i < t.length && t[i].w !== "#End") {
    if (t[i].w !== "$Level:") { i++; continue; }
    const lname = t[i + 1]?.s ?? "";
    const li = LEVELS.findIndex((n) => n.toLowerCase() === lname.toLowerCase());
    i += 2;
    if (li < 0) { stopped = lname; break; }
    while (i < t.length && t[i].w !== "$Level:" && t[i].w !== "#End") {
      const kind = t[i].w, name = t[i + 1]?.s;
      if ((kind === "+Flag:" || kind === "+Int:" || kind === "+Float:") && name !== undefined) {
        const h = nameHash(name);
        if (kind === "+Flag:") { if (!flagKnown.has(h)) { flagKnown.add(h); levels[li].flags.push(name); } }
        else if (!intKnown.has(h)) { if (kind === "+Int:") { intKnown.add(h); levels[li].ints.push(name); } else levels[li].floats.push(name); }
        i += 2;
      } else i++;
    }
  }
  if (!stopped && t[i]?.w === "#End") {
    i++;
    while (i < t.length && t[i].w !== "#End") {
      if (t[i].w !== "$Quest:") { i++; continue; }
      const q = { index: quests.length, name: t[i + 1]?.s ?? "", main: false, invalidate: null };
      i += 2;
      if (t[i]?.w === "+Main") { q.main = true; i += 2; }
      while (i < t.length && t[i].w !== "$Quest:" && t[i].w !== "#End") {
        if (t[i].w === "+Invalidate:" && t[i + 1]?.s !== undefined) { q.invalidate = t[i + 1].s; i += 2; } else i++;
      }
      if (quests.length < 200) quests.push(q); // quest_count stops at 0xc8
    }
  }
  return { levels, quests, stopped };
}

// <level>.tbl #Automap: the layer count is the largest $AutomapPC: number (level_parse_automap_table 0x4d4020).
function parseAutomapLayers(text) {
  const t = tokens(text);
  let i = t.findIndex((x) => x.w === "#Automap"), max = 0;
  if (i < 0) return 0;
  for (i++; i < t.length; ) {
    if (t[i].w === "$AutomapPC:" || t[i].w === "$AutomapPSX:") {
      const n = Number(t[i + 2]?.w);
      if (t[i].w === "$AutomapPC:" && n > max) max = n;
      i += 4;
    } else break;
  }
  return max;
}

export function loadTables(opts = {}) {
  const lang = opts.lang ?? "english", get = tableSource(opts), warnings = [], sources = {};
  const tables = { lang, items: null, levels: null, quests: null, sources, warnings };
  const it = get("items.tbl");
  if (it) {
    const names = parseItems(it.text, lang);
    if (names) { tables.items = new Map(names.map((n) => [nameHash(n), n])); sources.items = it.from; }
    else warnings.push(`items.tbl has no #${lang} section`);
  } else warnings.push("items.tbl not found: items are shown by hash");
  const ev = get("events.tbl");
  if (ev) {
    const e = parseEvents(ev.text, lang);
    if (!e) warnings.push(`events.tbl has no #${lang} section`);
    else {
      if (e.stopped) warnings.push(`events.tbl: unknown level "${e.stopped}" ends the table (the game stops there too)`);
      let missing = [];
      for (const lv of e.levels) {
        const f = get(`${lv.name}.tbl`);
        lv.layers = f ? parseAutomapLayers(f.text) : 0;
        if (!f) missing.push(lv.name);
      }
      tables.levels = e.levels; tables.quests = e.quests; sources.events = ev.from;
      tables.missingLevelTables = missing; // the game reads 0 layers for these too (open fails)
    }
  } else warnings.push("events.tbl not found: story flags are not available");
  return tables;
}

// ---------------------------------------------------------------------------------------------------------------
// Save layout

const s = (b, i) => b.readInt32LE(0x1b0 + i * 4);
// Section offsets from the size table at 0x1b0, buffer order: thumbnail, level data, INVT, conversation, ITEM, LENT,
// CMRA, CONT, quest bits, level items, two-stage, automap labels, hot buttons (savegame_build_layout 0x4d4160).
export function sectionOffsets(b) {
  const order = [0, 1, 4, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12]; // size-table index of each buffer section
  const off = {};
  let o = 0x200;
  for (const i of order) { off[SECTIONS[i]] = o; o += s(b, i); }
  off.end = o;
  return off;
}
export const chunkAt = (b, tag) => {
  const off = sectionOffsets(b), name = { INVT: "inv", ITEM: "items", LENT: "le", CMRA: "camera", CONT: "item_object" }[tag];
  const o = off[name];
  return o !== undefined && b.toString("latin1", o, o + 4) === tag ? o : -1;
};
export function checkSave(b) {
  if (b.length < 0x200 || b.toString("latin1", 0, 4) !== "SAVg") return "not a Summoner save (no SAVg tag)";
  if (b.readInt32LE(4) !== 0x27) return `version ${b.readInt32LE(4)}, Summoner v1.40 expects 39`;
  if (sectionOffsets(b).end !== b.length) return `the size table adds up to ${sectionOffsets(b).end} bytes, the file has ${b.length}`;
  return null;
}
const cstr = (b, o, n) => { const e = b.indexOf(0, o); return b.toString("latin1", o, e < 0 || e > o + n ? o + n : e); };

// Per-level data blocks (savegame_layout_level_data 0x4d1860): 51 visit-count dwords, then per level
// ceil((flags + layers*4096) / 8) + 4*(ints + floats) bytes. Returns null with a reason when the tables do not
// reproduce the header's level-data size, so nothing is read or written at a guessed offset.
export function levelBlocks(b, tables) {
  if (!tables?.levels) return { error: "events.tbl not loaded" };
  const base = sectionOffsets(b)["level data"];
  let o = base + 0xcc;
  const blocks = tables.levels.map((lv, index) => {
    const bits = lv.flags.length + lv.layers * 4096, size = ((bits + 7) >>> 3) + 4 * (lv.ints.length + lv.floats.length);
    const blk = { index, name: lv.name, offset: o, size, visits: b.readInt32LE(base + 4 * index), level: lv, intsAt: o + ((bits + 7) >>> 3) };
    o += size;
    return blk;
  });
  if (o - base !== s(b, 1)) {
    const miss = tables.missingLevelTables ?? [];
    const why = miss.length > 1 ? `; ${miss.length} level tables (<level>.tbl) were not found` : " (tables of another game version, or a modded table?)";
    return { error: `the tables give ${o - base} bytes of level data, the save has ${s(b, 1)}${why}` };
  }
  return { blocks };
}
export const getFlagBit = (b, blk, i) => (b[blk.offset + (i >> 3)] >> (i & 7)) & 1;
export function setFlagBit(b, blk, i, v) { if (v) b[blk.offset + (i >> 3)] |= 1 << (i & 7); else b[blk.offset + (i >> 3)] &= ~(1 << (i & 7)); }
export const questBit = (b, n) => (b[sectionOffsets(b).quest_flag + (n >> 3)] >> (n & 7)) & 1;
export function setQuestBit(b, n, v) { const o = sectionOffsets(b).quest_flag + (n >> 3); if (v) b[o] |= 1 << (n & 7); else b[o] &= ~(1 << (n & 7)); }

// Find a level by name (case-insensitive) or index.
export function findLevel(name) {
  if (/^\d+$/.test(name)) return Number(name) < LEVELS.length ? Number(name) : -1;
  return LEVELS.findIndex((n) => n.toLowerCase() === name.toLowerCase());
}

// ITEM chunk: the item types of the game, in table order (index = equipment value).
export function itemTypes(b) {
  const o = chunkAt(b, "ITEM");
  if (o < 0) return null;
  const n = b.readInt32LE(o + 4), hashes = [];
  for (let i = 0; i < n; i++) hashes.push(b.readUInt32LE(o + 8 + 4 * i));
  return hashes;
}
export const itemName = (tables, hash) => tables?.items?.get(hash >>> 0) ?? null;
const itemLabel = (tables, hash) => itemName(tables, hash) ?? `item ${hex(hash)}`;

// INVT entries {hash, count, field}.
export function readInventory(b) {
  const o = chunkAt(b, "INVT");
  if (o < 0) return null;
  const n = b.readInt32LE(o + 8), entries = [];
  for (let i = 0; i < n; i++) entries.push({ hash: b.readUInt32LE(o + 12 + 12 * i), count: b.readInt32LE(o + 16 + 12 * i), field: b.readInt32LE(o + 20 + 12 * i) });
  return { gold: b.readInt32LE(o + 4), entries, lengthOk: b.readInt32LE(o + 12 + 12 * n) === 12 + 12 * n + 4 };
}
// Rewrites the INVT entries (gold untouched); the rest of the section's capacity is zeroed.
export function writeInventory(b, entries) {
  const o = chunkAt(b, "INVT"), cap = s(b, 4);
  if (entries.length > INVT_MAX_ENTRIES || 16 + 12 * entries.length + 4 > cap) throw new Error("the party inventory is full");
  b.fill(0, o + 8, o + cap);
  b.writeInt32LE(entries.length, o + 8);
  entries.forEach((e, i) => { b.writeUInt32LE(e.hash >>> 0, o + 12 + 12 * i); b.writeInt32LE(e.count, o + 16 + 12 * i); b.writeInt32LE(e.field, o + 20 + 12 * i); });
  b.writeInt32LE(12 + 12 * entries.length + 4, o + 12 + 12 * entries.length);
}

// LENT records. Party members (type 7) carry 8 extra bytes (savegame_write_character 0x4cf733).
export function entities(b) {
  const lent = chunkAt(b, "LENT");
  if (lent < 0) return null;
  const count = b.readInt32LE(lent + 0x30), list = [];
  let o = lent + 0x34;
  for (let i = 0; i < count && o + 0x180 <= b.length; i++) {
    const type = b.readInt32LE(o + 4);
    list.push({ index: i, at: o, hash: b.readUInt32LE(o), type });
    o += 4 + 0x17c + (type === 7 ? 8 : 0);
  }
  return { at: lent, partyState: b.readUInt32LE(lent + 0x0c), list };
}
export function partyMembers(b) {
  return entities(b)?.list.filter((e) => e.type === 7) ?? [];
}
export function memberInfo(b, e, tables, types) {
  const r = e.at + 4, equip = [];
  for (let k = 0; k < 9; k++) {
    const it = b.readInt32LE(r + 0x6c + k * 4);
    if (it < 0) continue;
    const h = types?.[it];
    equip.push({ slot: k, index: it, hash: h !== undefined ? hex(h) : null, name: h !== undefined ? itemName(tables, h) : null });
  }
  const skills = [];
  for (let k = 0; k < 36; k++) skills.push(b[r + 0x120 + k]);
  return {
    record: e.index, hash: hex(e.hash), name: PARTY_NAMES.get(e.hash) ?? null,
    level: b.readInt16LE(r + 0x144), xp: b.readInt32LE(r + 0x146), hp: b.readFloatLE(r + 0x14a), maxhp: b.readFloatLE(r + 0x14e),
    ap: b.readInt16LE(r + 0x152), maxap: b.readInt16LE(r + 0x154), skillpoints: b.readInt32LE(r + 0x17c + 4), dead: !!(b.readUInt32LE(r + 0x1c) & 0x80),
    skills, equipped: equip,
  };
}

// Everything savinfo shows, as one plain object (also the --json output).
export function describeSave(b, tables, { all = false } = {}) {
  const err = checkSave(b);
  const out = { size: b.length, ok: !err, error: err ?? undefined };
  if (b.length < 0x200 || b.toString("latin1", 0, 4) !== "SAVg") return out;
  out.version = b.readInt32LE(4);
  out.level = cstr(b, 0x08, 0x80); out.script = cstr(b, 0x88, 0x20);
  out.levelTime = b.readFloatLE(0xac); out.description = cstr(b, 0xb0, 0x100);
  out.sections = Object.fromEntries(SECTIONS.map((n, i) => [n, s(b, i)]));
  if (err) return out;
  const types = itemTypes(b);
  out.itemTypeCount = types?.length ?? null;
  const inv = readInventory(b);
  if (inv) out.inventory = { gold: inv.gold, lengthOk: inv.lengthOk, entries: inv.entries.map((e) => ({ name: itemName(tables, e.hash), hash: hex(e.hash), count: e.count, field: e.field, known: types ? types.includes(e.hash) : null })) };
  const ents = entities(b);
  if (ents) {
    out.partyState = hex(ents.partyState);
    out.entityCount = ents.list.length;
    out.party = ents.list.filter((e) => e.type === 7).map((e) => memberInfo(b, e, tables, types));
    out.dyingSummons = ents.list.filter((e) => e.type === 8).map((e) => ({ record: e.index, hash: hex(e.hash) }));
    if (all) out.entities = ents.list.map((e) => ({ record: e.index, hash: hex(e.hash), type: e.type, typeName: ENTITY_TYPES[e.type] ?? null }));
  }
  const lb = levelBlocks(b, tables);
  if (lb.error) out.levelDataError = lb.error;
  else {
    // All 51 levels with every flag and variable (a GUI needs the unset ones too); text output filters.
    out.levels = lb.blocks.map((blk) => {
      const lv = blk.level, flags = {}, ints = {}, floats = {};
      lv.flags.forEach((n, i) => { flags[n] = getFlagBit(b, blk, i); });
      lv.ints.forEach((n, i) => { ints[n] = b.readInt32LE(blk.intsAt + 4 * i); });
      lv.floats.forEach((n, i) => { floats[n] = b.readFloatLE(blk.intsAt + 4 * (lv.ints.length + i)); });
      return { index: blk.index, name: blk.name, visits: blk.visits, flagCount: lv.flags.length, flagsSet: lv.flags.filter((n) => flags[n]), flags, ints, floats };
    });
  }
  if (tables?.quests) out.journal = tables.quests.filter((q) => questBit(b, q.index)).map((q) => ({ index: q.index, name: q.name, main: q.main }));
  else { out.journalBits = []; for (let n = 0; n < 200; n++) if (questBit(b, n)) out.journalBits.push(n); }
  return out;
}
export { itemLabel };
