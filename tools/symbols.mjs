// Symbol-file tool for symbols/sum.symbols (see docs/decomp-plan.md).
//
//   node tools/symbols.mjs check                 validate the symbol file
//   node tools/symbols.mjs apply-rea             (not possible: REA's Windows Ghidra is read-only; use tools/decomp.mjs)
//   node tools/symbols.mjs apply-x32dbg          push names into x32dbg as labels (Sum.exe must be loaded)
//   node tools/symbols.mjs fetch-functions       save Ghidra's function list to symbols/ghidra-functions.tsv
//   node tools/symbols.mjs progress              named/unnamed functions per address range and module
import { readFileSync, writeFileSync, existsSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { reaCall } from "./lib/rea-queue.mjs";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const SYMBOLS = join(repoRoot, "symbols", "sum.symbols");
const FUNCS = join(repoRoot, "symbols", "ghidra-functions.tsv");
const SUM = join(repoRoot, "Summoner", "Sum.exe");

export function loadSymbols(path = SYMBOLS) {
  const out = [], errors = [];
  readFileSync(path, "utf8").split(/\r?\n/).forEach((raw, i) => {
    const line = raw.replace(/\s+$/, "");
    if (!line || line.startsWith("#")) return;
    const [body, ...c] = line.split(";");
    const f = body.trim().split(/\s+/);
    if (f.length < 5) { errors.push(`line ${i + 1}: expected 5 fields: ${line}`); return; }
    const [addr, name, kind, module, conf] = f;
    if (!/^0x[0-9a-f]+$/.test(addr)) errors.push(`line ${i + 1}: bad address ${addr} (lowercase hex with 0x)`);
    if (!/^[A-Za-z_][A-Za-z0-9_]*$/.test(name)) errors.push(`line ${i + 1}: bad name ${name}`);
    if (!/^(func|data:.+)$/.test(kind)) errors.push(`line ${i + 1}: bad kind ${kind}`);
    if (!/^[VOI]$/.test(conf)) errors.push(`line ${i + 1}: bad confidence ${conf}`);
    out.push({ line: i + 1, addr: parseInt(addr, 16), addrText: addr, name, kind, module, conf, comment: c.join(";").trim() });
  });
  const byAddr = new Map(), byName = new Map();
  let prev = -1;
  for (const s of out) {
    if (byAddr.has(s.addr)) errors.push(`line ${s.line}: duplicate address ${s.addrText} (line ${byAddr.get(s.addr).line})`);
    if (byName.has(s.name)) errors.push(`line ${s.line}: duplicate name ${s.name} (line ${byName.get(s.name).line})`);
    byAddr.set(s.addr, s); byName.set(s.name, s);
  }
  for (const kind of ["func", "data"]) {
    prev = -1;
    for (const s of out.filter((x) => x.kind.startsWith(kind))) {
      if (s.addr < prev) errors.push(`line ${s.line}: ${kind} entries not sorted by address at ${s.addrText}`);
      prev = s.addr;
    }
  }
  return { symbols: out, errors };
}

export function loadFunctions() {
  if (!existsSync(FUNCS)) return null;
  return readFileSync(FUNCS, "utf8").split(/\r?\n/).filter((l) => l && !l.startsWith("#")).map((l) => {
    const [addr, name, kind] = l.split("\t");
    return { addr: parseInt(addr, 16), name, kind };
  });
}

async function applyRea(symbols) {
  // REA's Windows Ghidra provider rejects database mutation ("Windows Ghidra P0 does not admit
  // database mutation"), so names can't be stored in the Ghidra session. tools/decomp.mjs applies
  // them to Ghidra's output instead.
  console.log(`REA/Ghidra on Windows is read-only; ${symbols.length} names are applied to output by tools/decomp.mjs instead.`);
}

async function applyX32dbg(symbols) {
  const cfg = JSON.parse(readFileSync(join(process.env.LOCALAPPDATA, "Programs", "x64dbg", "x32", "mcp_config.json"), "utf8"));
  const url = `http://127.0.0.1:${cfg.Port}/`;
  let session = null, id = 0;
  const rpc = async (method, params, notify = false) => {
    const headers = { "Content-Type": "application/json", Accept: "application/json, text/event-stream", Authorization: `Bearer ${cfg.AuthToken}` };
    if (session) headers["Mcp-Session-Id"] = session;
    const res = await fetch(url, { method: "POST", headers, body: JSON.stringify(notify ? { jsonrpc: "2.0", method, params } : { jsonrpc: "2.0", id: ++id, method, params }) });
    session = res.headers.get("mcp-session-id") || session;
    if (notify) return null;
    const text = await res.text();
    const m = text.match(/^data: (.*)$/m);
    return JSON.parse(m ? m[1] : text);
  };
  const call = async (name, args) => {
    const r = await rpc("tools/call", { name, arguments: args });
    return (r.result?.content || []).map((c) => c.text ?? "").join("\n");
  };
  await rpc("initialize", { protocolVersion: "2024-11-05", capabilities: {}, clientInfo: { name: "symbols.mjs", version: "1" } });
  await rpc("notifications/initialized", {}, true);
  const state = await call("GetDebugState", {});
  if (!/isDebugging: true/.test(state)) throw new Error("x32dbg has no debug session; load Sum.exe first (tools/start-x32dbg.ps1 -Game)");
  let ok = 0;
  for (const s of symbols) {
    const out = await call("ExecuteDebuggerCommand", { command: `lbl ${s.addrText}, "${s.name}"` });
    if (/error|fail|invalid/i.test(out)) console.log(`  ${s.addrText} ${s.name}: ${out.split("\n")[0]}`);
    else ok++;
  }
  console.log(`labelled ${ok}/${symbols.length} addresses in x32dbg`);
}

async function fetchFunctions() {
  const r = await reaCall("list_procedures", {});
  let items = Array.isArray(r.json) ? r.json : null;
  if (r.isError || !items) {
    console.log("list_procedures did not return a list:", r.text.slice(0, 300));
    process.exit(1);
  }
  const lines = ["# Ghidra 12.1.4 default analysis of Sum.exe v1.40: address<TAB>ghidra-name<TAB>kind", "# Generated by: node tools/symbols.mjs fetch-functions"];
  for (const it of items) {
    if (!/^0x/.test(it.address)) continue; // skip EXTERNAL:
    const kind = /^FUN_/.test(it.value) ? "unnamed" : /^(thunk_|Ordinal_)/.test(it.value) ? "thunk" : "ghidra-named";
    lines.push(`${it.address}\t${it.value}\t${kind}`);
  }
  writeFileSync(FUNCS, lines.join("\n") + "\n");
  console.log(`saved ${lines.length - 2} functions to ${FUNCS}`);
}

function progress(symbols) {
  const funcs = loadFunctions();
  if (!funcs) { console.log("run fetch-functions first"); process.exit(1); }
  const named = new Set(symbols.filter((s) => s.kind === "func").map((s) => s.addr));
  const crtStart = 0x56c740; // first C-runtime function (sprintf); see docs/sum-exe/source-map.md
  const game = funcs.filter((f) => f.addr < crtStart), crt = funcs.filter((f) => f.addr >= crtStart);
  const pct = (a, b) => (b ? ((100 * a) / b).toFixed(1) : "0.0");
  const gameNamed = game.filter((f) => named.has(f.addr)).length;
  console.log(`functions: ${funcs.length} total (${game.length} game/engine below 0x${crtStart.toString(16)}, ${crt.length} C runtime/library)`);
  console.log(`named in symbols/sum.symbols: ${gameNamed} / ${game.length} game/engine functions (${pct(gameNamed, game.length)}%)`);
  console.log(`Ghidra already identified ${funcs.filter((f) => f.kind === "ghidra-named").length} library functions by signature`);
  const conf = { V: 0, O: 0, I: 0 };
  for (const s of symbols) conf[s.conf]++;
  console.log(`all symbols: ${symbols.length} (V ${conf.V}, O ${conf.O}, I ${conf.I}); data ${symbols.filter((s) => s.kind !== "func").length}`);
  const step = 0x10000;
  console.log("\nper 64 KB of .text (named/total):");
  for (let a = 0x400000; a < 0x580000; a += step) {
    const inR = funcs.filter((f) => f.addr >= a && f.addr < a + step);
    if (!inR.length) continue;
    const n = inR.filter((f) => named.has(f.addr)).length;
    const bar = "#".repeat(Math.round((20 * n) / inR.length)).padEnd(20, ".");
    console.log(`  0x${a.toString(16)} ${bar} ${String(n).padStart(4)}/${inR.length}`);
  }
}

if (process.argv[1] && !process.argv[1].endsWith("symbols.mjs")) { /* imported as a module */ } else {
const [cmd] = process.argv.slice(2);
const { symbols, errors } = loadSymbols();
if (cmd === "check" || !cmd) {
  if (errors.length) { console.log(errors.join("\n")); process.exit(1); }
  console.log(`OK: ${symbols.length} symbols (${symbols.filter((s) => s.kind === "func").length} functions)`);
} else {
  if (errors.length) { console.log("symbol file has errors; run check"); process.exit(1); }
  if (cmd === "apply-rea") await applyRea(symbols);
  else if (cmd === "apply-x32dbg") await applyX32dbg(symbols);
  else if (cmd === "fetch-functions") await fetchFunctions();
  else if (cmd === "progress") progress(symbols);
  else { console.log("unknown command"); process.exit(1); }
}
}
