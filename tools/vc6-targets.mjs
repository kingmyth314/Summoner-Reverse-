// Lists, for every function a strict sweep marked `target` (bytes match, a call / data reference does not), which
// reference is wrong: the symbol, the address (or contents) ours expects and what Sum.exe has.
//
//   node tools/vc6-targets.mjs <sweep list> [--root src/decomp] [--out report.txt]
//
// The sweep list comes from `VC6_STRICT=1 node tools/vc6-sweep.mjs --out list.txt`. VC6_DIR as for vc6-compare.
import { readFileSync, writeFileSync, existsSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { resolve, dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const opt = (n, d) => { const i = args.indexOf(n); return i >= 0 ? args[i + 1] : d; };
const list = args[0];
const root = resolve(opt("--root", join(repo, "src", "decomp")));
const out = opt("--out");
const files = [...new Set(readFileSync(list, "utf8").split(/\r?\n/).filter((l) => l.endsWith(" target")).map((l) => l.split(" ")[0]))];
const env = { ...process.env, DECOMP_INCLUDE: join(root, "include"), VC6_STRICT: "1" };
if (existsSync(join(root, "vc6-flags.txt"))) env.VC6_FLAGS_FILE = join(root, "vc6-flags.txt");
const lines = [];
for (const f of files) {
  let txt;
  try { txt = execFileSync("node", [join(repo, "tools", "vc6-compare.mjs"), join(root, f), "--json"], { cwd: repo, env, maxBuffer: 1 << 28 }).toString(); }
  catch (e) { txt = (e.stdout || "").toString(); }
  const r = JSON.parse(txt)[0] ?? JSON.parse(txt);
  for (const fn of r.functions.filter((x) => x.status === "target"))
    for (const t of fn.targetWarnings) lines.push(`${f} ${fn.addr} ${fn.name} +0x${t.offset.toString(16)} ${t.symbol}: ours ${t.expected}, Sum.exe ${t.found}`);
}
const text = lines.join("\n") + "\n";
if (out) writeFileSync(out, text); else process.stdout.write(text);
console.error(`${files.length} files, ${lines.length} wrong references`);
