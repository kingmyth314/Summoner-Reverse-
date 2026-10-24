// Upscale Summoner's textures from the player's own install into a loose-file texture pack (docs/future/texture-upscale.md).
// The game reads a loose file before its packfile copy, so the pack is plain TGAs in the data folders; the original
// game uses them for 3D surfaces, and the experimental build's [Experimental] HiResInterface also for the interface.
//
// usage: node tools/texture-upscale.mjs --game <install dir> --out <pack dir> --esrgan <realesrgan-ncnn-vulkan.exe>
//          [--model realesrgan-x4plus] [--only <substring or glob>] [--vpp chars,levelt,...] [--limit N]
//          [--max4 256] [--max2 512] [--keep-work]
//
// Per image (.tga in the packfiles; VBM, PCX, 8-bit and the old -mip files are skipped):
//   - factor 4 up to --max4 pixels on the longer side, 2 up to --max2, else left as it is (the OpenGL renderer's
//     texture cap is 2048);
//   - the colour goes through Real-ESRGAN (always 4x; a 2x factor is a 2x2 average of that), with the edges padded
//     by wrapping so tiling textures stay seamless;
//   - an alpha channel goes through the model separately as a grey image; when the original alpha is only 0 / 255
//     the result is thresholded back to 0 / 255;
//   - 24-bit images use pure green (0, 255, 0) as transparent: those pixels are filled from their neighbours before
//     upscaling (no green halo), the key mask is scaled separately, and afterwards the keyed area is exact green and
//     no other pixel is;
//   - when the packfile has <name>-mip1.tga etc., a new chain is written at the new sizes (one more level per
//     doubling), as the game reads mip levels only while each is half the one before.
// Where the files go: the game reads a bitmap's header from any data folder but its pixels from the folder the load
// asked for, so levelt.vpp's textures go to data\maps\levels (s3d loads them with path_maps_levels) and everything
// else to data\interface\pc (path_interface; the other loads search every folder).
//
// The output is derived from the game's art: never commit or publish it. The work folder (PNG files for the model)
// is <out>\.work and is emptied file by file afterwards unless --keep-work.
import { readFileSync, writeFileSync, mkdirSync, readdirSync, unlinkSync, existsSync, openSync, readSync, closeSync, rmdirSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { execFileSync } from "node:child_process";
import { fileURLToPath, pathToFileURL } from "node:url";
import zlib from "node:zlib";

const here = dirname(fileURLToPath(import.meta.url));
const { readDirectory } = await import(pathToFileURL(join(here, "vpp.mjs")).href);

// ---- arguments ----
const args = process.argv.slice(2);
const opt = (k, d) => { const i = args.indexOf(k); return i >= 0 ? args[i + 1] : d; };
const game = opt("--game"), out = opt("--out"), esrgan = opt("--esrgan");
const model = opt("--model", "realesrgan-x4plus"), only = opt("--only"), limit = +opt("--limit", "0");
const max4 = +opt("--max4", "256"), max2 = +opt("--max2", "512"), keepWork = args.includes("--keep-work");
const vpps = opt("--vpp", "chars,levelt,items,effects,summoner,cutscene").split(",");
if (!game || !out || !esrgan) {
  console.error("usage: node tools/texture-upscale.mjs --game <install dir> --out <pack dir> --esrgan <exe> [options]");
  process.exit(2);
}

// ---- TGA ----
function readTga(b) {
  const idl = b[0], type = b[2], w = b.readUInt16LE(12), h = b.readUInt16LE(14), bpp = b[16], desc = b[17];
  if (b[1] !== 0 || (type !== 2 && type !== 10) || (bpp !== 24 && bpp !== 32)) return null;
  const px = bpp / 8, src = Buffer.alloc(w * h * px);
  let p = 18 + idl;
  if (type === 2) b.copy(src, 0, p, p + src.length);
  else {
    let o = 0;
    while (o < src.length) {
      const c = b[p++], n = (c & 127) + 1;
      if (c & 128) { for (let i = 0; i < n; i++) b.copy(src, o + i * px, p, p + px); p += px; }
      else { b.copy(src, o, p, p + n * px); p += n * px; }
      o += n * px;
    }
  }
  // To top-down RGBA rows (alpha 255 for 24-bit).
  const topDown = (desc & 0x20) !== 0, rgba = Buffer.alloc(w * h * 4);
  for (let y = 0; y < h; y++) {
    const sy = topDown ? y : h - 1 - y;
    for (let x = 0; x < w; x++) {
      const s = (sy * w + x) * px, d = (y * w + x) * 4;
      rgba[d] = src[s + 2]; rgba[d + 1] = src[s + 1]; rgba[d + 2] = src[s]; rgba[d + 3] = px === 4 ? src[s + 3] : 255;
    }
  }
  return { w, h, bpp, desc, rgba };
}

function writeTga(path, w, h, bpp, desc, rgba) {
  const px = bpp / 8, hdr = Buffer.alloc(18), data = Buffer.alloc(w * h * px);
  hdr[2] = 2; hdr.writeUInt16LE(w, 12); hdr.writeUInt16LE(h, 14); hdr[16] = bpp;
  hdr[17] = (desc & 0x20) | (px === 4 ? 8 : 0);       // keep the original's row order
  const topDown = (desc & 0x20) !== 0;
  for (let y = 0; y < h; y++) {
    const sy = topDown ? y : h - 1 - y;
    for (let x = 0; x < w; x++) {
      const s = (y * w + x) * 4, d = (sy * w + x) * px;
      data[d] = rgba[s + 2]; data[d + 1] = rgba[s + 1]; data[d + 2] = rgba[s];
      if (px === 4) data[d + 3] = rgba[s + 3];
    }
  }
  writeFileSync(path, Buffer.concat([hdr, data]));
}

// ---- PNG (8-bit RGB / RGBA / grey, no interlace) ----
const CRC = new Int32Array(256).map((_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; return c; });
const crc32 = (b) => { let c = -1; for (const x of b) c = CRC[(c ^ x) & 255] ^ (c >>> 8); return (c ^ -1) >>> 0; };
function chunk(type, data) {
  const len = Buffer.alloc(4); len.writeUInt32BE(data.length);
  const td = Buffer.concat([Buffer.from(type, "latin1"), data]), crc = Buffer.alloc(4); crc.writeUInt32BE(crc32(td));
  return Buffer.concat([len, td, crc]);
}
function writePngRgb(path, w, h, rgb) {
  const raw = Buffer.alloc((w * 3 + 1) * h);
  for (let y = 0; y < h; y++) rgb.copy(raw, y * (w * 3 + 1) + 1, y * w * 3, (y + 1) * w * 3);
  const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(w, 0); ihdr.writeUInt32BE(h, 4); ihdr[8] = 8; ihdr[9] = 2;
  writeFileSync(path, Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk("IHDR", ihdr),
    chunk("IDAT", zlib.deflateSync(raw)), chunk("IEND", Buffer.alloc(0))]));
}
function readPng(path) {
  const b = readFileSync(path);
  let p = 8, w = 0, h = 0, ct = 0, idat = [];
  while (p < b.length) {
    const len = b.readUInt32BE(p), type = b.toString("latin1", p + 4, p + 8), data = b.subarray(p + 8, p + 8 + len);
    if (type === "IHDR") { w = data.readUInt32BE(0); h = data.readUInt32BE(4); if (data[8] !== 8 || data[12] !== 0) throw new Error(`${path}: unsupported PNG`); ct = data[9]; }
    else if (type === "IDAT") idat.push(data);
    p += 12 + len;
  }
  const ch = { 0: 1, 2: 3, 4: 2, 6: 4 }[ct]; if (!ch) throw new Error(`${path}: colour type ${ct}`);
  const raw = zlib.inflateSync(Buffer.concat(idat)), stride = w * ch, img = Buffer.alloc(stride * h);
  for (let y = 0; y < h; y++) {
    const f = raw[y * (stride + 1)], row = raw.subarray(y * (stride + 1) + 1, (y + 1) * (stride + 1)), o = y * stride;
    for (let x = 0; x < stride; x++) {
      const a = x >= ch ? img[o + x - ch] : 0, up = y ? img[o - stride + x] : 0, c = y && x >= ch ? img[o - stride + x - ch] : 0;
      let v = row[x];
      if (f === 1) v += a; else if (f === 2) v += up; else if (f === 3) v += (a + up) >> 1;
      else if (f === 4) { const pa = Math.abs(up - c), pb = Math.abs(a - c), pc = Math.abs(a + up - 2 * c); v += pa <= pb && pa <= pc ? a : pb <= pc ? up : c; }
      img[o + x] = v & 255;
    }
  }
  const rgb = Buffer.alloc(w * h * 3);   // as RGB (grey repeated)
  for (let i = 0; i < w * h; i++) for (let k = 0; k < 3; k++) rgb[i * 3 + k] = img[i * ch + (ch >= 3 ? k : 0)];
  return { w, h, rgb };
}

