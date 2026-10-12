// Client for tools/rea-daemon.mjs: submit REA tool calls through the queue folder and await results.
import { writeFileSync, renameSync, existsSync, readFileSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..", "..");
export const queueDir = join(repoRoot, ".rea-queue");
let seq = 0;

export function daemonReady() {
  return existsSync(join(queueDir, "daemon.ready")) && !existsSync(join(queueDir, "daemon.exit"));
}

// Call one REA tool. Returns { isError, text, json } where json is the parsed result (or null).
export async function reaCall(name, args = {}, { timeoutMs = 900000 } = {}) {
  if (!daemonReady()) throw new Error("REA daemon not running (start: node tools/rea-daemon.mjs)");
  const id = `${Date.now()}-${process.pid}-${String(seq++).padStart(4, "0")}`;
  writeFileSync(join(queueDir, id + ".tmpreq"), JSON.stringify({ name, arguments: args }));
  renameSync(join(queueDir, id + ".tmpreq"), join(queueDir, id + ".req.json"));
  const res = join(queueDir, id + ".res.json");
  const deadline = Date.now() + timeoutMs;
  while (!existsSync(res)) {
    if (existsSync(join(queueDir, "daemon.exit"))) throw new Error("REA daemon exited");
    if (Date.now() > deadline) throw new Error(`timeout waiting for ${name}`);
    await new Promise((r) => setTimeout(r, 200));
  }
  const out = JSON.parse(readFileSync(res, "utf8"));
  let json = null;
  try {
    const o = JSON.parse(out.text);
    json = o.evidence_id ? (o.normalized_result ?? o.raw_result) : (o.result ?? o);
  } catch {}
  return { isError: out.isError, text: out.text, json };
}
