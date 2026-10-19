// The save and memory check of the standalone rebuilt exe (tools/link-build.mjs) against the original, level by level,
// in the rebuilt exe's test folder (Sum-rebuilt.exe and Sum-orig.exe side by side, one summonerfix.ini). For each level:
// the original --runs times and the rebuilt exe once, each starting from the newest test save, warping there with the
// full party (`level`), dumping every data symbol of the address table (`dumpdata`) and quicksaving. Then the
// rebuilt save against the original runs (tools/save-compare.mjs, the hybrid's save comparison) and the rebuilt dump
// against the original dumps (tools/link-memcheck.mjs --also). The save covers game state (positions, flags, stats,
// inventory, script state), the dump the globals that never reach a save; neither sees rendering or timing.
//
//   node tools/link-savecheck.mjs <out dir> LEVEL:SCRIPT ... [--runs 3] [--game DIR] [--ini-backup FILE]
//
// <out dir> must have no spaces (agent arguments are split on spaces). The test folder (default
// SummonerToolchain/rebuilt-test) must hold savetest/ with the start save; [Debug] TestSaves=1 keeps every save there.
// The ini is restored from --ini-backup (default <game>/summonerfix.ini.savecheck, made from the folder's ini on the
// first run) before each session and at the end. Results are appended to <out dir>/results.txt as each level ends.
import { readFileSync, writeFileSync, copyFileSync, existsSync, utimesSync, rmSync, mkdirSync, appendFileSync } from "node:fs";
import { spawnSync } from "node:child_process";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { compareSaves } from "./save-compare.mjs";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const opt = (name, def) => { const i = args.indexOf(name); return i >= 0 ? args[i + 1] : def; };
const game = opt("--game", join(repo, "..", "..", "SummonerToolchain", "rebuilt-test"));
const runs = +opt("--runs", 3);
const ini = join(game, "summonerfix.ini");
const backup = opt("--ini-backup", join(game, "summonerfix.ini.savecheck"));
const quicksave = join(game, "savetest", "quicksave.sav");
const OLD = new Date("2000-01-01T00:00:00");
const optArgs = new Set(["--runs", "--game", "--ini-backup"]);
const positional = args.filter((a, i) => !a.startsWith("--") && !optArgs.has(args[i - 1]));
const [out, ...levels] = positional;
if (!out || !levels.length || /\s/.test(out)) {
  console.error("usage: node tools/link-savecheck.mjs <out dir, no spaces> LEVEL:SCRIPT ... [--runs 3]");
  process.exit(2);
}
if (!existsSync(quicksave)) throw new Error(`${quicksave} is missing: put the start save and a quicksave.sav in savetest/`);
mkdirSync(out, { recursive: true });
if (!existsSync(backup)) copyFileSync(ini, backup);
const table = join(out, "table.addr");
copyFileSync(join(game, "Sum-rebuilt.addr"), table);
const results = join(out, "results.txt");

const env = (exe) => ({ ...process.env, SUMMONER_GAME: game, SUMMONER_EXE: exe });
const kill = (exe) => spawnSync("taskkill", ["/F", "/IM", exe]);
const agent = (exe, cmds, timeout = 180000) =>
  spawnSync("node", [join(repo, "tools", "agent.mjs"), "run", cmds], { cwd: repo, env: env(exe), encoding: "latin1", timeout }).stdout ?? "";

function setKey(text, section, key, value) {
  const re = new RegExp(`(^\\[${section}\\][^\\[]*)`, "m"), m = re.exec(text);
  if (!m) return text + `\r\n[${section}]\r\n${key}=${value}\r\n`;
  const line = new RegExp(`^${key}=.*$`, "m");
  return text.replace(m[1], line.test(m[1]) ? m[1].replace(line, `${key}=${value}`) : m[1].replace(/\s*$/, `\r\n${key}=${value}\r\n`));
}

// Start a session of one exe on the newest test save.
function session(exe) {
  kill("Sum-orig.exe");
  kill("Sum-rebuilt.exe");
  rmSync(join(game, "agent", "cmd.txt"), { force: true });
  rmSync(join(game, "errorlog.txt"), { force: true });
  let text = readFileSync(backup, "latin1");
  text = setKey(text, "Debug", "TestSaves", "1");
  writeFileSync(ini, text, "latin1");
  spawnSync("node", [join(repo, "tools", "agent.mjs"), "start", "--wait", "60"], { cwd: repo, env: env(exe), encoding: "latin1" });
  // Main menu: Load Saved Game, the newest save, then wait for the level.
  return agent(exe, "wait 4000; click 744 392; wait 2500; click 798 368; wait 15000");
}

// One run: warp, dump, quicksave. Returns the problem or "".
function runOnce(exe, level, sav, dump) {
  const [name, script] = level.split(":");
  utimesSync(quicksave, OLD, OLD);
  const before = readFileSync(quicksave);
  session(exe);
  const res = agent(exe, `poke 609fe0 1; level ${name} ${script}; wait 8000; postkey Esc; wait 1500; dumpdata ${dump} ${table}; call0 40e7c0; wait 3000`);
  kill(exe);
  const log = readFileSync(join(game, "summonerfix.log"), "latin1");
  const after = readFileSync(quicksave);
  utimesSync(quicksave, OLD, OLD);
  const m = /(MessageBox from[^\r\n]*|access violation[^\r\n]*|ExitProcess\(3221225477\)[^\r\n]*)/i.exec(log);
  if (m) return m[1] + (existsSync(join(game, "errorlog.txt")) ? " (errorlog.txt)" : "");
  if (after.equals(before)) return "no new save" + (/timed out/.test(res) ? " (no frames)" : "");
  writeFileSync(sav, after);
  return "";
}

const note = (line) => { console.log(line); appendFileSync(results, line + "\n"); };
try {
  for (const level of levels) {
    const tag = level.split(":")[0].replace(/[^A-Za-z0-9_-]/g, "_");
    const p = (s) => join(out, `${tag}-${s}`);
    let problem = "";
    for (let i = 0; i < runs && !problem; i++) {
      const why = runOnce("Sum-orig.exe", level, p(`orig${i}.sav`), p(`orig${i}.bin`));
      if (why) problem = `original run ${i}: ${why}`;
    }
    if (problem) { note(`${level}: skipped, ${problem}`); continue; }
    const why = runOnce("Sum-rebuilt.exe", level, p("reb.sav"), p("reb.bin"));
    if (why) { note(`${level}: REBUILT FAILED: ${why}`); continue; }
    const bad = compareSaves([...Array(runs).keys()].map((i) => p(`orig${i}.sav`)), p("reb.sav"));
    const also = runs > 1 ? ["--also", [...Array(runs - 1).keys()].map((i) => p(`orig${i + 1}.bin`)).join(",")] : [];
    const mem = spawnSync("node", [join(repo, "tools", "link-memcheck.mjs"), p("orig0.bin"), p("reb.bin"), ...also, "--show", "40"],
      { cwd: repo, encoding: "latin1" }).stdout ?? "";
    note(`${level}: save ${bad.length} dwords differ beyond the original runs' spread; memory ${mem.split(/\r?\n/)[0]}`);
    for (const [, , line] of bad.slice(0, 25)) note("  save " + line);
    for (const l of mem.split(/\r?\n/).slice(1).filter(Boolean)) note("  mem" + l);
  }
} finally {
  kill("Sum-orig.exe");
  kill("Sum-rebuilt.exe");
  rmSync(join(game, "agent", "cmd.txt"), { force: true });
  copyFileSync(backup, ini);
}
