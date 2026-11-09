// The test saves of testsaves/ (testsaves/README.md, docs/tools.md): one save just before each fight of the boss list,
// made from a new game in agent mode, never from a player's saves.
//
//   node tools/testsaves/testsaves.mjs base                 a new game's first save (Joseph, level 1, masad)
//   node tools/testsaves/testsaves.mjs build ID|all         raw save per recipe: party, level, place, savenow
//   node tools/testsaves/testsaves.mjs finish ID|all        testsaves/ID.sav: the tier's gear and items, gold, flags, description
//   node tools/testsaves/testsaves.mjs verify ID|all        load testsaves/ID.sav, start the fight, fight 25 s, compare party HP
//   node tools/testsaves/testsaves.mjs gear FILE TIER       the tier's gear on every party member of an existing save
//                                                           (the play-through set), its items and gold; Joseph wears the
//                                                           two newest summoning rings the party carries
//
// Needs SUMMONER_GAME (a test folder with the original Sum.exe as SUMMONER_EXE, default Sum-orig.exe, SummonerFix
// with [Debug] Agent=1 and TestSaves=1, and tables.vpp). Work files go to build/testsaves (TESTSAVES_WORK).
//
// A recipe (recipes.json): level and script for the agent's `level` command; party as Name:level (Joseph is raised
// with `xpto`, the others added with `join` after arriving: party_member_join crashes in masad, the prologue);
// place: "npc" (npcgoto the boss, then nudge), "nav" (stand outside the trigger navpoint `at`, on the line to where
// the party arrived) or "arrive" (fights whose intro plays on arrival: the save is made after it, and `replay`, the
// level's intro flag, is cleared so that loading the save plays the intro and starts the fight); flags: story flags
// set before the level loads and in the save (for example Korel and Zane appear only with a Lenele flag); go: the
// trigger navpoint a tester walks to; label: an NPC sent to label 1, as the conversation choice that makes him
// hostile does (verify only); tier 0-3: the KIT below.
import { execFileSync } from "node:child_process";
import { copyFileSync, utimesSync, existsSync, rmSync, readFileSync, mkdirSync, appendFileSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url)), repo = resolve(here, "..", "..");
const G = process.env.SUMMONER_GAME;
if (!G) { console.error("set SUMMONER_GAME to the test folder"); process.exit(2); }
const W = process.env.TESTSAVES_WORK || join(repo, "build", "testsaves");
mkdirSync(W, { recursive: true });
const env = { ...process.env, SUMMONER_GAME: G, SUMMONER_EXE: process.env.SUMMONER_EXE || "Sum-orig.exe" };
const agent = (...a) => { try { return execFileSync("node", [join(repo, "tools/agent.mjs"), ...a], { env, encoding: "utf8", timeout: 240000 }); } catch (e) { return (e.stdout || "") + "ERR " + e.message.split("\n")[0]; } };
const run = (c) => agent("run", c);
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const recipes = JSON.parse(readFileSync(join(here, "recipes.json"), "utf8"));
const LEVELS = ["Wolong", "Catacombs", "test", "masad", "worldmap1", "lenele1b", "lenele1c", "lenele1d", "lenele1e", "sewer", "rand-hills01",
  "IkaemosBottomInt", "IkaemosTopInt", "IkaemosExt", "KhosaniLab", "IonaExt", "WolongCaverns", "TempleInt", "rand-forest01", "rand-forestnite1",
  "Liangshan", "Rand-Desert", "IonaExt02", "KhosaniStrng", "LPalaceInt", "tancredhouse", "LPalaceInt02", "KhosaniLab2", "Wolong2", "eleh",
  "lenele2aa", "lenele2ab", "lenele3a", "lenele3d", "jadetemple", "lenele1aa", "lenele1ab", "TempleInt2", "IkaemosExt2", "IkaemosBottomInt2",
  "Rand-HillsNite01", "Rand-Iceland01", "Rand-Grassland01", "Rand-Orenia01", "Rand-OreniaNite01", "Rand-DesertNite01", "Rand-GrasslandNite01",
  "Rand-IcelandNite01", "endgame", "lenele2d", "sewerboss"];

