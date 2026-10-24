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
// Where the files go: the game reads a bitmap's header from any data folder (bm_read_header, PATH_ANY) but its
// pixels from the folder the load asked for (bm_lock, the entry's path_id: path_interface, path_maps_levels, or every
// folder). The header and the pixels must come from the same file, or a larger image overflows the buffer (seen
// 2026-10-24), so every file is written to data\interface\pc and hard-linked into data\maps\levels.
// A name in two packfiles (58 are, 7 with different contents) is taken from the packfile the game finds first
// (packfile_init's order, then the level packfiles), as packfile_find does; the later copies are never used.
//
// The output is derived from the game's art: never commit or publish it. The work folder (PNG files for the model)
// is <out>\.work and is emptied file by file afterwards unless --keep-work.
import { readFileSync, writeFileSync, mkdirSync, readdirSync, unlinkSync, existsSync, openSync, readSync, closeSync, rmdirSync, linkSync } from "node:fs";
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
const namesFile = opt("--names");   // a file with one texture name per line: only those
const onlyNames = namesFile ? new Set(readFileSync(namesFile, "utf8").split(String.fromCharCode(10)).map((s) => s.trim().toLowerCase()).filter(Boolean)) : null;
const max4 = +opt("--max4", "256"), max2 = +opt("--max2", "512"), keepWork = args.includes("--keep-work");
const threads = opt("--threads");   // the model's -j load:proc:save
const tile = opt("--tile");   // the model's -t (default automatic; 128 did not help against the broken images below)
const vpps = opt("--vpp", "summoner,chars,items,effects,cutscene,levelt").split(",");   // the game's packfile order
const FOLDERS = ["data/interface/pc", "data/maps/levels"];   // written to the first, hard-linked into the second
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

// One output file: written in the first folder, the same file (a hard link) in the others.
function place(name, write) {
  const paths = FOLDERS.map((f) => { mkdirSync(join(out, f), { recursive: true }); return join(out, f, name); });
  for (const p of paths) if (existsSync(p)) unlinkSync(p);
  write(paths[0]);
  for (const p of paths.slice(1)) linkSync(paths[0], p);
}

// ---- VBM (bm_read_vbm_header, bm_lock_vbm) ----
// A 32-byte header (".vbm", version 1, width, height, format, fps, frames, mip count), then every frame with all its
// mip levels, raw 16-bit, frame after frame. Formats: 0 = 1555 (a set top bit is TRANSPARENT, as the OpenGL
// renderer reads it: gl_convert_1555_to_rgba8888), 1 = 4444 (alpha in the top nibble), anything else = 565.
function decode16(px, fmt) {
  if (fmt === 0) return [Math.round(((px >> 10) & 31) * 255 / 31), Math.round(((px >> 5) & 31) * 255 / 31), Math.round((px & 31) * 255 / 31), px & 0x8000 ? 0 : 255];
  if (fmt === 1) return [((px >> 8) & 15) * 17, ((px >> 4) & 15) * 17, (px & 15) * 17, (px >> 12) * 17];
  return [Math.round(((px >> 11) & 31) * 255 / 31), Math.round(((px >> 5) & 63) * 255 / 63), Math.round((px & 31) * 255 / 31), 255];
}
function encode16(r, g, b, a, fmt) {
  const q = (v, bits) => Math.round(v * ((1 << bits) - 1) / 255);
  if (fmt === 0) return (a >= 128 ? 0 : 0x8000) | (q(r, 5) << 10) | (q(g, 5) << 5) | q(b, 5);
  if (fmt === 1) return (q(a, 4) << 12) | (q(r, 4) << 8) | (q(g, 4) << 4) | q(b, 4);
  return (q(r, 5) << 11) | (q(g, 6) << 5) | q(b, 5);
}
function levelPixels(w, h, levels) { let n = 0; for (let l = 0; l < levels; l++) n += Math.max(1, w >> l) * Math.max(1, h >> l); return n; }
function readVbm(b) {
  if (b.toString("latin1", 0, 4) !== ".vbm") return null;
  const v = { ver: b.readInt32LE(4), w: b.readInt32LE(8), h: b.readInt32LE(12), fmt: b.readInt32LE(16), fps: b.readInt32LE(20),
    frames: b.readInt32LE(24), mips: b.readInt32LE(28) };
  const per = levelPixels(v.w, v.h, v.mips + 1);
  if (b.length < 32 + v.frames * per * 2) return null;
  v.frameRgba = [];
  for (let f = 0; f < v.frames; f++) {
    const base = 32 + f * per * 2, rgba = Buffer.alloc(v.w * v.h * 4);
    for (let i = 0; i < v.w * v.h; i++) { const c = decode16(b.readUInt16LE(base + i * 2), v.fmt); rgba.set(c, i * 4); }
    v.frameRgba.push(rgba);
  }
  return v;
}
function writeVbm(path, v, w, h, mips, frames) {
  const per = levelPixels(w, h, mips + 1), out = Buffer.alloc(32 + frames.length * per * 2);
  out.write(".vbm", 0, "latin1");
  [v.ver, w, h, v.fmt, v.fps, frames.length, mips].forEach((x, i) => out.writeInt32LE(x, 4 + i * 4));
  let o = 32;
  for (const f of frames) {
    let m = { w, h, rgba: f };
    for (let l = 0; l <= mips; l++) {
      if (l) m = halve(m.rgba, m.w, m.h, false);
      for (let i = 0; i < m.w * m.h; i++) { out.writeUInt16LE(encode16(m.rgba[i * 4], m.rgba[i * 4 + 1], m.rgba[i * 4 + 2], m.rgba[i * 4 + 3], v.fmt), o); o += 2; }
    }
  }
  writeFileSync(path, out);
}
// Factor for a VBM: as a TGA for a still image; for an animation 4x up to 64 pixels, else 2x; then halved while the
// file would hold more than VBM_MAX_BYTES (the game keeps every frame in memory, bm_lock_vbm).
const VBM_MAX_BYTES = 16 * 2 ** 20;
function vbmFactor(v) {
  const m = Math.max(v.w, v.h);
  let f = v.frames === 1 ? (m <= max4 ? 4 : m <= max2 ? 2 : 1) : (m <= 64 ? 4 : m <= max2 ? 2 : 1);
  const mipsAt = (k) => (v.mips ? v.mips + Math.log2(k) : 0);
  while (f > 1 && levelPixels(v.w * f, v.h * f, mipsAt(f) + 1) * 2 * v.frames > VBM_MAX_BYTES) f /= 2;
  return f;
}

