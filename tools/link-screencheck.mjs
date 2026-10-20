// The screen check of the standalone rebuilt exe (tools/link-build.mjs) against the original: the same screens
// (menus, the game's panels, the prologue) captured in both, pixel by pixel. The original runs --runs times (3); pixels
// that differ between its runs (animations, particles, timing) are masked, with a 6-pixel margin, and only the rebuilt exe's other
// differences count. It complements tools/link-savecheck.mjs, which cannot see rendering.
//
//   node tools/link-screencheck.mjs <out dir> [SCENARIO ...] [--runs 3] [--game DIR] [--threshold 40]
//
// Characters walking in the game view move differently from run to run (the Lenele gate guard took five places in six
// original runs), so the game scenarios need three or more original runs for the mask.
//
// Without names every scenario runs (list below). Per scenario <out dir> gets <name>-orig0.. .png, -reb.png and, when
// pixels differ, -diff.png (the rebuilt capture with the differing pixels in red and the masked ones dimmed);
// results.txt has one line per scenario. Uses the test folder (default SummonerToolchain/rebuilt-test) as
// link-savecheck.mjs does, the newest test save for the "game" scenarios, and restores summonerfix.ini at the end.
import { readFileSync, writeFileSync, copyFileSync, existsSync, rmSync, mkdirSync, appendFileSync } from "node:fs";
import { spawnSync } from "node:child_process";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { deflateSync, crc32 } from "node:zlib";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const opt = (name, def) => { const i = args.indexOf(name); return i >= 0 ? args[i + 1] : def; };
const game = opt("--game", join(repo, "..", "..", "SummonerToolchain", "rebuilt-test"));
const threshold = +opt("--threshold", 40);
const runs = Math.max(2, +opt("--runs", 3));
const ini = join(game, "summonerfix.ini");
const backup = join(game, "summonerfix.ini.savecheck");
const optArgs = new Set(["--game", "--threshold", "--runs"]);
const [out, ...only] = args.filter((a, i) => !a.startsWith("--") && !optArgs.has(args[i - 1]));

// Main menu buttons (1280x720 captures): new game 858,296; load 858,368; multiplayer 892,440; options 935,512;
// credits 935,584. "game" scenarios start from the newest test save; the keys are the game's (docs/sum-exe/controls.md).
const LOAD = "click 744 392; wait 2500; click 798 368; wait 15000";
const SCENARIOS = [
  ["main-menu", "wait 4000"],
  ["load-list", "wait 4000; click 858 368; wait 2500"],
  ["options-menu", "wait 4000; click 935 512; wait 2500"],
  ["multiplayer-box", "wait 4000; click 892 440; wait 2500"],
  ["credits", "wait 4000; click 935 584; wait 4000"],
  ["prologue-4s", "wait 4000; click 858 296; wait 4000"],
  ["prologue-12s", "wait 4000; click 858 296; wait 12000"],
  ["village-cinematic", "wait 4000; click 858 296; wait 3000; key Esc; wait 5000"],
  ["game-hud", `wait 4000; ${LOAD}`],
  ["character", `wait 4000; ${LOAD}; key R; wait 1500`],
  ["inventory", `wait 4000; ${LOAD}; key I; wait 1500`],
  ["spells", `wait 4000; ${LOAD}; key C; wait 1500`],
  ["automap", `wait 4000; ${LOAD}; key M; wait 1500`],
  ["help", `wait 4000; ${LOAD}; key F1; wait 1500`],
  ["options-game", `wait 4000; ${LOAD}; key F2; wait 1500`],
  ["play-menu", `wait 4000; ${LOAD}; key Esc; wait 1500`],
];
const todo = only.length ? SCENARIOS.filter(([n]) => only.includes(n)) : SCENARIOS;
if (!out || /\s/.test(out) || !todo.length) {
  console.error("usage: node tools/link-screencheck.mjs <out dir, no spaces> [scenario ...]\nscenarios: " + SCENARIOS.map(([n]) => n).join(" "));
  process.exit(2);
}
mkdirSync(out, { recursive: true });
if (!existsSync(backup)) copyFileSync(ini, backup);

const env = (exe) => ({ ...process.env, SUMMONER_GAME: game, SUMMONER_EXE: exe });
const kill = (exe) => spawnSync("taskkill", ["/F", "/IM", exe]);
const agent = (exe, cmds) =>
  spawnSync("node", [join(repo, "tools", "agent.mjs"), "run", cmds], { cwd: repo, env: env(exe), encoding: "latin1", timeout: 180000 }).stdout ?? "";

function setKey(text, section, key, value) {
  const re = new RegExp(`(^\\[${section}\\][^\\[]*)`, "m"), m = re.exec(text);
  if (!m) return text + `\r\n[${section}]\r\n${key}=${value}\r\n`;
  const line = new RegExp(`^${key}=.*$`, "m");
  return text.replace(m[1], line.test(m[1]) ? m[1].replace(line, `${key}=${value}`) : m[1].replace(/\s*$/, `\r\n${key}=${value}\r\n`));
}

