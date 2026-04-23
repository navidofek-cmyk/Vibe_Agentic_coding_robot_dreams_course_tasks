# gnano — git-aware nano clone

Terminálový textový editor inspirovaný nano, napsaný v C++17. Bez ncurses, čisté ANSI escape kódy. Plná git integrace přes subprocess (bez libgit2). Markdown preview. SQLite pro historii souborů.

## Funkce

- Více otevřených souborů najednou (taby) s indikátorem neuložených změn
- Boční panel pro procházení souborového systému se stavem git souborů
- Syntax highlighting: C/C++, Python, JSON, Markdown, YAML, Shell, Makefile
- **Markdown preview mode** — `Ctrl+V` přepíná mezi zdrojovým a vykresleným pohledem
- Hledání textu (`Ctrl+F`)
- Fuzzy finder souborů (`Ctrl+P`)
- Grep search v projektu (`Ctrl+T`) — na pozadí ve vlákně
- Seznam naposledy otevřených souborů v SQLite (`Ctrl+R`)
- **Git integrace** (Ctrl+G prefix):
  - Badge v tab baru: větev + stav (zelená=čistá, žlutá=změny, červená=konflikty)
  - Gutter: `+` přidáno, `~` změněno, `_` smazáno (diff vs HEAD)
  - Blame, diff, log, branch picker, strom commitu
  - Stage/unstage souborů
- Perzistentní pozice kurzoru (obnoví se při znovuotevření souboru)

## Závislosti

Žádné externí závislosti kromě standardní C++17 knihovny a POSIX threadů.
SQLite je přibalen v `src/sqlite/` (amalgamation).
Git operace využívají `popen()` subprocess.

## Sestavení

```bash
make          # release build → build/nanoclone
make debug    # debug build s -g
make run      # sestaví a spustí
make test     # spustí testy (test_highlighters + test_git)
make clean    # smaže build/
```

## Spuštění

```bash
./run.sh                        # spustí z kořene projektu
./build/nanoclone               # prázdný editor
./build/nanoclone soubor.txt    # otevře soubor
./build/nanoclone a.txt b.cpp   # otevře více souborů najednou
```

## Klávesové zkratky

### Editor

| Zkratka | Akce |
|---------|------|
| `Ctrl+S` | Uložit aktuální soubor |
| `Ctrl+X` | Zavřít aktuální buffer / ukončit |
| `Ctrl+N` | Nový prázdný buffer |
| `Ctrl+O` | Otevřít / zavřít boční panel |
| `Ctrl+F` | Hledat text |
| `Ctrl+R` | Naposledy otevřené soubory |
| `Ctrl+P` | Fuzzy finder (hledání souborů) |
| `Ctrl+T` | Grep search v projektu |
| `Ctrl+A` | Přepnout na předchozí tab |
| `Ctrl+E` | Přepnout na další tab |
| `Ctrl+V` | Přepnout Markdown preview / source mode |
| `Ctrl+G` | Git příkaz prefix (viz níže) |
| `PgUp / PgDn` | Rychlá navigace |
| `Home / End` | Začátek / konec řádku |

### Git (Ctrl+G prefix)

| Zkratka | Akce |
|---------|------|
| `Ctrl+G B` | Git blame aktuálního souboru |
| `Ctrl+G D` | Git diff aktuálního souboru |
| `Ctrl+G L` | Git log aktuálního souboru |
| `Ctrl+G C` | Výběr větve (branch picker) |
| `Ctrl+G S` | Stage / unstage aktuálního souboru |
| `Ctrl+G T` | Strom commitu (tree view) |
| `Ctrl+G R` | Obnovit git stav |

### Boční panel

| Zkratka | Akce |
|---------|------|
| `Enter` | Otevřít soubor / vstoupit do adresáře |
| `[..]` + `Enter` | Přejít o úroveň výš |
| `Esc` | Zpět do editoru |

### Obecné

| Zkratka | Akce |
|---------|------|
| `Esc` | Zrušit aktuální akci / zavřít dialog |
| `S / D / C` | Při zavírání neuloženého souboru: Save / Discard / Cancel |

## Markdown Preview

Otevřete libovolný `.md` soubor a stiskněte `Ctrl+V` pro přepnutí do preview módu. Opětovné stisknutí přepne zpět do zdrojového módu pro editaci.

Podporované prvky v preview:
- Nadpisy H1–H6 (každá úroveň jinou barvou; H1/H2 s podtržením)
- Bold `**text**`, italic `*text*`
- Inline kód `` `code` `` a kódové bloky ```` ``` ````
- Blockquotes `> text`
- Odrážkové a číslované seznamy
- Task listy `- [ ]` / `- [x]`
- Horizontální linky `---`
- Odkazy `[text](url)` a obrázky `![alt](url)`

## Syntax Highlighting

| Přípona | Highlighter |
|---------|-------------|
| `.cpp`, `.cc`, `.h`, `.hpp`, `.c` | C/C++ |
| `.py` | Python |
| `.json` | JSON |
| `.md`, `.markdown` | Markdown |
| `.yaml`, `.yml` | YAML |
| `.sh`, `.bash` | Shell |
| `Makefile`, `*.mk` | Makefile |

## Struktura projektu

```
git-aware-nano/
├── Makefile
├── CMakeLists.txt
├── run.sh
├── PROMPT.md
└── src/
    ├── main.cpp               # vstupní bod
    ├── highlight.h            # SyntaxHighlighter interface + ColorSpan
    ├── highlighters.h/cpp     # C++, Python, JSON, Markdown, YAML, Shell, Makefile highlighters
    ├── markdown_preview.h/cpp # Markdown → ANSI preview renderer
    ├── buffer.h/cpp           # textový buffer, kurzor, scroll, hledání, preview flag
    ├── database.h/cpp         # SQLite — poslední soubory + pozice kurzoru
    ├── filepanel.h/cpp        # prohlížeč adresářů
    ├── editor.h/cpp           # ANSI UI, tabu, vstup, git overlay
    ├── fuzzy.h/cpp            # fuzzy hledání souborů
    ├── grep.h/cpp             # grep search na pozadí (std::thread)
    ├── terminal.h/cpp         # raw mode, readKey, ANSI helpers
    ├── sqlite/
    │   ├── sqlite3.h          # SQLite amalgamation
    │   └── sqlite3.c
    ├── git/
    │   ├── git_process.h/cpp  # popen wrapper + shellQuote
    │   ├── git_repo.h/cpp     # detekce repa, RepoInfo, refresh
    │   ├── git_diff.h/cpp     # diff vs HEAD → GutterMark
    │   ├── git_status.h/cpp   # git status --porcelain → map
    │   ├── git_blame.h/cpp    # git blame --porcelain → BlameEntry
    │   └── git_log.h/cpp      # log, větve, checkout, stage, tree
    └── tests/
        ├── test_highlighters.cpp  # 37 unit testů pro highlighters
        └── test_git.cpp           # 16 smoke testů pro git modul
```

## Databáze

Ukládá se do `~/.local/share/nanoclone/recent.db` (SQLite, WAL mode).

Tabulka `recent_files`:
- `path` — absolutní cesta k souboru
- `last_opened` — Unix timestamp
- `cursor_row`, `cursor_col` — uložená pozice kurzoru
