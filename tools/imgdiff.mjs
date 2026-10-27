// Compares two images pixel by pixel, for the renderer parity tests (the same screen under OpenGL and under Vulkan):
// RGB only, alpha ignored. A pixel differs when any channel differs by more than the tolerance. Prints one line: the
// size, the percent of differing pixels, the percent beyond a one-pixel shift (below), the mean absolute difference
// per channel, the largest difference, the PSNR and the bounding box of the differing pixels.
//
// "Beyond a 1-px shift": a differing pixel counts only when A's pixel matches (within the tolerance) none of the 3x3
// pixels around the same position in B, or B's pixel none of the 3x3 around it in A. A polygon edge moved by one
// pixel then does not count; a new or missing detail, even one pixel wide, does. Both percents are of all compared
// pixels; the exit code goes by this one.
//
// Inputs: uncompressed BMP (24 or 32 bits, bottom-up or top-down) or PNG (non-interlaced; grey, RGB, palette, grey
// with alpha and RGBA, 8 bits; also 16 bits, and 1/2/4 bits for grey and palette). --out writes a 24-bit BMP: a
// darkened grey copy of A, the pixels that differ beyond a shift in red, the shift-only ones in yellow.
// Exit code: 0 when the beyond-shift percent is <= --max-percent, 1 when above, 2 on an error (an unreadable file;
// different sizes unless --resize, which compares the overlapping top-left area and says so).
// usage: node tools/imgdiff.mjs A B [--out DIFF.bmp] [--tolerance 8] [--max-percent 0.5] [--resize] [--json]
import { readFileSync, writeFileSync } from "node:fs";
import { inflateSync } from "node:zlib";

const usage = "usage: node tools/imgdiff.mjs A B [--out DIFF.bmp] [--tolerance 8] [--max-percent 0.5] [--resize] [--json]";
const args = process.argv.slice(2);
const valued = ["--out", "--tolerance", "--max-percent"];
const opt = (name, def) => (args.includes(name) ? args[args.indexOf(name) + 1] : def);
const files = args.filter((a, i) => !a.startsWith("--") && !valued.includes(args[i - 1]));
const json = args.includes("--json"), resize = args.includes("--resize");
const tol = Number(opt("--tolerance", 8)), maxPercent = Number(opt("--max-percent", 0.5)), outPath = opt("--out");
const unknown = args.filter((a) => a.startsWith("--") && !valued.includes(a) && a !== "--json" && a !== "--resize");

function fail(msg) {
  if (json) console.log(JSON.stringify({ error: msg }));
  else console.error(`imgdiff: ${msg}`);
  process.exit(2);
}
if (files.length !== 2 || unknown.length || !(tol >= 0) || !(maxPercent >= 0)) fail(unknown.length ? `unknown option ${unknown[0]}\n${usage}` : usage);

// --- BMP: BITMAPINFOHEADER or later, BI_RGB (24/32) or BI_BITFIELDS with byte-aligned masks (32) ---
function readBmp(b) {
  const off = b.readUInt32LE(10), hsize = b.readUInt32LE(14);
  if (hsize < 40) throw new Error("BMP: OS/2 header not supported");
  const w = b.readInt32LE(18), hRaw = b.readInt32LE(22), bpp = b.readUInt16LE(28), comp = b.readUInt32LE(30);
  const h = Math.abs(hRaw), topDown = hRaw < 0;
  if (bpp !== 24 && bpp !== 32) throw new Error(`BMP: ${bpp} bits per pixel not supported (24 or 32)`);
  let shifts = [16, 8, 0];                       // R, G, B bit positions in a little-endian pixel
  if (comp === 3 && bpp === 32) {
    const masks = hsize >= 52 ? [b.readUInt32LE(54), b.readUInt32LE(58), b.readUInt32LE(62)]
                              : [b.readUInt32LE(14 + hsize), b.readUInt32LE(18 + hsize), b.readUInt32LE(22 + hsize)];
    shifts = masks.map((m) => {
      const s = [0, 8, 16, 24].find((k) => m >>> 0 === (0xff << k) >>> 0);
      if (s === undefined) throw new Error(`BMP: channel mask 0x${m.toString(16)} not supported`);
      return s;
    });
  } else if (comp !== 0) throw new Error(`BMP: compression ${comp} not supported`);
  const stride = (w * (bpp / 8) + 3) & ~3, px = new Uint8Array(w * h * 3);
  if (off + stride * (h - 1) + w * (bpp / 8) > b.length) throw new Error("BMP: file too short");
  for (let y = 0; y < h; y++) {
    const row = off + (topDown ? y : h - 1 - y) * stride;
    for (let x = 0; x < w; x++) {
      const o = (y * w + x) * 3;
      if (bpp === 24) { const s = row + x * 3; px[o] = b[s + 2]; px[o + 1] = b[s + 1]; px[o + 2] = b[s]; }
      else { const v = b.readUInt32LE(row + x * 4); px[o] = (v >>> shifts[0]) & 255; px[o + 1] = (v >>> shifts[1]) & 255; px[o + 2] = (v >>> shifts[2]) & 255; }
    }
  }
  return { w, h, px };
}