// Gear by tier, each hero in his or her weapon class (Joseph swords, Flece daggers, Rosalind staves, Jekhar axes and
// heavy armour); healing and AP items, summoning rings in the party inventory (Joseph wears two). A Cleansing Elixir
// holds 5 charges and never stacks, so the party carries one (2026-11-08; until then 5 and 10 were asked for, and
// savedit wrote every added potion with 0 charges, which the game refuses to use: testsaves/README.md).
const KIT = [
  { gold: 300, items: ["Health Tonic=10", "Recovery Tonic=5"], rings: ["Ring of Darkness"],
    Joseph: ["weapon=Fiery Falchion", "shield=Buckler", "torso=Leather Jerkin", "hands=Silk Gloves", "legs=Blue Pants", "feet=Leather Boots", "finger1=Ring of Darkness"],
    Flece: ["weapon=Sharp Dagger", "torso=Leather Armor", "hands=Leather Gloves", "feet=Leather Boots"],
    Rosalind: ["torso=Quilted Robe", "hands=Cloth Gloves", "feet=Leather Boots"] },
  { gold: 3000, items: ["Health Tonic=20", "Healing Draught=15", "Recovery Tonic=10", "Cleansing Tonic=5"], rings: ["Ring of Darkness", "Ring of Fire", "Ring of Stone"],
    Joseph: ["weapon=Sword of Spirits", "shield=Round Shield", "torso=Chainmail Tunic", "hands=Leather Gauntlets", "legs=Studded Pants", "feet=Studded Boots", "neck=Necklace of Defense", "finger1=Ring of Darkness", "finger2=Ring of Fire"],
    Flece: ["weapon=Fiery Falchion", "torso=Leather Jerkin", "hands=Thief's Gauntlets", "legs=Studded Pants", "feet=High Leather Boots", "neck=Thief's Torque", "finger1=Ring of Thieves"],
    Rosalind: ["weapon=Hardwood Staff", "torso=Blessed Surcoat", "hands=Cloth Gloves", "legs=Silk Breeches", "feet=Leather Boots", "neck=Blessed Necklace", "finger1=Ring of Blessings"],
    Jekhar: ["weapon=Heavy War Axe", "torso=Boiled Leather", "hands=Iron Gauntlets", "feet=Heavy Boots", "neck=Barbarian's Torque", "finger1=Berserker Ring"] },
  { gold: 10000, items: ["Healing Draught=30", "Healing Elixir=10", "Recovery Draught=15", "Cleansing Elixir=1"], rings: ["Ring of Darkness", "Ring of Fire", "Ring of Stone", "Ring of Water", "Ring of Light", "Ring of Jade"],
    Joseph: ["weapon=Logaros Longsword", "shield=Kite Shield", "torso=Hero's Chainmail", "hands=Steel Gauntlets", "legs=Chainmail Leggings", "feet=Battle Boots", "neck=Medallion of Power", "finger1=Ring of Stone", "finger2=Ring of Water"],
    Flece: ["weapon=Gutting Dirk", "torso=Shadow Haramaki", "hands=Shadow Gauntlets", "legs=Prowler Leggings", "feet=Boots of Stealth", "neck=Necklace of Stealth", "finger1=Ring of Shadows"],
    Rosalind: ["weapon=Nagitana", "torso=Arcticus Mantellum", "hands=Magus Gauntlets", "legs=Magus Leggings", "feet=Magus Boots", "neck=Collar of Health", "finger1=Ring of Recovery"],
    Jekhar: ["weapon=Foecleaver", "torso=Breastplate", "hands=Plate Gauntlets", "feet=Sabatons", "neck=Berserker's Torque", "finger1=Onslaught Ring"] },
  { gold: 30000, items: ["Healing Elixir=50", "Elixir of Recovery=20", "Recovery Draught=20", "Cleansing Elixir=1"], rings: ["Ring of Darkness", "Ring of Fire", "Ring of Stone", "Ring of Water", "Ring of Light", "Ring of Jade", "Ring of Forest", "Ring of Four Winds"],
    Joseph: ["weapon=Sword of Summoners", "torso=Summoner Chainmail", "hands=Summoner Gauntlets", "legs=Summoner Leggings", "feet=Summoner Sabaton", "neck=Summoner's Torque", "finger1=Ring of Jade", "finger2=Ring of Four Winds"],
    Flece: ["weapon=Decapitator", "torso=Mellifuous Wrappings", "hands=Gloves of Pilfering", "legs=Hero's Leggings", "feet=Boots of Stealth", "neck=Necklace of Anshu", "finger1=Ring of Invisibility"],
    Rosalind: ["weapon=Thunder Nagitana", "torso=Draco Robe", "hands=Midnight Gauntlets", "legs=Magus Leggings", "feet=Magus Boots", "neck=Medallion of Vigor", "finger1=Ring of the Gods"],
    Jekhar: ["weapon=Ghork Smashor", "torso=Hero's Platemail", "hands=Hero's Gauntlets", "feet=Hero's Sabaton", "neck=Battle Torque", "finger1=Onslaught Ring", "finger2=Ring of Puissance"] },
];

