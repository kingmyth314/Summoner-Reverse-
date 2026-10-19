// Standalone link of the decompiled source: every unit of src/decomp compiled with its VC6 flags (vc6-flags.txt),
// plus the link support in src/link (DirectX 7 GUIDs, import-library stubs for binkw32.dll and EAX.DLL, the
// temporary libjpeg stub), linked against VC6's libraries into build/link/Sum-rebuilt.exe.
//
//   node tools/link-build.mjs [--recompile] [--force] [--define NAME ...] [--out DIR]
//
// Objects are cached in DIR/obj and rebuilt when the source is newer (or with --recompile; a header change needs
// --recompile). --force links even with unresolved symbols (/FORCE:UNRESOLVED), for inventories. --define adds
// a /D to every game unit (for example SHIPPED_NOCD, the no-CD edit the shipped Sum.exe carries, D2600). Prints the
// unresolved and multiply defined symbols. Needs VC6_DIR (the VC98 folder of Visual C++ 6.0 SP5).
import { readFileSync, writeFileSync, mkdirSync, existsSync, statSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { join, resolve, dirname, basename } from "node:path";
import { fileURLToPath } from "node:url";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const flag = (n) => args.includes(n);
const opt = (n, d) => { const i = args.indexOf(n); return i >= 0 ? args[i + 1] : d; };
const defines = args.flatMap((a, i) => (a === "--define" ? [args[i + 1]] : []));
const VC = process.env.VC6_DIR;
if (!VC || !existsSync(join(VC, "BIN", "CL.EXE"))) {
  console.error("VC6_DIR is not set, or has no BIN\\CL.EXE: set it to the VC98 folder of Visual C++ 6.0.");
  process.exit(2);
}
const out = resolve(opt("--out", join(repo, "build", "link")));
const objDir = join(out, "obj");
mkdirSync(objDir, { recursive: true });
const src = join(repo, "src", "decomp");
const env = { ...process.env, INCLUDE: `${process.env.DECOMP_INCLUDE || join(src, "include")};${join(VC, "INCLUDE")}`, LIB: join(VC, "LIB"), PATH: `${join(VC, "BIN")};${process.env.PATH}` };
const run = (exe, argv, cwd = out) => {
  try { return execFileSync(join(VC, "BIN", exe), argv, { env, cwd, encoding: "latin1", maxBuffer: 1 << 28 }); }
  catch (e) { return (e.stdout || "") + (e.stderr || "") + `\n(exit ${e.status})`; }
};

const flags = new Map();
for (const l of readFileSync(join(src, "vc6-flags.txt"), "latin1").split(/\r?\n/)) {
  if (!l || l.startsWith("#")) continue;
  const i = l.indexOf(" ");
  flags.set(l.slice(0, i), l.slice(i + 1).trim());
}
const units = [...new Set(readFileSync(join(src, "vc6-matches.txt"), "latin1").split(/\r?\n/).filter(Boolean).map((l) => l.split(" ")[0]))];
const objs = [];
let compiled = 0, failed = 0;
const fresh = (obj, source) => existsSync(obj) && !flag("--recompile") && statSync(obj).mtimeMs >= statSync(source).mtimeMs;
for (const u of units) {
  const cpp = join(src, u), obj = join(objDir, u.replace(/[\/\\]/g, "_").replace(/\.cpp$/, ".obj"));
  objs.push(obj);
  if (fresh(obj, cpp)) continue;
  const f = (flags.get(u) ?? "/O2 /GX /MT").split(/\s+/);
  const text = run("CL.EXE", ["/nologo", "/c", ...f, ...defines.map((d) => `/D${d}`), `/Fo${obj}`, cpp]);
  compiled++;
  if (/error C\d+|\(exit/.test(text)) { failed++; console.log(`compile failed: ${u}\n${text.split("\n").filter((x) => /error/.test(x)).slice(0, 5).join("\n")}`); }
}
// Link support: plain C files; the import-library stubs become DLLs only for their .lib.
const support = join(repo, "src", "link");
for (const c of ["dx7guids.c", "jpeg_stub.c"]) {
  const obj = join(objDir, c.replace(/\.c$/, ".obj"));
  objs.push(obj);
  if (!fresh(obj, join(support, c))) run("CL.EXE", ["/nologo", "/c", "/O2", "/MT", `/Fo${obj}`, join(support, c)]);
}
const stubDir = join(out, "stubs");
mkdirSync(stubDir, { recursive: true });
if (!existsSync(join(stubDir, "binkw32.lib")) || flag("--recompile"))
  run("CL.EXE", ["/nologo", "/LD", join(support, "binkw32.c"), "/link", "/NOENTRY", "/NODEFAULTLIB", "/OUT:binkw32.dll"], stubDir);
if (!existsSync(join(stubDir, "EAX.lib")) || flag("--recompile")) {
  // CL passes /link options on unquoted, so the .def goes next to the stub under a relative name.
  writeFileSync(join(stubDir, "eax.def"), readFileSync(join(support, "eax.def")));
  run("CL.EXE", ["/nologo", "/LD", join(support, "eax.c"), "/link", "/NOENTRY", "/NODEFAULTLIB", "/DEF:eax.def", "/OUT:EAX.DLL"], stubDir);
}

writeFileSync(join(out, "objs.rsp"), objs.map((o) => `"${o}"`).join("\r\n") + "\r\n");
const libs = ["kernel32.lib", "user32.lib", "gdi32.lib", "winmm.lib", "advapi32.lib", "shell32.lib", "ole32.lib", "wsock32.lib",
  "ddraw.lib", "dsound.lib", "dinput.lib", "dxguid.lib", "opengl32.lib", "glu32.lib", "comdlg32.lib", "version.lib", "rasapi32.lib",
  "wininet.lib", "msacm32.lib", join(stubDir, "binkw32.lib"), join(stubDir, "EAX.lib")];
const text = run("LINK.EXE", ["/nologo", "/OUT:Sum-rebuilt.exe", "/MAP:Sum-rebuilt.map", "/SUBSYSTEM:WINDOWS", "/OPT:REF", "/OPT:ICF",
  ...(flag("--force") ? ["/FORCE:UNRESOLVED"] : []), "@objs.rsp", ...libs]);
writeFileSync(join(out, "link.txt"), text);
const names = (re) => [...new Set([...text.matchAll(re)].map((m) => m[1]))].sort();
const unres = names(/unresolved external symbol (?:"[^"]*" \()?([^\s)"]+)/g);
const multi = names(/LNK(?:2005|4006): "?(?:[^"]*" \()?([^\s)"]+)/g);
console.log(`${units.length} units (${compiled} compiled, ${failed} failed); unresolved ${unres.length}; multiply defined ${multi.length}`);
for (const n of unres) console.log(`  unresolved ${n}`);
for (const n of multi) console.log(`  multiple ${n}`);
const exe = join(out, "Sum-rebuilt.exe");
console.log(existsSync(exe) && !/fatal error|\(exit [^0]/.test(text) ? `-> ${exe}` : "link failed (build/link/link.txt)");
