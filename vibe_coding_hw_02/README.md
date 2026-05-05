# Python Backend Developer Agent

Konfigurace Claude Code agenta specializovaného na Python backend vývoj.

## Zadání

> **Nasdílejte nastavení Vašeho kódovacího agenta, využijte MCP Servery, Skilly, Subagenty. NEPOUŽÍVEJTE PLUGINS ANI MARKETPLACE!**
>
> Kódovací agent: **Claude Code** ← zvoleno
>
> Deadline: 8. 5. 2026 | Max bodů: 100

## Použití

```bash
cd vibe_coding_hw_02
claude
```

Agent se automaticky nakonfiguruje podle `CLAUDE.md` a `.claude/` složky.

## Architektura

```
vibe_coding_hw_02/
├── CLAUDE.md                          ← instrukce agenta (stack, pravidla, workflow)
├── .claude/
│   ├── settings.json                  ← MCP servery
│   ├── agents/
│   │   ├── security-reviewer.md       ← subagent: bezpečnostní audit
│   │   ├── test-writer.md             ← subagent: generování testů
│   │   └── architect.md               ← subagent: návrh architektury
│   └── skills/
│       ├── python-backend/SKILL.md    ← FastAPI, SQLAlchemy, Pydantic best practices
│       └── api-design/SKILL.md        ← REST API konvence
├── mcp_servers/
│   └── python_linter/                 ← vlastní MCP server (ruff + mypy)
│       ├── server.py
│       ├── pyproject.toml
│       └── tests/test_server.py
└── todo_app/                          ← demo app vygenerovaná agentem
```

## MCP Servery

| Server | Spuštění | K čemu |
|--------|----------|--------|
| `git` | `uvx mcp-server-git` | git operace |
| `sqlite` | `uvx mcp-server-sqlite` | SQLite DB |
| `fetch` | `uvx mcp-server-fetch` | čtení dokumentace z webu |
| `postgres` | `uvx mcp-server-postgres` | Postgres DB |
| `python-linter` | `uv run` (vlastní) | ruff + mypy analýza kódu |

## Subagenti

### `security-reviewer`
Provede bezpečnostní audit Python kódu — SQL injection, slabé hashování, pickle, path traversal. Zavolej po každé změně autentizace nebo práci s hesly.

### `test-writer`
Napíše pytest testy pro zadaný kód — happy path, edge cases, error cases, security regrese.

### `architect`
Navrhne architekturu pro novou funkcionalitu — vrstvená architektura, DI, SRP, testovatelnost.

## Skills

### `python-backend`
Best practices pro FastAPI + SQLAlchemy async + Pydantic v2 — správné vzory pro endpointy, DB session, error handling, env konfiguraci.

### `api-design`
REST API konvence — URL struktura, HTTP status kódy, response formát, verzování, idempotence.

## Custom MCP Server — python-linter

Vlastní MCP server napsaný pomocí `FastMCP`. Poskytuje tři nástroje:

- `check_code` — spustí `ruff` a vrátí nalezené problémy
- `type_check` — spustí `mypy` a vrátí typové chyby
- `check_all` — oba najednou

```bash
# Spuštění MCP serveru
uv run --project mcp_servers/python_linter python mcp_servers/python_linter/server.py

# Testy MCP serveru
uv run --project mcp_servers/python_linter --group dev pytest mcp_servers/python_linter/tests/ -v
```

## Demo aplikace (todo_app)

FastAPI TODO list vygenerovaný agentem — ukázka co agent dokáže vytvořit s touto konfigurací.

```bash
cd todo_app
uv run pytest tests/ -v          # 10 testů
uv run uvicorn todo_app.main:app  # spuštění serveru
```

**Endpointy:**
- `GET /tasks` — seznam úkolů
- `POST /tasks` — vytvoření úkolu
- `PATCH /tasks/{id}/complete` — dokončení úkolu
- `DELETE /tasks/{id}` — smazání úkolu
