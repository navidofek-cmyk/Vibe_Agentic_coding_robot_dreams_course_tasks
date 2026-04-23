# PROMPT PRO CLAUDE CODE — projekt `gnano`

> Zkopíruj celý obsah tohoto souboru (od čáry níž) a vlož do Claude Code
> v adresáři naklonovaného repa `nano-clone/`. Claude si projde kód,
> navrhne plán a zeptá se na otevřené otázky.

---

Pracuješ na projektu **`gnano`** (pracovní název, výchozí kód je
v repu `nano-clone`) — terminálový textový editor v C++17 s ncurses
a SQLite. Zdroj je v aktuálním adresáři.

## EXISTUJÍCÍ ARCHITEKTURA

```
nano-clone/
├── Makefile
├── CMakeLists.txt
├── README.md
└── src/
    ├── main.cpp          # vstupní bod
    ├── editor.{h,cpp}    # ncurses UI, taby, vstup
    ├── buffer.{h,cpp}    # textový buffer, kurzor, scroll, search
    ├── filepanel.{h,cpp} # file browser (boční panel)
    ├── database.{h,cpp}  # SQLite — recent files
    └── highlight.h       # interface SyntaxHighlighter (zatím stub)
```

**Závislosti:** `libncurses-dev`, `libsqlite3-dev`
**Existující klávesy:** `Ctrl+S` save, `Ctrl+X` close, `Ctrl+N` new,
`Ctrl+O` panel, `Ctrl+F` find, `Ctrl+R` recent, `F3/F4` taby,
`PgUp/PgDn`, `Home/End`.

## CO CHCI PŘIDAT

Čtyři velké oblasti. Dodržuj styl existujícího kódu, C++17 features,
RAII, žádné zbytečné abstrakce. Žádný refactoring existujícího kódu
bez mé explicitní žádosti.

---

## KROK 0 — PRŮZKUM A NÁVRH (dělej to JAKO PRVNÍ)

**0.1)** Projdi existující kód. Napiš shrnutí:
- jak vypadá render loop v `editor.cpp`
- jak se vykresluje jeden řádek (kam napojit highlight + gutter)
- jak je definovaný `SyntaxHighlighter` a `ColorSpan` v `highlight.h`
- jak se zpracovávají klávesy (kvůli novým zkratkám)
- jak je uložený text v bufferu (vector<string>? rope? gap buffer?)

**0.2)** Vygeneruj **dva file tree**:
- **A)** aktuální stav projektu (s krátkým popisem každého souboru)
- **B)** cílový stav po všech rozšířeních s označením `[NEW]` / `[MODIFIED]`

**0.3)** Navrhni architekturu nových modulů. Pro každý modul:
- název souboru
- veřejné API (funkce/třídy)
- jednu větu: za co je zodpovědný

**0.4)** Zeptej se mě na otevřené otázky PŘED začátkem implementace:
- jaký výkon highlightu očekávám (kolik řádků plynule?)
- preferuju vlastní scanner (rychlost) nebo `std::regex` (rychlost vývoje)?
- má být git gutter zapínatelný/vypínatelný v configu?
- je OK přidat `libgit2` závislost?
- unicode symboly (•☐┌) nebo ASCII fallback (*[ ]+)?
- mám použít jméno `gnano` nebo něco jiného?

**NEPOUŠTĚJ SE DO IMPLEMENTACE, dokud neodsouhlasím návrh.**

---

## OBLAST 1 — SYNTAX HIGHLIGHTING

Konkrétní highlightery dědící ze `SyntaxHighlighter`. Factory podle
přípony.

**Jazyky:** C, C++, Python, TypeScript, JavaScript, CSS, HTML, Markdown,
JSON, YAML, Shell, Makefile, plain text fallback.

**Pro každý jazyk minimálně:**
- klíčová slova
- řetězce (včetně escape sekvencí, template stringů kde to dává smysl)
- komentáře (řádkové i blokové)
- čísla (hex, binární, float, suffixy jako `u`, `L`, `f`)
- operátory
- názvy funkcí (volání i definice)
- typy a built-iny (C/C++: `int`, `size_t`; Python: `list`, `dict`)
- preprocessor direktivy (C/C++: `#include`, `#define`)
- dekorátory (Python `@`, TS `@Component`)
- HTML: tagy a atributy zvlášť od obsahu

**Klíčová rozhodnutí:**
- Žádné externí knihovny (ne `tree-sitter`, ne Pygments). Čistý C++.
- Stavový automat + `std::regex`, regexy prekompilované jako statické členy.
- Pro C/C++/Python zvaž ruční scanner (výkon na velkých souborech).
- Multi-line tokeny (`/* */`, `"""..."""`, ` ```...``` `): potřebuješ
  **line state** předávaný mezi řádky. Navrhni mechanismus v bufferu
  (např. `std::vector<int> lineStates` udržovaný při editaci).

**Barvy:**
- Semantické tokeny: `KEYWORD`, `STRING`, `COMMENT`, `NUMBER`, `TYPE`,
  `PREPROCESSOR`, `DECORATOR`, `TAG`, `ATTRIBUTE`, `OPERATOR`, `FUNCTION`.
