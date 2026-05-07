# Vibe Agentic Coding Robot Dreams Course Tasks

Repozitář obsahuje projekty z kurzu agentic coding, rozdělené podle zadání.

---

## HW Agentic Engineering – 1

### `agentic_engineering_hw_01/` — Multi-Agent Code Reviewer (Claude Agent SDK)

Multi-agent systém pro automatizované code review Python souborů.

- **Supervisor pattern** — orchestruje Quality Agent, Tests Agent a Security Swarm paralelně
- **Swarm pattern** — 5 autonomních security checkerů (SQL injection, secrets, deserializace, path traversal, auth)
- Používá `AgentDefinition` + `ClaudeSDKClient` + MCP tool pro AST analýzu kódu

```bash
cd agentic_engineering_hw_01
uv run code-reviewer examples/buggy_app.py
```

---

## Vibe Coding HW 02 — Nastavení kódovacích agentů

> Zadání: Nasdílejte nastavení Vašeho kódovacího agenta, využijte MCP Servery, Skilly, Subagenty.
> Kódovací agenti: **Codex** + **Claude Code**. Bez pluginů a marketplace.

### Splnění zadání (`coding-agent-setup/`)

| Požadavek | Splněno | Detail |
|---|---|---|
| Claude Code | ✅ | `claude-code/CLAUDE.md` + `settings.json` |
| Codex CLI | ✅ | `codex/AGENTS.md` + `config.yaml` |
| MCP Servery | ✅ | filesystem, github, brave-search, sqlite, memory |
| Subagenti | ✅ | Explore, Plan, General-purpose (paralelní) |
| Skilly / slash commands | ✅ | `/lesson`, `/check`, `/explain`, `/progress`, `/subagent-workflow` |
| Bez pluginů/marketplace | ✅ | pouze npx MCP servery |

### `coding-agent-setup/` — Python Learning Assistant

Konfigurace Claude Code + Codex CLI jako interaktivního Python mentora pro studenty.
Agent nevypisuje hotová řešení — klade otázky, spouští testy a sleduje pokrok studenta.

```bash
cd coding-agent-setup/workspace
claude    # spustí Python Learning Assistant
```

### `openfoam_agents/` — OpenFOAM Learning Assistant *(bonus/rozšíření)*

Stejná filozofie, jiná doména: agent provází studenty CFD simulacemi v OpenFOAM.
Navíc obsahuje `simulation.py` — živou ukázku multi-agent smyčky přes `claude -p` CLI.

- **4 agenti:** orchestrátor + Explore + Plan + General-purpose
- **5 MCP serverů:** filesystem, github, brave-search, sqlite, memory
- **5 slash příkazů:** `/case`, `/check`, `/explain`, `/run`, `/progress`
- **2 cvičení:** lid-driven cavity (icoFoam) + Pitz Daily turbulence (simpleFoam k-ε)
- **student-teacher simulace:** student agent doplňuje OpenFOAM soubory, učitel reaguje sókratovsky

```bash
cd openfoam_agents
claude            # spustí OpenFOAM Learning Assistant
python3 simulation.py   # spustí student-teacher smyčku
```

---

## Ostatní projekty

### `hw_01/` — OpenAI Tool Calling
Ukázka tool callingu přes OpenAI API, spouštěna přes `uv`.

### `nano-clone/` — Terminálový editor (C++17)
Textový editor inspirovaný nano. Více bufferů, boční file panel, syntax highlighting pro C++/Python/JS/TS/HTML/CSS, historie posledních souborů v SQLite.

### `git-aware-nano/` — Git-aware editor (C++17)
Rozšířená verze nano-clone s plnou git integrací. Syntax highlighting, Markdown preview, fuzzy finder, grep search, git gutter, blame, diff, log, branch picker, tree view.

```bash
cd git-aware-nano
make
./build/nanoclone [soubor]
```

### `maze_game/` — Terminálová hra bludiště (Python)
Hra pro terminál (SSH-friendly, čistý `curses`). Generátor bludiště Recursive Backtracker, BFS solver, 5 úrovní.

```bash
python3 maze_game/maze.py
```

### `vibe_coding_hw_02/` — Python Backend Developer Agent (Claude Code)
Konfigurace Claude Code agenta pro Python backend vývoj s vlastním `python-linter` MCP serverem.

```bash
cd vibe_coding_hw_02
claude
```
