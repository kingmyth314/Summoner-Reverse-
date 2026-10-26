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
// Not levels to play: the developers' test level and the world map (no party to place).
let levels = LEVELS.filter((l) => l !== "test" && l !== "worldmap1");
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
  agent("wait 4000; click 744 392; wait 2500; click 798 368; wait 15000");
}

const bad = /(MessageBox from[^\r\n]*|access violation[^\r\n]*|ExitProcess\([^)]*\)[^\r\n]*|crash[^\r\n]*)/i;
let failures = 0;
try {
  for (const level of levels) {
    if (!running()) session();
    const pos = logSize();
    const t0 = Date.now();
    // Level change with the whole party; Esc past an opening cutscene; then a golem and three walls of fire.
    const res = agent(`poke 609fe0 1; level ${level} ${level}; wait 10000; postkey Esc; wait 1500; info; summon 38; wait 3000; ` +
      `firewall 3; wait 4000; capture soak; info`);
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
    note(`${level}: ok (${secs} s), fps ${fps || "?"}`);
  }
} finally {
  agent("quit", 20000);
  kill();
  rmSync(join(game, "agent", "cmd.txt"), { force: true });
  copyFileSync(backup, ini);
}
note(`${levels.length} levels, ${failures} failed`);
