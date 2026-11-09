// Side quests played in agent mode (testsaves/README.md, "Side quests"; docs/tools.md).
//
//   node tools/testsaves/sidequests.mjs list                 the quests of events.tbl with who starts and ends them
//   node tools/testsaves/sidequests.mjs run [ID ...]         play the specs of sidequests.json (all, or the IDs given)
//
// Each spec starts from a fresh load of a play-through save (testsaves/playthrough/), optionally moves with `exitto`,
// and runs steps:
//   { talk, goto?, file, char, target, gate? }  talk to the character (a click on him; `npcgoto goto` first) and walk the
//       conversation to the target topic along the shortest path of its dialogue table (file, character block char).
//       When the target cannot be reached as the save stands, or with gate: true, the event flags (`+Unhide by event`)
//       and items (`+Unhide by item`) that unlock the target or a topic on a path to it are set first, standing in for
//       the objective; each is reported.
//   { give: [item, ...] }   party_inventory_add_by_name (0x46bb00) with full charges, standing in for finding the item
//   { identify: [item, ...] }   mark the item types identified, standing in for the Lore skill
//   { kill: "script object" }   npcgoto and npckill (Sir Horum for the Lord of Logaros)
//   { cmd: "agent commands" }   anything else
//   { exitto: "LEVEL SCRIPT ENTRY" }   change level as a level exit does
// After each talk the game is saved (savenow) and the journal, inventory, gold and Joseph's experience are compared with
// the save before. Results go to build/sidequests/results.txt.
//
// Needs SUMMONER_GAME (a test folder: the original Sum.exe as SUMMONER_EXE, default Sum-orig.exe, SummonerFix with
// [Debug] Agent=1 and TestSaves=1, tables.vpp). Work files go to build/sidequests (SIDEQUESTS_WORK).
import { execFileSync } from "node:child_process";
import { copyFileSync, utimesSync, existsSync, rmSync, readFileSync, readdirSync, appendFileSync, mkdirSync, statSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { LEVELS, loadTables } from "../lib/savedata.mjs";

const here = dirname(fileURLToPath(import.meta.url)), repo = resolve(here, "..", "..");
const G = process.env.SUMMONER_GAME;
if (!G) { console.error("set SUMMONER_GAME to the test folder"); process.exit(2); }
const W = process.env.SIDEQUESTS_WORK || join(repo, "build", "sidequests"), TBL = join(W, "tbl"), TABLES = join(G, "tables.vpp");
mkdirSync(W, { recursive: true });
if (!existsSync(join(TBL, "events.tbl"))) execFileSync("node", [join(repo, "tools/vpp.mjs"), "extract", TABLES, "*.tbl", "--out", TBL]);
const env = { ...process.env, SUMMONER_GAME: G, SUMMONER_EXE: process.env.SUMMONER_EXE || "Sum-orig.exe" };
const agent = (...a) => { try { return execFileSync("node", [join(repo, "tools/agent.mjs"), ...a], { env, encoding: "latin1", timeout: 240000 }); } catch (e) { return (e.stdout || "") + "ERR " + e.message.split("\n")[0]; } };
const run = (c) => agent("run", c);
const flags = () => parseInt((/5FBFD8: (..)/.exec(run("peek 5fbfd8 1")) || [, "0"])[1], 16);
const quick = join(G, "savetest", "quicksave.sav");
const mtime = (f) => { try { return statSync(f).mtimeMs; } catch { return 0; } };

// ---- dialogue tables ----
const tblFile = (name) => readdirSync(TBL).find((f) => f.toLowerCase() === name.toLowerCase());
const keysIn = (s) => [...s.matchAll(/\{([^}]*)\}/g)].map((m) => m[1].trim());
const display = (key) => key.replace(/#[^"}]*$/, "").replace(/^"(.*)"$/, "$1").trim();
// One character's conversation: topics (key -> adds, initial adds, unhides), initial topics, event and item gates.
function graph(file, char) {
  const lines = readFileSync(join(TBL, tblFile(file)), "latin1").split(/\r?\n/);
  const topics = new Map(), gates = new Map(), itemGates = new Map(); let initial = [], inChar = false, cur = null;
  const push = (map, k, v) => { if (!map.has(k)) map.set(k, []); map.get(k).push(v); };
  for (const raw of lines) {
    const l = raw.replace(/\/\/.*$/, ""); let m;
    if ((m = /^\s*\$Character:\s*"([^"]+)"/.exec(l))) { inChar = m[1] === char; cur = null; continue; }
    if (!inChar) continue;
    if ((m = /^\s*\+Topic:\s*(.*)$/.exec(l))) { cur = { adds: [], addInitial: [], unhides: [] }; topics.set(keysIn(m[1])[0], cur); continue; }
    if ((m = /^\s*\$Initial Topics:\s*(.*)$/i.exec(l))) { initial = keysIn(m[1]); continue; }
    if (!cur) continue;
    if ((m = /^\s*\+Add Initial Topic:\s*(.*)$/i.exec(l))) cur.addInitial.push(...keysIn(m[1]));
    else if ((m = /^\s*\+Add Topic:\s*(.*)$/i.exec(l))) cur.adds.push(...keysIn(m[1]));
    if ((m = /^\s*\+Unhide(?: topic| by item| by event| by gold)?:\s*(.*)$/i.exec(l)) && !/From:/i.test(l)) {
      const ks = keysIn(m[1]), f = [...m[1].replace(/\{[^}]*\}/g, "").matchAll(/"([^"]+)"/g)].map((x) => x[1]);
      cur.unhides.push(...ks);
      if (/by event/i.test(l)) for (const k of ks) push(gates, k, { level: f[0], flag: f[1] });
      if (/by item/i.test(l)) for (const k of ks) for (const it of f) push(itemGates, k, it);
    }
  }
  return { topics, initial, gates, itemGates };
}
function distances(g, target) {
  const rev = new Map();
  for (const [k, t] of g.topics) for (const n of [...t.adds, ...t.addInitial, ...t.unhides]) { if (!rev.has(n)) rev.set(n, []); rev.get(n).push(k); }
  const dist = new Map([[target, 0]]), q = [target];
  while (q.length) { const k = q.shift(); for (const p of rev.get(k) || []) if (!dist.has(p)) { dist.set(p, dist.get(k) + 1); q.push(p); } }
  return dist;
}