// ---- fonts (gr_font_read_file, gr_font_create) ----
// A .vf file: signature, version, [format], character count, first ASCII, default spacing, height, kern pair count,
// (version 0: two more ints), pixel data size; the kern pairs (3 bytes each); per character spacing, width, pixel
// offset (ints), kerning entry, user data (shorts); the pixel data; for the palette format 256 palette ints.
// gr_font_create packs the characters left to right, row by row, into a 64, 128 or 256-pixel square of 4444 pixels
// (alpha in the top nibble). The pack here is the same, so <font>-hires.tga lines up with the game's own atlas at
// 2x or 4x, and the experimental build draws from it at the original size (HiResInterface; experimental/hires.cpp).
const FONT_RGB16 = 0x0f0f0f0f, FONT_ALPHA4 = 0xf, FONT_PAL8 = -0x10;
function readVf(b) {
  let p = 0; const i32 = () => { const v = b.readInt32LE(p); p += 4; return v; };
  const f = { sig: i32(), ver: i32() };
  f.format = f.ver < 1 ? FONT_ALPHA4 : i32();
  f.numChars = i32(); f.firstAscii = i32(); f.spacing = i32(); f.height = i32(); f.numKern = i32();
  if (f.ver < 1) { i32(); i32(); }
  f.pixelSize = i32();
  p += f.numKern * 3;
  f.chars = [];
  for (let c = 0; c < f.numChars; c++) { const spacing = i32(), width = i32(), off = i32(); p += 4; f.chars.push({ spacing, width, off }); }
  f.pixels = b.subarray(p, p + f.pixelSize); p += f.pixelSize;
  if (f.format === FONT_PAL8) { f.palette = []; for (let k = 0; k < 256; k++) f.palette.push(b.readUInt32LE(p + k * 4)); }
  return f;
}
// The game's atlas size and character positions (gr_font_create), and each character's RGBA pixels.
function fontAtlas(f) {
  const n = f.format === FONT_RGB16 ? f.pixelSize >>> 1 : f.pixelSize;
  const size = n * 2 < 0x1000 ? 64 : n * 2 < 0x4000 ? 128 : 256;
  const glyphs = []; let x = 0, y = 0;
  for (const c of f.chars) {
    if (c.width + x >= size) { y += f.height; x = 0; }
    const rgba = Buffer.alloc(c.width * f.height * 4);
    for (let row = 0; row < f.height; row++) for (let col = 0; col < c.width; col++) {
      const k = row * c.width + col; let px;
      if (f.format === FONT_RGB16) px = f.pixels.readUInt16LE(c.off + k * 2);
      else if (f.format === FONT_PAL8) { const v = f.palette[f.pixels[c.off + k]]; px = ((v >>> 24) & 15) << 12 | (v & 15) << 8 | ((v >>> 8) & 15) << 4 | ((v >>> 16) & 15); }
      else { const a = Math.min(14, f.pixels[c.off + k]); px = (Math.round(a * 255 / 14) >> 4) << 12 | 0xfff; }
      rgba.set([((px >> 8) & 15) * 17, ((px >> 4) & 15) * 17, (px & 15) * 17, (px >> 12) * 17], k * 4);
    }
    glyphs.push({ x, y, w: c.width, rgba });
    x += c.width;
  }
  return { size, glyphs };
}
// Characters spaced apart for the model (transparent gutters, colour spread into them so edges do not darken).
const FONT_GUTTER = 6;
function fontSheet(f, atlas) {
  const cols = 16, cellW = Math.max(...atlas.glyphs.map((g) => g.w)) + 2 * FONT_GUTTER, cellH = f.height + 2 * FONT_GUTTER;
  const rows = Math.ceil(atlas.glyphs.length / cols), w = cols * cellW, h = rows * cellH, rgba = Buffer.alloc(w * h * 4);
  atlas.glyphs.forEach((g, i) => {
    const ox = (i % cols) * cellW + FONT_GUTTER, oy = Math.floor(i / cols) * cellH + FONT_GUTTER;
    g.sheetX = ox; g.sheetY = oy;
    for (let y = 0; y < f.height; y++) g.rgba.copy(rgba, ((oy + y) * w + ox) * 4, y * g.w * 4, (y + 1) * g.w * 4);
  });
  // spread colour into transparent pixels (a few passes), alpha stays 0
  for (let pass = 0; pass < FONT_GUTTER; pass++) {
    const src = Buffer.from(rgba);
    for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
      const i = (y * w + x) * 4; if (src[i + 3]) continue;
      let n = 0, s = [0, 0, 0];
      for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
        const xx = x + dx, yy = y + dy; if (xx < 0 || yy < 0 || xx >= w || yy >= h) continue;
        const j = (yy * w + xx) * 4; if (src[j + 3] || src[j] || src[j + 1] || src[j + 2]) { n++; for (let k = 0; k < 3; k++) s[k] += src[j + k]; }
      }
      if (n) for (let k = 0; k < 3; k++) rgba[i + k] = Math.round(s[k] / n);
    }
  }
  return { w, h, bpp: 32, desc: 0x28, rgba };
}

