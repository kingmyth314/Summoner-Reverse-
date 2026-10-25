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
import { execFileSync, spawnSync } from "node:child_process";
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
// --compiler clang (docs/future/modern-port.md, rung L1): the same units, defines and libraries, compiled by clang in
// its MSVC mode (target i686-pc-windows-msvc, _MSC_VER 1200, VC6's headers) and linked by lld in its MSVC mode
// against VC6's libraries, so the exe keeps the MSVC name mangling the address table needs. CLANG_DIR is the folder
// with clang++.exe and ld.lld.exe. Output: build/<config dir>-clang, <exe name>-clang.exe.
const useClang = opt("--compiler", "vc6") === "clang";
// MODERN_BUILD (rung L2): the plain C++ form where the matching source keeps a VC6-only device or the original's
// undefined behaviour (docs/future/modern-port.md). Only the clang build defines it.
if (useClang && !defines.includes("MODERN_BUILD")) defines.push("MODERN_BUILD");
const CLANG = process.env.CLANG_DIR;
if (useClang && (!CLANG || !existsSync(join(CLANG, "clang++.exe")))) {
  console.error("--compiler clang needs CLANG_DIR: the folder with clang++.exe and ld.lld.exe (LLVM 16 or later).");
  process.exit(2);
}
// --arch x64 (rung L3, clang only): the x86_64 build, with llvm-mingw's 64-bit headers, C runtime and import
// libraries instead of VC6's (VC6 has none for 64-bit). OpenGL only: Direct3D 7, Glide, EAX and Bink have no 64-bit
// DLLs (src/link/x64_stubs.c answers their calls with failure). No address table: SummonerFix is 32-bit, so a 64-bit
// exe runs without it.
const x64 = opt("--arch", "x86") === "x64";
if (x64 && !useClang) { console.error("--arch x64 needs --compiler clang"); process.exit(2); }
const exeName = config.name + (useClang ? "-clang" : "") + (x64 ? "-x64" : "");
const VC = process.env.VC6_DIR;
if (!VC || !existsSync(join(VC, "BIN", "CL.EXE"))) {
  console.error("VC6_DIR is not set, or has no BIN\\CL.EXE: set it to the VC98 folder of Visual C++ 6.0.");
  process.exit(2);
}
const out = resolve(opt("--out", join(repo, "build", config.dir + (useClang ? "-clang" : "") + (x64 ? "-x64" : ""))));
const objDir = join(out, "obj");
mkdirSync(objDir, { recursive: true });
const src = join(repo, "src", "decomp");
const env = { ...process.env, INCLUDE: `${process.env.DECOMP_INCLUDE || join(src, "include")};${join(VC, "INCLUDE")}`, LIB: join(VC, "LIB"), PATH: `${join(VC, "BIN")};${process.env.PATH}` };
const run = (exe, argv, cwd = out) => {
  try { return execFileSync(join(VC, "BIN", exe), argv, { env, cwd, encoding: "latin1", maxBuffer: 1 << 28 }); }
  catch (e) { return (e.stdout || "") + (e.stderr || "") + `\n(exit ${e.status})`; }
};

// clang: x87 maths as VC6 (no SSE), no strict aliasing (the decompiled code reads memory through other types), no C++
// exceptions (the game throws none; clang's MSVC exception tables need a newer runtime than VC6's), SEH kept. Two
// VC6 extensions the matching source relies on are accepted: the address of a temporary (D15902), a narrowing case
// value (gr_font.cpp). A function that falls off its end is an error: the two the matching source has (D15400,
// D21001) return their VC6 value under __clang__.
// Warnings: -Wall -Wextra without the classes that only report the decompiled style (unused names, signed/unsigned
// compares, the VC6 headers' upper-case file names). Each unit's warnings go to <obj>.warn and the build prints the
// total; rung L2 is done when it is 0.
const clangFlags = [...(x64 ? ["--target=x86_64-w64-mingw32", "-fms-extensions", "-g"]
  : ["--target=i686-pc-windows-msvc", "-fms-extensions", "-fms-compatibility", "-fms-compatibility-version=12.0", "-march=pentiumpro", "-mfpmath=387"]),
  "-fno-strict-aliasing", "-fno-exceptions", "-fno-cxx-exceptions", "-fno-sized-deallocation", "-O2",
  "-Wall", "-Wextra", "-Wno-unused-variable", "-Wno-unused-parameter", "-Wno-unused-but-set-variable", "-Wno-unused-function",
  "-Wno-unknown-pragmas", "-Wno-missing-field-initializers", "-Wno-sign-compare", "-Wno-nonportable-include-path",
  "-Wno-error=address-of-temporary", "-Wno-c++11-narrowing", "-Werror=return-type", "-D_MT", "-D_WIN32", "-DWIN32",
  ...(x64 ? [] : ["-isystem", join(VC, "INCLUDE")])];
