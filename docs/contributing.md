---
title: Documentation standard
nav_order: 7
---

# Documentation standard

Every finding is written up in this `docs/` folder as it is made, so the repository can
be pushed to GitHub and published with GitHub Pages at any time.

## Where things go

| Content | Location |
|---|---|
| A topic about `Sum.exe` | `docs/sum-exe/<topic>.md` with `parent: Sum.exe` |
| Another binary (for example `Summoner.exe`) | new section `docs/<binary>/index.md` with `has_children: true` |
| A third-party patch or mod | `docs/patches/<name>.md` with `parent: Patches` |
| Every new name | `symbols/sum.symbols` via `node tools/symbols.mjs add` (the source of truth); `docs/reference/address-map.md` only for addresses that need narrative context |
| Every external link or downloaded file | `docs/reference/sources.md` |
| Tool or environment changes | `docs/setup.md` or `docs/tools.md` |
| Headline findings | the table in `docs/index.md` and the root `README.md` |

## Page rules

1. **Front matter.** Every page starts with `title`, `nav_order`, and `parent` when it
   belongs to a section (Just the Docs theme).
2. **Summary first.** Open with one or two sentences that state the conclusion.
3. **Evidence level on every claim:** **Observed** (read from code, assembly or bytes),
   **Inferred** (concluded from code, not run), or **Unknown** (open question).
4. **Name functions twice:** our label and Ghidra's default, for example
   `FrameTimer` (`FUN_00501510`). Addresses are lowercase hex with `0x`.
5. **Show the code.** Quote the assembly for anything that hinges on exact instructions,
   and use cleaned-up pseudocode for structure.
6. **Evidence table.** End analysis pages with the REA evidence IDs for key claims.
7. **Corrections stay visible.** When a finding changes, fix the page and add a short
   `{: .important }` note that says what changed.
8. **Diagrams** use Mermaid code blocks, which render on both GitHub and the Pages site.

## What never goes in the repository

- Game files: executables, DLLs, `.vpp`, `.bik` (enforced by `.gitignore`).
- Third-party binaries such as patch DLLs. Record source, size and SHA-256 instead.
- Credentials or personal paths beyond what is needed to reproduce the setup.

## Publishing to GitHub Pages

1. Create a GitHub repository and push this folder.
2. In the repository, open **Settings > Pages**, choose **Deploy from a branch**, pick the
   branch, and set the folder to `/docs`.
3. GitHub builds the site with the Just the Docs remote theme from `docs/_config.yml`.

To preview locally (optional; requires Ruby):

```bash
cd docs
bundle init
bundle add github-pages --group jekyll_plugins
bundle exec jekyll serve
```