// ---- helpers ----
const PAD = 4;
// One channel set (3 bytes per pixel, from rgba via pick) padded by wrapping.
function padded(w, h, pick) {
  const W = w + 2 * PAD, H = h + 2 * PAD, b = Buffer.alloc(W * H * 3);
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
    const sx = ((x - PAD) % w + w) % w, sy = ((y - PAD) % h + h) % h;
    pick(sy * w + sx, b, (y * W + x) * 3);
  }
  return { W, H, b };
}
// The model's 4x output, cropped back and reduced to factor f (2x2 average for f = 2): RGB, w*f x h*f.
function unpad(img, w, h, f) {
  const W4 = img.w, o = PAD * 4, s = 4 / f, W = w * f, H = h * f, r = Buffer.alloc(W * H * 3);
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) for (let k = 0; k < 3; k++) {
    let sum = 0;
    for (let dy = 0; dy < s; dy++) for (let dx = 0; dx < s; dx++) sum += img.rgb[((o + y * s + dy) * W4 + o + x * s + dx) * 3 + k];
    r[(y * W + x) * 3 + k] = Math.round(sum / (s * s));
  }
  return r;
}
const isKey = (p, i) => p[i * 4] === 0 && p[i * 4 + 1] === 255 && p[i * 4 + 2] === 0;
// Keyed pixels take the average of non-keyed neighbours, in passes, so the model sees no green.
function fillKeyed(t) {
  const { w, h } = t, c = Buffer.from(t.rgba), keyed = new Uint8Array(w * h);
  for (let i = 0; i < w * h; i++) keyed[i] = isKey(c, i) ? 1 : 0;
  for (let pass = 0; pass < 64; pass++) {
    let left = 0; const next = keyed.slice();
    for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
      const i = y * w + x; if (!keyed[i]) continue;
      let n = 0, s = [0, 0, 0];
      for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
        const j = ((y + dy + h) % h) * w + ((x + dx + w) % w);
        if (!keyed[j]) { n++; for (let k = 0; k < 3; k++) s[k] += c[j * 4 + k]; }
      }
      if (n) { for (let k = 0; k < 3; k++) c[i * 4 + k] = Math.round(s[k] / n); next[i] = 0; } else left++;
    }
    keyed.set(next); if (!left) break;
  }
  for (let i = 0; i < w * h; i++) if (keyed[i]) { c[i * 4] = c[i * 4 + 1] = c[i * 4 + 2] = 0; }   // all-key image
  return c;
}
function halve(rgba, w, h, keyMode) {
  const W = Math.max(1, w >> 1), H = Math.max(1, h >> 1), r = Buffer.alloc(W * H * 4);
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
    const idx = [];
    for (let dy = 0; dy < 2; dy++) for (let dx = 0; dx < 2; dx++) idx.push((Math.min(h - 1, y * 2 + dy) * w + Math.min(w - 1, x * 2 + dx)));
    const d = (y * W + x) * 4;
    if (keyMode) {   // the key wins by majority, else the average of the non-keyed pixels
      const live = idx.filter((i) => !isKey(rgba, i));
      if (live.length < 2) { r[d] = 0; r[d + 1] = 255; r[d + 2] = 0; r[d + 3] = 255; continue; }
      for (let k = 0; k < 3; k++) r[d + k] = Math.round(live.reduce((s, i) => s + rgba[i * 4 + k], 0) / live.length);
      if (r[d] === 0 && r[d + 1] === 255 && r[d + 2] === 0) r[d + 1] = 254;
      r[d + 3] = 255;
    } else for (let k = 0; k < 4; k++) r[d + k] = Math.round(idx.reduce((s, i) => s + rgba[i * 4 + k], 0) / 4);
  }
  return { w: W, h: H, rgba: r };
}

