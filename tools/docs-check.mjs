// Checks the documentation before a push (docs/contributing.md): every page in docs/ has front matter with a title
// and a nav_order, and every relative link in docs/, README.md and AGENTS.md points at a file that exists and, with
// a #fragment, at a heading of that page. A fragment has to be the heading's id under both rules, GitHub's (the
// repository view) and kramdown's (the Pages site): they differ for headings that start with a digit or contain
// underscores, so such headings cannot be linked to. External links are not fetched.
// usage: node tools/docs-check.mjs [--quiet]
import { existsSync, readFileSync, readdirSync, statSync } from "node:fs";
import { dirname, join, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const repo = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const quiet = process.argv.includes("--quiet");

function walk(dir, out = []) {
  for (const n of readdirSync(dir)) {
    const p = join(dir, n);
    if (statSync(p).isDirectory()) walk(p, out);
    else if (n.endsWith(".md")) out.push(p);
  }
  return out;
}

function plain(text) {
  return text
    .replace(/`([^`]*)`/g, "$1")
    .replace(/\[([^\]]*)\]\([^)]*\)/g, "$1")
    .replace(/\*{1,2}([^*]+)\*{1,2}/g, "$1")   // emphasis; underscores stay (they are part of names)
    .toLowerCase()
    .trim();
}

// GitHub: punctuation dropped (letters, digits, spaces, - and _ kept), spaces to dashes.
function slug(text) {
  return plain(text).replace(/[^a-z0-9 _-]/g, "").replace(/ /g, "-");
}

// kramdown: only letters, digits, spaces and dashes kept, everything before the first letter dropped, spaces to
// dashes.
function kramdownSlug(text) {
  return plain(text).replace(/[^a-z0-9 -]/g, "").replace(/^[^a-z]+/, "").replace(/ /g, "-");
}

const anchorCache = new Map();
function anchors(file) {
  if (!anchorCache.has(file)) {
    const set = new Set();
    let inCode = false;
    for (const line of readFileSync(file, "utf8").split(/\r?\n/)) {
      if (/^```/.test(line)) inCode = !inCode;
      const m = !inCode && /^#{1,6}\s+(.*?)\s*#*\s*$/.exec(line);
      if (m && slug(m[1]) === kramdownSlug(m[1]))   // linkable only where both sites give the same id
        set.add(slug(m[1]));
      // An explicit anchor (<a name="..."></a> before a heading that names a function, such as FUN_00501510) has the
      // same id on both sites.
      for (const a of line.matchAll(/<a\s+(?:name|id)="([^"]+)"/g))
        set.add(a[1]);
    }
    anchorCache.set(file, set);
  }
  return anchorCache.get(file);
}

const problems = [];
const pages = walk(join(repo, "docs"));
for (const page of pages) {
  const text = readFileSync(page, "utf8");
  const rel = relative(repo, page).replace(/\\/g, "/");
  const fm = /^---\r?\n([\s\S]*?)\r?\n---/.exec(text);
  if (!fm) problems.push(`${rel}: no front matter`);
  else {
    if (!/^title:/m.test(fm[1])) problems.push(`${rel}: front matter without title`);
    if (!/^nav_order:/m.test(fm[1]) && !/^nav_exclude:/m.test(fm[1])) problems.push(`${rel}: front matter without nav_order`);
  }
}

let links = 0;
for (const file of [...pages, join(repo, "README.md"), join(repo, "AGENTS.md")]) {
  if (!existsSync(file)) continue;
  const rel = relative(repo, file).replace(/\\/g, "/");
  let inCode = false;
  readFileSync(file, "utf8").split(/\r?\n/).forEach((line, i) => {
    if (/^```/.test(line)) inCode = !inCode;
    if (inCode) return;
    for (const m of line.replace(/`[^`]*`/g, "").matchAll(/\]\(([^)\s]+)\)/g)) {
      const target = m[1];
      if (/^(https?:|mailto:)/.test(target)) continue;
      links++;
      const [path, frag] = target.split("#");
      const dest = path ? resolve(dirname(file), decodeURIComponent(path)) : file;
      if (!existsSync(dest)) {
        problems.push(`${rel}:${i + 1}: missing ${target}`);
        continue;
      }
      if (frag && dest.endsWith(".md") && !anchors(dest).has(frag))
        problems.push(`${rel}:${i + 1}: no heading #${frag} in ${relative(repo, dest).replace(/\\/g, "/")}`);
    }
  });
}

for (const p of problems) console.log(p);
if (!quiet || problems.length) console.log(`${pages.length} pages, ${links} relative links: ${problems.length} problem(s)`);
process.exit(problems.length ? 1 : 0);