// The summoning rings in the order the story gives them (act 1 to the Jade Temple).
const STORY_RINGS = ["Ring of Darkness", "Ring of Light", "Ring of Fire", "Ring of Stone", "Ring of Water", "Ring of Four Winds", "Ring of Forest", "Ring of Jade"];
const saveInfo = (f) => JSON.parse(execFileSync("node", [join(repo, "tools/savinfo.mjs"), "--json", "--tables", join(G, "tables.vpp"), f], { encoding: "utf8" })).saves[0];
const pick = (id) => { const l = recipes.filter((r) => id === "all" || r.id === id); if (!l.length) throw new Error(`no recipe ${id}`); return l; };
const pos = (o) => { const m = /(-?[\d.]+) (-?[\d.]+) (-?[\d.]+)/.exec(o || ""); return m ? m.slice(1).map(Number) : null; };
function scriptTable(script) {
  const f = join(W, "tbl", `${script}_script.tbl`);
  if (!existsSync(f)) execFileSync("node", [join(repo, "tools/vpp.mjs"), "extract", join(G, "tables.vpp"), "_script.tbl", "--out", join(W, "tbl")]);
  const hit = existsSync(f) ? f : null;
  return hit ? readFileSync(hit, "latin1") : "";
}
function navpoint(script, name) {
  const t = scriptTable(script), i = t.indexOf(`"${name}"`);
  const m = i >= 0 && /\$Position:\s*<\s*(-?[\d.]+),\s*(-?[\d.]+),\s*(-?[\d.]+)/.exec(t.slice(i));
  return m ? m.slice(1).map(Number) : null;
}
// Esc through cutscenes (game_flags 0x20) and tutorial pages or the pause menu (0x4) until play runs quietly.
async function skipToPlay(needCalm = 3) {
  let calm = 0;
  for (let i = 0; i < 80 && calm < needCalm; i++) {
    const o = run("info; peek 5fbfd8 4");
    if (/ERR/.test(o) && !/state=/.test(o)) return false;
    const f = parseInt((/5FBFD8: ([0-9A-F]{2})/.exec(o) || [])[1] || "0", 16);
    if (/state=4/.test(o) && !(f & 0x24)) calm++; else calm = 0;
    if (f & 0x20) run("key Esc; wait 1500"); else if (f & 0x4) run("key Esc; wait 1000"); else await sleep(1500);
  }
  return calm >= needCalm;
}
function startFrom(save) {
  agent("stop");
  if (existsSync(join(G, "agent", "cmd.txt"))) rmSync(join(G, "agent", "cmd.txt"));
  const dst = join(G, "savetest", "summoner40.sav"); // the newest save is the Load screen's first row
  copyFileSync(save, dst); const now = new Date(); utimesSync(dst, now, now);
  agent("start", "--wait", "90");
  run("click left 858 368; wait 2000; click left 798 368; wait 20000");
  return /state=4/.test(run("info"));
}

