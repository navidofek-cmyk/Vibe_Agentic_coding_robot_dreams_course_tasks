# Vibe Agentic Coding Robot Dreams Course Tasks

Repozitář obsahuje samostatné projekty vytvořené v kurzu agentic coding.

## Projekty

### `agentic_engineering_hw_01/` — Multi-Agent Code Reviewer (Claude Agent SDK)
Multi-agent systém pro automatizované code review Python souborů. Implementuje dva multi-agent patterny:
- **Supervisor pattern** — orchestruje Quality Agent, Tests Agent a Security Swarm paralelně
- **Swarm pattern** — 5 autonomních security checkerů (SQL injection, secrets, deserializace, path traversal, auth)

Používá `AgentDefinition` + `ClaudeSDKClient` + MCP tool pro AST analýzu kódu.

```bash
cd agentic_engineering_hw_01
uv run code-reviewer examples/buggy_app.py
```

### `vibe_coding_hw_02/` — Python Backend Developer Agent (Claude Code)
Konfigurace Claude Code agenta pro Python backend vývoj:
- **MCP servery:** git, sqlite, fetch, postgres + vlastní `python-linter` (ruff + mypy přes FastMCP)
- **Subagenti:** security-reviewer, test-writer, architect
- **Skills:** python-backend (FastAPI/SQLAlchemy), api-design (REST konvence)
- **Demo app:** FastAPI TODO list vygenerovaný agentem (10 testů)

```bash
cd vibe_coding_hw_02
claude  # spustí nakonfigurovaného agenta
```

### `openfoam_agents/` — OpenFOAM Learning Assistant (Claude Code + multi-agent simulace)
CFD výukový asistent postavený na Claude Code CLI. Agent provází studenty světem OpenFOAM od struktury case souborů po analýzu výsledků simulace.
- **4 agenti:** orchestrátor + Explore + Plan + General-purpose (paralelní generování testů)
- **5 MCP serverů:** filesystem, github, brave-search, sqlite, memory
- **5 slash příkazů:** `/case`, `/check`, `/explain`, `/run`, `/progress`
- **2 cvičení:** lid-driven cavity (icoFoam) + Pitz Daily turbulence (simpleFoam k-ε)
- **student-teacher simulace:** `simulation.py` — dva Claude agenti přes `claude -p` CLI, pytest feedback loop

```bash
cd openfoam_agents
claude   # spustí OpenFOAM Learning Assistant

# nebo spusť simulaci student vs. učitel:
python3 simulation.py
```

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
