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
make test     # spustí všechny testy (156 testů ve 4 suitách)
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

### Split view (Ctrl+G T → commit → soubor → Enter)

| Zkratka | Akce |
|---------|------|
| `Ctrl+G T` → Enter | Otevře split: vlevo HEAD, vpravo stará verze |
| `↑ / ↓` | Scrolluje oba panely synchronizovaně |
| `Tab` | Přepne focus mezi levým a pravým panelem |
| `Ctrl+B` | Zapne/vypne split mode ručně |
| `Ctrl+X` | Zavře split a aktuální buffer |

Změněné řádky jsou zvýrazněny: **zelená** = nová verze (vlevo), **červená** = stará verze (vpravo). Prázdný tmavý řádek = přidaný/odebraný řádek (LCS alignment).

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
        ├── test_highlighters.cpp  # 37 testů — syntax highlighting
        ├── test_git.cpp           # 16 testů — git operace (smoke)
        ├── test_buffer.cpp        # 59 testů — Buffer (insert/delete/find/scroll/IO)
        ├── test_markdown.cpp      # 44 testů — markdown preview renderer
        ├── test_properties.cpp    # 8 testů  — property-based (Buffer invarianty, highlighter bounds)
        └── test_benchmark.cpp     # benchmarky výkonu (highlight, mdpreview, Buffer)
```

## Databáze

Ukládá se do `~/.local/share/nanoclone/recent.db` (SQLite, WAL mode).

Tabulka `recent_files`:
- `path` — absolutní cesta k souboru
- `last_opened` — Unix timestamp
- `cursor_row`, `cursor_col` — uložená pozice kurzoru

## Testování

### 1. Lokální spuštění

```bash
# Vše najednou (164 testů + benchmarky)
make test

# Jednotlivé suite
./build/tests/test_highlighters   # 37 testů
./build/tests/test_git            # 16 testů
./build/tests/test_buffer         # 59 testů
./build/tests/test_markdown       # 44 testů
./build/tests/test_properties     # 8 testů
./build/tests/test_benchmark      # výkon
```

| Suite | Testů | Co pokrývá |
|-------|-------|------------|
| `test_highlighters` | 37 | C++/Python/JSON/Markdown highlighting, factory |
| `test_git` | 16 | detect, fileDiff, fileStatuses, fileLog, branches (živé repo) |
| `test_buffer` | 59 | insert/delete, kurzor, find, scroll, load/save, join řádků |
| `test_markdown` | 44 | headings, bold/italic, inline code, links, code blocks, lists |
| `test_properties` | 8 | Buffer invarianty, highlight bounds, mdpreview šířka/počet |
| `test_benchmark` | — | highlight 1.1M ř/s · mdpreview 4.8M ř/s · insert 50k v 0.4ms |
| **Celkem** | **164** | |

### 2. Paměťové testování (AddressSanitizer)

Spustitelné bez TTY — detekuje memory leaks, use-after-free, buffer overflow:

```bash
# test_buffer
g++ -std=c++17 -fsanitize=address,undefined -g -O1 -fno-stack-protector -Isrc \
    src/tests/test_buffer.cpp src/buffer.cpp -o /tmp/test_buf_asan
ASAN_OPTIONS=detect_leaks=1 /tmp/test_buf_asan

# test_markdown
g++ -std=c++17 -fsanitize=address,undefined -g -O1 -fno-stack-protector -Isrc \
    src/tests/test_markdown.cpp src/markdown_preview.cpp -o /tmp/test_md_asan
ASAN_OPTIONS=detect_leaks=1 /tmp/test_md_asan

# test_highlighters
g++ -std=c++17 -fsanitize=address,undefined -g -O1 -fno-stack-protector -Isrc \
    src/tests/test_highlighters.cpp src/highlighters.cpp -o /tmp/test_hl_asan
ASAN_OPTIONS=detect_leaks=1 /tmp/test_hl_asan
```

**Výsledek: 0 memory errors, 0 leaks** na všech třech suitách.

### 3. CI na GitHubu (automatické)

Každý push do `main`/`master` automaticky spustí pipeline definovanou v `.github/workflows/ci.yml`:

1. **maze_game testy** — `python3 maze_game/test_maze.py` (19 testů)
2. **gnano build** — `make -C git-aware-nano`
3. **gnano testy** — `make test` (164 testů)
4. **ASAN** — test_buffer, test_markdown, test_highlighters s `-fsanitize=address,undefined`
5. **Benchmark** — kontrola výkonnostního regrese

Stav CI (zelené/červené checkmarky) je vidět přímo u každého commitu na GitHubu.

> **Poznámka:** Interaktivní část (`render()`, key handlers) potřebuje TTY — není unit-testovatelná. Pokrytí: ~60–65 % codebase.
