// Long play of a rebuilt exe in its test folder (docs/tools.md): from the newest test save, every level in turn with
// the full party (`level NAME NAME`), a summon raised and walls of fire cast in each, a capture per level. The game
// keeps running from level to level; after a crash or hang it is restarted from the save. Results go to
// OUT/results.txt, captures to OUT/<level>.png.
// usage: node tools/link-soak.mjs OUT [--exe Sum-fixed-clang-x64.exe] [--game DIR] [--from LEVEL] [--only A,B]
import { spawnSync, execFileSync } from "node:child_process";
import { appendFileSync, copyFileSync, existsSync, mkdirSync, readFileSync, rmSync, statSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { LEVELS } from "./lib/savedata.mjs";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const opt = (name, def) => (args.includes(name) ? args[args.indexOf(name) + 1] : def);
const out = args.find((a, i) => !a.startsWith("--") && !String(args[i - 1]).startsWith("--"));
if (!out) {
  console.error("usage: node tools/link-soak.mjs OUT [--exe EXE] [--game DIR] [--from LEVEL] [--only A,B]");
  process.exit(2);
}
if (!opt("--game") && !process.env.SUMMONER_GAME) {
  console.error("link-soak: give the test folder with --game DIR or SUMMONER_GAME");
  process.exit(2);
}
const game = resolve(opt("--game", process.env.SUMMONER_GAME));
const exe = opt("--exe", "Sum-fixed-clang-x64.exe");
const ini = join(game, "summonerfix.ini"), backup = join(out, "summonerfix.ini.bak"), log = join(game, "summonerfix.log");
// Not levels to play: the developers' test level, the world map (no party to place) and endgame (the `level` warp
// crashes the original's load of it in File::sync_float, 2026-10-25).
let levels = LEVELS.filter((l) => l !== "test" && l !== "worldmap1" && l !== "endgame");
if (opt("--only")) levels = opt("--only").split(",");
if (opt("--from")) levels = levels.slice(Math.max(0, levels.findIndex((l) => l.toLowerCase() === opt("--from").toLowerCase())));

mkdirSync(out, { recursive: true });
copyFileSync(ini, backup);
const results = join(out, "results.txt");
const note = (line) => { console.log(line); appendFileSync(results, line + "\n"); };
const env = { ...process.env, SUMMONER_GAME: game, SUMMONER_EXE: exe };
const agent = (cmds, timeout = 180000) =>
  spawnSync("node", [join(repo, "tools", "agent.mjs"), "run", cmds], { cwd: repo, env, encoding: "latin1", timeout }).stdout ?? "";
const kill = () => spawnSync("taskkill", ["/F", "/IM", exe]);
const running = () => {
  try { return execFileSync("tasklist", ["/FI", `IMAGENAME eq ${exe}`, "/NH"], { encoding: "utf8" }).toLowerCase().includes(exe.toLowerCase()); }
  catch { return false; }
};
const logSize = () => (existsSync(log) ? statSync(log).size : 0);
const logSince = (pos) => (existsSync(log) ? readFileSync(log, "latin1").slice(pos) : "");

function setKey(text, section, key, value) {
  const re = new RegExp(`(^\\[${section}\\][^\\[]*)`, "m"), m = re.exec(text);
  if (!m) return text + `\r\n[${section}]\r\n${key}=${value}\r\n`;
  const line = new RegExp(`^${key}=.*$`, "m");
  return text.replace(m[1], line.test(m[1]) ? m[1].replace(line, `${key}=${value}`) : m[1].replace(/\s*$/, `\r\n${key}=${value}\r\n`));
}

// Start the exe on the newest test save (the main menu's Load Saved Game, then the first save in the list).
function session() {
  kill();
  rmSync(join(game, "agent", "cmd.txt"), { force: true });
  writeFileSync(ini, setKey(setKey(readFileSync(backup, "latin1"), "Debug", "TestSaves", "1"), "Game", "SkipIntroMovies", "1"), "latin1");
  spawnSync("node", [join(repo, "tools", "agent.mjs"), "start", "--wait", "60"], { cwd: repo, env, encoding: "latin1" });
  // The first start of a freshly copied exe can be slow enough that the click on the save comes before the list is
  // ready (2026-10-26): while the game is not in play, the save is clicked again.
  let res = agent("wait 4000; click 744 392; wait 2500; click 798 368; wait 15000; info");
  for (let i = 0; i < 2 && !/state=4\b/.test(res); i++) res = agent("click 744 392; wait 3000; click 798 368; wait 15000; info");
}

// Waits until the game plays (state 4) with no cutscene running (game_flags 0x5fbfd8 bit 0x20), pressing Esc
// every 1.5 s to skip one; up to `secs`. Returns the seconds waited, or -1 when it never got there.
function waitPlay(secs = 90) {
  const t0 = Date.now();
  while (Date.now() - t0 < secs * 1000) {
    if (!running()) return -1;
    const r = agent("info; peek 5fbfd8 1", 30000);
    const state = /state=(\d+)/.exec(r), flags = /5FBFD8: ([0-9A-F]{2})/i.exec(r);
    if (state && state[1] === "4" && flags && !(parseInt(flags[1], 16) & 0x20)) return Math.round((Date.now() - t0) / 1000);
    agent("postkey Esc; wait 1500", 30000);
  }
  return -1;
}

const bad = /(MessageBox from[^\r\n]*|access violation[^\r\n]*|ExitProcess\([^)]*\)[^\r\n]*|crash[^\r\n]*)/i;
let failures = 0;
try {
  for (const level of levels) {
    if (!running()) session();
    const pos = logSize();
    const t0 = Date.now();
    // Level change with the whole party; wait until play, past an opening cutscene; then a golem and three walls of
    // fire, and again until no cutscene runs, so the next level change never comes during one.
    let res = agent(`poke 609fe0 1; level ${level} ${level}; wait 8000`);
    const toPlay = waitPlay();
    if (toPlay >= 0) {
      res += agent(`info; summon 38; wait 3000; firewall 3; wait 4000; capture soak; info`);
      waitPlay();
    }
    const secs = ((Date.now() - t0) / 1000).toFixed(0);
    const text = logSince(pos);
    const m = bad.exec(text);
    const fps = [...res.matchAll(/fps[ =:]*([\d.]+)/gi)].map((x) => x[1]).join("/");
    const png = join(game, "agent", "soak.png");
    if (existsSync(png)) copyFileSync(png, join(out, `${level}.png`)), rmSync(png, { force: true });
    if (!running() || m || /timed out/.test(res)) {
      failures++;
      note(`${level}: FAILED (${secs} s): ${m ? m[1] : !running() ? "the exe is gone" : "no answer from agent mode"}`);
      kill();
      continue;
    }
    if (toPlay < 0) {
      failures++;
      note(`${level}: FAILED (${secs} s): never in play without a cutscene`);
      kill();
      continue;
    }
    note(`${level}: ok (${secs} s, in play after ${toPlay + 8} s), fps ${fps || "?"}`);
  }
} finally {
  agent("quit", 20000);
  kill();
  rmSync(join(game, "agent", "cmd.txt"), { force: true });
  copyFileSync(backup, ini);
}
note(`${levels.length} levels, ${failures} failed`);