// --- PNG: chunks, zlib, the five filters ---
function readPng(b) {
  let pos = 8, ihdr, plte, idat = [];
  while (pos + 8 <= b.length) {
    const len = b.readUInt32BE(pos), type = b.toString("latin1", pos + 4, pos + 8), data = b.subarray(pos + 8, pos + 8 + len);
    if (type === "IHDR") ihdr = data; else if (type === "PLTE") plte = data; else if (type === "IDAT") idat.push(data);
    else if (type === "IEND") break;
    pos += 12 + len;
  }
  if (!ihdr) throw new Error("PNG: no IHDR");
  const w = ihdr.readUInt32BE(0), h = ihdr.readUInt32BE(4), depth = ihdr[8], ctype = ihdr[9], interlace = ihdr[12];
  if (interlace) throw new Error("PNG: interlaced images not supported");
  const chans = { 0: 1, 2: 3, 3: 1, 4: 2, 6: 4 }[ctype];
  if (!chans) throw new Error(`PNG: colour type ${ctype} not supported`);
  if (ctype === 3 && !plte) throw new Error("PNG: palette image without PLTE");
  const bitsPerPx = chans * depth, bpp = Math.max(1, bitsPerPx >> 3), stride = Math.ceil((w * bitsPerPx) / 8);
  const raw = inflateSync(Buffer.concat(idat));
  if (raw.length < (stride + 1) * h) throw new Error("PNG: image data too short");
  const cur = new Uint8Array(stride), prev = new Uint8Array(stride), px = new Uint8Array(w * h * 3);
  const sample = (row, i) => {                  // the i-th sample of a row, scaled to 8 bits (16 bits: high byte)
    if (depth === 8) return row[i];
    if (depth === 16) return row[i * 2];
    const bit = i * depth, v = (row[bit >> 3] >> (8 - depth - (bit & 7))) & ((1 << depth) - 1);
    return ctype === 3 ? v : Math.round((v * 255) / ((1 << depth) - 1));
  };
  for (let y = 0; y < h; y++) {
    const base = y * (stride + 1), f = raw[base];
    for (let i = 0; i < stride; i++) {
      const x = raw[base + 1 + i], a = i >= bpp ? cur[i - bpp] : 0, up = prev[i], c = i >= bpp ? prev[i - bpp] : 0;
      let v;
      switch (f) {
        case 0: v = x; break;
        case 1: v = x + a; break;
        case 2: v = x + up; break;
        case 3: v = x + ((a + up) >> 1); break;
        case 4: { const p = a + up - c, pa = Math.abs(p - a), pb = Math.abs(p - up), pc = Math.abs(p - c);
                  v = x + (pa <= pb && pa <= pc ? a : pb <= pc ? up : c); break; }
        default: throw new Error(`PNG: bad filter ${f} in row ${y}`);
      }
      cur[i] = v & 255;
    }
    for (let x = 0; x < w; x++) {
      const o = (y * w + x) * 3;
      if (ctype === 3) { const k = sample(cur, x) * 3; px[o] = plte[k]; px[o + 1] = plte[k + 1]; px[o + 2] = plte[k + 2]; }
      else if (ctype === 0 || ctype === 4) px[o] = px[o + 1] = px[o + 2] = sample(cur, x * chans);
      else for (let c = 0; c < 3; c++) px[o + c] = sample(cur, x * chans + c);
    }
    prev.set(cur);
  }
  return { w, h, px };
}

function load(path) {
  let b;
  try { b = readFileSync(path); } catch (e) { fail(`cannot read ${path}: ${e.code || e.message}`); }
  try {
    if (b.length > 8 && b.readUInt32BE(0) === 0x89504e47) return readPng(b);
    if (b.length > 54 && b.toString("latin1", 0, 2) === "BM") return readBmp(b);
    throw new Error("neither a BMP nor a PNG");
  } catch (e) { fail(`${path}: ${e.message}`); }
}

function writeBmp(path, w, h, px) {
  const stride = (w * 3 + 3) & ~3, b = Buffer.alloc(54 + stride * h);
  b.write("BM", 0, "latin1"); b.writeUInt32LE(b.length, 2); b.writeUInt32LE(54, 10); b.writeUInt32LE(40, 14);
  b.writeInt32LE(w, 18); b.writeInt32LE(h, 22); b.writeUInt16LE(1, 26); b.writeUInt16LE(24, 28);
  b.writeUInt32LE(stride * h, 34);
  for (let y = 0; y < h; y++) {
    const row = 54 + (h - 1 - y) * stride;
    for (let x = 0; x < w; x++) { const o = (y * w + x) * 3, s = row + x * 3; b[s] = px[o + 2]; b[s + 1] = px[o + 1]; b[s + 2] = px[o]; }
  }
  writeFileSync(path, b);
}

