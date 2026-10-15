// Propose translation-unit (source file) boundaries from data locality, call locality and code style.
// usage: node tu-split2.mjs [--lo 0x401000] [--hi 0x56a000] [--window 60] [--out file] [--score]
//          [--blind] [--hide file.cpp] [--loo] [--eval] [--repo path] [--symbols file] [--truth file]
//          [--gamma 10] [--cut -0.5] [--init 1] [--quiet 2] [--calls 1] [--inv 1] [--init-use 300]
//          [--no-style] [--style-bonus 5] [--no-glue-init] [--no-groups] [--no-endref]
//          experiments that measured worse: [--local-calls] [--glue-init <edge weight>]
// (--gap is accepted for compatibility with tools/tu-split.mjs and ignored.)
//
// MSVC 6 lays out each .cpp file's functions, its .rdata, its .data (initialised globals and string literals) and its
// .bss as contiguous blocks, all in the same link order. This version does not rely on module tags to find the
// boundaries; tags only constrain the result (and --blind drops them, to measure the method).
//  1. Functions are nodes of a graph. Edges join
//     - consecutive users of a "private" global (every user within --window functions; localize_strings never counts).
//       `cmp reg, imm` against a data address is the end bound of an array (it equals the next object's address),
//       so it counts as the array's last byte, not as a use of the next object;
//     - a function and the functions it calls, inside the window (--calls);
//     - two functions whose owned data are out of order (--inv): j > i uses a datum below one of i's. A datum used by
//       one function only is owned by it when its address rank is near the data used around it (not a foreign table).
//       Across files the data are in link order, so an inversion means one file;
//     - a static initialiser and the first later function that uses the object it builds (--init-use, up to that
//       many functions away: a file's initialisers sit in front of its code).
//  2. Static-init code: the 652 entries of the table at 0x588004 (in address order), their jmp targets, and the
//     helpers right after them (atexit registration, destructor; a console command's handler) form init groups.
//     No cut inside a run of init-group functions (--no-glue-init), and a cut gets a bonus (--init) where such a run
//     starts after at least --quiet other functions: every file with dynamic initialisers has them in front.
//  3. Code style: most files are compiled /O2 (no frame pointer, 0x90 padding). A few (living_entity.cpp, objects.cpp
//     and the cutscene code after gameplay.cpp) are compiled without optimisation: every function opens an ebp frame
//     and functions are padded with 0xcc. Such a run is one file: no cut inside, a bonus at both ends (--style-bonus).
//  4. Dynamic programming picks the cuts maximising sum over units of (internal edge weight - gamma * deg^2 / 2m)
//     plus the cut bonuses (--cut is the base bonus of any cut, negative = a cost per file). The first term keeps
//     connected code together, the second (the modularity null model) splits big weakly connected units.
//  5. Unless --blind: no unit may hold two different .cpp tags, and no cut is made between two functions with the
//     same .cpp tag (tags were reviewed; this keeps a file whole). Tags win over the glue of 2 and 3. --hide f
//     ignores one file's tags.
// --score prints boundary recall/precision against the .cpp tags (blind) and the tag-constrained result.
// --loo hides each tagged file in turn and checks that its two boundaries are still found.
// --eval prints the measurements of RESULTS.md (each signal on its own and combined, split-half check).
// --symbols reads another symbol file (default symbols/sum.symbols); --truth scores against another file's tags
// (default: the same file). Function addresses must match.
// Each unit line: range, n functions, why it starts (init / style / data), middle half of its .bss, tags, names.
// Needs build/sum.asm (node tools/asm.mjs builds it), symbols/sum.symbols and Summoner/Sum.exe.
import { readFileSync, writeFileSync, existsSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const arg = (k, d) => { const i = process.argv.indexOf(k); return i > 0 ? process.argv[i + 1] : d; };
const flag = (k) => process.argv.includes(k);
const here = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const root = arg("--repo", here);
const LO = parseInt(arg("--lo", "0x401000"), 16), HI = parseInt(arg("--hi", "0x56a000"), 16);
const DEFAULTS = {
  W: +arg("--window", "60"), gamma: +arg("--gamma", "10"), cut: +arg("--cut", "-0.5"),
  init: +arg("--init", "1"), quiet: +arg("--quiet", "2"), groups: !flag("--no-groups"), initUse: +arg("--init-use", "300"),
  glueInit: arg("--glue-init", null) !== null ? +arg("--glue-init") : !flag("--no-glue-init"), // number = soft edge
  calls: +arg("--calls", "1"), localCalls: flag("--local-calls"), inv: +arg("--inv", "1"),
  style: !flag("--no-style"), styleBonus: +arg("--style-bonus", "5"), kinds: "SDRB", invSec: "db",
};
const ENDREF = !flag("--no-endref");
const out = arg("--out", null);
const RDATA = 0x57e4b8, DATA = 0x588000, BSS = 0x5b3000, DATA_HI = 0x2fc0000; // .rdata after the IAT, .data, .bss
const SHARED = (d) => d >= 0x25cd4dc && d < 0x25cd4dc + 4 * 1000; // localize_strings
const isFile = (m) => /\.cpp$/i.test(m);
const hex = (v) => "0x" + v.toString(16);

// ---- inputs ----------------------------------------------------------------------------------------------------
const exe = readFileSync(join(root, "Summoner", "Sum.exe"));
const byte = (va) => exe[va - 0x400000]; // .text, .rdata and .data have file offset = RVA
const fns = [];
const readFuncs = (path) => {
  const r = new Map();
  for (const l of readFileSync(path, "utf8").split(/\r?\n/)) {
    const m = /^0x([0-9a-f]+)\s+(\S+)\s+func\s+(\S+)/.exec(l);
    if (m) { const a = parseInt(m[1], 16); if (a >= LO && a < HI) r.set(a, { a, name: m[2], mod: m[3] }); }
  }
  return r;
};
const symPath = arg("--symbols", join(root, "symbols", "sum.symbols"));
for (const f of readFuncs(symPath).values()) fns.push(f);
fns.sort((x, y) => x.a - y.a);
{ const t = readFuncs(arg("--truth", symPath)); for (const f of fns) f.tmod = t.get(f.a)?.mod ?? "-"; } // tags to score against
const N = fns.length;
const index = new Map(fns.map((f, i) => [f.a, i]));

// string literal test for .data references (ASCII or UTF-16, at least 2 characters, NUL terminated)
const isStr = (va) => {
  if (va < DATA || va >= BSS) return false;
  let n = 0; while (n < 300 && byte(va + n) >= 0x09 && byte(va + n) < 0x7f) n++;
  if (n >= 2 && byte(va + n) === 0) return true;
  n = 0; while (n < 300 && byte(va + 2 * n) >= 0x09 && byte(va + 2 * n) < 0x7f && byte(va + 2 * n + 1) === 0) n++;
  return n >= 2 && !byte(va + 2 * n) && !byte(va + 2 * n + 1);
};

// static initialisers: table entries and the targets of their jmp thunks
for (let a = DATA + 4; ; a += 4) {
  const v = exe.readUInt32LE(a - 0x400000); if (!v) break;
  if (index.has(v)) fns[index.get(v)].init = true;
  if (byte(v) === 0xe9) { const t = v + 5 + exe.readInt32LE(v - 0x400000 + 1); if (index.has(t)) fns[index.get(t)].init = true; }
}

// references per function: S = .data string, D = other .data, R = .rdata, B = .bss; calls = direct callees
for (const f of fns) { f.S = []; f.D = []; f.R = []; f.B = []; f.calls = []; }
{
  let fi = -1;
  for (const l of readFileSync(join(root, "build", "sum.asm"), "utf8").split("\n")) {
    const at = /^\s*([0-9a-f]+):/.exec(l);
    if (!at) continue;
    const a = parseInt(at[1], 16);
    if (a < LO || a >= HI) continue;
    while (fi + 1 < N && fns[fi + 1].a <= a) fi++;
    if (fi < 0) continue;
    const f = fns[fi], t = l.slice(l.indexOf("\t"));
    const cm = /\t(call|jmp)\s+0x([0-9a-f]+)/.exec(t);
    if (cm) { const d = parseInt(cm[2], 16); if (index.has(d) && index.get(d) !== fi) f.calls.push(index.get(d)); continue; }
    const endRef = ENDREF && /\tcmp\s+e[a-z]{2},\s+0x[0-9a-f]+\s*$/.test(t); // array end bound
    for (const m of t.matchAll(/\b0x([0-9a-f]{6,7})\b/g)) {
      const d = parseInt(m[1], 16) - (endRef ? 1 : 0);
      if (SHARED(d)) continue;
      if (d >= RDATA && d < DATA) f.R.push(d);
      else if (d >= DATA && d < BSS) (isStr(d) ? f.S : f.D).push(d);
      else if (d >= BSS && d < DATA_HI) f.B.push(d);
      else if (index.has(d) && index.get(d) !== fi) f.calls.push(index.get(d)); // function pointer
    }
  }
}

// initialiser groups: an initialiser's helpers right after it (the atexit registration and the destructor it
// registers, sinit_X / init_X / register_dtor_X / dtor_X) belong to the same group of static-init code
for (let pass = 0; pass < 2; pass++) fns.forEach((f, i) => {
  if (!f.init && !f.group) return;
  f.group = true;
  for (const j of f.calls) if (j > i && j <= i + 4) fns[j].group = true;
});

// code style: runs of unoptimised functions (ebp frame, 0xcc padding)
const style = (() => {
  const ebp = (i) => byte(fns[i].a) === 0x55 && byte(fns[i].a + 1) === 0x8b && byte(fns[i].a + 2) === 0xec;
  const cc = (i) => i + 1 < N && byte(fns[i + 1].a - 1) === 0xcc;
  const inside = new Uint8Array(N), edge = new Uint8Array(N), runs = [];
  // up to 3 frameless functions (inline COMDATs taken from optimised files) may sit inside a run
  const next = (j) => { for (let k = j + 1; k <= j + 4 && k < N; k++) if (ebp(k)) return k; return -1; };
  for (let i = 0; i < N; ) {
    if (!ebp(i)) { i++; continue; }
    let j = i; for (let k; (k = next(j)) > 0; ) j = k;
    let ncc = 0; for (let k = i; k <= j; k++) if (cc(k)) ncc++;
    // an isolated ebp frame is inline asm or SEH; a file needs a run that is mostly 0xcc padded
    if (j - i + 1 >= 4 && ncc * 2 >= j - i + 1) { runs.push([i, j]); for (let c = i; c < j; c++) inside[c] = 1; if (i > 0) edge[i - 1] = 1; edge[j] = 1; }
    i = j + 1;
  }
  return { inside, edge, runs };
})();

// ---- graph -----------------------------------------------------------------------------------------------------
function buildEdges(o) {
  const W = o.W, E = [];
  const users = new Map(); // datum -> { k: section kind, n: sorted user indices }
  fns.forEach((f, i) => { for (const k of o.kinds) for (const d of f[k]) {
    const u = users.get(d) ?? users.set(d, { k, n: [] }).get(d); if (u.n[u.n.length - 1] !== i) u.n.push(i); } });
  for (const [, u] of users) {
    const n = u.n; if (n.length < 2 || n[n.length - 1] - n[0] > W) continue;
    for (let t = 1; t < n.length; t++) E.push([n[t - 1], n[t], 1]);
  }
  // --local-calls: weight a call edge by the share of the callee's callers that lie inside the window (a public
  // function called from all over the game is weak evidence that a nearby caller is in its file)
  const near = new Uint32Array(N), all = new Uint32Array(N);
  fns.forEach((f, i) => { for (const j of new Set(f.calls)) { all[j]++; if (Math.abs(i - j) <= W) near[j]++; } });
  if (o.calls) fns.forEach((f, i) => { for (const j of new Set(f.calls)) if (Math.abs(i - j) <= W)
    E.push([Math.min(i, j), Math.max(i, j), o.calls * (o.localCalls ? near[j] / all[j] : 1)]); });
  if (o.inv) inversionEdges(users, o, E);
  if (typeof o.glueInit === "number") for (let c = 0; c + 1 < N; c++) if (fns[c].group && fns[c + 1].group) E.push([c, c + 1, o.glueInit]);
  // an initialiser constructs its own file's object: join it to the first later non-initialiser that uses that
  // object, even beyond the window (a long block of initialisers in front of the code)
  if (o.initUse) fns.forEach((f, i) => {
    if (!f.init) return;
    for (const d of [...f.D, ...f.B]) {
      const u = users.get(d); if (!u) continue;
      const j = u.n.find((k) => k > i && !fns[k].group);
      if (j !== undefined && j - i <= o.initUse) E.push([i, j, 1]);
    }
  });
  return E;
}

// j > i, j's owned datum below i's: one file. Section groups: d = .data (strings and globals), b = .bss.
// (.rdata is left out: its float constants are COMDATs placed in the first file that uses them.)
function inversionEdges(users, o, E) {
  const grp = (k) => (k === "S" || k === "D" ? "d" : k === "R" ? "r" : "b"), R = 100;
  for (const g of o.invSec) {
    const all = [...users].filter(([, u]) => grp(u.k) === g).map(([d]) => d).sort((a, b) => a - b);
    const rank = new Map(all.map((d, r) => [d, r]));
    const own = fns.map(() => []), any = fns.map(() => []);
    for (const [d, u] of users) if (grp(u.k) === g) { for (const i of u.n) any[i].push(rank.get(d)); if (u.n.length === 1) own[u.n[0]].push(rank.get(d)); }
    // owned and local: within R ranks of the median rank of everything used by the 21 functions around
    const local = own.map((v, j) => {
      if (!v.length) return v;
      const near = []; for (let i = Math.max(0, j - 10); i <= Math.min(N - 1, j + 10); i++) near.push(...any[i]);
      near.sort((a, b) => a - b); const m = near[near.length >> 1];
      return v.filter((r) => Math.abs(r - m) <= R);
    });
    const mx = local.map((v) => (v.length ? Math.max(...v) : -1));
    for (let j = 0; j < N; j++) {
      if (!local[j].length) continue;
      const lo = Math.min(...local[j]);
      for (let i = Math.max(0, j - o.W); i < j; i++) if (mx[i] > lo) { E.push([i, j, o.inv]); break; } // earliest i
    }
  }
}

// an initialiser run starts after c: fns[c + 1] is an initialiser and none of the `quiet` functions before it is one.
// Console commands come as (thunk, initialiser, handler) triples; a block of them is one run, not one file each.
const initStart = (c, o) => {
  const g = o.groups ? "group" : "init", quiet = o.quiet;
  if (!fns[c + 1]?.[g]) return false;
  for (let i = c; i > c - quiet && i >= 0; i--) if (fns[i][g]) return false;
  return true;
};

// ---- segmentation ------------------------------------------------------------------------------------------------
// tags: null (blind) or { hide }: unit may not mix two .cpp tags; no cut inside a tagged file's span.
function segment(o, tags) {
  const E = buildEdges(o);
  const adj = Array.from({ length: N }, () => []), deg = new Float64Array(N); let m = 0;
  for (const [i, j, w] of E) { adj[i].push([j, w]); deg[i] += w; deg[j] += w; m += w; }
  for (const a of adj) a.sort((x, y) => x[0] - y[0]);
  const pdeg = new Float64Array(N + 1); for (let i = 0; i < N; i++) pdeg[i + 1] = pdeg[i] + deg[i];
  // cut bonus and forbidden cuts (cut c = between fns[c] and fns[c + 1])
  const bonus = new Float64Array(N), forbid = new Uint8Array(N);
  for (let c = 0; c + 1 < N; c++) {
    bonus[c] = o.cut + (initStart(c, o) ? o.init : 0);
    if (o.style) { if (style.edge[c]) bonus[c] += o.styleBonus; if (style.inside[c]) forbid[c] = 1; }
    // one block of static-init code (a style edge wins: code built with other options is another file)
    if (o.glueInit === true && fns[c].group && fns[c + 1].group && !(o.style && style.edge[c])) forbid[c] = 1;
  }
  const tag = fns.map((f) => (tags && isFile(f.mod) && f.mod !== tags.hide ? f.mod : null));
  if (tags) {
    let last = -1;
    for (let j = 0; j < N; j++) if (tag[j]) {
      if (last >= 0 && tag[last] !== tag[j]) { // tags win over the style and init glue: keep one cut possible
        let open = false; for (let c = last; c < j; c++) if (!forbid[c]) open = true;
        if (!open) for (let c = last; c < j; c++) forbid[c] = 0;
      }
      last = j;
    }
    last = -1;
    for (let j = 0; j < N; j++) if (tag[j]) { if (last >= 0 && tag[last] === tag[j]) for (let c = last; c < j; c++) forbid[c] = 1; last = j; }
  }
  const best = new Float64Array(N + 1).fill(-Infinity), from = new Int32Array(N + 1);
  best[0] = 0;
  for (let e = 0; e < N; e++) {
    let inner = 0, seen = null;
    for (let s = e; s >= 0 && e - s < 600; s--) {
      if (s < e) for (const [j, w] of adj[s]) { if (j > e) break; inner += w; } // edges of s inside [s, e]
      if (tag[s]) { if (seen && seen !== tag[s]) break; seen = tag[s]; } // two .cpp tags: no longer unit fits
      if (s > 0 && forbid[s - 1]) continue;
      const d = pdeg[e + 1] - pdeg[s];
      const v = best[s] + inner - o.gamma * d * d / (2 * m) + (s > 0 ? bonus[s - 1] : 0);
      if (v > best[e + 1]) { best[e + 1] = v; from[e + 1] = s; }
    }
  }
  if (best[N] === -Infinity) throw new Error("no segmentation fits the tag constraints (a tagged file longer than 600 functions?)");
  const cuts = []; for (let e = N; e > 0; ) { const s = from[e]; if (s > 0) cuts.push(s - 1); e = s; }
  return cuts.reverse();
}

// ---- scoring against the .cpp tags -----------------------------------------------------------------------------
// A tagged file spans its first..last tagged function. A transition is a pair of consecutive tagged functions with
// different tags; it is found when a cut falls in the untagged gap between them. A cut inside a span is wrong.
// Cuts in gaps count as correct (untagged files may sit there); cuts elsewhere are not judged.
function truth(hide = null) {
  const spans = new Map(), trans = []; let last = -1;
  for (let j = 0; j < N; j++) {
    const m = fns[j].tmod; if (!isFile(m) || m === hide) continue;
    const s = spans.get(m); if (s) s.last = j; else spans.set(m, { first: j, last: j });
    if (last >= 0 && fns[last].tmod !== m) trans.push({ a: fns[last].tmod, b: m, lo: last, hi: j - 1 });
    last = j;
  }
  return { spans, trans };
}
const T = truth();
function score(cuts, only = null) {
  const set = new Uint8Array(N); for (const c of cuts) set[c] = 1;
  const inside = new Uint8Array(N), gap = new Uint8Array(N);
  for (const [f, s] of T.spans) if (!only || only.includes(f)) for (let c = s.first; c < s.last; c++) inside[c] = 1;
  const tr = only ? T.trans.filter((t) => only.includes(t.a) || only.includes(t.b)) : T.trans;
  for (const t of tr) for (let c = t.lo; c <= t.hi; c++) gap[c] = 1;
  const miss = tr.filter((t) => { for (let c = t.lo; c <= t.hi; c++) if (set[c]) return false; return true; });
  let fp = 0, good = 0; const fps = [];
  for (const c of cuts) if (inside[c]) { fp++; fps.push(c); } else if (gap[c]) good++;
  return { tp: tr.length - miss.length, n: tr.length, fp, good, cuts: cuts.length, miss, fps };
}
const pct = (a, b) => (b ? (100 * a / b).toFixed(1) : "-") + "%";
const fmt = (r) => `recall ${r.tp}/${r.n} ${pct(r.tp, r.n)}  precision ${r.good}/${r.good + r.fp} ${pct(r.good, r.good + r.fp)}  fp ${r.fp}  units ${r.cuts + 1}`;
function loo(o, segFn = (hide) => segment(o, { hide })) {
  let tp = 0, n = 0, fp = 0, good = 0; const bad = [];
  for (const f of T.spans.keys()) {
    const r = score(segFn(f), [f]); tp += r.tp; n += r.n; fp += r.fp; good += r.good;
    if (r.tp < r.n || r.fp) bad.push(`${f} ${r.tp}/${r.n}${r.fp ? ` fp ${r.fp}` : ""}`);
  }
  return { tp, n, fp, good, bad };
}
const fmtLoo = (r) => `recall ${r.tp}/${r.n} ${pct(r.tp, r.n)}  precision ${r.good}/${r.good + r.fp} ${pct(r.good, r.good + r.fp)}  fp ${r.fp}`;

// ---- port of tools/tu-split.mjs (for --eval): pieces between uncrossed gaps, joined unless the .bss jumps -------
function original(tags) {
  const W = 60, GAP = 0x40, isThunk = (i) => /_thunk$/.test(fns[i]?.name ?? "");
  const data = fns.map((f) => [...f.R, ...f.S, ...f.D, ...f.B]), users = new Map();
  data.forEach((ds, i) => { for (const d of ds) { const u = users.get(d); if (!u) users.set(d, [i, i]); else u[1] = i; } });
  const crossed = new Uint32Array(N);
  for (const [, [a, b]] of users) if (b - a <= W) for (let i = a; i < b; i++) crossed[i]++;
  const forced = new Uint8Array(N);
  if (tags) { let lt = -1; for (let j = 0; j < N; j++) { const m = fns[j].mod; if (!isFile(m) || m === tags.hide) continue;
    if (lt >= 0 && fns[lt].mod !== m) { const cost = (i) => crossed[i] * 4 + (isThunk(i + 1) ? 0 : 1); let b = -1;
      for (let i = lt; i < j; i++) if (!isThunk(i) && (b < 0 || cost(i) <= cost(b))) b = i; if (b >= 0) forced[b] = 1; }
    lt = j; } }
  const units = [];
  for (let i = 0, s = 0; i < N; i++) if ((!crossed[i] && !isThunk(i)) || forced[i] || i === N - 1) {
    const bss = []; for (let j = s; j <= i; j++) for (const d of data[j]) if (d >= BSS && users.get(d)[1] - users.get(d)[0] <= W) bss.push(d);
    const u = units[units.length - 1], lo = bss.length ? Math.min(...bss) : 0;
    if (!u || forced[u.e] || (u.bss && bss.length && lo > u.hi + GAP)) units.push({ e: i, bss: bss.length, hi: bss.length ? Math.max(...bss) : -1 });
    else { u.e = i; u.bss += bss.length; if (bss.length) u.hi = Math.max(u.hi, ...bss); }
    s = i + 1;
  }
  return units.slice(0, -1).map((u) => u.e);
}

// ---- main --------------------------------------------------------------------------------------------------------
if (flag("--eval")) {
  const run = (name, o) => console.log(`| ${name} | ${fmt(score(segment({ ...DEFAULTS, ...o }, null)))} |`);
  console.log("blind (no tags used):");
  console.log(`| original tu-split.mjs, tags hidden | ${fmt(score(original(null)))} |`);
  console.log(`| original tu-split.mjs, with tags | ${fmt(score(original({})))} |`);
  const none = { calls: 0, inv: 0, init: 0, style: false, groups: false, quiet: 1, glueInit: false, initUse: 0 };
  run("shared data only (S+D+R+B)", none);
  const c1 = { ...none, calls: 1 }; run("+ calls", c1);
  const c2 = { ...c1, init: 1 }; run("+ init bonus at every initialiser run", c2);
  const c3 = { ...c2, groups: true, quiet: 2 }; run("+ init groups (atexit helpers, ccmd triples)", c3);
  const c4 = { ...c3, glueInit: true, initUse: 300 }; run("+ no cut inside init code, init-to-user edges", c4);
  const c5 = { ...c4, inv: 1 }; run("+ inversions", c5);
  run("+ style (= default)", {});
  console.log("ablations of the default:");
  run("no .data strings (S)", { kinds: "DRB" });
  run("no .data globals (D)", { kinds: "SRB" });
  run("no .data at all (S, D)", { kinds: "RB", invSec: "b" });
  run("no .rdata (R)", { kinds: "SDB" });
  run("no .bss (B)", { kinds: "SDR", invSec: "d" });
  run("only .bss (B)", { kinds: "B", invSec: "b" });
  run("no calls", { calls: 0 });
  run("no init bonus", { init: 0 });
  run("no inversions", { inv: 0 });
  run("no style", { style: false });
  run("no init groups (quiet 2 on raw initialisers)", { groups: false });
  run("no glue inside init code", { glueInit: false });
  run("no init-to-user edges", { initUse: 0 });
  run("init bonus 2", { init: 2 });
  for (const g of [3, 20]) run(`gamma ${g}`, { gamma: g });
  for (const c of [-0.25, -1]) run(`cut ${c}`, { cut: c });
  // split-half: tune gamma/cut/init on the even-numbered transitions, report the odd ones (and the reverse)
  const half = (k) => { const f = T.trans.filter((_, i) => i % 2 === k), keep = new Set(f.map((t) => t.lo));
    return (cuts) => { const r = score(cuts); const miss = r.miss.filter((t) => keep.has(t.lo)).length;
      return { tp: f.length - miss, n: f.length }; }; };
  for (const k of [0, 1]) {
    let bestF = -1, bestO = null;
    for (const gamma of [3, 6, 10, 15, 20]) for (const cut of [-0.25, -0.5, -1, -2]) for (const init of [0.5, 1, 2]) {
      const o = { ...DEFAULTS, gamma, cut, init }, cuts = segment(o, null), r = score(cuts), h = half(k)(cuts);
      const prec = r.good / (r.good + r.fp), rec = h.tp / h.n, f1 = 2 * prec * rec / (prec + rec); // fp over all spans
      if (f1 > bestF) { bestF = f1; bestO = o; }
    }
    const cuts = segment(bestO, null), test = half(1 - k)(cuts), r = score(cuts);
    console.log(`split-half: tuned on ${k ? "odd" : "even"} transitions -> gamma ${bestO.gamma} cut ${bestO.cut} init ${bestO.init}; ` +
      `held-out recall ${test.tp}/${test.n} ${pct(test.tp, test.n)}, fp ${r.fp}, units ${r.cuts + 1}`);
  }
  console.log("leave-one-out with tags (each file's tags hidden in turn):");
  console.log(`| original tu-split.mjs | ${fmtLoo(loo(null, (hide) => original({ hide })))} |`);
  const l = loo(DEFAULTS); console.log(`| tu-split2 default | ${fmtLoo(l)} |`); console.log("  misses: " + l.bad.join("; "));
  process.exit(0);
}

const tags = flag("--blind") ? null : { hide: arg("--hide", null) };
const cuts = segment(DEFAULTS, tags);
const lines = [], tagUnits = new Map(); let mixed = 0, initOnly = 0;
for (let k = 0, s = 0; k <= cuts.length; k++) {
  const e = k < cuts.length ? cuts[k] : N - 1;
  const mods = new Map(); for (let j = s; j <= e; j++) if (fns[j].mod !== "-") mods.set(fns[j].mod, (mods.get(fns[j].mod) ?? 0) + 1);
  const files = [...mods.keys()].filter(isFile);
  if (files.length > 1) mixed++;
  for (const f of files) (tagUnits.get(f) ?? tagUnits.set(f, new Set()).get(f)).add(k);
  const tag = [...mods].sort((x, y) => y[1] - x[1]).map(([m, c]) => `${m}:${c}`).join(",") || "-";
  const bss = []; for (let j = s; j <= e; j++) bss.push(...fns[j].B);
  bss.sort((a, b) => a - b); const b = bss.length ? `${hex(bss[bss.length >> 2])}..${hex(bss[(3 * bss.length) >> 2])}` : "-"; // middle half
  // why the unit starts here: init = initialiser run, style = unoptimised-code run edge, data = graph only
  let only = true; for (let j = s; j <= e; j++) if (!fns[j].group) only = false;
  if (only) initOnly++; // a unit of static-init code alone: its file's code was cut off (see RESULTS.md)
  const why = s === 0 ? "start" : style.edge[s - 1] ? "style" : initStart(s - 1, DEFAULTS) ? "init" : "data";
  lines.push(`${hex(fns[s].a)}-${hex(fns[e].a)} n=${e - s + 1} ${why} bss=${b} ${tag} ${fns[s].name}..${fns[e].name}`);
  s = e + 1;
}
const split = [...tagUnits].filter(([, s]) => s.size > 1);
const summary = `# ${cuts.length + 1} units; ${mixed} mix two .cpp tags; ${split.length} of ${tagUnits.size} tagged files split over several units` +
  (tags ? "" : " (blind: tags not used)");
if (flag("--score")) {
  console.log(summary);
  for (const [f, s] of split) console.log(`#   split: ${f} x${s.size}`);
  console.log(`# ${initOnly} units hold static-init code only`);
  const blind = score(segment(DEFAULTS, null));
  console.log(`# blind boundaries: ${fmt(blind)}`);
  for (const t of blind.miss) console.log(`#   missed: ${t.a} -> ${t.b} (gap ${hex(fns[t.lo].a)}..${hex(fns[t.hi + 1].a)})`);
  for (const c of blind.fps) console.log(`#   cut inside a file: ${hex(fns[c + 1].a)} ${fns[c + 1].name} (${[...T.spans].find(([, s]) => s.first <= c && c < s.last)[0]})`);
  if (flag("--loo")) { const l = loo(DEFAULTS); console.log(`# leave-one-out: ${fmtLoo(l)}`); for (const b of l.bad) console.log(`#   ${b}`); }
} else if (flag("--loo")) { const l = loo(DEFAULTS); console.log(`# leave-one-out: ${fmtLoo(l)}`); for (const b of l.bad) console.log(`#   ${b}`); }
const text = lines.join("\n") + "\n" + summary + "\n";
if (out) writeFileSync(out, text); else if (!flag("--score") && !flag("--loo")) process.stdout.write(text);