// ---- collect the images ----
const jobs = [];
for (const v of vpps) {
  const path = join(game, v + ".vpp"); if (!existsSync(path)) continue;
  const dir = readDirectory(path), names = new Set(dir.files.map((f) => f.name.toLowerCase()));
  const fd = openSync(path, "r");
  for (const f of dir.files) {
    const n = f.name;
    if (!/\.tga$/i.test(n) || /-mip\d+\.tga$/i.test(n)) continue;
    if (only && !(only.includes("*") ? new RegExp("^" + only.replace(/[.+^${}()|[\]\\]/g, "\\$&").replace(/\*/g, ".*") + "$", "i").test(n) : n.toLowerCase().includes(only.toLowerCase()))) continue;
    const b = Buffer.alloc(f.size); readSync(fd, b, 0, f.size, f.offset);
    const t = readTga(b); if (!t) continue;
    const m = Math.max(t.w, t.h), factor = m <= max4 ? 4 : m <= max2 ? 2 : 1;
    if (factor === 1) continue;
    let mips = 0; while (names.has(n.replace(/\.tga$/i, `-mip${mips + 1}.tga`).toLowerCase())) mips++;
    jobs.push({ vpp: v, name: n, t, factor, mips, folder: v === "levelt" ? "data/maps/levels" : "data/interface/pc" });
    if (limit && jobs.length >= limit) break;
  }
  closeSync(fd);
  if (limit && jobs.length >= limit) break;
}
console.log(`${jobs.length} images to upscale with ${model}`);

