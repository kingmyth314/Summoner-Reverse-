// Tests of the hybrid game (tools/hybrid-build.mjs, src/summonerfix/hybrid.c): the save comparison, the halving
// search and the session start the hybrid work uses (docs/decomp-plan.md, "The hybrid game"; docs/tools.md).
//
//   node tools/hybrid-test.mjs session [KEY=VALUE@Section ...]
//       restore the ini (--ini-backup / SUMMONERFIX_INI_BACKUP), set [Debug] TestSaves=1 and the given keys, start
//       the game in agent mode and load the newest test save (Summoner/savetest)
//   node tools/hybrid-test.mjs scenario <dir> <tag> [--runs 3] [--level NAME:SCRIPT] [--steps "agent commands"]
//       run the scenario (by default `level` with the full party, wait, Esc) on the original game --runs times and
//       on the hybrid once (with coverage, agent\cov_<tag>.txt), quicksaving after each: <dir>/<tag>-orig<N>.sav and
//       <tag>-hyb.sav. Quicksaves go to savetest/quicksave.sav, which is back-dated so the next session loads the
//       same save
//   node tools/hybrid-test.mjs compare <dir> <tag> [--runs 3] [--show 25]
//       the hybrid's save against the original runs, dword by dword after the thumbnail: a value that varies between
//       the original runs may vary as much in the hybrid; LENT records are walked and named
//   node tools/hybrid-test.mjs search <orig.sav> --level NAME:SCRIPT --region lo-hi|a,b,c [--majority]
//       halve the replaced functions ([Hybrid] Only) until the ones that change the given save bytes are left;
//       --majority counts a step as failing on two of three runs (for a difference that comes and goes)
//
// Needs the hybrid DLL in build/hybrid-all (copied to the game folder) and SummonerFix's agent mode. The user's own
// saves (Summoner/savegame) are never touched: TestSaves keeps everything in Summoner/savetest.
import { readFileSync, writeFileSync, copyFileSync, existsSync, utimesSync, rmSync, mkdirSync } from "node:fs";
import { spawnSync } from "node:child_process";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { compareSaves } from "./save-compare.mjs";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const game = join(repo, "Summoner");
const ini = join(game, "summonerfix.ini");
const quicksave = join(game, "savetest", "quicksave.sav");
const OLD = new Date("2000-01-01T00:00:00");
const args = process.argv.slice(2);
const opt = (name, def) => { const i = args.indexOf(name); return i >= 0 ? args[i + 1] : def; };
const flag = (name) => args.includes(name);
const backup = opt("--ini-backup", process.env.SUMMONERFIX_INI_BACKUP);

const kill = () => spawnSync("taskkill", ["/F", "/IM", "Sum.exe"]);
const agent = (cmds, timeout = 180000) =>
  spawnSync("node", [join(repo, "tools", "agent.mjs"), "run", cmds], { cwd: repo, encoding: "latin1", timeout }).stdout ?? "";

function setKey(text, section, key, value) {
  const re = new RegExp(`(^\\[${section}\\][^\\[]*)`, "m"), m = re.exec(text);
  if (!m) return text + `\r\n[${section}]\r\n${key}=${value}\r\n`;
  const line = new RegExp(`^${key}=.*$`, "m");
  return text.replace(m[1], line.test(m[1]) ? m[1].replace(line, `${key}=${value}`) : m[1].replace(/\s*$/, `\r\n${key}=${value}\r\n`));
}

// Start a session on the newest test save. keys: ["KEY=VALUE@Section", ...].
function session(keys) {
  if (!backup || !existsSync(backup)) throw new Error("set --ini-backup or SUMMONERFIX_INI_BACKUP to a copy of the game's summonerfix.ini");
  kill();
  rmSync(join(game, "agent", "cmd.txt"), { force: true }); // a command left by a killed session would run at the next start
  let text = readFileSync(backup, "latin1");
  text = setKey(text, "Debug", "TestSaves", "1");
  for (const kv of keys) {
    const [k, section] = kv.split("@"), at = k.indexOf("=");
    text = setKey(text, section, k.slice(0, at), k.slice(at + 1));
  }
  writeFileSync(ini, text, "latin1");
  spawnSync("node", [join(repo, "tools", "agent.mjs"), "start", "--wait", "60"], { cwd: repo, encoding: "latin1" });
  // Main menu: Load Saved Game, the newest save, then wait for the level.
  return agent("wait 4000; click 744 392; wait 2500; click 798 368; wait 15000; capture s0");
}
function endSession() {
  kill();
  rmSync(join(game, "agent", "cmd.txt"), { force: true });
  if (backup) copyFileSync(backup, ini);
}

// One quicksave after the steps; returns the problem or "".
function runOnce(steps, hybrid, out, tag) {
  utimesSync(quicksave, OLD, OLD);
  const before = readFileSync(quicksave);
  session(hybrid ? ["Dll=hybrid.dll@Hybrid", "Coverage=1@Hybrid", "Diagnostics=1@Debug"] : ["Diagnostics=1@Debug"]);
  // Coverage after the quicksave, so the save chain counts as run.
  const res = agent(`${steps}; call0 40e7c0; wait 3000${hybrid ? `; hybridcov cov_${tag}` : ""}`);
  kill();
  const log = readFileSync(join(game, "summonerfix.log"), "latin1");
  const after = readFileSync(quicksave);
  utimesSync(quicksave, OLD, OLD);
  const m = /(MessageBox from[^\r\n]*|access violation[^\r\n]*)/.exec(log);
  if (m) return m[1];
  if (after.equals(before)) return "no new save" + (/timed out/.test(res) ? " (no frames)" : "");
  writeFileSync(out, after);
  return "";
}

