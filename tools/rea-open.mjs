// Open Summoner\Sum.exe in the running REA daemon session (backslash path; forward slashes don't bind).
// usage: node tools/rea-open.mjs
import { reaCall, daemonReady } from "./lib/rea-queue.mjs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const repoRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const exe = join(repoRoot, "Summoner", "Sum.exe").replace(/\//g, "\\");
for (let i = 0; i < 60 && !daemonReady(); i++) await new Promise((r) => setTimeout(r, 1000));
const o = await reaCall("open_binary", { path: exe, provider_id: "ghidra" });
if (o.isError) { console.log("open failed:", o.text.slice(0, 400)); process.exit(1); }
const s = await reaCall("binary_session", {});
console.log(`opened ${exe}: session open = ${s.json?.open}`);
