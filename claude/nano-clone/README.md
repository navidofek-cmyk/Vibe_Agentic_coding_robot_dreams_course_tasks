# nanoclone

Terminálový textový editor inspirovaný nano, napsaný v C++17.  
Žádné externí závislosti — pouze standardní knihovna a kompilátor.

## Funkce

- Více otevřených souborů najednou (taby)
- Boční panel pro procházení souborového systému
- Hledání textu (`Ctrl+F`)
- Seznam naposledy otevřených souborů (`Ctrl+R`)
- Syntax highlighting pro C/C++, Python, JS/TS, HTML, CSS

## Závislosti

Žádné. Stačí `g++` s podporou C++17:

```bash
sudo apt-get install build-essential   # pokud ještě nemáš g++
```

## Sestavení

```bash
make          # release build → build/nanoclone
make debug    # debug build s -g
make run      # sestaví a spustí
make clean    # smaže build/
```

## Spuštění

```bash
./build/nanoclone               # prázdný editor
./build/nanoclone soubor.cpp    # otevře soubor (automaticky syntax highlight)
./build/nanoclone a.txt b.py    # otevře více souborů jako taby
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
| `Ctrl+A` | Předchozí tab |
| `Ctrl+T` | Další tab |
| `PgUp / PgDn` | Rychlá navigace |
| `Home / End` | Začátek / konec řádku |

### Boční panel

| Zkratka | Akce |
|---------|------|
| `↑ / ↓` | Navigace |
| `Enter` | Otevřít soubor / vstoupit do adresáře |
| `Esc` | Zpět do editoru |

### Při zavírání neuloženého souboru

| Klávesa | Akce |
|---------|------|
| `S` | Uložit a zavřít |
| `D` | Zahodit změny |
| `C` / `Esc` | Zrušit |

## Struktura projektu

```
nano-clone/
├── Makefile
├── CMakeLists.txt
└── src/
    ├── main.cpp           # vstupní bod
    ├── highlight.h        # ColorSpan, SyntaxHighlighter base class
    ├── highlighters.h/cpp # C/C++, Python, JS/TS, HTML, CSS + factory
    ├── buffer.h/cpp       # textový buffer, kurzor, scroll, vyhledávání
    ├── database.h/cpp     # seznam posledních souborů (~/.local/share/nanoclone/)
    ├── filepanel.h/cpp    # prohlížeč adresářů (boční panel)
    ├── terminal.h/cpp     # raw termios, ANSI I/O, Key:: konstanty
    └── editor.h/cpp       # render loop, správa tabů, mode dispatch
```

## Přidání syntax highlighting

Implementuj třídu děděnou z `SyntaxHighlighter`:

```cpp
#include "highlight.h"

class MyHighlighter : public SyntaxHighlighter {
public:
    std::vector<ColorSpan> highlight(const std::string& line, int row) override {
        // vrať seznam ColorSpan { col, len, HL::KEYWORD/STRING/... }
    }
    bool supportsFile(const std::string& filename) const override {
        return filename.ends_with(".xyz");
    }
};
```

Dostupné typy barev (`HL::`): `KEYWORD`, `STRING`, `COMMENT`, `NUMBER`, `PREPROC`, `TYPE`, `TAG`, `ATTR`, `SELECTOR`, `PROPERTY`.

Zaregistruj ji v `makeHighlighter()` v `highlighters.cpp`.

## Naposledy otevřené soubory

Ukládají se do `~/.local/share/nanoclone/recent.txt` (plain text, bez SQLite).
