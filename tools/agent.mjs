// Drive Sum.exe through SummonerFix's agent mode ([Debug] Agent=1, src/summonerfix/agent.c).
// usage:
//   node tools/agent.mjs start [--width 1024] [--wait 60]   enable agent mode in the game's ini and launch Sum.exe
//   node tools/agent.mjs run "info; key Esc; wait 500; capture menu"   run commands, print the result
//   node tools/agent.mjs shot [name]                        capture and convert to PNG (prints the path)
//   node tools/agent.mjs stop                               quit the game (kills it after 15 s if needed)
// Captures land in Summoner\agent\NAME.bmp; `run` and `shot` also write NAME.png next to them.
// Coordinates are screenshot pixels (half the window's client size, e.g. 1280x720 for a 2560x1440 screen).
import { readFileSync, writeFileSync, existsSync, renameSync, unlinkSync, mkdirSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { spawn, execFileSync } from "node:child_process";
import { deflateSync, crc32 } from "node:zlib";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
// SUMMONER_GAME / SUMMONER_EXE: another game folder and exe, e.g. the standalone rebuilt exe's test folder
// (docs/tools.md, tools/link-build.mjs).
const game = process.env.SUMMONER_GAME || join(repo, "Summoner");
const exe = process.env.SUMMONER_EXE || "Sum.exe";
const dir = join(game, "agent");
const ini = join(game, "summonerfix.ini");
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

function setIni(section, key, value) {
  let s = readFileSync(ini, "latin1");
  const sec = new RegExp(`(^\\[${section}\\][^\\[]*)`, "m");
  const m = sec.exec(s);
  if (!m) s += `\r\n[${section}]\r\n${key}=${value}\r\n`;
  else {
    const body = m[1];
    const line = new RegExp(`^${key}=.*$`, "m");
    const nb = line.test(body) ? body.replace(line, `${key}=${value}`) : body.replace(/\s*$/, `\r\n${key}=${value}\r\n`);
    s = s.replace(body, nb);
  }
  writeFileSync(ini, s, "latin1");
}

function running() {
  try {
    return execFileSync("tasklist", ["/FI", `IMAGENAME eq ${exe}`, "/NH"], { encoding: "utf8" }).toLowerCase().includes(exe.toLowerCase());
  } catch {
    return false;
  }
}

// 24-bit bottom-up BMP -> PNG
export function bmpToPng(bmpPath, pngPath) {
  const b = readFileSync(bmpPath);
  const off = b.readUInt32LE(10), w = b.readInt32LE(18), h = b.readInt32LE(22), stride = (w * 3 + 3) & ~3;
  const raw = Buffer.alloc((w * 3 + 1) * h);
  for (let y = 0; y < h; y++) {
    const src = off + (h - 1 - y) * stride, dst = y * (w * 3 + 1);
    raw[dst] = 0;
    for (let x = 0; x < w; x++) {
      raw[dst + 1 + x * 3] = b[src + x * 3 + 2];
      raw[dst + 2 + x * 3] = b[src + x * 3 + 1];
      raw[dst + 3 + x * 3] = b[src + x * 3];
    }
  }
  const chunk = (type, data) => {
    const len = Buffer.alloc(4); len.writeUInt32BE(data.length);
    const td = Buffer.concat([Buffer.from(type, "latin1"), data]);
    const crc = Buffer.alloc(4); crc.writeUInt32BE(crc32(td) >>> 0);
    return Buffer.concat([len, td, crc]);
  };
  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(w, 0); ihdr.writeUInt32BE(h, 4); ihdr[8] = 8; ihdr[9] = 2;
  writeFileSync(pngPath, Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", ihdr),
    chunk("IDAT", deflateSync(raw, { level: 6 })), chunk("IEND", Buffer.alloc(0))]));
}

async function run(commands, timeoutMs = 120000) {
  if (!existsSync(join(dir, "ready.txt"))) throw new Error("agent not ready (is the game running with Agent=1?)");
  const res = join(dir, "result.txt");
  if (existsSync(res)) unlinkSync(res);
  writeFileSync(join(dir, "cmd.tmp"), commands.replace(/;/g, "\n"));
  renameSync(join(dir, "cmd.tmp"), join(dir, "cmd.txt"));
  const t0 = Date.now();
  while (!existsSync(res)) {
    if (Date.now() - t0 > timeoutMs) throw new Error("timed out waiting for the agent");
    if (!running()) throw new Error("Sum.exe is not running");
    await sleep(50);
  }
  const out = readFileSync(res, "utf8");
  unlinkSync(res);
  const pngs = [];
  for (const m of out.matchAll(/capture (\S+)\.bmp: ok/g)) {
    const png = join(dir, `${m[1]}.png`);
    bmpToPng(join(dir, `${m[1]}.bmp`), png);
    pngs.push(png);
  }
  return { out, pngs };
}

const [cmd, ...rest] = process.argv.slice(2);
const opt = (name, def) => { const i = rest.indexOf(`--${name}`); return i >= 0 ? rest[i + 1] : def; };
if (cmd === "start") {
  if (running()) { console.log("Sum.exe already running"); process.exit(0); }
  mkdirSync(dir, { recursive: true });
  if (existsSync(join(dir, "ready.txt"))) unlinkSync(join(dir, "ready.txt"));
  setIni("Debug", "Agent", "1");
  setIni("Debug", "CaptureEvery", "0");
  const width = opt("width");
  if (width) setIni("Display", "GameWidth", width);
  spawn(join(game, exe), [], { cwd: game, detached: true, stdio: "ignore" }).unref();
  const limit = Date.now() + Number(opt("wait", "60")) * 1000;
  while (!existsSync(join(dir, "ready.txt")) && Date.now() < limit) await sleep(250);
  console.log(existsSync(join(dir, "ready.txt")) ? "agent ready" : "agent not ready (check summonerfix.log)");
} else if (cmd === "run") {
  const { out, pngs } = await run(rest.join(" "));
  process.stdout.write(out);
  for (const p of pngs) console.log("png:", p);
} else if (cmd === "shot") {
  const name = rest[0] ?? `shot-${Date.now()}`;
  const { out, pngs } = await run(`capture ${name}`);
  process.stdout.write(out);
  for (const p of pngs) console.log("png:", p);
} else if (cmd === "stop") {
  if (running()) {
    try { await run("quit", 5000); } catch {}
    for (let i = 0; i < 30 && running(); i++) await sleep(500);
    if (running()) { execFileSync("taskkill", ["/IM", exe, "/F"]); console.log("killed Sum.exe"); }
    else console.log("Sum.exe exited");
  } else console.log("not running");
  if (existsSync(join(dir, "ready.txt"))) unlinkSync(join(dir, "ready.txt"));
} else {
  console.log("usage: node tools/agent.mjs start|run|shot|stop ...");
}
