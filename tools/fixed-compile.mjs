// Compiles one unit of a src/decomp tree as the fixed build does (its vc6-flags.txt flags plus /DFIXED_BUILD, and
// /DEXPERIMENTAL_BUILD with --experimental) and reports the compiler's errors and warnings. A check for FIXED_BUILD
// changes (docs/builds.md); the byte matching (vc6-compare, vc6-sweep) never sees these defines.
//
//   node tools/fixed-compile.mjs [--root src/decomp] <file under root> [--experimental]
//
// Exit 0 when it compiles. Needs VC6_DIR (the VC98 folder of Visual C++ 6.0 SP5). Writes only to the system temp folder.
import { readFileSync, mkdtempSync, rmSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { tmpdir } from "node:os";
import { join, resolve, dirname, basename } from "node:path";
import { fileURLToPath } from "node:url";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const take = (n, d) => { const i = args.indexOf(n); if (i < 0) return d; const v = args[i + 1]; args.splice(i, 2); return v; };
const root = resolve(take("--root", join(repo, "src", "decomp")));
const experimental = args.includes("--experimental");
const rel = args.find((a) => !a.startsWith("--"));
const VC = process.env.VC6_DIR;
if (!rel || !VC) { console.error("usage: node tools/fixed-compile.mjs [--root DIR] <file> [--experimental]  (VC6_DIR must be set)"); process.exit(2); }
const unit = rel.replace(/\\/g, "/").replace(/^.*?src\/decomp\//, "");
const flags = new Map();
for (const l of readFileSync(join(root, "vc6-flags.txt"), "latin1").split(/\r?\n/)) {
  if (!l || l.startsWith("#")) continue;
  const i = l.indexOf(" ");
  flags.set(l.slice(0, i), l.slice(i + 1).trim());
}
const f = (flags.get(unit) ?? "/O2 /GX /MT").split(/\s+/);
const tmp = mkdtempSync(join(tmpdir(), "fixedc-"));
const env = { ...process.env, INCLUDE: `${join(root, "include")};${join(VC, "INCLUDE")}`, PATH: `${join(VC, "BIN")};${process.env.PATH}` };
let text, ok = true;
try {
  text = execFileSync(join(VC, "BIN", "CL.EXE"), ["/nologo", "/c", "/W3", ...f, "/DFIXED_BUILD", ...(experimental ? ["/DEXPERIMENTAL_BUILD"] : []),
    `/Fo${join(tmp, basename(unit).replace(/\.cpp$/, ".obj"))}`, join(root, unit)], { env, cwd: tmp, encoding: "latin1" });
} catch (e) { text = (e.stdout || "") + (e.stderr || ""); ok = false; }
rmSync(tmp, { recursive: true, force: true });
// Paths are shortened to the tree (notes from this output must not carry anyone's folder names).
const short = (l) => l.split(root + "\\").join("").split(root + "/").join("").split(VC).join("<VC98>");
const lines = text.split(/\r?\n/).filter((l) => /error|warning/i.test(l)).map(short);
console.log(`${unit}: ${ok ? "compiles" : "FAILS"} with FIXED_BUILD${experimental ? " + EXPERIMENTAL_BUILD" : ""}; ${lines.length} diagnostics`);
for (const l of lines.slice(0, 30)) console.log("  " + l.trim());
process.exit(ok ? 0 : 1);
