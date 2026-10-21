// Acceptance tests of the C savegame editor (work/saveedit.exe) against the Node engine (tools/savedit.mjs).
// usage: run from the repository folder:  node <this folder>/test.mjs [--only NAME] [--saves N]
//   a. round trip: saveedit --roundtrip on every corpus save gives a byte-identical file
//   b. differential: every case below runs with node tools/savedit.mjs and with saveedit.exe on the same input;
//      an accepted edit must give byte-identical outputs, a refused one must be refused by both with nothing
//      written. Item, flag and member names are taken from each save at run time (savinfo --json and the
//      tables), so no game data is stored here.
// Outputs go to work/run-<time>/ next to this script. A file is deleted (by its exact path) only after its
// case passed; files of failed cases stay for inspection. Results: work/run-<time>/results.txt.
import { spawnSync } from "node:child_process";
import { readFileSync, writeFileSync, existsSync, mkdirSync, readdirSync, unlinkSync, rmdirSync, copyFileSync, appendFileSync } from "node:fs";
import { join, dirname, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const HERE = dirname(fileURLToPath(import.meta.url));
const REPO = process.cwd();
if (!existsSync(join(REPO, "tools", "savedit.mjs"))) { console.error("run this from the repository folder"); process.exit(2); }
const EXE = join(REPO, "build", "saveeditor", "saveedit.exe");
const TABLES = resolve(REPO, "Summoner", "tables.vpp");
// --corpus DIR: a folder of COPIES of saves (never a game's savegame folder); --out DIR: where the run folder goes
// (default: the system temp folder).
const argOf = (n) => { const i = process.argv.indexOf(n); return i > 0 ? process.argv[i + 1] : undefined; };
const CORPUS = argOf("--corpus");
if (!CORPUS) { console.error("usage: node tools/saveedit-test.mjs --corpus DIR [--out DIR] [--saves N] [--only TEXT]"); process.exit(2); }
if (/[\\/]savegame[\\/]?$/i.test(CORPUS)) { console.error("--corpus must be a folder of copies, not a savegame folder"); process.exit(2); }
const RUN = join(argOf("--out") ?? join((await import("node:os")).tmpdir(), "saveedit-test"), `run-${new Date().toISOString().replace(/[:.]/g, "-")}`);
mkdirSync(RUN, { recursive: true });
const LOG = join(RUN, "results.txt");
const log = (s) => { console.log(s); appendFileSync(LOG, s + "\n"); };

const argv = process.argv.slice(2);
const only = argv.includes("--only") ? argv[argv.indexOf("--only") + 1] : null;
const maxSaves = argv.includes("--saves") ? Number(argv[argv.indexOf("--saves") + 1]) : Infinity;

const lib = await import(pathToFileURL(join(REPO, "tools", "lib", "savedata.mjs")).href);
const tables = lib.loadTables({ tablesVpp: TABLES });
const saves = readdirSync(CORPUS).filter((n) => n.toLowerCase().endsWith(".sav")).sort().slice(0, maxSaves);

const runNode = (args) => spawnSync(process.execPath, [join("tools", "savedit.mjs"), ...args, "--tables", TABLES], { cwd: REPO, encoding: "latin1" });
const runC = (args) => spawnSync(EXE, [...args, "--tables", TABLES], { cwd: REPO, encoding: "latin1" });
const firstDiff = (a, b) => { const n = Math.min(a.length, b.length); for (let i = 0; i < n; i++) if (a[i] !== b[i]) return i; return a.length === b.length ? -1 : n; };
const rm = (p) => { if (existsSync(p)) unlinkSync(p); };

// ---------------------------------------------------------------------------------------------------------
// a. round trip
let rtPass = 0, rtFail = 0;
log(`== round trip (${saves.length} saves)`);
for (const s of saves) {
  if (only && only !== "roundtrip") break;
  const inp = join(CORPUS, s), out = join(RUN, `rt-${s}`);
  const r = spawnSync(EXE, ["--roundtrip", inp, out], { encoding: "latin1" });
  const ok = r.status === 0 && existsSync(out) && Buffer.compare(readFileSync(inp), readFileSync(out)) === 0;
  if (ok) { rtPass++; rm(out); } else { rtFail++; log(`FAIL roundtrip ${s}: exit ${r.status} ${r.stderr}`); }
}
log(`round trip: ${rtPass} pass, ${rtFail} fail`);

// ---------------------------------------------------------------------------------------------------------
// b. differential cases, built per save
function casesFor(file) {
  const b = readFileSync(file);
  const types = lib.itemTypes(b), inv = lib.readInventory(b), party = lib.partyMembers(b);
  const info = lib.describeSave(b, tables);
  const named = inv.entries.filter((e) => tables.items.get(e.hash));
  const item = named[0] ? tables.items.get(named[0].hash) : null;
  const held = (h) => inv.entries.filter((e) => e.hash === h).reduce((a, e) => a + e.count, 0);
  const multi = named.find((e) => held(e.hash) >= 2);
  const itemMulti = multi ? tables.items.get(multi.hash) : item;
  const notHeld = types.map((h) => tables.items.get(h)).find((n, i) => n && !held(types[i]));
  const levels = info.levels;
  const lvSet = levels.find((l) => l.flagsSet.length) ?? null;
  const lvAny = levels.find((l) => l.flagCount) ?? null;
  const unsetOf = (l) => Object.keys(l.flags).find((n) => !l.flags[n]);
  const lvUnset = levels.find((l) => unsetOf(l)) ?? null;
  const spaced = levels.flatMap((l) => Object.keys(l.flags).filter((n) => n.includes(" ")).map((n) => [l, n]))[0] ?? null;
  // a flag that is the +Invalidate: of a quest active in this save
  let inval = null;
  for (const q of tables.quests) {
    if (!q.invalidate || !lib.questBit(b, q.index)) continue;
    const l = levels.find((x) => Object.keys(x.flags).some((n) => lib.nameHash(n) === lib.nameHash(q.invalidate)));
    if (l) { inval = [l, Object.keys(l.flags).find((n) => lib.nameHash(n) === lib.nameHash(q.invalidate))]; break; }
  }
  const names = party.map((p) => ["Joseph", "Flece", "Jekhar", "Rosalind"].find((n) => lib.nameHash(n) === p.hash)).filter(Boolean);
  const c = [];
  const add = (name, args, expect = "ok") => c.push({ name, args, expect });
  add("desc", ["--desc", "Edited by the saveedit test"]);
  add("desc latin1", ["--desc", "Café über ÆØÅ ÿ"]);
  add("desc non-latin1", ["--desc", "dash — euro €"]);
  add("desc 255", ["--desc", "x".repeat(255)]);
  add("desc 256 (refused)", ["--desc", "y".repeat(256)], "refuse");
  add("desc empty", ["--desc", ""]);
  add("gold", ["--gold", "5000"]);
  add("gold max", ["--gold", "2147483647"]);
  add("gold min", ["--gold", "-2147483648"]);
  add("gold out of range (refused)", ["--gold", "2147483648"], "refuse");
  add("gold not a number (refused)", ["--gold", "12x"], "refuse");
  if (item) {
    add("add-item", ["--add-item", item]);
    add("add-item =3", ["--add-item", `${item}=3`]);
    add("add-item upper case", ["--add-item", item.toUpperCase()]);
    add("add-item padded", ["--add-item", `  ${item.toLowerCase()} =2`]);
    add("add-item =0 (refused)", ["--add-item", `${item}=0`], "refuse");
    add("remove-item all", ["--remove-item", item]);
    add("remove-item =1", ["--remove-item", `${itemMulti}=1`]);
    add("remove-item too many (refused)", ["--remove-item", `${item}=99999`], "refuse");
    add("set-item-count 7", ["--set-item-count", `${item}=7`]);
    add("set-item-count 0", ["--set-item-count", `${item}=0`]);
    add("set-item-count no count (refused)", ["--set-item-count", item], "refuse");
  }
  if (notHeld) {
    add("set-item-count not held =0", ["--set-item-count", `${notHeld}=0`]);
    add("set-item-count not held =2", ["--set-item-count", `${notHeld}=2`]);
    add("remove-item not held (refused)", ["--remove-item", notHeld], "refuse");
  }
  add("unknown item (refused)", ["--add-item", "No Such Item Zz"], "refuse");
  if (lvUnset) {
    const f = unsetOf(lvUnset);
    add("flag name on", ["--flag", `${lvUnset.name}:${f}=1`]);
    add("flag index on", ["--flag", `${lvUnset.index}:${f}=1`]);
    add("flag lower-case level on", ["--flag", `${lvUnset.name.toLowerCase()}:${f}=1`]);
  }
  if (lvSet) {
    add("flag name off", ["--flag", `${lvSet.name}:${lvSet.flagsSet[0]}=0`]);
    add("flag index off", ["--flag", `${lvSet.index}:${lvSet.flagsSet[0]}=0`]);
    add("flag set again on", ["--flag", `${lvSet.name}:${lvSet.flagsSet[0]}=1`]);
  }
  if (spaced) add("flag name with spaces", ["--flag", `${spaced[0].name}:${spaced[1]}=1`]);
  if (inval) add("flag that invalidates a quest", ["--flag", `${inval[0].name}:${inval[1]}=1`]);
  if (lvAny) {
    add("flag wrong level (refused)", ["--flag", `${lvAny.index === 0 ? 1 : 0}:${Object.keys(lvAny.flags)[0]}x=1`], "refuse");
    add("flag bad value (refused)", ["--flag", `${lvAny.name}:${Object.keys(lvAny.flags)[0]}=2`], "refuse");
  }
  add("flag unknown level (refused)", ["--flag", "nolevel:x=1"], "refuse");
  add("flag index 51 (refused)", ["--flag", "51:x=1"], "refuse");
  add("member all fields", ["--member", "0", "--level", "50", "--xp", "123456", "--hp", "321.5", "--maxhp", "400.25", "--ap", "77", "--maxap", "88", "--skillpoints", "9"]);
  add("member default fields", ["--level", "-3", "--xp", "-5", "--hp", "0.1", "--maxhp", "-0", "--ap", "-32768", "--maxap", "32767", "--skillpoints", "-2147483648"]);
  add("hp 12.5", ["--hp", "12.5"]);
  add("hp long decimal", ["--hp", "3.14159265358979323846264338327950288"]);
  add("hp huge", ["--maxhp", "9".repeat(60)]);
  add("hp exponent (refused)", ["--hp", "1e3"], "refuse");
  add("level 32767", ["--level", "32767"]);
  add("level 32768 (refused)", ["--level", "32768"], "refuse");
  add("level twice (refused)", ["--level", "1", "--level", "2"], "refuse");
  add("skill", ["--skill", "3=20"]);
  add("skill bounds", ["--skill", "35=255", "--skill", "0=0"]);
  add("skill id 36 (refused)", ["--skill", "36=1"], "refuse");
  add("skill value 256 (refused)", ["--skill", "0=256"], "refuse");
  add("skill twice (refused)", ["--skill", "1=10", "--skill", "1=11"], "refuse");
  if (names.length >= 2) {
    add("member by name", ["--member", names[1], "--level", "33", "--skill", "2=30"]);
    add("member name then index", ["--member", names[names.length - 1], "--level", "12", "--member", "0", "--xp", "1", "--level", "13"]);
    add("member name lower case", ["--member", names[1].toLowerCase(), "--ap", "5"]);
  }
  add("member out of range (refused)", ["--member", String(party.length), "--level", "1"], "refuse");
  add("member unknown name (refused)", ["--member", "Nobody", "--level", "1"], "refuse");
  add("member only: no edits (refused)", ["--member", "0"], "refuse");
  add("two edits", ["--gold", "777", "--desc", "two edits"]);
  if (item && lvUnset) add("combined", ["--gold", "1234", "--add-item", `${item}=2`, "--flag", `${lvUnset.name}:${unsetOf(lvUnset)}=1`, "--member", "0", "--level", "10", "--skill", "4=40", "--desc", "combined"]);
  add("desc twice (refused)", ["--desc", "a", "--desc", "b"], "refuse");
  add("unknown option (refused)", ["--golden", "1"], "refuse");
  add("missing value (refused)", ["--gold"], "refuse");
  add("output exists (refused)", ["--gold", "1"], "exists");
  add("same file in and out (refused)", ["--gold", "1"], "same");
  // table options (the other sources of the tables)
  add("--lang upper case", ["--gold", "6", "--lang", "ENGLISH"]);
  add("--lang without a section: gold", ["--gold", "6", "--lang", "klingon"]);
  if (lvUnset) {
    const fl = ["--flag", `${lvUnset.name}:${unsetOf(lvUnset)}=1`];
    add("--lang without a section: flag (refused)", [...fl, "--lang", "klingon"], "refuse");
    add("--events-tbl missing: flag (refused)", [...fl, "--events-tbl", join(TDIR, "missing.tbl")], "refuse");
    add("--tables-dir: flag", [...fl, "--tables-dir", TDIR]);
    add("--items-tbl and --events-tbl: flag", [...fl, "--items-tbl", join(TDIR, "items.tbl"), "--events-tbl", join(TDIR, "events.tbl")]);
  }
  if (item) add("--tables-dir: add-item", ["--add-item", item, "--tables-dir", TDIR]);
  return c;
}
// extracted tables for the --tables-dir / --items-tbl / --events-tbl cases (temporary, inside the run folder)
const TDIR = join(RUN, "tables");
{
  const r = spawnSync(process.execPath, [join("tools", "vpp.mjs"), "extract", TABLES, "*.tbl", "--out", TDIR], { cwd: REPO, encoding: "latin1" });
  if (r.status !== 0) { console.error("cannot extract the tables: " + r.stderr); process.exit(2); }
}

const stats = new Map();
const bump = (name, ok) => { const s = stats.get(name) ?? { pass: 0, fail: 0 }; ok ? s.pass++ : s.fail++; stats.set(name, s); };
let n = 0;
for (const s of saves) {
  if (only === "roundtrip") break;
  const inp = join(CORPUS, s);
  for (const cs of casesFor(inp)) {
    if (only && !cs.name.includes(only)) continue;
    const tag = `${s.replace(/\.sav$/i, "")}-${n++}`;
    let outN = join(RUN, `${tag}-node.sav`), outC = join(RUN, `${tag}-c.sav`), inN = inp, inC = inp, ok, why = "";
    const placeholder = Buffer.from("placeholder: must not be overwritten\n");
    if (cs.expect === "exists") { writeFileSync(outN, placeholder); writeFileSync(outC, placeholder); }
    if (cs.expect === "same") { // a private copy, given as both input and output
      inN = outN; inC = outC; copyFileSync(inp, outN); copyFileSync(inp, outC);
    }
    const rn = runNode([inN, outN, ...cs.args]), rc = runC([inC, outC, ...cs.args]);
    if (cs.expect === "exists" || cs.expect === "same") {
      const ref = cs.expect === "exists" ? placeholder : readFileSync(inp);
      const keptN = Buffer.compare(readFileSync(outN), ref) === 0, keptC = Buffer.compare(readFileSync(outC), ref) === 0;
      ok = rn.status !== 0 && rc.status !== 0 && keptN && keptC;
      if (!ok) why = `exit node ${rn.status} c ${rc.status}, file kept node ${keptN} c ${keptC}`;
    } else if (cs.expect === "refuse") {
      ok = rn.status !== 0 && rc.status !== 0 && !existsSync(outN) && !existsSync(outC);
      if (!ok) why = `exit node ${rn.status} c ${rc.status}, written node ${existsSync(outN)} c ${existsSync(outC)}`;
    } else {
      const wn = rn.status === 0 && existsSync(outN), wc = rc.status === 0 && existsSync(outC);
      if (wn && wc) {
        const a = readFileSync(outN), bb = readFileSync(outC), d = firstDiff(a, bb);
        ok = d < 0;
        if (!ok) why = `outputs differ at 0x${d.toString(16)}`;
      } else { ok = false; why = `exit node ${rn.status} c ${rc.status}`; }
    }
    bump(cs.name, ok);
    if (ok) { rm(outN); rm(outC); }
    else log(`FAIL ${cs.name} on ${s} [${cs.args.map((x) => JSON.stringify(x)).join(" ")}]: ${why}\n  node: ${(rn.stderr || rn.stdout).trim()}\n  c:    ${(rc.stderr || rc.stdout).trim()}`);
  }
}
log(`== differential (${saves.length} saves)`);
let tp = 0, tf = 0;
for (const [name, s] of stats) { log(`${s.fail ? "FAIL" : "pass"}  ${name}: ${s.pass} pass, ${s.fail} fail`); tp += s.pass; tf += s.fail; }
log(`differential: ${stats.size} cases, ${tp} runs pass, ${tf} fail`);
log(`round trip: ${rtPass}/${rtPass + rtFail}`);
// remove the extracted tables again: each file by its exact name, then the (now empty) folder
for (const f of readdirSync(TDIR)) unlinkSync(join(TDIR, f));
rmdirSync(TDIR);
process.exit(rtFail || tf ? 1 : 0);