const runClang = (cpp, obj, extraDefs, includes) => {
  const r = spawnSync(join(CLANG, cpp.endsWith(".c") ? "clang.exe" : "clang++.exe"),
    [...clangFlags, ...(cpp.endsWith(".c") ? [] : ["-std=c++14"]), ...extraDefs.map((d) => `-D${d}`), ...includes.flatMap((i) => ["-I", i]), "-c", cpp, "-o", obj],
    { cwd: out, encoding: "latin1", maxBuffer: 1 << 28 });
  const text = (r.stdout || "") + (r.stderr || "");
  writeFileSync(obj + ".warn", text.split(/\r?\n/).filter((l) => /warning:/.test(l)).join("\n"));
  return r.status === 0 ? "" : text + `\n(exit ${r.status})`;
};
const decompIncludes = () => [process.env.DECOMP_INCLUDE || join(src, "include"), join(process.env.DECOMP_INCLUDE || join(src, "include"), "vsdk")];
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
  const text = useClang ? runClang(cpp, obj, defines, decompIncludes())
    : run("CL.EXE", ["/nologo", "/c", ...f, ...defines.map((d) => `/D${d}`), `/Fo${obj}`, cpp]);
  compiled++;
  if (/error C\d+|error:|\(exit/.test(text)) { failed++; console.log(`compile failed: ${u}\n${text.split("\n").filter((x) => /error/.test(x)).slice(0, 5).join("\n")}`); }
}
// The experimental build's own units (src/decomp/experimental/*.cpp, docs/experimental-build.md): not part of
// Sum.exe, so not in vc6-matches.txt; compiled only with EXPERIMENTAL_BUILD.
if (defines.includes("EXPERIMENTAL_BUILD")) {
  const xdir = join(src, "experimental");
  for (const f of existsSync(xdir) ? readdirSync(xdir).filter((n) => n.endsWith(".cpp")).sort() : []) {
    const cpp = join(xdir, f), obj = join(objDir, "experimental_" + f.replace(/\.cpp$/, ".obj"));
    objs.push(obj);
    if (fresh(obj, cpp)) continue;
    const text = useClang ? runClang(cpp, obj, defines, decompIncludes())
      : run("CL.EXE", ["/nologo", "/c", "/O2", "/GX", "/MT", ...defines.map((d) => `/D${d}`), `/Fo${obj}`, cpp]);
    compiled++;
    if (/error C\d+|error:|\(exit/.test(text)) { failed++; console.log(`compile failed: experimental/${f}\n${text.split("\n").filter((x) => /error/.test(x)).slice(0, 5).join("\n")}`); }
  }
}
// The in-source host (src/decomp/modern/*.cpp, docs/future/modern-port.md rung L3b): what SummonerFix gives a
// rebuilt exe, for when it is not there. Not part of Sum.exe; compiled only into the clang builds (MODERN_BUILD).
if (defines.includes("MODERN_BUILD")) {
  const mdir = join(src, "modern");
  for (const f of existsSync(mdir) ? readdirSync(mdir).filter((n) => n.endsWith(".cpp")).sort() : []) {
    const cpp = join(mdir, f), obj = join(objDir, "modern_" + f.replace(/\.cpp$/, ".obj"));
    objs.push(obj);
    if (fresh(obj, cpp)) continue;
    const text = runClang(cpp, obj, defines, decompIncludes());
    compiled++;
    if (/error:|\(exit/.test(text)) { failed++; console.log(`compile failed: modern/${f}\n${text.split("\n").filter((x) => /error/.test(x)).slice(0, 5).join("\n")}`); }
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
const stubs = x64 ? ["x64_stubs.c"] : ["dx7guids.c"];   // x64: llvm-mingw's libdxguid has the GUIDs
if (haveJpeg) {
  const jdir = join(out, "jpeg");
  mkdirSync(jdir, { recursive: true });
  writeFileSync(join(jdir, "jconfig.h"), readFileSync(join(jpegSrc, "jconfig.vc")));
  const jenv = { ...env, INCLUDE: `${jdir};${jpegSrc};${join(VC, "INCLUDE")}` };
  for (const n of jpegObjs) {
    const obj = join(jdir, n + ".obj");
    objs.push(obj);
    if (fresh(obj, join(jpegSrc, n + ".c"))) continue;
    if (x64) { const t = runClang(join(jpegSrc, n + ".c"), obj, [], [jdir, jpegSrc]); if (t) { failed++; console.log(`libjpeg compile failed: ${n}.c\n${t}`); } continue; }
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
  if (fresh(obj, join(support, c))) continue;
  if (x64) { const t = runClang(join(support, c), obj, [], []); if (t) { failed++; console.log(`compile failed: ${c}\n${t}`); } }
  else run("CL.EXE", ["/nologo", "/c", "/O2", "/MT", `/Fo${obj}`, join(support, c)]);
}
if (useClang) {
  const obj = join(objDir, "clang_support.obj"), cpp = join(support, "clang_support.cpp");
  objs.push(obj);
  if (!fresh(obj, cpp)) { const t = runClang(cpp, obj, [], []); if (t) { failed++; console.log(`compile failed: clang_support.cpp\n${t}`); } }
}
const stubDir = join(out, "stubs");
mkdirSync(stubDir, { recursive: true });
if (x64) {
  // nothing: the Bink and EAX import libraries are VC6-built 32-bit stubs; x64_stubs.c stands in for both
} else if (!existsSync(join(stubDir, "binkw32.lib")) || flag("--recompile"))
  run("CL.EXE", ["/nologo", "/LD", join(support, "binkw32.c"), "/link", "/NOENTRY", "/NODEFAULTLIB", "/OUT:binkw32.dll"], stubDir);
if (!x64 && (!existsSync(join(stubDir, "EAX.lib")) || flag("--recompile"))) {
  // CL passes /link options on unquoted, so the .def goes next to the stub under a relative name.
  writeFileSync(join(stubDir, "eax.def"), readFileSync(join(support, "eax.def")));
  run("CL.EXE", ["/nologo", "/LD", join(support, "eax.c"), "/link", "/NOENTRY", "/NODEFAULTLIB", "/DEF:eax.def", "/OUT:EAX.DLL"], stubDir);
}

writeFileSync(join(out, "objs.rsp"), objs.map((o) => `"${o}"`).join("\r\n") + "\r\n");
const libs = ["kernel32.lib", "user32.lib", "gdi32.lib", "winmm.lib", "advapi32.lib", "shell32.lib", "ole32.lib", "wsock32.lib",
  "ddraw.lib", "dsound.lib", "dinput.lib", "dxguid.lib", "opengl32.lib", "glu32.lib", "comdlg32.lib", "version.lib", "rasapi32.lib",
  "wininet.lib", "msacm32.lib", join(stubDir, "binkw32.lib"), join(stubDir, "EAX.lib")];
const linkArgs = ["/nologo", `/OUT:${exeName}.exe`, `/MAP:${exeName}.map`, "/SUBSYSTEM:WINDOWS", "/OPT:REF", "/OPT:ICF",
  ...(flag("--force") ? ["/FORCE:UNRESOLVED"] : []), "@objs.rsp", ...libs];
let text;
if (x64) {
  // The clang driver links with lld in its MinGW mode against llvm-mingw's 64-bit runtime and import libraries.
  const mingwLibs = ["user32", "gdi32", "winmm", "advapi32", "shell32", "ole32", "wsock32", "ddraw", "dsound", "dinput",
    "dxguid", "opengl32", "glu32", "comdlg32", "version", "rasapi32", "wininet", "msacm32"].map((l) => `-l${l}`);
  writeFileSync(join(out, "objs-x64.rsp"), objs.map((o) => `"${o.replace(/\\/g, "/")}"`).join("\n") + "\n");
  const r = spawnSync(join(CLANG, "clang++.exe"), ["--target=x86_64-w64-mingw32", "-mwindows", "-static", "-o", `${exeName}.exe`,
    "@objs-x64.rsp", ...mingwLibs, `-Wl,-Map=${exeName}.map`, "-Wl,--image-base,0x400000",
    ...(flag("--force") ? ["-Wl,--unresolved-symbols=ignore-all"] : [])],
    { cwd: out, encoding: "latin1", maxBuffer: 1 << 28 });
  text = (r.stdout || "") + (r.stderr || "") + (r.status ? `\n(exit ${r.status})` : "");
  // Not large-address aware (and no high-entropy ASLR): Windows then keeps the exe, its heap, stacks and every later
  // allocation below 2 GB, so the 4-byte PTR32 fields of the relocated file formats (vsdk.h) hold any address the
  // game makes. lld's MinGW driver has no switch for it, so the PE header's flags are cleared here.
  const exePath = join(out, `${exeName}.exe`);
  if (!r.status && existsSync(exePath)) {
    const pe = readFileSync(exePath), at = pe.readUInt32LE(0x3c);
    pe.writeUInt16LE(pe.readUInt16LE(at + 0x16) & ~0x20, at + 0x16);   // IMAGE_FILE_LARGE_ADDRESS_AWARE
    pe.writeUInt16LE(pe.readUInt16LE(at + 24 + 70) & ~0x20, at + 24 + 70);   // IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA
    writeFileSync(exePath, pe);
  }
} else if (useClang) {
  // lld in MSVC mode, VC6's libraries; /SAFESEH:NO because VC6's objects carry no SafeSEH tables.
  try { text = execFileSync(join(CLANG, "ld.lld.exe"), ["-flavor", "link", ...linkArgs, `/LIBPATH:${join(VC, "LIB")}`, "/SAFESEH:NO", "/ENTRY:WinMainCRTStartup",
    // lld loads library members in reference order: without this, LIBCMT's console startup (crt0.obj) comes in first
    // for __amsg_exit and then clashes with the WinMain one (wincrt0.obj), which VC6's linker resolves first.
    "/INCLUDE:_WinMainCRTStartup",
    // lld turns on ASLR by default; the exe must load at 0x400000 as VC6's does (the address table, SummonerFix).
    "/DYNAMICBASE:NO", "/FIXED",
    // VC6 kept string literals writable, and the original wrote into one (localize_set_string rewrote "NULL Format
    // String"; MODERN_BUILD passes a copy since rung L2). --writable-literals merges .rdata into .data as before,
    // for a writer not found yet; without it a write into a literal faults where it happens.
    ...(flag("--writable-literals") ? ["/MERGE:.rdata=.data"] : [])],
    { cwd: out, encoding: "latin1", maxBuffer: 1 << 28, stdio: ["ignore", "pipe", "pipe"] }); }
  catch (e) { text = (e.stdout || "") + (e.stderr || "") + `\n(exit ${e.status})`; }
} else text = run("LINK.EXE", linkArgs);
writeFileSync(join(out, "link.txt"), text);
const names = (re) => [...new Set([...text.matchAll(re)].map((m) => m[1]))].sort();
const unres = useClang ? names(/undefined symbol: ([^\r\n]+)/g) : names(/unresolved external symbol (?:"[^"]*" \()?([^\s)"]+)/g);
const multi = useClang ? names(/duplicate symbol: ([^\r\n]+)/g) : names(/LNK(?:2005|4006): "?(?:[^"]*" \()?([^\s)"]+)/g);
console.log(`${units.length} units (${compiled} compiled, ${failed} failed); unresolved ${unres.length}; multiply defined ${multi.length}`);
if (useClang) {
  // Every object's warnings (a header's warning once), into warnings.txt.
  const warns = [...new Set(objs.filter((o) => existsSync(o + ".warn")).flatMap((o) => readFileSync(o + ".warn", "latin1").split("\n").filter(Boolean)))].sort();
  writeFileSync(join(out, "warnings.txt"), warns.join("\n") + "\n");
  console.log(`clang warnings: ${warns.length} (warnings.txt)`);
}
for (const n of unres) console.log(`  unresolved ${n}`);
for (const n of multi) console.log(`  multiple ${n}`);
const exe = join(out, `${exeName}.exe`);