const A = load(files[0]), B = load(files[1]);
const sizeA = `${A.w}x${A.h}`, sizeB = `${B.w}x${B.h}`;
if ((A.w !== B.w || A.h !== B.h) && !resize) fail(`different sizes: ${sizeA} and ${sizeB} (--resize compares the overlapping top-left area)`);
const w = Math.min(A.w, B.w), h = Math.min(A.h, B.h), n = w * h;
if (!n) fail("empty image");
const pa = (x, y) => (y * A.w + x) * 3, pb = (x, y) => (y * B.w + x) * 3;
const near = (P, i, Q, j) => Math.abs(P[i] - Q[j]) <= tol && Math.abs(P[i + 1] - Q[j + 1]) <= tol && Math.abs(P[i + 2] - Q[j + 2]) <= tol;

// Does pixel i of P match one of the 3x3 pixels around (x, y) in Q (inside the compared area)?
function matchesAround(P, i, Q, qi, x, y) {
  for (let dy = -1; dy <= 1; dy++) {
    const yy = y + dy;
    if (yy < 0 || yy >= h) continue;
    for (let dx = -1; dx <= 1; dx++) {
      const xx = x + dx;
      if (xx >= 0 && xx < w && near(P, i, Q, qi(xx, yy))) return true;
    }
  }
  return false;
}

const sum = [0, 0, 0];
let sq = 0, maxd = 0, diff = 0, structural = 0, x0 = w, y0 = h, x1 = -1, y1 = -1;
const mark = outPath ? new Uint8Array(n) : null;   // 1 = shift only, 2 = beyond a shift
for (let y = 0; y < h; y++) {
  for (let x = 0; x < w; x++) {
    const i = pa(x, y), j = pb(x, y);
    let over = false;
    for (let c = 0; c < 3; c++) {
      const d = Math.abs(A.px[i + c] - B.px[j + c]);
      sum[c] += d; sq += d * d;
      if (d > maxd) maxd = d;
      if (d > tol) over = true;
    }
    if (!over) continue;
    diff++;
    if (x < x0) x0 = x; if (x > x1) x1 = x; if (y < y0) y0 = y; if (y > y1) y1 = y;
    const real = !matchesAround(A.px, i, B.px, pb, x, y) || !matchesAround(B.px, j, A.px, pa, x, y);
    if (real) structural++;
    if (mark) mark[y * w + x] = real ? 2 : 1;
  }
}

const pct = (k) => (100 * k) / n;
const mse = sq / (n * 3), psnr = mse ? 10 * Math.log10((255 * 255) / mse) : Infinity;
const result = {
  a: files[0], b: files[1], width: w, height: h,
  ...(A.w !== B.w || A.h !== B.h ? { resized: { a: sizeA, b: sizeB } } : {}),
  tolerance: tol,
  differPixels: diff, differPercent: +pct(diff).toFixed(4),
  beyondShiftPixels: structural, beyondShiftPercent: +pct(structural).toFixed(4),
  meanAbsDiff: { r: +(sum[0] / n).toFixed(4), g: +(sum[1] / n).toFixed(4), b: +(sum[2] / n).toFixed(4) },
  maxDiff: maxd,
  psnr: Number.isFinite(psnr) ? +psnr.toFixed(2) : null,   // null: identical
  box: diff ? { x0, y0, x1, y1 } : null,                   // inclusive
  maxPercent, pass: pct(structural) <= maxPercent,
};

if (outPath) {
  const out = new Uint8Array(n * 3);
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const i = pa(x, y), o = (y * w + x) * 3, m = mark[y * w + x];
      const g = Math.round((A.px[i] * 0.299 + A.px[i + 1] * 0.587 + A.px[i + 2] * 0.114) * 0.4);
      if (m === 2) { out[o] = 255; out[o + 1] = 0; out[o + 2] = 0; }
      else if (m === 1) { out[o] = 255; out[o + 1] = 220; out[o + 2] = 0; }
      else out[o] = out[o + 1] = out[o + 2] = g;
    }
  }
  try { writeBmp(outPath, w, h, out); } catch (e) { fail(`cannot write ${outPath}: ${e.code || e.message}`); }
  result.out = outPath;
}

if (json) console.log(JSON.stringify(result, null, 2));
else {
  const m = result.meanAbsDiff, bx = result.box;
  console.log(
    `${w}x${h}${result.resized ? ` (overlap of ${sizeA} and ${sizeB})` : ""}: ` +
    `${result.differPercent}% differ (tolerance ${tol}), ${result.beyondShiftPercent}% beyond a 1-px shift; ` +
    `mean |d| R ${m.r} G ${m.g} B ${m.b}; max ${maxd}; PSNR ${result.psnr === null ? "inf" : result.psnr + " dB"}; ` +
    `box ${bx ? `${bx.x0},${bx.y0}-${bx.x1},${bx.y1}` : "none"}; ${result.pass ? "pass" : "FAIL"} (max ${maxPercent}%)` +
    (outPath ? `; diff ${outPath}` : ""));
}
process.exit(result.pass ? 0 : 1);
