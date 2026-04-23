# Vibe Agentic Coding Robot Dreams Course Tasks

Repozitář obsahuje samostatné projekty vytvořené v kurzu agentic coding.

## Projekty

### `hw_01/` — OpenAI Tool Calling
Ukázka tool callingu přes OpenAI API, spouštěna přes `uv`.

### `nano-clone/` — Terminálový editor (C++17)
Textový editor inspirovaný nano. Více bufferů, boční file panel, syntax highlighting pro C++/Python/JS/TS/HTML/CSS, historie posledních souborů v SQLite.

### `claude/git-aware-nano/` — Git-aware editor (C++17)
Rozšířená verze nano-clone s plnou git integrací. Bez ncurses, čisté ANSI. Syntax highlighting (C++/Python/JSON/Markdown/YAML/Shell), Markdown preview, fuzzy finder, grep search, git gutter, blame, diff, log, branch picker, tree view.

**Sestavení a spuštění:**
```bash
cd claude/git-aware-nano
make
./build/nanoclone [soubor]
```

### `maze_game/` — Terminálová hra bludiště (Python)
Hra pro terminál (SSH-friendly, čistý `curses`). Generátor bludiště algoritmem Recursive Backtracker, BFS solver, 5 úrovní s rostoucí obtížností.

**Herní prvky:** klíče `k`, mince `•`, speed boost `>`, fog of war, leaderboard v `scores.json`

**Spuštění:**
```bash
python3 maze_game/maze.py
```

**Testy:**
```bash
python3 maze_game/test_maze.py
```
