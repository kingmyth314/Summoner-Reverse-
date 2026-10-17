// Find free functions whose header declaration and .cpp definition have different parameter lists (a mismatch
// compiles silently as an overload). usage: node tools/decomp-sigcheck.mjs [src/decomp]
// Known and accepted: charinfo_load_move_anim (struct keyword in the header only).
import { readFileSync, readdirSync, statSync } from "node:fs";
import { join } from "node:path";
const root = process.argv[2] || "src/decomp";
const walk = (d, ext, out = []) => { for (const n of readdirSync(d)) { const p = join(d, n); if (statSync(p).isDirectory()) walk(p, ext, out); else if (p.endsWith(ext)) out.push(p); } return out; };
const norm = (params) => params.replace(/\/\*.*?\*\//g, "").split(",").map((p) => p.trim()
  .replace(/=.*$/, "").replace(/\s+/g, " ").replace(/\s*\*\s*/g, "*").replace(/\s*&\s*/g, "&")
  .replace(/^(.*?[\s*&])([A-Za-z_]\w*)(\[[^\]]*\])?$/, "$1$3").trim()).join(",").replace(/^void$/, "");
const decl = new Map();
const re = /^(?!\s)(?:extern\s+)?[A-Za-z_][\w\s*&:<>]*?[\s*&]([A-Za-z_]\w*)\s*\(([^)]*)\)\s*;/gm;
for (const f of walk(join(root, "include"), ".h")) for (const m of readFileSync(f, "utf8").matchAll(re)) decl.set(m[1], [norm(m[2]), f]);
const dre = /^(?!\s)[A-Za-z_][\w\s*&:<>]*?[\s*&]([A-Za-z_]\w*)\s*\(([^)]*)\)\s*\n\{/gm;
let n = 0;
for (const f of walk(root, ".cpp")) for (const m of readFileSync(f, "utf8").matchAll(dre)) {
  const d = decl.get(m[1]); if (!d) continue;
  const p = norm(m[2]); if (p !== d[0]) { n++; console.log(`${m[1]}: header (${d[0]}) vs ${f.split(/[\\/]/).pop()} (${p})`); }
}
console.log(n, "mismatches");
