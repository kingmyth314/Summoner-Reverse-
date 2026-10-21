// Standalone link of the decompiled source: every unit of src/decomp compiled with its VC6 flags (vc6-flags.txt),
// plus the link support in src/link (DirectX 7 GUIDs, import-library stubs for binkw32.dll and EAX.DLL, the
// temporary libjpeg stub), linked against VC6's libraries into build/link/Sum-rebuilt.exe.
//
//   node tools/link-build.mjs [--config shipped|fixed|experimental] [--recompile] [--force] [--define NAME ...] [--out DIR]
//
// Objects are cached in DIR/obj and rebuilt when the source is newer (or with --recompile; a header change needs
// --recompile). A build with other --define options than the last one (DIR/defines.txt) recompiles everything: a
// unit compiled without SHIPPED_NOCD asks for the CD. --force links even with unresolved symbols (/FORCE:UNRESOLVED), for inventories. --define adds
// a /D to every game unit (for example SHIPPED_NOCD, the no-CD edit the shipped Sum.exe carries, D2600). Prints the
// unresolved and multiply defined symbols. Needs VC6_DIR (the VC98 folder of Visual C++ 6.0 SP5).
import { readFileSync, writeFileSync, mkdirSync, existsSync, statSync, readdirSync, rmSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { join, resolve, dirname, basename } from "node:path";
import { fileURLToPath } from "node:url";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const args = process.argv.slice(2);
const flag = (n) => args.includes(n);
const opt = (n, d) => { const i = args.indexOf(n); return i >= 0 ? args[i + 1] : d; };
// --config: the build configurations of docs/builds.md, each a define set, its own output folder (so switching does
// not recompile the others) and its own exe name. Without --config the build is as before (--define only,
// build/link, Sum-rebuilt.exe). FIXED_BUILD / EXPERIMENTAL_BUILD never reach vc6-compare, vc6-sweep or hybrid-build: the byte
// matching only ever sees the shipped code.
const CONFIGS = {
  shipped: { defines: [], dir: "link", name: "Sum-rebuilt" },
  fixed: { defines: ["FIXED_BUILD", "SHIPPED_NOCD", "REBUILT_OPENGL"], dir: "link-fixed", name: "Sum-fixed" },
  experimental: { defines: ["FIXED_BUILD", "EXPERIMENTAL_BUILD", "SHIPPED_NOCD", "REBUILT_OPENGL"], dir: "link-exp", name: "Sum-exp" },
};
const configName = opt("--config");
if (configName && !CONFIGS[configName]) { console.error(`unknown --config ${configName} (shipped, fixed, experimental)`); process.exit(2); }
const config = CONFIGS[configName ?? "shipped"];
const defines = [...(configName ? config.defines : []), ...args.flatMap((a, i) => (a === "--define" ? [args[i + 1]] : []))]
  .filter((d, i, a) => a.indexOf(d) === i);
const exeName = config.name;
const VC = process.env.VC6_DIR;
if (!VC || !existsSync(join(VC, "BIN", "CL.EXE"))) {
  console.error("VC6_DIR is not set, or has no BIN\\CL.EXE: set it to the VC98 folder of Visual C++ 6.0.");
  process.exit(2);
}
const out = resolve(opt("--out", join(repo, "build", config.dir)));
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
const definesFile = join(out, "defines.txt"), definesNow = defines.slice().sort().join(" ");
const definesChanged = !existsSync(definesFile) || readFileSync(definesFile, "latin1").trim() !== definesNow;
if (definesChanged) console.log(`defines changed (now "${definesNow}"): recompiling every unit`);
const fresh = (obj, source) => existsSync(obj) && !flag("--recompile") && !definesChanged && statSync(obj).mtimeMs >= statSync(source).mtimeMs;
for (const u of units) {
  const cpp = join(src, u), obj = join(objDir, u.replace(/[\/\\]/g, "_").replace(/\.cpp$/, ".obj"));
  objs.push(obj);
  if (fresh(obj, cpp)) continue;
  const f = (flags.get(u) ?? "/O2 /GX /MT").split(/\s+/);
  const text = run("CL.EXE", ["/nologo", "/c", ...f, ...defines.map((d) => `/D${d}`), `/Fo${obj}`, cpp]);
  compiled++;
  if (/error C\d+|\(exit/.test(text)) { failed++; console.log(`compile failed: ${u}\n${text.split("\n").filter((x) => /error/.test(x)).slice(0, 5).join("\n")}`); }
}
// The experimental build's own units (src/decomp/experimental/*.cpp, docs/experimental-build.md): not part of
// Sum.exe, so not in vc6-matches.txt; compiled only with EXPERIMENTAL_BUILD.
if (defines.includes("EXPERIMENTAL_BUILD")) {
  const xdir = join(src, "experimental");
  for (const f of existsSync(xdir) ? readdirSync(xdir).filter((n) => n.endsWith(".cpp")).sort() : []) {
    const cpp = join(xdir, f), obj = join(objDir, "experimental_" + f.replace(/\.cpp$/, ".obj"));
    objs.push(obj);
    if (fresh(obj, cpp)) continue;
    const text = run("CL.EXE", ["/nologo", "/c", "/O2", "/GX", "/MT", ...defines.map((d) => `/D${d}`), `/Fo${obj}`, cpp]);
    compiled++;
    if (/error C\d+|\(exit/.test(text)) { failed++; console.log(`compile failed: experimental/${f}\n${text.split("\n").filter((x) => /error/.test(x)).slice(0, 5).join("\n")}`); }
  }
}
// libjpeg 6b (Sum.exe links it statically): built from the IJG source when it is in third_party/jpeg6b/jpeg-6b
// (jpegsrc.v6b.tar.gz, not in the repository; docs/reference/sources.md), its common and compression objects as
// the IJG's makefile.vc lists them, with jconfig.vc as jconfig.h. Without it the no-op stub is linked.
const jpegSrc = resolve(opt("--jpeg", join(repo, "third_party", "jpeg6b", "jpeg-6b")));
const jpegObjs = ["jcomapi", "jutils", "jerror", "jmemmgr", "jmemnobs", "jcapimin", "jcapistd", "jctrans", "jcparam",
  "jdatadst", "jcinit", "jcmaster", "jcmarker", "jcmainct", "jcprepct", "jccoefct", "jccolor", "jcsample", "jchuff",
  "jcphuff", "jcdctmgr", "jfdctfst", "jfdctflt", "jfdctint"];
const haveJpeg = existsSync(join(jpegSrc, "jcapimin.c"));
const stubs = ["dx7guids.c"];
if (haveJpeg) {
  const jdir = join(out, "jpeg");
  mkdirSync(jdir, { recursive: true });
  writeFileSync(join(jdir, "jconfig.h"), readFileSync(join(jpegSrc, "jconfig.vc")));
  const jenv = { ...env, INCLUDE: `${jdir};${jpegSrc};${join(VC, "INCLUDE")}` };
  for (const n of jpegObjs) {
    const obj = join(jdir, n + ".obj");
    objs.push(obj);
    if (fresh(obj, join(jpegSrc, n + ".c"))) continue;
    try { execFileSync(join(VC, "BIN", "CL.EXE"), ["/nologo", "/c", "/O2", "/MT", `/Fo${obj}`, join(jpegSrc, n + ".c")], { env: jenv, cwd: jdir, encoding: "latin1" }); }
    catch (e) { failed++; console.log(`libjpeg compile failed: ${n}.c\n${e.stdout}`); }
  }
} else {
  stubs.push("jpeg_stub.c");
  console.log("libjpeg 6b source not found: linking the no-op stub (JPEG screenshots write nothing)");
}
// Link support: plain C files; the import-library stubs become DLLs only for their .lib.
const support = join(repo, "src", "link");
for (const c of stubs) {
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
const text = run("LINK.EXE", ["/nologo", `/OUT:${exeName}.exe`, `/MAP:${exeName}.map`, "/SUBSYSTEM:WINDOWS", "/OPT:REF", "/OPT:ICF",
  ...(flag("--force") ? ["/FORCE:UNRESOLVED"] : []), "@objs.rsp", ...libs]);
writeFileSync(join(out, "link.txt"), text);
const names = (re) => [...new Set([...text.matchAll(re)].map((m) => m[1]))].sort();
const unres = names(/unresolved external symbol (?:"[^"]*" \()?([^\s)"]+)/g);
const multi = names(/LNK(?:2005|4006): "?(?:[^"]*" \()?([^\s)"]+)/g);
console.log(`${units.length} units (${compiled} compiled, ${failed} failed); unresolved ${unres.length}; multiply defined ${multi.length}`);
for (const n of unres) console.log(`  unresolved ${n}`);
for (const n of multi) console.log(`  multiple ${n}`);
const exe = join(out, `${exeName}.exe`);

