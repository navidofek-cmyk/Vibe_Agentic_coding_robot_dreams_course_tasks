# Terminal Maze Game — Python, SSH-friendly

Vytvoř hratelnou bludiště hru pro terminál v čistém Pythonu.
Hra musí fungovat přes SSH (žádné GUI, žádné externí knihovny kromě `curses` ze stdlib).

---

## PRAVIDLA

- Pouze `curses` (stdlib) — žádný `pygame`, `blessed`, `rich`, `urwid`
- Spustitelné přes `python maze.py`
- Funguje na Linux i macOS (přes SSH i lokálně)
- Bludiště musí být vždy **řešitelné** — ověř BFS před startem každé úrovně
- Každá etapa = samostatný git commit s jasnou zprávou
- Testy: unit test pro generátor bludiště + BFS solver

---

## ETAPA 1 — MVP

- Bludiště 20×10 generované algoritmem **Recursive Backtracker** (DFS)
- Hráč `@`, zdi `█`, cíl `X`, prázdno ` `
- Ovládání: WASD nebo šipky
- HUD nahoře: počet kroků, uplynulý čas
- Výhra: obrazovka "You win!  Steps: N  Time: Xs"
- Celá hra v jednom souboru `maze.py`

## ETAPA 2 — Úrovně

- 5 úrovní s rostoucí obtížností (větší bludiště: 20×10 → 60×20)
- Mezi úrovněmi obrazovka "Level N complete!"
- Celkové skóre = součet kroků přes všechny úrovně (méně = lepší)

## ETAPA 3 — Herní prvky

- Klíče `k` — musíš sebrat všechny před vstupem do cíle
- Zamčené dveře `D` — otevřou se po sebrání klíče
- Mince `•` — snižují výsledné skóre (-10 bodů za kus)

## ETAPA 4 — Leaderboard

- Highscore tabulka uložená do `scores.json`
- Top 5 výsledků zobrazených po konci hry
- Záznam: jméno hráče (zadá se při startu), skóre, datum

---

## TECHNICKÉ POŽADAVKY

- `TERM=xterm-256color` — použij 256 barev kde to dává smysl
- Zachytávej `curses.KEY_RESIZE` a překresli při změně velikosti terminálu
- Generátor bludiště a BFS solver jako samostatné funkce (testovatelné bez `curses`)
- Testy v `test_maze.py` spustitelné přes `python test_maze.py`
