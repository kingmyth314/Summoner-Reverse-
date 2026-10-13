// List or extract Summoner .vpp packfiles (format from vsdk file_packfile.cpp; see docs/sum-exe/vsdk-packfile.md).
// usage: node tools/vpp.mjs list <file.vpp> [filter]
//        node tools/vpp.mjs extract <file.vpp> [filter] [--out <dir>]   (default out: extracted/<vpp name>/)
// filter is a case-insensitive substring or a glob with * and ?. Extracted game data is git-ignored; never commit it.
import { openSync, readSync, closeSync, mkdirSync, writeFileSync } from "node:fs";
import { basename, join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const SECTOR = 0x800, MAGIC = 0x51890ace, ENTRY = 0x40, NAME_LEN = 0x3c;
const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");

function readAt(fd, pos, len) {
  const b = Buffer.alloc(len);
  readSync(fd, b, 0, len, pos);
  return b;
}

export function readDirectory(path) {
  const fd = openSync(path, "r");
  try {
    const hdr = readAt(fd, 0, 16);
    const magic = hdr.readUInt32LE(0), version = hdr.readUInt32LE(4), count = hdr.readUInt32LE(8), total = hdr.readUInt32LE(12);
    if (magic !== MAGIC || version === 0) throw new Error(`${path}: not a packfile (magic 0x${magic.toString(16)})`);
    const dirSectors = Math.ceil(count / (SECTOR / ENTRY));
    const dir = readAt(fd, SECTOR, dirSectors * SECTOR);
    let sector = 1 + dirSectors; // packfile_assign_offsets: data follows the header and directory sectors
    const files = [];
    for (let i = 0; i < count; i++) {
      const e = dir.subarray(i * ENTRY, (i + 1) * ENTRY);
      const end = e.indexOf(0);
      const name = e.toString("latin1", 0, end < 0 || end > NAME_LEN ? NAME_LEN : end);
      const size = e.readUInt32LE(NAME_LEN);
      files.push({ name, size, offset: sector * SECTOR });
      sector += (size + SECTOR - 1) >>> 11;
    }
    return { version, count, total, files };
  } finally {
    closeSync(fd);
  }
}

function matcher(filter) {
  if (!filter) return () => true;
  if (/[*?]/.test(filter)) {
    const re = new RegExp("^" + filter.replace(/[.+^${}()|[\]\\]/g, "\\$&").replace(/\*/g, ".*").replace(/\?/g, ".") + "$", "i");
    return (n) => re.test(n);
  }
  const f = filter.toLowerCase();
  return (n) => n.toLowerCase().includes(f);
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const args = process.argv.slice(2);
  const outIdx = args.indexOf("--out");
  const outArg = outIdx >= 0 ? args.splice(outIdx, 2)[1] : null;
  const [cmd, vpp, filter] = args;
  if (!["list", "extract"].includes(cmd) || !vpp) {
    console.error("usage: node tools/vpp.mjs list|extract <file.vpp> [filter] [--out <dir>]");
    process.exit(2);
  }
  const { version, count, total, files } = readDirectory(vpp);
  const want = files.filter((f) => matcher(filter)(f.name));
  if (cmd === "list") {
    console.log(`${basename(vpp)}: version ${version}, ${count} files, ${total} bytes`);
    for (const f of want) console.log(`${String(f.size).padStart(10)}  ${f.name}`);
    console.log(`${want.length} listed`);
  } else {
    const out = outArg ?? join(repoRoot, "extracted", basename(vpp, ".vpp"));
    mkdirSync(out, { recursive: true });
    const fd = openSync(vpp, "r");
    for (const f of want) writeFileSync(join(out, basename(f.name)), readAt(fd, f.offset, f.size));
    closeSync(fd);
    console.log(`extracted ${want.length} files to ${out}`);
  }
}