const cmd = process.argv[2];
if (cmd === "list") {
  const t = loadTables({ tablesVpp: TABLES });
  const side = t.quests.filter((q) => !q.main), where = new Map(side.map((q) => [q.name.toLowerCase(), { start: new Set(), end: new Set() }]));
  for (const f of readdirSync(TBL).filter((f) => /_dlg\.tbl$/i.test(f) && !/^miniquests/i.test(f))) {
    let ch = "";
    for (const l of readFileSync(join(TBL, f), "latin1").split(/\r?\n/)) {
      let m;
      if ((m = /^\s*\$Character:\s*"([^"]+)"/.exec(l))) ch = m[1];
      else if ((m = /^\s*\+(Unhide|Hide) Quest:\s*\{"([^"]+)"/i.exec(l)) && where.has(m[2].toLowerCase())) where.get(m[2].toLowerCase())[m[1].toLowerCase() === "unhide" ? "start" : "end"].add(`${ch} (${f.replace(/_dlg\.tbl$/i, "")})`);
    }
  }
  for (const q of side) { const w = where.get(q.name.toLowerCase()); console.log(`${q.index} ${q.name}\n  start: ${[...w.start].join("; ") || "-"}\n  end:   ${[...w.end].join("; ") || "-"}`); }
  process.exit(0);
}
if (cmd !== "run") { console.log("usage: node tools/testsaves/sidequests.mjs list | run [ID ...]"); process.exit(2); }

// ---- playing ----
const specs = JSON.parse(readFileSync(join(here, "sidequests.json"), "utf8")).filter((s) => process.argv.length < 4 || process.argv.slice(3).includes(s.id));
const LOG = join(W, "results.txt");
const out = (s) => { console.log(s); appendFileSync(LOG, s + "\n"); };
const info = (f) => JSON.parse(execFileSync("node", [join(repo, "tools/savinfo.mjs"), "--json", "--tables", TABLES, f], { encoding: "utf8" })).saves[0];
const state = (f) => { const i = info(f); return { journal: (i.journal || []).map((q) => q.name), inv: new Map(i.inventory.entries.map((e) => [e.name, e.count])), xp: i.party?.find((m) => m.name === "Joseph")?.xp ?? 0, gold: i.inventory.gold }; };
const diff = (a, b) => {
  const r = [];
  for (const q of b.journal) if (!a.journal.includes(q)) r.push(`+quest "${q}"`);
  for (const q of a.journal) if (!b.journal.includes(q)) r.push(`-quest "${q}"`);
  for (const [n, c] of b.inv) if ((a.inv.get(n) || 0) !== c) r.push(`${n} ${a.inv.get(n) || 0}->${c}`);
  for (const [n, c] of a.inv) if (!b.inv.has(n)) r.push(`${n} ${c}->0`);
  if (b.xp !== a.xp) r.push(`xp +${b.xp - a.xp}`);
  if (b.gold !== a.gold) r.push(`gold ${a.gold}->${b.gold}`);
  return r.join(", ") || "no change";
};
const page = (d) => { const b = /box (\d+) (\d+)/.exec(d); if (b) run(`move ${+b[1] + 190} ${b[2]}; wait 200; click left ${+b[1] + 190} ${b[2]}; wait 1200`); };
const topicsOf = (d) => [...d.matchAll(/topic \d+ at (\d+) (\d+): "?([^"\r\n]+?)"?\r?$/gm)].map((m) => ({ x: +m[1], y: +m[2], text: m[3] }));
const clickTopic = (t) => { const x = t.x + 165, y = t.y - 15; run(`move ${x} ${y}; wait 200; click left ${x} ${y}; wait 1500`); };
// Over when dialogue_mode is 0 or its state is 0: after a Farewell the window keeps mode 3 with state 0 (2026-11-08).
const closed = (d) => /mode 0/.test(d) || /state 0/.test(d) || !/mode/.test(d);
const give = (it) => run(`callstr 46bb00 ${it.replace(/ /g, "~")} 1 -1; wait 300`).trim().split("\n")[0];
// Mark an item type identified, standing in for the Lore skill: a quest item flagged "not identified" in items.tbl
// does not count for `+Unhide by item` until then (party_count_item_by_name 0x4ce9a0 counts identified items only).
// item_type_find (0x44b450) gives the index (in summonerfix.log); bit 0 of item_types[index].flags (+0x5c) is cleared.
function identify(name) {
  run(`callstr 44b450 ${name.replace(/ /g, "~")}; wait 300`);
  const log = readFileSync(join(G, "summonerfix.log"), "latin1").split(/\r?\n/).reverse().find((l) => /callstr 44B450/.test(l)) || "";
  const m = /= 0x([0-9A-F]+)/i.exec(log); if (!m) return "item_type_find gave no result";
  const idx = parseInt(m[1], 16); if (idx > 0x7fffffff) return "not found";
  const a = 0x8cbec0 + idx * 0x100 + 0x5c, b = parseInt((/: (..)/.exec(run(`peek ${a.toString(16)} 1`)) || [, "0"])[1], 16);
  run(`poke ${a.toString(16)} ${(b & ~1).toString(16)}`);
  return `type ${idx}, flags ${b.toString(16)} -> ${(b & ~1).toString(16)}`;
}
function talkTo(st) {
  const g = graph(st.file, st.char), dist = distances(g, st.target), want = display(st.target);
  if (!g.topics.size) return `no topics for ${st.char} in ${st.file}`;
  const best = (text) => Math.min(...[...g.topics.keys()].filter((k) => display(k) === text).map((k) => dist.get(k) ?? Infinity), Infinity);
  let talks = 0; const clicked = [];
  for (let n = 0; n < 90; n++) { // Novice Sama's chain is 15 topics, each with pages of text
    const d = run("dlg 3");
    if (closed(d)) {
      if (clicked.includes(want)) return "done: " + clicked.join(" > ");
      if (talks++ >= 4) return "gave up after 4 talks: " + clicked.join(" > ");
      if (st.goto) run(`npcgoto ${st.goto}; wait 800`);
      run(`talkto ${st.talk.replace(/ /g, "~")}; wait 5000`);
      continue;
    }
    if (/mode 1/.test(d)) { page(d); continue; }
    const vis = topicsOf(d);
    if (!vis.length) { run("wait 1000"); continue; }
    const fw = vis.find((t) => /^Farewell/.test(t.text));
    if (clicked.includes(want)) { if (fw) { clickTopic(fw); continue; } return "done: " + clicked.join(" > "); }
    const tgt = vis.find((t) => t.text === want);
    if (tgt) { clickTopic(tgt); clicked.push(want); continue; }
    const ranked = vis.map((t) => ({ t, d: best(t.text) })).filter((x) => x.d < Infinity && x.d > 0).sort((a, b) => a.d - b.d);
    if (ranked.length) { clickTopic(ranked[0].t); clicked.push(ranked[0].t.text); continue; }
    if (fw) { clickTopic(fw); clicked.push("Farewell"); continue; }
    return "stuck at: " + vis.map((t) => t.text).join(" | ");
  }
  return "too many steps: " + clicked.join(" > ");
}
function applyGates(st) {
  const g = graph(st.file, st.char);
  for (const k of distances(g, st.target).keys()) {
    for (const { level, flag } of g.gates.get(k) || []) {
      const li = LEVELS.findIndex((n) => n.toLowerCase() === String(level).toLowerCase());
      out(`  gate flag ${level}:${flag} (for ${display(k)}): ${li < 0 ? "level not found" : run(`flagl ${li} ${flag} 1`).trim().split("\n")[0]}`);
    }
    for (const it of g.itemGates.get(k) || []) out(`  gate item ${it} (for ${display(k)}): ${give(it)}`);
  }
}
function snap(tag) {
  for (let i = 0; i < 10; i++) {
    const d = run("dlg 1"); if (closed(d)) break;
    if (/mode 1/.test(d)) page(d); else { const fw = topicsOf(run("dlg 3")).find((t) => /^Farewell/.test(t.text)); if (fw) clickTopic(fw); else run("key escape; wait 800"); }
  }
  const t = mtime(quick); run("savenow; wait 2500");
  if (mtime(quick) === t) return null;
  const f = join(W, `${tag}.sav`); copyFileSync(quick, f); return f;
}
for (const sp of specs) {
  out(`== ${sp.id}: ${sp.quest}`);
  try {
    let base = join(repo, "testsaves", "playthrough", `${sp.base}.sav`);
    if (sp.edits?.length) { const e = join(W, `${sp.id}-base.sav`); rmSync(e, { force: true }); execFileSync("node", [join(repo, "tools/savedit.mjs"), base, e, "--tables", TABLES, ...sp.edits]); base = e; }
    agent("stop"); rmSync(join(G, "agent", "cmd.txt"), { force: true });
    const dst = join(G, "savetest", "summoner40.sav"); copyFileSync(base, dst); const now = new Date(); utimesSync(dst, now, now);
    agent("start", "--wait", "90");
    // The party immortal (0x941704) so that no fight ends a run; the newest save is the first on the Load screen.
    run("poke 941704 01; click left 858 368; wait 2000; click left 798 368; wait 15000");
    if (sp.exitto) run(`exitto ${sp.exitto}; wait 15000`);
    for (let i = 0; i < 30 && (flags() & 0x24); i++) run("key escape; wait 2000");
    let prev = state(base), k = 0;
    for (const st of sp.steps) {
      k++;
      if (st.give) { for (const it of st.give) out(`  give ${it}: ${give(it)}`); continue; }
      if (st.identify) { for (const it of st.identify) out(`  identify ${it}: ${identify(it)}`); continue; }
      if (st.kill) { out(`  kill ${st.kill}: ${run(`npcgoto ${st.kill}; wait 800; npckill ${st.kill}; wait 6000`).trim().split("\n").slice(-2)[0]}`); continue; }
      if (st.cmd) { out(`  cmd ${st.cmd.slice(0, 60)}: ${run(st.cmd).trim().split("\n").slice(-2)[0]}`); continue; }
      if (st.exitto) { out(`  exitto ${st.exitto}: ${run(`exitto ${st.exitto}; wait 15000`).trim().split("\n")[0]}`); for (let i = 0; i < 30 && (flags() & 0x24); i++) run("key escape; wait 2000"); continue; }
      if (st.gate) applyGates(st);
      if (st.goto) run(`npcgoto ${st.goto}; wait 1000`);
      let r = talkTo(st);
      if (!/^done/.test(r) && !st.gate) { out(`  talk ${st.talk} -> ${display(st.target)}: ${r}; retrying with gates`); snap(`${sp.id}-${k}-pre`); applyGates(st); r = talkTo(st); }
      const f = snap(`${sp.id}-${k}`), now2 = f ? state(f) : prev;
      out(`  talk ${st.talk} -> ${display(st.target)}: ${r}`);
      out(`    ${f ? diff(prev, now2) : "NOT SAVED"}`);
      prev = now2;
    }
  } catch (e) { out(`  ERROR ${String(e.message).split("\n")[0]}`); }
  agent("stop");
}
