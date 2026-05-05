# Chat History — TODO API projekt

**Datum:** 2026-05-05

---

## Zadání

> Vytvoř FastAPI aplikaci — jednoduchý REST API pro správu úkolů (TODO list).
> Endpoint pro vytvoření, seznam, dokončení a smazání úkolu.
> SQLite databáze přes SQLAlchemy async. Pydantic schémata. Type hints všude.
> Napiš i pytest testy. Použij python-linter MCP server pro ověření kódu.

---

## Co vzniklo

Struktura projektu:

```
todo_app/
├── pyproject.toml
├── todo_app/
│   ├── database.py     # async engine, session factory, get_db dependency
│   ├── models.py       # SQLAlchemy Task model (mapped_column, Mapped)
│   ├── schemas.py      # Pydantic TaskCreate + TaskResponse
│   ├── main.py         # FastAPI app, lifespan (auto-create tabulky)
│   └── routers/
│       └── tasks.py    # 4 endpointy
└── tests/
    ├── conftest.py     # in-memory SQLite, dependency override, AsyncClient
    └── test_tasks.py   # 10 testů
```

### Endpointy

| Metoda | URL | Akce |
|--------|-----|------|
| `POST` | `/tasks` | Vytvoří úkol |
| `GET` | `/tasks` | Vrátí seznam (desc by created_at) |
| `PATCH` | `/tasks/{id}/complete` | Označí jako hotový |
| `DELETE` | `/tasks/{id}` | Smaže úkol |

### Stack

- Python 3.12+, FastAPI, SQLAlchemy 2.0 async, aiosqlite, Pydantic v2
- pytest + pytest-asyncio (asyncio_mode = auto), httpx AsyncClient
- ruff + mypy (strict)

### Výsledky ověření

- **ruff**: All checks passed!
- **mypy**: žádné typové chyby
- **pytest**: 10 passed in 0.31s

---

## Spuštění

```bash
cd todo_app
uv run uvicorn todo_app.main:app --reload --port 8989
```

- API: http://localhost:8989
- Swagger docs: http://localhost:8989/docs

### Příklady curl

```bash
curl -X POST http://localhost:8989/tasks -H "Content-Type: application/json" -d '{"title": "Nakoupit"}'
curl http://localhost:8989/tasks
curl -X PATCH http://localhost:8989/tasks/1/complete
curl -X DELETE http://localhost:8989/tasks/1
```
