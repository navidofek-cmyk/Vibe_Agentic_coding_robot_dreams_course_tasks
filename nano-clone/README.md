# nanoclone

Terminálový textový editor inspirovaný nano, napsaný v C++17.

## Funkce

- Více otevřených souborů najednou (taby)
- Boční panel pro procházení souborového systému
- Hledání textu
- Seznam naposledy otevřených souborů (uložen v SQLite)
- Připraveno pro budoucí syntax highlighting

## Závislosti

```bash
sudo apt-get install libncurses-dev libsqlite3-dev
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
| `F3` | Přepnout na předchozí tab |
| `F4` | Přepnout na další tab |
| `PgUp / PgDn` | Rychlá navigace |
| `Home / End` | Začátek / konec řádku |

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

## Struktura projektu

```
nano-clone/
├── Makefile
├── CMakeLists.txt
└── src/
    ├── main.cpp          # vstupní bod
    ├── highlight.h       # interface pro syntax highlighting (budoucí rozšíření)
    ├── buffer.h/cpp      # textový buffer, kurzor, scroll, vyhledávání
    ├── database.h/cpp    # SQLite — seznam posledních souborů
    ├── filepanel.h/cpp   # prohlížeč adresářů
    └── editor.h/cpp      # ncurses UI, správa tabů, vstup
```

## Přidání syntax highlighting

Implementuj třídu děděnou z `SyntaxHighlighter` v `highlight.h`:

```cpp
class MyCppHighlighter : public SyntaxHighlighter {
public:
    std::vector<ColorSpan> highlight(const std::string& line, int row) override;
    bool supportsFile(const std::string& filename) const override;
};
```

A předej ji bufferu:

```cpp
buffer->setHighlighter(new MyCppHighlighter());
```

`ColorSpan` určuje sloupec, délku a ncurses color pair pro každý zvýrazněný úsek.

## Naposledy otevřené soubory

Ukládají se do `~/.local/share/nanoclone/recent.db` (SQLite).