- Mapování přes `Theme` třídu, default pro tmavý terminál.
- Konfigurace z `~/.config/gnano/theme.conf` až v druhé fázi.

**Struktura:**
```
src/syntax/
├── highlighter_base.h       # utility, LineState, helpers
├── factory.{h,cpp}          # getHighlighterForFile(const std::string&)
├── theme.{h,cpp}            # Token → ncurses color pair
├── cpp_highlighter.{h,cpp}
├── c_highlighter.{h,cpp}
├── python_highlighter.{h,cpp}
├── ts_highlighter.{h,cpp}
├── js_highlighter.{h,cpp}
├── css_highlighter.{h,cpp}
├── html_highlighter.{h,cpp}
├── markdown_highlighter.{h,cpp}
├── json_highlighter.{h,cpp}
├── yaml_highlighter.{h,cpp}
├── shell_highlighter.{h,cpp}
└── makefile_highlighter.{h,cpp}
```

---

## OBLAST 2 — MARKDOWN DVA REŽIMY

`.md` soubory mají `Ctrl+P` přepínač mezi **Source** a **Preview** režimem.

### 2.1) SOURCE MODE (výchozí, editovatelný)

- Standardní `markdown_highlighter` dědící ze `SyntaxHighlighter`
- Zvýrazněno:
  - `#` nadpisy (každá úroveň jinou barvou)
  - `**bold**` markery
  - `*italic*` / `_italic_` markery
  - `` `inline code` ``
  - ` ``` code blocks ``` `
  - `> citace`
  - `- * +` odrážky, `1.` číslované
  - `[text](url)` — text jedna barva, URL jiná
  - `![alt](url)` obrázky
  - `---` / `***` horizontální čára
  - `| tabulky |` — rámečky barevně
  - `- [ ]` / `- [x]` task listy

### 2.2) PREVIEW MODE (read-only, render)

- **Nadpisy:** `A_BOLD` + barva, pod H1/H2 linka `─`
- **Bold:** text s `A_BOLD`, markery skryté
- **Italic:** text s `A_UNDERLINE`, markery skryté
- **Inline code:** jiná barva + pozadí
- **Code blocks:** celý blok jiná barva; pokud je `lang` známý,
  zavolat `getHighlighterForLanguage(lang)` a aplikovat jeho barvy uvnitř
- **Odrážky:** `- item` → `  • item`, `1. item` → `  1. item`
- **Citace:** `> quote` → `│ quote` dim barvou
- **Odkazy:** `[text](url)` → `text` modře `A_UNDERLINE`, URL
  zobraz ve status baru když je kurzor nad odkazem
- **Obrázky:** `![alt](url)` → `[📷 alt]`
- **Horizontální čára:** `---` → celý řádek `─`
- **Tabulky:** zarovnat sloupce, nakreslit `┌─┬─┐ │ ├─┼─┤ └─┴─┘`
- **Task listy:** `- [ ]` → `☐`, `- [x]` → `☑`
- **Wrap** dlouhých řádků podle šířky terminálu

**ASCII fallback** když `LANG` není UTF-8: `•` → `*`, `☐` → `[ ]`,
`☑` → `[x]`, `─` → `-`, `│` → `|`, `┌` → `+`, ...

**Implementace:** flag `renderMode` na bufferu, switch v render funkci editoru.

**MVP IMPLEMENTACE obsahuje JEN:** nadpisy, bold, italic, odrážky,
code block monochromně, citace. Tabulky, nested listy, syntax
highlighting uvnitř code bloků — zvláštní etapa později.

---

## OBLAST 3 — GIT INTEGRACE

**Závislost:** `libgit2` přes `pkg-config --libs libgit2`.
**NE subprocess volání `git`.**

**Moduly:**
```
src/git/
├── git_repo.{h,cpp}      # detekce repa, lazy open, RAII wrapper
├── git_diff.{h,cpp}      # diff vs HEAD per-line
├── git_blame.{h,cpp}     # blame pro soubor
└── git_branches.{h,cpp}  # list, checkout, current
```

### 3.1) STATUS BAR

Rozšiř existující status bar o:
- jméno větve
- `●` indikátor (modified/staged/clean — různé barvy)
- `↑N ↓M` ahead/behind vs upstream (pokud existuje)

### 3.2) GIT GUTTER

Mezi číslem řádku a textem:
- `+` zeleně (přidáno vs HEAD)
- `~` žlutě (změněno)
- `_` červeně (smazáno mezi řádky)

Refresh: po save + na F5. **NE při každém keystroku.**

### 3.3) PŘÍKAZY (prefix `Ctrl+G`, pak písmeno)

| Zkratka | Akce |
|---|---|
| `Ctrl+G B` | blame aktuálního řádku (popup/statusbar) |
| `Ctrl+G D` | diff souboru vs HEAD (nový read-only buffer) |
| `Ctrl+G L` | log souboru → výběr commitu → jeho diff |
| `Ctrl+G S` | stage/unstage aktuálního souboru |
| `Ctrl+G C` | přepnout větev (výběr ze seznamu) |
| `Ctrl+G T` | repo tree v commitu (viz 4.4) |