// ---- the model, in batches (one folder in, one out) ----
const work = join(out, ".work"), win = join(work, "in"), wout = join(work, "out");
for (const d of [win, wout]) mkdirSync(d, { recursive: true });
const BATCH = 200;
let done = 0;
for (let b0 = 0; b0 < jobs.length; b0 += BATCH) {
  const batch = jobs.slice(b0, b0 + BATCH);
  batch.forEach((j, i) => {
    const t = j.t, key = t.bpp === 24 && (() => { for (let p = 0; p < t.w * t.h; p++) if (isKey(t.rgba, p)) return true; return false; })();
    j.key = key; j.id = `j${b0 + i}`;
    const col = key ? fillKeyed(t) : t.rgba;
    let p = padded(t.w, t.h, (s, b, d) => { b[d] = col[s * 4]; b[d + 1] = col[s * 4 + 1]; b[d + 2] = col[s * 4 + 2]; });
    writePngRgb(join(win, j.id + "_c.png"), p.W, p.H, p.b);
    if (t.bpp === 32) {
      p = padded(t.w, t.h, (s, b, d) => { b[d] = b[d + 1] = b[d + 2] = t.rgba[s * 4 + 3]; });
      writePngRgb(join(win, j.id + "_a.png"), p.W, p.H, p.b);
    }
    if (key) {
      p = padded(t.w, t.h, (s, b, d) => { b[d] = b[d + 1] = b[d + 2] = isKey(t.rgba, s) ? 255 : 0; });
      writePngRgb(join(win, j.id + "_k.png"), p.W, p.H, p.b);
    }
  });
  execFileSync(esrgan, ["-i", win, "-o", wout, "-n", model, "-s", "4", "-f", "png"],
    { cwd: dirname(esrgan), stdio: ["ignore", "ignore", "pipe"] });
  for (const j of batch) {
    const { t, factor: f } = j, W = t.w * f, H = t.h * f;
    const col = unpad(readPng(join(wout, j.id + "_c.png")), t.w, t.h, f), rgba = Buffer.alloc(W * H * 4);
    for (let i = 0; i < W * H; i++) { rgba[i * 4] = col[i * 3]; rgba[i * 4 + 1] = col[i * 3 + 1]; rgba[i * 4 + 2] = col[i * 3 + 2]; rgba[i * 4 + 3] = 255; }
    if (t.bpp === 32) {
      const a = unpad(readPng(join(wout, j.id + "_a.png")), t.w, t.h, f);
      let binary = true; for (let i = 0; i < t.w * t.h; i++) { const v = t.rgba[i * 4 + 3]; if (v !== 0 && v !== 255) { binary = false; break; } }
      for (let i = 0; i < W * H; i++) rgba[i * 4 + 3] = binary ? (a[i * 3] >= 128 ? 255 : 0) : a[i * 3];
    }
    if (j.key) {
      const k = unpad(readPng(join(wout, j.id + "_k.png")), t.w, t.h, f);
      for (let i = 0; i < W * H; i++) {
        if (k[i * 3] >= 128) { rgba[i * 4] = 0; rgba[i * 4 + 1] = 255; rgba[i * 4 + 2] = 0; }
        else if (rgba[i * 4] === 0 && rgba[i * 4 + 1] === 255 && rgba[i * 4 + 2] === 0) rgba[i * 4 + 1] = 254;
      }
    }
    const dir = join(out, j.folder); mkdirSync(dir, { recursive: true });
    writeTga(join(dir, j.name), W, H, t.bpp, t.desc, rgba);
    if (j.mips) {
      const levels = j.mips + Math.log2(f);
      let m = { w: W, h: H, rgba };
      for (let l = 1; l <= levels && (m.w > 1 || m.h > 1); l++) {
        m = halve(m.rgba, m.w, m.h, j.key);
        writeTga(join(dir, j.name.replace(/\.tga$/i, `-mip${l}.tga`)), m.w, m.h, t.bpp, t.desc, m.rgba);
      }
    }
    done++;
  }
  for (const d of [win, wout]) for (const f of readdirSync(d)) unlinkSync(join(d, f));   // single files only
  console.log(`${done} / ${jobs.length}`);
}
if (!keepWork) { rmdirSync(win); rmdirSync(wout); rmdirSync(work); }   // empty folders only
console.log(`done: ${out}`);
