// Long-lived REA MCP session (no per-call timeout) driven by a request folder.
// Ghidra's first analysis of Sum.exe takes ~100 s, longer than the MCP client timeout;
// keeping one session open means that cost is paid once.
//
// usage: node tools/rea-daemon.mjs [queueDir]     (default: <repo>/.rea-queue)
// Write <id>.req.json = {"name": tool, "arguments": {...}}; the daemon writes <id>.res.json.
import { spawn } from "node:child_process";
import { readdirSync, readFileSync, writeFileSync, renameSync, mkdirSync } from "node:fs";
import { join, dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const qdir = resolve(process.argv[2] ?? join(repoRoot, ".rea-queue"));
mkdirSync(qdir, { recursive: true });
const rea = join(process.env.APPDATA, "npm", "node_modules", "rea-agents", "scripts", "rea.mjs");
const child = spawn(process.execPath, [rea, "mcp"], { stdio: ["pipe", "pipe", "pipe"] });
child.on("exit", (c) => { writeFileSync(join(qdir, "daemon.exit"), String(c)); process.exit(1); });
child.stderr.on("data", () => {});

let buf = "", nextId = 10, busy = null, ready = false;
const send = (m) => child.stdin.write(JSON.stringify(m) + "\n");

child.stdout.on("data", (d) => {
  buf += d;
  const lines = buf.split("\n");
  buf = lines.pop();
  for (const line of lines) {
    let msg;
    try { msg = JSON.parse(line); } catch { continue; }
    if (msg.id === 1) {
      send({ jsonrpc: "2.0", method: "notifications/initialized" });
      ready = true;
      writeFileSync(join(qdir, "daemon.ready"), new Date().toISOString());
      console.log(`rea-daemon ready, queue: ${qdir}`);
    } else if (busy && msg.id === busy.rpcId) {
      const r = msg.result ?? {};
      const text = msg.error ? JSON.stringify(msg.error) : (r.content ?? []).map((c) => c.text ?? "").join("\n");
      const out = { isError: !!(msg.error || r.isError), elapsedMs: Date.now() - busy.started, text };
      writeFileSync(join(qdir, busy.id + ".tmp"), JSON.stringify(out));
      renameSync(join(qdir, busy.id + ".tmp"), join(qdir, busy.id + ".res.json"));
      busy = null;
    }
  }
});

setInterval(() => {
  if (!ready || busy) return;
  const req = readdirSync(qdir).filter((f) => f.endsWith(".req.json")).sort()[0];
  if (!req) return;
  const id = req.slice(0, -".req.json".length);
  const p = join(qdir, req);
  let name, args;
  try {
    ({ name, arguments: args } = JSON.parse(readFileSync(p, "utf8")));
  } catch (e) {
    // A malformed request must not kill the session (and its Ghidra analysis): answer with an error.
    renameSync(p, join(qdir, id + ".bad"));
    writeFileSync(join(qdir, id + ".res.json"), JSON.stringify({ isError: true, elapsedMs: 0, text: `invalid request file: ${e.message}` }));
    return;
  }
  renameSync(p, join(qdir, id + ".sent"));
  busy = { id, rpcId: nextId++, started: Date.now() };
  send({ jsonrpc: "2.0", id: busy.rpcId, method: "tools/call", params: { name, arguments: args ?? {} } });
}, 250);

send({ jsonrpc: "2.0", id: 1, method: "initialize", params: { protocolVersion: "2025-06-18", capabilities: {}, clientInfo: { name: "rea-daemon", version: "1" } } });