// ---- collect the images ----
const jobs = [], seen = new Set();
for (const v of vpps) {
  const path = join(game, v + ".vpp"); if (!existsSync(path)) continue;
  const dir = readDirectory(path), names = new Set(dir.files.map((f) => f.name.toLowerCase()));
  const fd = openSync(path, "r");
  for (const f of dir.files) {
    const n = f.name;
    if (!/\.tga$/i.test(n) || /-mip\d+\.tga$/i.test(n)) continue;
    if (seen.has(n.toLowerCase())) continue;   // the game uses the first packfile's copy
    seen.add(n.toLowerCase());
    if (onlyNames && !onlyNames.has(n.toLowerCase())) continue;
    if (only && !(only.includes("*") ? new RegExp("^" + only.replace(/[.+^${}()|[\]\\]/g, "\\$&").replace(/\*/g, ".*") + "$", "i").test(n) : n.toLowerCase().includes(only.toLowerCase()))) continue;
    const b = Buffer.alloc(f.size); readSync(fd, b, 0, f.size, f.offset);
    const t = readTga(b); if (!t) continue;
    const m = Math.max(t.w, t.h), factor = m <= max4 ? 4 : m <= max2 ? 2 : 1;
    if (factor === 1) continue;
    let mips = 0; while (names.has(n.replace(/\.tga$/i, `-mip${mips + 1}.tga`).toLowerCase())) mips++;
    jobs.push({ vpp: v, name: n, t, factor, mips });
    if (limit && jobs.length >= limit) break;
  }
  closeSync(fd);
  if (limit && jobs.length >= limit) break;
}
// VBM animations and stills: one job per frame; the file is written when its last frame is done.
// Fonts: one job per font (its characters spaced apart); the finish packs them into <font>-hires.tga.
const wanted = (n) => (!onlyNames || onlyNames.has(n.toLowerCase())) &&
  (!only || (only.includes("*") ? new RegExp("^" + only.replace(/[.+^${}()|[\]\\]/g, "\\$&").replace(/\*/g, ".*") + "$", "i").test(n) : n.toLowerCase().includes(only.toLowerCase())));
