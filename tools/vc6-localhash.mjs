// vc6-localhash.mjs: the frame order VC6 gives the locals of an unoptimised function (/Od, or #pragma optimize off).
//
// Usage: node tools/vc6-localhash.mjs [--probe] name1 name2 ...   (names in declaration order, one scope)
//
// VC6 does not place /Od locals in declaration order but by a hash of their names (DECISIONS.md D6101, D23500,
// D25200): h = h * 4 + (h >> 4) + c over the characters (32-bit unsigned), bucket = (h ^ (h >> 16)) & 15. Bucket 0
// gets the highest address (ebp-4 downwards); within a bucket the later declaration is higher. Block-scope locals
// follow the function scope's, `this` comes last. Prints the predicted order, highest address first, with each bucket.
// --probe also compiles a test function with these names as int locals (needs VC6_DIR) and prints VC6's order.
import { writeFileSync, readFileSync, mkdtempSync, rmSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { join } from "node:path";
import { tmpdir } from "node:os";

export function bucket(name) {
  let h = 0;
  for (const ch of name) h = ((h << 2) + (h >>> 4) + ch.charCodeAt(0)) >>> 0;
  return (h ^ (h >>> 16)) & 15;
}

export function frameOrder(names) {
  return names.map((n, i) => [n, bucket(n), i]).sort((a, b) => a[1] - b[1] || b[2] - a[2]).map((x) => x[0]);
}

if (process.argv[1] && process.argv[1].endsWith("vc6-localhash.mjs")) {
  const args = process.argv.slice(2);
  const probe = args[0] === "--probe";
  const names = probe ? args.slice(1) : args;
  if (!names.length) { console.error("usage: node tools/vc6-localhash.mjs [--probe] name1 name2 ..."); process.exit(2); }
  console.log("predicted: " + frameOrder(names).map((n) => `${n}:${bucket(n)}`).join(" "));
  if (probe) {
    const VC = process.env.VC6_DIR;
    if (!VC) { console.error("--probe needs VC6_DIR"); process.exit(2); }
    const dir = mkdtempSync(join(tmpdir(), "vc6hash-"));
    const src = `int zz_use(int *);\nvoid f()\n{\n${names.map((n) => `    int ${n};\n`).join("")}` +
      `${names.map((n) => `    zz_use(&${n});\n`).join("")}}\n`;
    writeFileSync(join(dir, "h.cpp"), src);
    const env = { ...process.env, INCLUDE: join(VC, "INCLUDE"), PATH: `${join(VC, "BIN")};${process.env.PATH}` };
    execFileSync(join(VC, "BIN", "CL.EXE"), ["/nologo", "/c", "/Od", "/FAsc", "/Fa" + dir + "/", "/Fo" + dir + "/", "h.cpp"],
      { env, cwd: dir, stdio: "ignore" });
    const cod = readFileSync(join(dir, "h.cod"), "latin1");
    const got = [...cod.matchAll(/^_(\w+)\$ = (-?\d+)/gm)].map((m) => [m[1], +m[2]]).sort((a, b) => b[1] - a[1]);
    console.log("VC6:       " + got.map(([n, o]) => `${n}:${o}`).join(" "));
    console.log(got.map((x) => x[0]).join() === frameOrder(names).join() ? "same order" : "DIFFERENT order");
    rmSync(dir, { recursive: true, force: true }); // dir is the mkdtempSync result, never empty
  }
}