function readBmp(path) {
  const b = readFileSync(path);
  const off = b.readUInt32LE(10), w = b.readInt32LE(18), h = b.readInt32LE(22), stride = (w * 3 + 3) & ~3;
  const px = Buffer.alloc(w * h * 3);
  for (let y = 0; y < h; y++) b.copy(px, y * w * 3, off + (h - 1 - y) * stride, off + (h - 1 - y) * stride + w * 3); // BGR, top-down
  return { w, h, px };
}
function writePng(path, { w, h, px }) {
  const raw = Buffer.alloc((w * 3 + 1) * h);
  for (let y = 0; y < h; y++)
    for (let x = 0; x < w; x++) {
      const s = (y * w + x) * 3, d = y * (w * 3 + 1) + 1 + x * 3;
      raw[d] = px[s + 2]; raw[d + 1] = px[s + 1]; raw[d + 2] = px[s];
    }
  const chunk = (type, data) => {
    const t = Buffer.from(type, "latin1"), len = Buffer.alloc(4), crc = Buffer.alloc(4);
    len.writeUInt32BE(data.length);
    crc.writeUInt32BE(crc32(Buffer.concat([t, data])) >>> 0);
    return Buffer.concat([len, t, data, crc]);
  };
  const hdr = Buffer.alloc(13);
  hdr.writeUInt32BE(w, 0); hdr.writeUInt32BE(h, 4); hdr[8] = 8; hdr[9] = 2;
  writeFileSync(path, Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", hdr), chunk("IDAT", deflateSync(raw)), chunk("IEND", Buffer.alloc(0))]));
}

// One capture of a scenario; returns the BMP path or a problem.
function capture(exe, name, steps, tag) {
  kill("Sum-orig.exe");
  kill("Sum-rebuilt.exe");
  rmSync(join(game, "agent", "cmd.txt"), { force: true });
  writeFileSync(ini, setKey(readFileSync(backup, "latin1"), "Debug", "TestSaves", "1"), "latin1");
  spawnSync("node", [join(repo, "tools", "agent.mjs"), "start", "--wait", "60"], { cwd: repo, env: env(exe), encoding: "latin1" });
  const shot = `sc_${tag}`;
  rmSync(join(game, "agent", `${shot}.bmp`), { force: true });
  agent(exe, `${steps}; capture ${shot}`);
  kill(exe);
  const log = readFileSync(join(game, "summonerfix.log"), "latin1");
  const m = /(access violation[^\r\n]*|ExitProcess\(3221225477\)[^\r\n]*)/i.exec(log);
  if (m) return { problem: m[1] };
  const bmp = join(game, "agent", `${shot}.bmp`);
  return existsSync(bmp) ? { bmp } : { problem: "no capture" };
}

const results = join(out, "results.txt");
const note = (line) => { console.log(line); appendFileSync(results, line + "\n"); };
try {
  for (const [name, steps] of todo) {
    const plan = [...[...Array(runs).keys()].map((i) => ["Sum-orig.exe", `orig${i}`]), ["Sum-rebuilt.exe", "reb"]];
    const caps = plan.map(([exe, tag]) => {
      const c = capture(exe, name, steps, tag);
      if (c.bmp) { const img = readBmp(c.bmp); writePng(join(out, `${name}-${tag}.png`), img); return img; }
      return c.problem + ` (${tag})`;
    });
    const bad = caps.find((c) => typeof c === "string");
    if (bad) { note(`${name}: ${bad}`); continue; }
    const origs = caps.slice(0, runs), r = caps[runs], a = origs[0];
    if (a.w !== r.w || a.h !== r.h) { note(`${name}: sizes differ (${a.w}x${a.h} / ${r.w}x${r.h})`); continue; }
    const { w, h } = a, differs = (p, q, i) => Math.max(Math.abs(p[i] - q[i]), Math.abs(p[i + 1] - q[i + 1]), Math.abs(p[i + 2] - q[i + 2])) > threshold;
    // The mask: pixels that differ between the original runs, grown by 6 pixels.
    const vary = new Uint8Array(w * h);
    for (let k = 0; k < w * h; k++) if (origs.some((o) => differs(a.px, o.px, k * 3))) vary[k] = 1;
    const mask = new Uint8Array(w * h);
    for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) if (vary[y * w + x])
      for (let dy = -6; dy <= 6; dy++) for (let dx = -6; dx <= 6; dx++) {
        const yy = y + dy, xx = x + dx;
        if (yy >= 0 && yy < h && xx >= 0 && xx < w) mask[yy * w + xx] = 1;
      }
    let count = 0, masked = 0, x0 = w, y0 = h, x1 = -1, y1 = -1;
    const view = Buffer.from(r.px);
    for (let k = 0; k < w * h; k++) {
      if (mask[k]) { masked++; view[k * 3] >>= 2; view[k * 3 + 1] >>= 2; view[k * 3 + 2] >>= 2; continue; }
      if (origs.some((o) => !differs(o.px, r.px, k * 3))) continue;
      count++;
      const x = k % w, y = (k / w) | 0;
      x0 = Math.min(x0, x); y0 = Math.min(y0, y); x1 = Math.max(x1, x); y1 = Math.max(y1, y);
      view[k * 3] = 0; view[k * 3 + 1] = 0; view[k * 3 + 2] = 255;
    }
    if (count) writePng(join(out, `${name}-diff.png`), { w, h, px: view });
    note(`${name}: ${count} pixels differ${count ? ` in ${x0},${y0}-${x1},${y1}` : ""} (${(100 * masked / (w * h)).toFixed(1)}% masked as varying between the original runs)`);
  }
} finally {
  kill("Sum-orig.exe");
  kill("Sum-rebuilt.exe");
  rmSync(join(game, "agent", "cmd.txt"), { force: true });
  copyFileSync(backup, ini);
}
