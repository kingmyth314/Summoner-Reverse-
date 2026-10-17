// node tools/vc6-flagsweep.mjs <decomp root> <out.json> [dir=vsdk]   (VC6_DIR: the SP5 VC98 folder)
// For every .cpp under <root>/<dir>, compiles it with its vc6-flags.txt line (or the default) and with two
// variants (/O2 -> /Ox plus /Ow, and /Ow added), and records the matching addresses of each. 4 at a time.
// A unit may switch to a variant only when its match set there contains every address it matched before
// (that is how D7502's vsdk lines were chosen, 2026-10-16).
import { readdirSync, readFileSync, writeFileSync } from "node:fs";
import { join, resolve, dirname } from "node:path";
import { fileURLToPath } from "node:url";
import { execFile } from "node:child_process";
const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const [root, outFile, dir = "vsdk"] = process.argv.slice(2);
const flags = new Map();
for (const l of readFileSync(join(root, "vc6-flags.txt"), "utf8").split(/\r?\n/)) {
  const m = /^([^#\s]\S*)\s+(\S.*?)\s*$/.exec(l); if (m) flags.set(m[1].toLowerCase(), m[2]);
}
const files = readdirSync(join(root, dir)).filter((f) => f.endsWith(".cpp")).map((f) => `${dir}/${f}`);
const env = { ...process.env, DECOMP_INCLUDE: join(root, "include") };
const jobs = [];
for (const f of files) {
  const cur = flags.get(f.toLowerCase()) || "/O2 /GX /MT";
  const ox = cur.replace(/\/O2\b/, "/Ox") + (/\/Ow\b/.test(cur) ? "" : " /Ow");
  const ow = /\/Ow\b/.test(cur) ? cur : cur + " /Ow";
  for (const [k, fl] of [["cur", cur], ["oxow", ox], ["ow", ow]]) jobs.push({ f, k, fl });
}
const res = {};
let i = 0;
function next() {
  if (i >= jobs.length) return Promise.resolve();
  const j = jobs[i++];
  return new Promise((ok) => {
    execFile("node", ["tools/vc6-compare.mjs", join(root, j.f), "--json", "--flags", j.fl], { cwd: repo, env, maxBuffer: 1 << 26 }, (e, so) => {
      let m = null;
      try { const x = JSON.parse(so); const r = Array.isArray(x) ? x[0] : x; if (!r.error) m = r.functions.filter((q) => q.status === "match").map((q) => q.addr); } catch {}
      (res[j.f] ||= {})[j.k] = { flags: j.fl, match: m };
      ok();
    });
  }).then(next);
}
await Promise.all([next(), next(), next(), next()]);
writeFileSync(outFile, JSON.stringify(res, null, 1));
let tot = { cur: 0, oxow: 0, ow: 0 };
for (const [f, r] of Object.entries(res)) for (const k of Object.keys(tot)) tot[k] += r[k]?.match?.length || 0;
console.log(files.length, "files", JSON.stringify(tot));