// The address table SummonerFix's agent mode uses in the rebuilt exe (src/summonerfix/rebuilt.c): Sum.exe address,
// size (up to the next sum.symbols entry) and the rebuilt address of every symbol both builds know, joined through
// the names hybrid-build resolved (build/hybrid-all/hybrid-addrs.txt; run a full hybrid build first).
const dump = join(repo, "build", "hybrid-all", "hybrid-addrs.txt");
const mapPath = join(out, `${exeName}.map`);
if (x64 && existsSync(dump) && existsSync(mapPath)) {
  // The x64 exe's table, for the in-source agent mode (src/decomp/modern/; peek, poke and the call commands take
  // Sum.exe addresses). lld's MinGW map lists demangled names ("quicksave()", "vfx_base::relocate_node(char*,
  // char*)", plain data names) at offsets from the image base; hybrid-build's names are MSVC-mangled. Both are reduced
  // to the qualified name without the parameter list, and only names that occur once on each side are joined.
  // Addresses are written against the preferred base 0x400000; the agent adds the actual load offset.
  const byQual = new Map();
  for (const l of readFileSync(mapPath, "latin1").split(/\r?\n/)) {
    const m = /^([0-9a-f]{8}) 00000000 +0 +(.+)$/.exec(l);
    if (!m || m[2].startsWith(".") || m[2].includes("/")) continue;
    const q = m[2].replace(/\(.*$/, "").trim(), va = 0x400000 + parseInt(m[1], 16);
    byQual.set(q, byQual.has(q) && byQual.get(q) !== va ? null : va);
  }
  // ?name@Scope@Outer@@... -> Outer::Scope::name; ??0X@@ / ??1X@@ -> X::X / X::~X; other special names are skipped.
  const msvcQual = (n) => {
    let special = null, s = n;
    if (s.startsWith("??0")) { special = "ctor"; s = s.slice(3); } else if (s.startsWith("??1")) { special = "dtor"; s = s.slice(3); }
    else if (s.startsWith("?") && !s.startsWith("??") && !s.startsWith("?$")) s = s.slice(1); else return null;
    const end = s.indexOf("@@");
    if (end < 0 || s.slice(0, end).includes("?$")) return null;
    const parts = s.slice(0, end).split("@").reverse();
    if (special) { const cls = parts[parts.length - 1]; parts.push(special === "ctor" ? cls : `~${cls}`); }
    return parts.join("::");
  };
  const starts = readFileSync(join(repo, "symbols", "sum.symbols"), "latin1").split(/\r?\n/).map((l) => /^0x([0-9a-f]+)\s/i.exec(l)).filter(Boolean)
    .map((m) => parseInt(m[1], 16)).sort((a, b) => a - b);
  const nextStart = (a) => { let lo = 0, hi = starts.length; while (lo < hi) { const mid = (lo + hi) >> 1; if (starts[mid] <= a) lo = mid + 1; else hi = mid; } return starts[lo] ?? a + 4; };
  const rows = new Map();
  for (const l of readFileSync(dump, "latin1").split(/\r?\n/)) {
    const [name, stem, a] = l.split("\t");
    if (!name || stem) continue;   // file statics are not in the map
    const q = /^[A-Za-z_]/.test(name) ? name : msvcQual(name);   // C names are unmangled on both sides
    const va = q ? byQual.get(q) : null, orig = parseInt(a, 16);
    if (va == null || rows.has(orig)) continue;
    rows.set(orig, va);
  }
  const sorted = [...rows].sort((x, y) => x[0] - y[0]);
  const lines = sorted.map(([o, v], i) => { const next = i + 1 < sorted.length ? sorted[i + 1][0] : o + 4; const size = o < 0x57db40 ? nextStart(o) - o : next - o; return `${o.toString(16)} ${size.toString(16)} ${v.toString(16)}`; });
  writeFileSync(join(out, `${exeName}.addr`), lines.join("\n") + "\n");
  console.log(`address table: ${lines.length} symbols (${exeName}.addr, against base 0x400000)`);
} else if (x64) {
  console.log("no address table: build/hybrid-all/hybrid-addrs.txt missing (run a full hybrid-build first)");
} else if (existsSync(dump) && existsSync(mapPath)) {
  const pub = new Map(), stat = new Map();
  let statics = false;
  for (const l of readFileSync(mapPath, "latin1").split(/\r?\n/)) {
    if (/Static symbols/.test(l)) statics = true;
    // VC6's LINK writes 8-digit addresses and an "f" for functions; lld's /MAP 16 digits and no "f".
    const m = /^\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8,16})\s+(?:f\s+)?(?:i\s+)?(\S+)\s*$/.exec(l);
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
  // A C++ name whose parameter list changed since the hybrid build wrote the dump (e.g. get_hit_by_spell's last
  // parameter became int, D25504) no longer matches exactly; it is matched by its qualified name without the
  // signature ("?name@Class@@"), when exactly one public symbol has it.
  const bySig = new Map();
  for (const n of pub.keys()) { const m = /^(\?[^@]+@(?:[^@]+@)*@)/.exec(n); if (m) bySig.set(m[1], bySig.has(m[1]) ? null : n); }
  const relaxed = (name) => { const m = /^(\?[^@]+@(?:[^@]+@)*@)/.exec(name); const n = m && bySig.get(m[1]); return n ? pub.get(n) : undefined; };
  const rows = new Map();
  for (const l of readFileSync(dump, "latin1").split(/\r?\n/)) {
    const [name, stem, a] = l.split("\t");
    if (!name) continue;
    const orig = parseInt(a, 16);
    const va = stem ? statByStem.get(`${name}\t${stem}`) : pub.get(name) ?? relaxed(name);
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
