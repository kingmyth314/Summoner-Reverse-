// Crops and enlarges part of a 24-bit bottom-up BMP (an agent-mode capture) into a PNG, to inspect small UI details.
//   node tools/crop-bmp.mjs <in.bmp> <out.png> <x> <y> <w> <h> [scale=3]
import fs from 'fs';
import zlib from 'zlib';
const [inp, out, X, Y, W, H, SC] = process.argv.slice(2);
const x0 = +X, y0 = +Y, w = +W, h = +H, sc = +(SC || 3);
const b = fs.readFileSync(inp);
const off = b.readUInt32LE(10), bw = b.readInt32LE(18), bh = b.readInt32LE(22);
const st = (bw * 3 + 3) & ~3;
const ow = w * sc, oh = h * sc;
const raw = Buffer.alloc((ow * 3 + 1) * oh);
for (let y = 0; y < oh; y++) {
  raw[y * (ow * 3 + 1)] = 0;
  for (let x = 0; x < ow; x++) {
    const sx = x0 + Math.floor(x / sc), sy = y0 + Math.floor(y / sc);
    const p = off + (bh - 1 - sy) * st + sx * 3;
    const q = y * (ow * 3 + 1) + 1 + x * 3;
    raw[q] = b[p + 2]; raw[q + 1] = b[p + 1]; raw[q + 2] = b[p];
  }
}
const crcT = new Int32Array(256).map((_, n) => { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1; return c; });
const crc = (buf) => { let c = -1; for (const v of buf) c = crcT[(c ^ v) & 0xff] ^ (c >>> 8); return (c ^ -1) >>> 0; };
const chunk = (t, d) => { const l = Buffer.alloc(4); l.writeUInt32BE(d.length); const td = Buffer.concat([Buffer.from(t), d]); const c = Buffer.alloc(4); c.writeUInt32BE(crc(td)); return Buffer.concat([l, td, c]); };
const ihdr = Buffer.alloc(13); ihdr.writeUInt32BE(ow, 0); ihdr.writeUInt32BE(oh, 4); ihdr[8] = 8; ihdr[9] = 2;
fs.writeFileSync(out, Buffer.concat([Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]), chunk('IHDR', ihdr), chunk('IDAT', zlib.deflateSync(raw)), chunk('IEND', Buffer.alloc(0))]));