### 3.4) GRACEFUL DEGRADATION

- Ne-git soubor: všechny git featury schované, gutter neviditelný
- `libgit2` chyba: log do status baru, pokračuj dál

---

## OBLAST 4 — PROHLEDÁVÁNÍ A PROCHÁZENÍ REPA

Tohle je **nejvíc user-facing užitečná část**. Dělej co nejdřív.

### 4.1) GIT-AWARE FILE PANEL (rozšíření existujícího)

U souborů v git repu přidej značky:
- `M` žlutě — modified
- `+` zeleně — untracked
- `S` modře — staged
- `!` červeně — conflict
- `.` dim — ignored (zobrazit volitelně, toggle `Ctrl+H`)

Refresh po save / F5.

### 4.2) FUZZY FILE FINDER (`Ctrl+P`)

- Popup: input řádek nahoře, seznam souborů pod tím
- Zdroj:
  - git repo: `git ls-files` ekvivalent přes libgit2 (index + untracked, bez ignored)
  - mimo git: rekurzivní scan (ignoruj `.git`, `build`, `node_modules`, `*.o`)
- Fuzzy matching: vlastní subsequence match + scoring
  - souvislé shody vyšší skóre
  - shoda v názvu souboru > shoda v cestě
  - CamelCase matching (`mVbf` → `myVeryBestFile.cpp`)
- `Enter` otevře v novém tabu, `Esc` zavře

### 4.3) GREP SEARCH (`Ctrl+T`)

*(Ctrl+Shift+F v terminálu nefunguje spolehlivě, proto Ctrl+T)*

- Input řádek: pattern
- Přepínače (Tab/Alt+písmeno): case-sensitive, whole word, regex on/off
- Výsledky: seznam `soubor:řádek: kontext` s zvýrazněným matchem
- `Enter` otevře soubor na tom řádku
- Implementace v threadu, výsledky proudí, přerušit `Esc`

### 4.4) REPO TREE V COMMITU (`Ctrl+G T`)

- Strom souborů tak, jak vypadal v commitu (hash/výběr ze seznamu)
- Značky vůči parent commitu:
  - `+` přidaný (zelená)
  - `M` modifikovaný (žlutá)
  - `-` smazaný (červená, strikethrough)
  - `R` přejmenovaný (modrá, `old → new`)
- `Enter`: obsah souboru v tom commitu (read-only buffer,
  titulek `filename @ abc1234`)

### 4.5) POJMENOVÁNÍ V UI

Status bar ukazuje aktivní panel:
`[Files] [Find] [Grep] [Tree@abc1234]`

Přepínač mezi panely: `F6` cykluje.

---

## ETAPY IMPLEMENTACE

Každá etapa končí **funkčním buildem** (`make clean && make` projde).
Po každé etapě se **ZASTAV**, napiš co je hotové, co chybí, čekej na
zpětnou vazbu.

| # | Etapa | Obsah |
|---|---|---|
| A | Syntax MVP | Theme + factory + C++ + Python highlighter + testy |
| B | Syntax plný | zbylé jazyky (C, JS, TS, CSS, HTML, JSON, YAML, Shell, Makefile) |
| C | File finder | fuzzy finder (`Ctrl+P`) — **udělej brzy, je to nejvíc užitečné!** |
| D | Grep search | fulltext v repu (`Ctrl+T`) |
| E | Markdown src | markdown highlighter pro source mode |
| F | Markdown prev | preview mode MVP (nadpisy/bold/italic/odrážky/code/quote) |
| G | Git status | repo detection + statusbar (větev, ●, ↑↓) |
| H | Git gutter | diff vs HEAD v gutteru |
| I | Git-aware files | značky M/+/S/! v file panelu |
| J | Git blame+diff | `Ctrl+G B`, `Ctrl+G D` |
| K | Git log+branch | `Ctrl+G L`, `Ctrl+G C`, `Ctrl+G S` |
| L | Git tree | `Ctrl+G T` — repo tree v commitu |
| M | (bonus) Markdown+ | tabulky, nested listy, highlight v code blocích |
| N | (bonus) Theme cfg | načítání theme.conf |

---

## PRAVIDLA

- **Malé commity** po každé dílčí funkci, jasné zprávy
- **Žádný refactoring** existujícího kódu bez explicitní žádosti
- **Testy** pro každý highlighter (vstup řádek → očekávané `ColorSpan`y)
- **Smoke testy** pro git modul (testovací repo → ověř načtení větví, diffu)
- **Aktualizuj README** po každé etapě (nové featury, zkratky, file tree)
- **Aktualizuj Makefile i CMakeLists.txt** souběžně
- **Dodržuj naming convention** existujícího kódu
- **Nepokračuj na další etapu**, dokud aktuální není potvrzená

---

## ZAČÍNÁME

Začni **KROKEM 0**. Projdi kód, vypiš file trees, navrhni architekturu,
polož otázky. Čekej na odpovědi.
