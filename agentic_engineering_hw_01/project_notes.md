# Code Review Supervisor — Project Notes

## Co projekt dělá

Multi-agent systém postavený na **Claude Agent SDK**, který z Python souboru vytvoří
automatizovaný code review report. Supervisor orchestruje tři specializované sub-agenty
běžící paralelně a výsledky syntetizuje do Markdown reportu s prioritizovaným akčním plánem.

```bash
uv run code-reviewer examples/buggy_app.py
# výstup: examples/buggy_app.review.md
```

## Architektura

Dva multi-agent patterny v jednom projektu:

```
Uživatel zadá soubor
        │
  [Supervisor]  ← orchestruje, syntetizuje, deduplikuje
        │
   ┌────┴──────────────────────┐
   │            │              │
[Security     [Quality]     [Tests]     ← 3 větve paralelně (asyncio.gather)
  Swarm]        Agent         Agent
   │             │ (+ MCP)
   │             └── analyze_code_structure (AST, in-process MCP server)
   │
   ├── [SQL Injection]
   ├── [Hardcoded Secrets]
   ├── [Deserializace]         ← Swarm: 5 checkerů, stagger 1.5 s, bez vzájemné koordinace
   ├── [Path Traversal]
   └── [Auth & Autorizace]
                │
          [Supervisor]
          - deduplikuje překrývající se nálezy
          - sestaví prioritizovaný akční plán
          - celkové hodnocení 1–10
                │
          Markdown report
```

## Moduly

### `src/code_reviewer/__main__.py`
CLI entry point. Načte `.env`, zpracuje argument (cesta k souboru), spustí
`CodeReviewSupervisor.review()` a uloží výsledek jako `<soubor>.review.md`.

### `src/code_reviewer/supervisor.py`
Jádro projektu:
- Konstanty `_QUALITY_SYSTEM`, `_TESTS_SYSTEM`, `_SUPERVISOR_SYSTEM` — system prompty
- `AgentDefinition` konstanty `_QUALITY_AGENT`, `_TESTS_AGENT`, `_SUPERVISOR_AGENT`
- `_sdk_env()` — předá `ANTHROPIC_API_KEY` do subprocess
- `_run_agent()` — generická funkce pro spuštění jednoho sub-agenta přes `ClaudeSDKClient`
- `_run_supervisor()` — spustí supervisor s výsledky všech agentů vloženými do promptu
- `CodeReviewSupervisor` — třída orchestrující celý pipeline
- Budget limity: $0.25 per Quality/Tests agent, $0.35 pro supervisor

### `src/code_reviewer/security_swarm.py`
Security Sub-Swarm (Swarm pattern):
- 5 `AgentDefinition` konstant + odpovídající system prompty
- `CHECKERS: dict[str, AgentDefinition]` — mapa jméno → definice (zachovává pořadí)
- `_run_checker()` — spustí jednoho checkera, max 3 otáčky, budget $0.15
- `run_security_swarm()` — staggeovaný start (1.5 s mezi checkery), `asyncio.gather()`,
  agregace do sekcí `#### <jméno>`
- Vlastní kopie `_sdk_env()` — duplikát (viz technický dluh)

### `src/code_reviewer/mcp_tools.py`
In-process MCP server pro Quality Agenta:
- `@tool analyze_code_structure` — přijme `file_path`, parsuje přes `ast`, vrátí počty
  funkcí/tříd/metod/importů jako Markdown text
- `CODE_ANALYSIS_SERVER = create_sdk_mcp_server(...)` — instance předávaná jako
  `mcp_servers={"code_analysis": ...}` do `ClaudeAgentOptions`
- `MCP_TOOL_NAME = "mcp__code_analysis__analyze_code_structure"`

### `examples/buggy_app.py`
Demo soubor s **úmyslnými** bezpečnostními chybami pro testování revieweru:
SQL injection ×4, hardcoded secrets, pickle RCE, path traversal, MD5, 10 parametrů.

## Konvence v kódu

| Oblast | Konvence |
|--------|----------|
| Jazyk komentářů a docstringů | **česky** |
| Identifikátory | angličtina / Python standard |
| System prompty | `_XXX_SYSTEM` (string konstanta, privátní) |
| AgentDefinition konstanty | `_XXX_AGENT` |
| Veřejné mapy/konstanty | `VŠECHNA_VELKÁ` |
| Interní pomocné funkce | `_run_xxx` |
| Budget konstanty | `MAX_BUDGET_XXX_USD` |
| Import budoucnosti | `from __future__ import annotations` ve všech modulech |
| Paralelismus | `async/await` + `asyncio.gather()` |

## Build, test, spuštění

```bash
# Instalace
uv sync

# Review souboru
uv run code-reviewer cesta/k/souboru.py

# Demo
uv run code-reviewer examples/buggy_app.py

# Unit testy bez API (21 testů, mockovaný SDK)
uv run pytest tests/ -k "not integration"

# Integrační test s reálným API
uv run pytest tests/ -m integration
```

API klíč patří do `.env` jako `ANTHROPIC_API_KEY=sk-ant-...` (gitignored).

## Technický dluh a slabá místa

### Duplikace `_sdk_env()`
Funkce je definována samostatně v `supervisor.py` i `security_swarm.py`.
Řešení: přesunout do `__init__.py` nebo `utils.py`.

### Swarm není plně paralelní
`run_security_swarm()` staggeuje start s `await asyncio.sleep(1.5)` — tasks se vytváří
postupně. Celkový čas = 4 × 1.5 s + čas nejpomalejšího. Stagger je záměrný (rate limiting),
ale implementace může být čistší (vytvořit všechny tasks najednou, pak sleepovat uvnitř).

### Žádné ošetření selhání jednoho agenta
`asyncio.gather()` bez `return_exceptions=True` — výjimka jednoho agenta (timeout,
rate limit) shodí celý review. Mělo by být obaleno v try/except nebo
`gather(..., return_exceptions=True)` s fallback textem.

### Neomezená délka vstupu pro Supervisor
Supervisor dostane konkatenaci výstupů všech tří agentů jako jeden prompt.
Pro velké soubory může přesáhnout context window nebo způsobit vysoké náklady.
Chybí truncation nebo chunking.

### `mcp_tools.py` — překryv functions/methods v AST analýze
`analyze_code_structure` uvádí všechny `FunctionDef` do `functions` listu bez ohledu
na to, zda jsou top-level nebo metody třídy — výpis se překrývá s `methods`.

### Žádný retry mechanismus
Při selhání API volání (network error, 529 overload) agent okamžitě skončí chybou.
Chybí exponential backoff.

### Testy neověřují MCP integraci
`test_supervisor.py` mockuje celý `ClaudeSDKClient`, takže nepokrývá cestu přes
`CODE_ANALYSIS_SERVER`. Pro MCP větev chybí dedikované testy.
