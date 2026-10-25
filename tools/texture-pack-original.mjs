// A texture pack for the ORIGINAL game (Sum.exe with SummonerFix) from a full pack made by tools/texture-upscale.mjs
// (docs/future/texture-upscale.md, "The pack for the original game").
//
// usage: node tools/texture-pack-original.mjs --game <install dir> --pack <full pack dir> --out <new pack dir> [--list]
//
// The original game cannot use upscaled images it draws by pixel rectangle (menus, HUD, icons, maps, cursors) or
// reads as click masks: they show partly and clicks land in the wrong place. Only the experimental build's
// HiResInterface handles those. Images the game draws as textures in 3D (levels, characters, items, effects) work in
// the original as they are. So this pack is the full pack without:
//   - every image of summoner.vpp (the interface art: menus, HUD, icons, the automap images, cursors, loading
//     screens, popups);
//   - the images the tables name as icons or automaps ($Icon, $Automap... in the .tbl files);
//   - cutscene.vpp images that no 3D asset names (drawn as 2D stills);
//   - <font>-hires.tga, which only the experimental build reads.
// The files are hard links to the full pack's (no extra space), in the same two folders, with their mip levels.
import { readdirSync, mkdirSync, linkSync, existsSync, unlinkSync, openSync, readSync, closeSync, writeFileSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
const { readDirectory } = await import(pathToFileURL(join(here, "vpp.mjs")).href);
const args = process.argv.slice(2), opt = (k) => { const i = args.indexOf(k); return i >= 0 ? args[i + 1] : null; };
const game = opt("--game"), pack = opt("--pack"), out = opt("--out");
if (!game || !pack || !out) {
  console.error("usage: node tools/texture-pack-original.mjs --game <install dir> --pack <full pack dir> --out <new pack dir> [--list]");
  process.exit(2);
}
const FOLDERS = ["data/interface/pc", "data/maps/levels"];
const VPPS = ["summoner", "chars", "items", "effects", "cutscene", "levelt", "levele", "levelm", "tables"];
const D3 = new Set(["vim", "s3d", "rfx", "vfx", "vlm", "vex", "mvf"]);
const stem = (n) => n.toLowerCase().replace(/(-mip\d+)?\.(tga|vbm)$/, "");

const excluded = new Map();   // stem -> reason
const named3d = new Set();
const cutsceneImages = [];
for (const v of VPPS) {
  const path = join(game, v + ".vpp");
  if (!existsSync(path)) continue;
  const fd = openSync(path, "r");
  for (const f of readDirectory(path).files) {
    const ext = f.name.toLowerCase().split(".").pop();
    if (ext === "tga" || ext === "vbm") {
      if (v === "summoner") excluded.set(stem(f.name), "summoner.vpp");
      if (v === "cutscene") cutsceneImages.push(f.name);
      continue;
    }
    if (ext !== "tbl" && !D3.has(ext)) continue;
    const b = Buffer.alloc(f.size);
    readSync(fd, b, 0, f.size, f.offset);
    const text = b.toString("latin1");
    if (ext === "tbl") {
      for (const m of text.matchAll(/^\s*\$(Icon|Automap\w*)\s*:\s*"([^"]+)"/gim)) excluded.set(stem(m[2]), `${f.name} $${m[1]}`);
    } else {
      for (const m of text.matchAll(/[\x20-\x7e]{3,60}/g)) named3d.add(stem(m[0].trim()));
    }
  }
  closeSync(fd);
}
for (const n of cutsceneImages) if (!named3d.has(stem(n))) excluded.set(stem(n), "cutscene.vpp, no 3D asset names it");

const src = join(pack, FOLDERS[0]);
const files = readdirSync(src);
let kept = 0, skipped = 0;
for (const d of FOLDERS) mkdirSync(join(out, d), { recursive: true });
const report = [];
for (const n of files) {
  const reason = /-hires\.tga$/i.test(n) ? "experimental build only (font)" : excluded.get(stem(n));
  if (reason) { skipped++; report.push(`skip ${n}: ${reason}`); continue; }
  for (const d of FOLDERS) {
    const t = join(out, d, n);
    if (existsSync(t)) unlinkSync(t);   // single files only
    linkSync(join(src, n), t);
  }
  kept++;
}
if (args.includes("--list")) writeFileSync(join(out, "skipped.txt"), report.join("\n") + "\n");
console.log(`kept ${kept} files, left out ${skipped} (${excluded.size} names excluded) -> ${out}`);