const [cmd, id = "all"] = process.argv.slice(2);
if (cmd === "base") {
  agent("stop");
  agent("start", "--wait", "90");
  run("click left 858 296; wait 5000");   // Start New Game
  await skipToPlay();
  run("savenow; wait 1500");
  copyFileSync(join(G, "savetest", "quicksave.sav"), join(W, "base-newgame.sav"));
  console.log(`base: ${join(W, "base-newgame.sav")}`);
  agent("stop");
} else if (cmd === "build") {
  for (const R of pick(id)) {
    const log = (m) => console.log(`[${R.id}] ${m}`);
    if (!startFrom(join(W, "base-newgame.sav"))) { log("the base save did not load"); continue; }
    for (const fl of [...(R.flags || []), ...(R.preflags || [])]) {
      const m = /^([^:]+):(.+)=([01])$/.exec(fl), li = LEVELS.findIndex((l) => l.toLowerCase() === m[1].toLowerCase());
      log(run(`flagl ${li} ${m[2]} ${m[3]}`).trim());
    }
    if (R.level !== "masad") { log(run(`level ${R.level} ${R.script}`).trim()); await sleep(15000); }
    log(`in play: ${await skipToPlay()}`);
    // join returns a hero already in the party (one the level's story added) unchanged, at the story's level: xpto
    // after it raises him to the recipe's level (2026-11-08; until then 03, 04 and 08 to 12 had companions 1 to 7
    // levels below the recipe).
    for (const [name, lv] of R.party.split(",").map((p) => p.split(":"))) {
      if (name !== "Joseph") { log(run(`join ${name} ${lv}`).trim()); await sleep(800); }
      log(run(`xpto ${name} ${lv}`).trim()); await sleep(800);
    }
    if (R.place === "npc") {
      log(run(`npcgoto ${R.at}`).trim());
      log(run(`wait 600; nudge ${R.nudge}`).trim());
    } else if (R.place === "nav") {
      const p = pos(run("nudge 0 0")), np = navpoint(R.script, R.at), stand = Math.abs(Number(R.nudge.split(" ")[1])) + 6;
      if (p && np) {
        let dx = p[0] - np[0], dz = p[2] - np[2]; const d = Math.hypot(dx, dz) || 1;
        if (d > stand) { dx = np[0] + dx / d * stand - p[0]; dz = np[2] + dz / d * stand - p[2]; log(run(`nudge ${dx.toFixed(2)} ${dz.toFixed(2)}`).trim()); }
      } else log(`navpoint ${R.at} not found`);
    }
    await sleep(2500);
    run(`capture ${R.id}-pos; savenow; wait 1500`);
    copyFileSync(join(G, "savetest", "quicksave.sav"), join(W, `raw-${R.id}.sav`));
    log(`saved raw-${R.id}.sav`);
  }
  agent("stop");
} else if (cmd === "finish") {
  const TITLE = (s) => s.replace(/^\d+-/, "").split("-").map((w) => (["and", "the", "of"].includes(w) ? w : w[0].toUpperCase() + w.slice(1))).join(" ");
  mkdirSync(join(repo, "testsaves"), { recursive: true });
  for (const R of pick(id)) {
    const raw = join(W, `raw-${R.id}.sav`), out = join(repo, "testsaves", `${R.id}.sav`);
    if (!existsSync(raw)) { console.log(`${R.id}: no raw save`); continue; }
    if (existsSync(out)) rmSync(out);
    // The members come from the save: heroes who join in the level (Rosalind and Jekhar in 12) are not in R.party.
    const rawInfo = saveInfo(raw), kit = KIT[R.tier], members = rawInfo.party.map((m) => m.name);
    const worn = new Set(Object.values(kit).flat().filter((e) => typeof e === "string" && /^finger\d=/.test(e)).map((e) => e.split("=")[1]));
    const a = [raw, out, "--tables", join(G, "tables.vpp"), "--gold", String(kit.gold), "--desc", `TEST ${R.id.slice(0, 2)}: ${TITLE(R.id)} (${R.level})`];
    // Each summoning ring once (2026-11-08): the raw save already holds the rings its story gave, and a worn ring is
    // not also in the inventory (as in a played game); until then the rings were added again and the worn ones kept.
    const held = new Set(rawInfo.inventory.entries.map((e) => e.name));
    for (const it of [...kit.items, ...kit.rings.filter((r) => !worn.has(r) && !held.has(r))]) a.push("--add-item", it);
    for (const r of worn) if (held.has(r)) a.push("--remove-item", r);
    for (const m of members) {
      if (!kit[m]) continue;
      a.push("--member", m);
      // The first fight keeps Joseph's starting sword, tunic and pants: he is level 3 in the prologue.
      for (const e of kit[m]) if (!(R.id.startsWith("01-") && m === "Joseph" && /^(weapon|shield|torso|legs)/.test(e))) a.push("--equip", e);
    }
    if (R.replay) a.push("--flag", R.replay + "=0");
    for (const fl of R.flags || []) a.push("--flag", fl);
    try { execFileSync("node", [join(repo, "tools/savedit.mjs"), ...a], { encoding: "utf8" }); console.log(`${R.id}: ok`); }
    catch (e) { console.log(`${R.id}: FAILED ${(e.stderr || e.message).trim().split("\n")[0]}`); }
  }
} else if (cmd === "verify") {
  const hp = (f) => execFileSync("node", [join(repo, "tools/savinfo.mjs"), "--tables", join(G, "tables.vpp"), f], { encoding: "utf8" })
    .split("\n").map((l) => /#\d+ (\w+): level \d+.*HP ([\d.]+)\//.exec(l)).filter(Boolean).map((m) => [m[1], Number(m[2])]);
  async function skip() {
    for (let i = 0; i < 40; i++) {
      const o = run("peek 5fbfd8 4"), f = parseInt((/5FBFD8: ([0-9A-F]{2})/.exec(o) || [])[1] || "0", 16);
      if (/ERR/.test(o)) return "game gone";
      // A cutscene plays out (up to 70 s): skipping one with Esc can drop its end handler (Titus stayed inert), then Esc.
    if (f & 0x20) run(i < 14 ? "wait 5000" : "key Esc; wait 1500"); else if (f & 0x4) run("key Esc; wait 800"); else return "ok";
    }
    return "still in a cutscene";
  }
  for (const R of pick(id)) {
    const save = join(repo, "testsaves", `${R.id}.sav`);
    if (!existsSync(save)) { console.log(`${R.id}: no save`); continue; }
    if (!startFrom(save)) { console.log(`${R.id}: did not load`); continue; }
    await skip();
    if (R.go && R.go !== "npc") run(`navgoto ${R.go}; wait 2500`);
    await skip();
    const g = run(`npcgoto ${R.boss}`).trim().split("\n")[0];
    if (R.label) run(`wait 1000; callstr 4ce3a0 ${R.label} 1`);   // script_npc_goto_label (0x4ce3a0)
    await sleep(3000);
    await skip();                                                 // an intro the approach started
    run(`npcgoto ${R.boss}`);
    await sleep(35000);
    await skip();
    run(`capture ${R.id}-fight; savenow; wait 1500`);
    const alive = /state=/.test(run("info"));
    const before = hp(save), after = alive ? hp(join(G, "savetest", "quicksave.sav")) : [];
    const fought = after.some(([n, h]) => { const b = before.find((x) => x[0] === n); return b && h < b[1] - 1; });
    const line = `${R.id}: ${!alive ? "GAME CLOSED" : fought ? "fight: the party took damage" : "no damage seen"}; ${g}; ` +
      after.map(([n, h]) => `${n} ${Math.round((before.find((x) => x[0] === n) || [0, 0])[1])}->${Math.round(h)}`).join(", ");
    console.log(line); appendFileSync(join(W, "verify.log"), line + "\n");
  }
  agent("stop");
} else if (cmd === "gear") {
  // The play-through set ended its fights through the damage function, so its heroes kept their starting gear and
  // gold stayed at 59 (2026-11-06). This puts a tier's kit on an existing save, rewritten in place.
  const file = resolve(id), kit = KIT[Number(process.argv[4])];
  if (!kit || !existsSync(file)) { console.log("usage: node tools/testsaves/testsaves.mjs gear FILE TIER(0-3)"); process.exit(2); }
  const info = saveInfo(file), tmp = file + ".gear.tmp", have = new Map(info.inventory.entries.map((e) => [e.name, e.count]));
  if (existsSync(tmp)) rmSync(tmp);
  const a = [file, tmp, "--tables", join(G, "tables.vpp")];
  if (info.inventory.gold < kit.gold) a.push("--gold", String(kit.gold));
  for (const it of kit.items) {
    const [n, c] = it.split("="), want = Number(c), now = have.get(n) || 0;
    if (now < want) a.push("--add-item", `${n}=${want - now}`);
  }
  const rings = STORY_RINGS.filter((r) => have.has(r)).slice(-2);
  for (const m of info.party) {
    if (!kit[m.name]) continue;
    a.push("--member", m.name);
    for (const e of kit[m.name]) if (!(m.name === "Joseph" && /^finger\d=/.test(e))) a.push("--equip", e);
    if (m.name === "Joseph")
      rings.forEach((r, i) => a.push("--equip", `finger${i + 1}=${r}`, "--remove-item", `${r}=1`));
  }
  execFileSync("node", [join(repo, "tools/savedit.mjs"), ...a], { encoding: "utf8" });
  rmSync(file); copyFileSync(tmp, file); rmSync(tmp);
  console.log(`${file}: tier ${process.argv[4]}, Joseph wears ${rings.join(" and ") || "no ring"}`);
} else console.log("usage: node tools/testsaves/testsaves.mjs base|build|finish|verify|gear [ID|all]");