function compare(dir, tag, runs, show) {
  const bad = compareSaves([...Array(runs).keys()].map((i) => join(dir, `${tag}-orig${i}.sav`)), join(dir, `${tag}-hyb.sav`));
  console.log(`${tag}: ${bad.length} dwords differ beyond the original runs' spread`);
  for (const [, , line] of bad.slice(0, show)) console.log("  " + line);
}

const levelSteps = (level) => { const [n, s] = level.split(":"); return `poke 609fe0 1; level ${n} ${s}; wait 8000; postkey Esc; wait 1500`; };

const cmd = args[0];
if (cmd === "session") {
  console.log(session(args.slice(1).filter((a) => a.includes("@"))).trim());
} else if (cmd === "scenario") {
  const [dir, tag] = [args[1], args[2]], runs = +opt("--runs", 3);
  const steps = opt("--steps", opt("--level") ? levelSteps(opt("--level")) : null);
  if (!dir || !tag || !steps) { console.error("scenario <dir> <tag> --level NAME:SCRIPT | --steps \"...\""); process.exit(2); }
  mkdirSync(dir, { recursive: true });
  copyFileSync(join(repo, "build", "hybrid-all", "hybrid.dll"), join(game, "hybrid.dll"));
  for (let i = 0; i < runs; i++) {
    const p = runOnce(steps, false, join(dir, `${tag}-orig${i}.sav`), tag);
    if (p) { console.log(`original run ${i}: ${p}`); endSession(); process.exit(1); }
  }
  const p = runOnce(steps, true, join(dir, `${tag}-hyb.sav`), tag);
  endSession();
  if (p) { console.log(`hybrid run: ${p}`); process.exit(1); }
  compare(dir, tag, runs, 25);
} else if (cmd === "compare") {
  compare(args[1], args[2], +opt("--runs", 3), +opt("--show", 25));
} else if (cmd === "search") {
  const A = readFileSync(args[1]), level = opt("--level"), region = opt("--region");
  if (!level || !region) { console.error("search <orig.sav> --level NAME:SCRIPT --region lo-hi|a,b,c [--majority]"); process.exit(2); }
  const bytes = region.includes(",")
    ? (b) => region.split(",").map((x) => b.subarray(parseInt(x, 16), parseInt(x, 16) + 4).toString("hex")).join(" ")
    : (b) => { const [lo, hi] = region.split("-").map((x) => parseInt(x, 16)); return b.subarray(lo, hi).toString("hex"); };
  const want = bytes(A);
  const all = readFileSync(join(repo, "build", "hybrid-all", "hybrid.txt"), "latin1").trim().split(/\r?\n/).map((l) => l.split(" ")[0]);
  copyFileSync(join(repo, "build", "hybrid-all", "hybrid.dll"), join(game, "hybrid.dll"));
  const one = (addrs, label) => {
    writeFileSync(join(game, "hybrid-only.txt"), addrs.join("\n") + "\n");
    utimesSync(quicksave, OLD, OLD);
    const before = readFileSync(quicksave);
    session(["Dll=hybrid.dll@Hybrid", "Only=hybrid-only.txt@Hybrid", "Diagnostics=1@Debug"]);
    agent(`${levelSteps(level)}; call0 40e7c0; wait 3000`);
    kill();
    const C = readFileSync(quicksave);
    utimesSync(quicksave, OLD, OLD);
    const why = C.equals(before) ? "no new save" : bytes(C) !== want ? "differs" : "";
    console.log(`${label}: ${addrs.length} functions: ${why || "same"}`);
    return why ? "bad" : "ok";
  };
  const test = (addrs, label) => {
    if (!flag("--majority")) return one(addrs, label);
    let bad = 0, ok = 0;
    while (bad < 2 && ok < 2) one(addrs, label) === "bad" ? bad++ : ok++;
    return bad >= 2 ? "bad" : "ok";
  };
  // A smallest set C of `set` with base + C failing (delta debugging: when neither half fails alone, each half is
  // searched with the other present).
  const find = (set, base) => {
    if (set.length === 1) return set;
    const a = set.slice(0, set.length >> 1), b = set.slice(set.length >> 1);
    if (test([...base, ...a], `${base.length}+${a.length}a`) === "bad") return find(a, base);
    if (test([...base, ...b], `${base.length}+${b.length}b`) === "bad") return find(b, base);
    const ca = find(a, [...base, ...b]);
    return [...ca, ...find(b, [...base, ...ca])];
  };
  if (test(all, "all") === "bad") {
    const names = new Map(readFileSync(join(repo, "build", "hybrid-all", "hybrid.txt"), "latin1").trim().split(/\r?\n/).map((l) => [l.split(" ")[0], l]));
    console.log("the functions:\n" + find(all, []).map((a) => names.get(a) ?? a).join("\n"));
  } else console.log("all the same");
  rmSync(join(game, "hybrid-only.txt"), { force: true });
  endSession();
} else {
  console.error("usage: node tools/hybrid-test.mjs session|scenario|compare|search ... (see the comment at the top)");
  process.exit(2);
}