let vbmCount = 0, fontCount = 0;
for (const v of vpps) {
  if (args.includes("--no-vbm") && args.includes("--no-fonts")) break;
  const path = join(game, v + ".vpp"); if (!existsSync(path)) continue;
  const dir = readDirectory(path), fd = openSync(path, "r");
  for (const f of dir.files) {
    const n = f.name, lower = n.toLowerCase();
    const isVbm = /\.vbm$/i.test(n) && !args.includes("--no-vbm"), isVf = /\.vf$/i.test(n) && !args.includes("--no-fonts");
    if ((!isVbm && !isVf) || seen.has(lower)) continue;
    seen.add(lower);
    if (!wanted(n)) continue;
    const b = Buffer.alloc(f.size); readSync(fd, b, 0, f.size, f.offset);
    if (isVbm) {
      const vb = readVbm(b); if (!vb) continue;
      const fac = vbmFactor(vb); if (fac === 1) continue;
      const parent = { got: 0, frames: new Array(vb.frames) };
      vb.frameRgba.forEach((rgba, i) => jobs.push({
        vpp: v, name: `${n} frame ${i + 1}`, factor: fac,
        t: { w: vb.w, h: vb.h, bpp: vb.fmt === 0 || vb.fmt === 1 ? 32 : 24, desc: 0x20, rgba, noKey: true },
        finish: (out) => {
          parent.frames[i] = out;
          if (++parent.got < vb.frames) return;
          const mips = vb.mips ? vb.mips + Math.log2(fac) : 0;
          place(n, (p) => writeVbm(p, vb, vb.w * fac, vb.h * fac, mips, parent.frames));
        },
      }));
      vbmCount++;
    } else {
      const font = readVf(b), atlas = fontAtlas(font), sheet = fontSheet(font, atlas);
      const fac = atlas.size * 4 <= 2048 ? 4 : 2;
      jobs.push({
        vpp: v, name: n, factor: fac, t: { ...sheet, noKey: true },
        finish: (out, W) => {
          const S = atlas.size * fac, img = Buffer.alloc(S * S * 4), gh = font.height * fac;
          for (const g of atlas.glyphs) for (let y = 0; y < gh; y++)
            out.copy(img, ((g.y * fac + y) * S + g.x * fac) * 4, ((g.sheetY * fac + y) * W + g.sheetX * fac) * 4, ((g.sheetY * fac + y) * W + (g.sheetX + g.w) * fac) * 4);
          place(n.replace(/\.vf$/i, "-hires.tga"), (p) => writeTga(p, S, S, 32, 0x28, img));
        },
      });
      fontCount++;
    }
  }
  closeSync(fd);
}
console.log(`${jobs.length} images to upscale with ${model} (${vbmCount} VBM files as frames, ${fontCount} fonts)`);

// ---- the model, in batches (one folder in, one out) ----
const work = join(out, ".work"), win = join(work, "in"), wout = join(work, "out");
const w1in = join(work, "one-in"), w1out = join(work, "one-out");
for (const d of [win, wout, w1in, w1out]) mkdirSync(d, { recursive: true });
const runModel = (i, o) => execFileSync(esrgan, ["-i", i, "-o", o, "-n", model, "-s", "4", "-f", "png",
  ...(threads ? ["-j", threads] : []), ...(tile ? ["-t", tile] : [])], { cwd: dirname(esrgan), stdio: ["ignore", "ignore", "pipe"] });

