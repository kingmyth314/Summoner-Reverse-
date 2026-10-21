---
title: Public release (checklist)
nav_order: 6.48
---

# Making the project public (checklist)

**Status:** assessment of 2026-10-21, with the advisor's review. This page is not legal advice.
**Decided by the owner (2026-10-21): tier 2** (this repository with `src/decomp`, no executables), with the personal
path removed from the files and from the history. Done the same day: the licence split (`LICENSE`,
`src/decomp/NOTICE.md`), the README for new readers with build requirements, credits and the legal section, the
personal paths replaced (`docs/setup.md` and two worker notes), and the history rewritten so that no version of any
file contains them (a mirror of the old history is kept privately). Making the repository public is the last step.

## The one real question

Most of this project's value is `src/decomp`, a reconstruction of `Sum.exe`, a game that is still sold (the rights
are THQ Nordic's). Comparable projects are public (the N64 and PS1 decompilations, for example), but they exist
because the rights holders tolerate them, not because a right protects them; "you need your own copy of the game" is
the convention, not a shield. A game that is still on sale makes a takedown more likely than for an abandoned one.
Only a lawyer can put a number on that risk. Everything else on this page is housekeeping.

Publishing is one-way: forks, mirrors and caches survive if the repository is made private again, so the first
public commit is the one that counts.

## Three tiers, lowest risk first

| Tier | What is published | How |
|---|---|---|
| 1 | SummonerFix (source, plus the DLL and `SummonerFixConfig.exe` as releases), `tools/`, `docs/`, `symbols/` | a new public repository with that subset, since a private repository cannot be partly public |
| 2 | tier 1 and `src/decomp` | the usual decompilation-project posture: make this repository public, or push its history to a new public one |
| 3 | tier 2 and built executables (`Sum-rebuilt.exe`, later `Sum-fixed.exe`) | **not recommended**: a working executable of the game is a different thing from source, and the VC6 static runtime raises redistribution questions of its own |

Tier 1 gives players what they actually use (SummonerFix and its config app) and carries the least risk. What tier 2
adds besides the reconstructed code: the game's own text that the code contains (684 `XSTR` string literals) and the
strings quoted in `sum.symbols` annotations.

## Audit (2026-10-21)

Checked, and clean:

- **Tracked files:** only `src/`, `docs/`, `tools/`, `symbols/`, `README.md`, `LICENSE`, `.gitignore`. No
  executables, DLLs, archives, game data (`.vpp`, `.tbl`, saves, bitmaps, sounds), compilers or third-party sources.
  `.gitignore` keeps out `Summoner/`, `extracted/`, `reference/`, `third_party/` and `build/`.
- **History (474 commits):** no such file was ever committed; the largest objects are versions of `sum.symbols`.
- **Secrets:** the x32dbg bearer token appears only as a placeholder (`<token from mcp_config.json>`), now and in
  history. Commits use a GitHub no-reply address.
- **The strategy guide:** never committed; the docs cite it by page only.
- **Code from the reference projects:** a line-by-line comparison (40+ characters, comments removed) finds 43 of
  32,569 lines shared with the Summoner 2 decompilation and 4 with Dash Faction. All are convergent rather than
  copied: strings and constants that are in `Sum.exe` itself (the console's messages, the frame-time limits; both
  games use Volition's engine), textbook matrix and vector lines, Ken Perlin's public noise variable names, DirectX SDK
  constants. The rule that those projects are used for names only held.
- **Game data in the docs:** the docs describe formats and rules (field offsets, how prices are computed), not the
  item, spell or monster tables themselves.

## Must change before any tier

1. **`LICENSE`.** MIT over the whole repository claims rights over `src/decomp` that the authors do not hold. Split
   it: MIT for `tools/`, `src/summonerfix/` and `docs/`; for `src/decomp` a notice that it is a reconstruction of the
   retail binary for study, interoperability and preservation, that no licence is granted for it, and that all
   rights in Summoner remain with the rights holder.
2. **`docs/setup.md`, line 23:** a personal folder path; replace it with a placeholder. The same path is in one
   line of history. Removing it from history needs a history rewrite (`git filter-repo`) and a force push, cheap for a
   one-author repository but the owner's decision.
3. **`README.md` for strangers:** what the project is, its state (5,445 of 5,926 functions byte-identical, every
   game function written, every boss fight run on the rebuilt exe), that building needs your own Visual C++ 6.0 SP5
   and your own copy of the game, "not affiliated with or endorsed by THQ Nordic or Volition", a trademark line, and
   the IJG acknowledgement for libjpeg if any binary that contains it is ever published.
4. **Credits:** the Summoner 2 decompilation and Dash Faction, as sources of names; check what Dash Faction's licence
   asks for.

## Smaller cleanups

- `tools/link-savecheck.mjs` and `link-screencheck.mjs` default to a test folder at `../../SummonerToolchain`; document
  `--game` (they already take it).
- The docs and `DECISIONS.md` read as a working log ("worker 5", "the user", round numbers). A line in the README
  saying so is enough.
- GitHub Pages: the `docs/` site can be served from a public repository at no cost.

## Decisions for the owner

1. Which tier, if any.
2. Whether to remove the personal path from history (rewrite and force push) or only from the current files.
3. For tier 1: the name of the new public repository and which SummonerFix version to release first.