// The address table SummonerFix's agent mode uses in the rebuilt exe (src/summonerfix/rebuilt.c): Sum.exe address,
// size (up to the next sum.symbols entry) and the rebuilt address of every symbol both builds know, joined through
// the names hybrid-build resolved (build/hybrid-all/hybrid-addrs.txt; run a full hybrid build first).
const dump = join(repo, "build", "hybrid-all", "hybrid-addrs.txt");
const mapPath = join(out, `${exeName}.map`);
if (existsSync(dump) && existsSync(mapPath)) {
  const pub = new Map(), stat = new Map();
  let statics = false;
  for (const l of readFileSync(mapPath, "latin1").split(/\r?\n/)) {
    if (/Static symbols/.test(l)) statics = true;
    const m = /^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})\s+(?:f\s+)?(?:i\s+)?(\S+)\s*$/.exec(l);
    if (!m) continue;
    const va = parseInt(m[2], 16), obj = m[3].replace(/^.*:/, "").replace(/\.obj$/i, "");
    if (statics) stat.set(`${m[1]}\t${obj}`, va); else if (!pub.has(m[1])) pub.set(m[1], va);
  }
  // Object stems: link-build names objects <dir>_<stem>; hybrid-build by <stem>.
  const statByStem = new Map();
  for (const [k, va] of stat) { const [n, o] = k.split("\t"); statByStem.set(`${n}\t${o.replace(/^(game|vsdk)_/, "")}`, va); }
  const starts = readFileSync(join(repo, "symbols", "sum.symbols"), "latin1").split(/\r?\n/).map((l) => /^0x([0-9a-f]+)\s/i.exec(l)).filter(Boolean)
    .map((m) => parseInt(m[1], 16)).sort((a, b) => a - b);
  const nextStart = (a) => { let lo = 0, hi = starts.length; while (lo < hi) { const mid = (lo + hi) >> 1; if (starts[mid] <= a) lo = mid + 1; else hi = mid; } return starts[lo] ?? a + 4; };
  const rows = new Map();
  for (const l of readFileSync(dump, "latin1").split(/\r?\n/)) {
    const [name, stem, a] = l.split("\t");
    if (!name) continue;
    const orig = parseInt(a, 16);
    const va = stem ? statByStem.get(`${name}\t${stem}`) : pub.get(name);
    if (va == null || rows.has(orig)) continue;
    rows.set(orig, va);
  }
  // Size: up to the next row (a struct's fields are not rows of their own), at most up to the next sum.symbols
  // entry that starts a different top-level object is not known here, so code rows use sum.symbols.
  const sorted = [...rows].sort((x, y) => x[0] - y[0]);
  const lines = sorted.map(([o, v], i) => { const next = i + 1 < sorted.length ? sorted[i + 1][0] : o + 4; const size = o < 0x57db40 ? nextStart(o) - o : next - o; return `${o.toString(16)} ${size.toString(16)} ${v.toString(16)}`; });
  writeFileSync(join(out, `${exeName}.addr`), lines.join("\n") + "\n");
  console.log(`address table: ${lines.length} symbols (${exeName}.addr)`);
} else console.log("no address table: build/hybrid-all/hybrid-addrs.txt missing (run a full hybrid-build first)");
console.log(existsSync(exe) && !/fatal error|\(exit [^0]/.test(text) ? `-> ${exe}` : `link failed (${join(out, "link.txt")})`);
if (existsSync(exe) && !/fatal error|\(exit [^0]/.test(text)) writeFileSync(definesFile, definesNow + "\n"); // only after a complete build
// <exe name>.fixes: the SummonerFix [Fixes] keys whose bug this build fixes in its source, from the
// "// FIXED_BUILD: <bug> <key>" markers. SummonerFix (rebuilt.c, RebuiltFixInSource) then leaves those patches out:
// the patch site's first bytes can survive the source fix while the code after it moved.
const fixesPath = join(out, `${exeName}.fixes`);
if (defines.includes("FIXED_BUILD")) {
  const keys = [];
  const walk = (d) => { for (const f of readdirSync(d)) { const p = join(d, f); if (statSync(p).isDirectory()) walk(p); else if (/\.(cpp|h)$/.test(f))
    for (const m of readFileSync(p, "latin1").matchAll(/\/\/\s*(FIXED_BUILD|EXPERIMENTAL_BUILD):\s+(\S+)\s+([A-Za-z0-9]+)/g))
      if (m[1] === "FIXED_BUILD" || defines.includes("EXPERIMENTAL_BUILD")) keys.push(`${m[3]} # ${m[2]} ${p.slice(src.length + 1).replace(/\\/g, "/")}`); } };
  walk(src);
  writeFileSync(fixesPath, keys.sort().join("\n") + (keys.length ? "\n" : ""));
  console.log(`source fixes: ${keys.length} ([Fixes] keys in ${exeName}.fixes)`);
} else if (existsSync(fixesPath)) rmSync(fixesPath);