// Is the model's output broken? On an Intel Arc (2026-10-24) large batches gave about one image in five black or
// noise, while the same image alone came out right. A correct result can still differ in tone: the model smooths
// fine speckle it takes for noise, and a dark, speckled texture comes out much darker (a floor from mean 46 to 19,
// the same every time), which correctTone below puts right. So the test is structure, not tone: the output shrunk
// back to the input's size must follow the input (correlation, averaged over the channels that vary). A flat input
// is compared by its mean instead. Scores: 0 = follows the input, 1 = no relation. One above RETRY_SCORE is run
// again alone (up to 3 times, the best kept); one still above FALLBACK_SCORE becomes a plain bilinear enlargement.
const RETRY_SCORE = 0.3, FALLBACK_SCORE = 0.5;
function modelScore(inPath, outPath) {
  if (!existsSync(outPath)) return Infinity;
  const a = readPng(inPath), b = readPng(outPath);
  if (b.w !== a.w * 4 || b.h !== a.h * 4) return Infinity;
  const n = a.w * a.h, small = new Float64Array(n * 3);
  for (let y = 0; y < a.h; y++) for (let x = 0; x < a.w; x++) for (let k = 0; k < 3; k++) {
    let s = 0;
    for (let dy = 0; dy < 4; dy++) for (let dx = 0; dx < 4; dx++) s += b.rgb[((y * 4 + dy) * b.w + x * 4 + dx) * 3 + k];
    small[(y * a.w + x) * 3 + k] = s / 16;
  }
  let total = 0, used = 0, flatDiff = 0;
  for (let k = 0; k < 3; k++) {
    let mi = 0, mo = 0;
    for (let i = 0; i < n; i++) { mi += a.rgb[i * 3 + k]; mo += small[i * 3 + k]; }
    mi /= n; mo /= n;
    let cov = 0, vi = 0, vo = 0;
    for (let i = 0; i < n; i++) { const di = a.rgb[i * 3 + k] - mi, dout = small[i * 3 + k] - mo; cov += di * dout; vi += di * di; vo += dout * dout; }
    if (vi / n < 4) { flatDiff = Math.max(flatDiff, Math.abs(mo - mi)); continue; }   // a flat channel
    used++;
    total += vo > 0 ? 1 - Math.max(0, cov / Math.sqrt(vi * vo)) : 1;
  }
  const corr = used ? total / used : 0;
  return Math.max(corr, flatDiff > 20 ? 1 : 0);
}
// Puts the input's tone back: the difference between the input and the output shrunk back to the input's size,
// enlarged smoothly (bilinear) and added. The model's detail stays; the output shrunk back then equals the input.
function correctTone(inPath, img) {
  const a = readPng(inPath), W = img.w, H = img.h, d = new Float64Array(a.w * a.h * 3);
  for (let y = 0; y < a.h; y++) for (let x = 0; x < a.w; x++) for (let k = 0; k < 3; k++) {
    let s = 0;
    for (let dy = 0; dy < 4; dy++) for (let dx = 0; dx < 4; dx++) s += img.rgb[((y * 4 + dy) * W + x * 4 + dx) * 3 + k];
    d[(y * a.w + x) * 3 + k] = a.rgb[(y * a.w + x) * 3 + k] - s / 16;
  }
  const rgb = Buffer.alloc(W * H * 3);
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
    const fx = Math.min(a.w - 1, Math.max(0, (x + 0.5) / 4 - 0.5)), fy = Math.min(a.h - 1, Math.max(0, (y + 0.5) / 4 - 0.5));
    const x0 = Math.floor(fx), y0 = Math.floor(fy), x1 = Math.min(a.w - 1, x0 + 1), y1 = Math.min(a.h - 1, y0 + 1), tx = fx - x0, ty = fy - y0;
    for (let k = 0; k < 3; k++) {
      const p = (xx, yy) => d[(yy * a.w + xx) * 3 + k];
      const v = img.rgb[(y * W + x) * 3 + k] + (p(x0, y0) * (1 - tx) + p(x1, y0) * tx) * (1 - ty) + (p(x0, y1) * (1 - tx) + p(x1, y1) * tx) * ty;
      rgb[(y * W + x) * 3 + k] = v < 0 ? 0 : v > 255 ? 255 : Math.round(v);
    }
  }
  return { w: W, h: H, rgb };
}
const modelOut = (id, suffix) => correctTone(join(win, id + suffix), readPng(join(wout, id + suffix)));
function bilinear4(inPath, outPath) {
  const a = readPng(inPath), W = a.w * 4, H = a.h * 4, r = Buffer.alloc(W * H * 3);
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
    const fx = Math.min(a.w - 1, Math.max(0, (x + 0.5) / 4 - 0.5)), fy = Math.min(a.h - 1, Math.max(0, (y + 0.5) / 4 - 0.5));
    const x0 = Math.floor(fx), y0 = Math.floor(fy), x1 = Math.min(a.w - 1, x0 + 1), y1 = Math.min(a.h - 1, y0 + 1), tx = fx - x0, ty = fy - y0;
    for (let k = 0; k < 3; k++) {
      const p = (xx, yy) => a.rgb[(yy * a.w + xx) * 3 + k];
      r[(y * W + x) * 3 + k] = Math.round((p(x0, y0) * (1 - tx) + p(x1, y0) * tx) * (1 - ty) + (p(x0, y1) * (1 - tx) + p(x1, y1) * tx) * ty);
    }
  }
  writePngRgb(outPath, W, H, r);
}
const stats = { retried: 0, fixed: 0, fallback: [] }, idName = new Map();
function checkBatch() {
  for (const f of readdirSync(win)) {
    const inP = join(win, f), outP = join(wout, f);
    let best = modelScore(inP, outP);
    if (best <= RETRY_SCORE) continue;
    stats.retried++;
    for (let tries = 0; tries < 3 && best > RETRY_SCORE; tries++) {
      for (const d of [w1in, w1out]) for (const x of readdirSync(d)) unlinkSync(join(d, x));   // single files only
      writeFileSync(join(w1in, f), readFileSync(inP));
      runModel(w1in, w1out);
      const s = modelScore(inP, join(w1out, f));
      if (s < best) { best = s; writeFileSync(outP, readFileSync(join(w1out, f))); }
    }
    if (best <= FALLBACK_SCORE) stats.fixed++;
    else { bilinear4(inP, outP); stats.fallback.push(`${idName.get(f.replace(/_[cak].png$/, ""))} (${f.slice(-5, -4)})`); }
  }
}
const BATCH = 200;
let done = 0;
for (let b0 = 0; b0 < jobs.length; b0 += BATCH) {
  const batch = jobs.slice(b0, b0 + BATCH);
  batch.forEach((j, i) => {
    const t = j.t, key = !t.noKey && t.bpp === 24 && (() => { for (let p = 0; p < t.w * t.h; p++) if (isKey(t.rgba, p)) return true; return false; })();
    j.key = key; j.id = `j${b0 + i}`; idName.set(j.id, j.name);
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
  runModel(win, wout);
  checkBatch();
  for (const j of batch) {
    const { t, factor: f } = j, W = t.w * f, H = t.h * f;
    const col = unpad(modelOut(j.id, "_c.png"), t.w, t.h, f), rgba = Buffer.alloc(W * H * 4);
    for (let i = 0; i < W * H; i++) { rgba[i * 4] = col[i * 3]; rgba[i * 4 + 1] = col[i * 3 + 1]; rgba[i * 4 + 2] = col[i * 3 + 2]; rgba[i * 4 + 3] = 255; }
    if (t.bpp === 32) {
      const a = unpad(modelOut(j.id, "_a.png"), t.w, t.h, f);
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
    if (j.finish) { j.finish(rgba, W, H); done++; continue; }
    place(j.name, (p) => writeTga(p, W, H, t.bpp, t.desc, rgba));
    if (j.mips) {
      const levels = j.mips + Math.log2(f);
      let m = { w: W, h: H, rgba };
      for (let l = 1; l <= levels && (m.w > 1 || m.h > 1); l++) {
        m = halve(m.rgba, m.w, m.h, j.key);
        const mm = m;
        place(j.name.replace(/\.tga$/i, `-mip${l}.tga`), (p) => writeTga(p, mm.w, mm.h, t.bpp, t.desc, mm.rgba));
      }
    }
    done++;
  }
  if (!keepWork) for (const d of [win, wout]) for (const f of readdirSync(d)) unlinkSync(join(d, f));   // single files only
  console.log(`${done} / ${jobs.length}`);
}
for (const d of [w1in, w1out]) for (const x of readdirSync(d)) unlinkSync(join(d, x));   // single files only
if (!keepWork) { for (const d of [win, wout, w1in, w1out]) rmdirSync(d); rmdirSync(work); }   // empty folders only
console.log(`checked: ${stats.retried} model outputs run again (${stats.fixed} fixed), ${stats.fallback.length} bilinear instead`);
if (stats.fallback.length) console.log("bilinear: " + stats.fallback.join(", "));
console.log(`done: ${out}`);
