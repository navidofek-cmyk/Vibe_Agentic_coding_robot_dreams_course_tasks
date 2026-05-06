# Code Review Supervisor — Project Context

## Co tento projekt dělá

Multi-agent systém postavený na **Claude Agent SDK**, který z Python souboru vytvoří
automatizovaný code review report. Supervisor orchestruje tři specializované sub-agenty
běžící paralelně a výsledky syntetizuje do Markdown reportu s prioritizovaným akčním plánem.

```bash
uv run code-reviewer examples/buggy_app.py
# výstup: examples/buggy_app.review.md
```

## Architektura (Supervisor + Security Sub-Swarm)

Dva multi-agent patterny v jednom projektu:

```
Uživatel zadá soubor
        │
  [Supervisor]  ← orchestruje, syntetizuje, deduplikuje
        │
   ┌────┴──────────────────────┐
   │            │              │
[Security     [Quality]     [Tests]     ← 3 větve paralelně
  Swarm]        Agent         Agent       asyncio.gather()
   │             │ (+ MCP)
   │             └── analyze_code_structure (AST, in-process MCP server)
   │
   ├── [SQLInjectionChecker]   ┐
   ├── [SecretsChecker]        │
   ├── [DeserializationChecker]├── Swarm (5 checkerů, stagger 1.5 s,
   ├── [PathTraversalChecker]  │        bez vzájemné koordinace)
   └── [AuthChecker]           ┘
   │            │              │
   └────────────┴──────────────┘
                │ výsledky jako plain text
          [Supervisor]
          - deduplikuje překrývající se nálezy
          - sestaví prioritizovaný akční plán
          - celkové hodnocení 1–10
                │
          Markdown report
```

## Agenti — role a system prompty

### Security Sub-Swarm (`security_swarm.py`)
5 specializovaných checkerů — každý jako `AgentDefinition`, každý hledá jeden typ zranitelnosti:

| Checker | System prompt | Hledá |
|---------|--------------|-------|
| `_SQL_AGENT` | `_SQL_SYSTEM` | f-stringy, %-formát, `.format()` v SQL dotazech |
| `_SECRETS_AGENT` | `_SECRETS_SYSTEM` | hardcoded hesla, API klíče, tokeny |
| `_DESERIALIZATION_AGENT` | `_DESERIALIZATION_SYSTEM` | pickle.loads(), yaml.load() bez SafeLoader |
| `_PATH_TRAVERSAL_AGENT` | `_PATH_TRAVERSAL_SYSTEM` | open() s uživatelským vstupem bez normalizace |
| `_AUTH_AGENT` | `_AUTH_SYSTEM` | MD5/SHA1 pro hesla, chybějící autorizace |

### QualityAgent (`supervisor.py` — `_QUALITY_AGENT`)
Kontroluje kódovou kvalitu podle Python best practices. Před analýzou volá
`mcp__code_analysis__analyze_code_structure` (AST metadata souboru).

### TestsAgent (`supervisor.py` — `_TESTS_AGENT`)
Navrhuje pytest testy pro každou funkci/třídu včetně security regresních testů
označených `# REGRESNÍ TEST — BUG`.

### Supervisor (`supervisor.py` — `_SUPERVISOR_AGENT`)
Syntetizuje výsledky všech tří agentů — deduplikuje, prioritizuje, hodnotí (1–10).

## Kde žijí instrukce agentů

**`system_prompt`** — string konstanta v Pythonu (`_QUALITY_SYSTEM`, `_SQL_SYSTEM` atd.).
Agent ji dostane vždy při každém spuštění, bez výjimky.

**`docs/skills/`** — SKILL.md soubory se stejným obsahem jako referenční dokumentace.
Kód je nepoužívá — jsou tam pro přehlednost a jako základ pro případný budoucí refaktor.

Poznatky ze zkoušení `skills=` parametru v `ClaudeAgentOptions`:
- Agent skill zavolá autonomně jen pokud ho potřebuje
- Pokud má plný `system_prompt`, skill typicky ignoruje (Quality Agent ho nikdy nezavolal)
- Tests Agent skill zavolal — jeho instrukce jsou více "domain knowledge" povahy

## Klíčový kód

### AgentDefinition — deklarativní popis agenta
```python
from claude_agent_sdk import AgentDefinition

_QUALITY_AGENT = AgentDefinition(
    description="Senior Python inženýr kontrolující kvalitu kódu",
    prompt=_QUALITY_SYSTEM,   # system prompt jako konstanta
    tools=["Read"],            # sub-agenti smí jen číst soubory
    model=MODEL,               # "claude-sonnet-4-6"
)
```

### SDK použití — ClaudeSDKClient s logováním tool volání
```python
from claude_agent_sdk import AssistantMessage, ClaudeAgentOptions, ClaudeSDKClient, ResultMessage, TextBlock, ToolUseBlock

options = ClaudeAgentOptions(
    system_prompt=agent_def.prompt,
    allowed_tools=agent_def.tools or [],
    mcp_servers=mcp_servers or {},
    max_turns=5,
    model=agent_def.model,
    cwd=cwd,
    permission_mode="acceptEdits",
    max_budget_usd=MAX_BUDGET_PER_AGENT_USD,
    env=_sdk_env(),
)
parts: list[str] = []
async with ClaudeSDKClient(options=options) as client:
    await client.query(task)
    async for message in client.receive_response():
        if isinstance(message, AssistantMessage):
            for block in message.content:
                if isinstance(block, TextBlock):
                    parts.append(block.text)
                elif isinstance(block, ToolUseBlock):
                    print(f"   🔧 {name}: {block.name}({list(block.input.keys())})")
        elif isinstance(message, ResultMessage):
            if message.total_cost_usd and message.total_cost_usd > 0:
                print(f"   💰 {name}: ${message.total_cost_usd:.4f} ({message.duration_ms}ms)")
```

### MCP server — in-process nástroj pro Quality Agenta
```python
from claude_agent_sdk import create_sdk_mcp_server, tool

@tool("analyze_code_structure", "popis", {"file_path": str})
async def analyze_code_structure(args: dict) -> dict:
    return await _analyze_impl(args)   # implementace v _analyze_impl() — testovatelná přímo

CODE_ANALYSIS_SERVER = create_sdk_mcp_server(
    name="code_analysis", version="1.0.0", tools=[analyze_code_structure]
)
MCP_TOOL_NAME = "mcp__code_analysis__analyze_code_structure"
```

Pozor: `@tool` dekorátor vrací `SdkMcpTool` objekt (ne callable) — pro testy je implementace
extrahována do `_analyze_impl()` a testuje se přímo bez MCP wrapperu.

### Paralelní spuštění — asyncio.gather() s error handling
```python
# Vrstva 2: 3 větve paralelně
raw = await asyncio.gather(
    security_task, quality_task, tests_task,
    return_exceptions=True,            # jeden selhavší agent neshodí celý review
)
security, quality, tests = (
    r if isinstance(r, str) else f"[chyba agenta: {r}]"
    for r in raw
)

# Vrstva 3 (uvnitř security_swarm.py): 5 checkerů paralelně se staggerem
async def _staggered(i: int, name: str, agent_def: AgentDefinition) -> str:
    if i > 0:
        await asyncio.sleep(i * SWARM_STAGGER_SECONDS)   # všechny tasky vzniknou najednou
    return await _run_checker(name, agent_def, task, cwd)

checker_tasks = [
    asyncio.create_task(_staggered(i, name, agent_def))
    for i, (name, agent_def) in enumerate(CHECKERS.items())
]
raw = await asyncio.gather(*checker_tasks, return_exceptions=True)
```

### Truncation vstupu supervisoru
```python
MAX_SECTION_CHARS = 12_000   # max délka výstupu jednoho agenta

def _truncate(text: str, limit: int = MAX_SECTION_CHARS) -> str:
    if len(text) <= limit:
        return text
    return text[:limit] + f"\n\n... [zkráceno — původní délka: {len(text)} znaků]"
```

### Předání výsledků Supervisorovi
Sub-agenti vrací plain text. Supervisor dostane zkrácené sekce vložené do promptu:
```python
prompt = (
    f"{task}\n\n"
    "## Bezpečnostní analýza\n" + _truncate(findings['security']) + "\n\n"
    "## Analýza kvality kódu\n" + _truncate(findings['quality']) + "\n\n"
    "## Analýza testovacího pokrytí\n" + _truncate(findings['tests']) + "\n\n"
    "Vytvoř finální Markdown report."
)
```

## Struktura projektu

```
project/
├── src/code_reviewer/
│   ├── __init__.py          — sdk_env() helper (sdílená funkce pro oba moduly)
│   ├── __main__.py          — CLI, načte .env, zavolá supervisor, uloží report
│   ├── supervisor.py        — _run_agent(), _run_supervisor(), _truncate(),
│   │                          CodeReviewSupervisor, AgentDefinition konstanty,
│   │                          3× system prompt (_QUALITY, _TESTS, _SUPERVISOR)
│   ├── security_swarm.py   — run_security_swarm(), _run_checker(),
│   │                          5× AgentDefinition checker + system prompt
│   └── mcp_tools.py        — _analyze_impl(), @tool analyze_code_structure,
│                              CODE_ANALYSIS_SERVER, MCP_TOOL_NAME
├── tests/
│   ├── test_supervisor.py   — unit + integrační testy (marker: integration)
│   ├── test_security_swarm.py — testy swarm struktury a agregace
│   └── test_mcp_tools.py   — testy _analyze_impl() (11 testů, bez MCP wrapperu)
├── examples/
│   ├── buggy_app.py         — jednoduchý demo soubor (89 řádků)
│   │                          SQL injection ×4, secrets, pickle, path traversal, MD5
│   └── buggy_api.py         — FastAPI e-shop demo (320 řádků)
│                              SQL injection ×13, RCE ×2, command injection ×3,
│                              hardcoded Stripe live key, PCI DSS violation
├── docs/skills/             — SKILL.md referenční dokumentace (kód je nepoužívá)
│   ├── quality-reviewer/SKILL.md
│   ├── tests-reviewer/SKILL.md
│   └── security-*/SKILL.md  (5 souborů)
├── chat_history/            — záznamy konverzací (gitignored)
├── README.md
├── CLAUDE.md                — tento soubor
├── pyproject.toml
├── .env                     — ANTHROPIC_API_KEY (gitignored!)
└── .gitignore
```

## Spouštění

```bash
# Review souboru
uv run code-reviewer cesta/k/souboru.py

# Demo — jednoduchý soubor
uv run code-reviewer examples/buggy_app.py

# Demo — FastAPI backend (složitější, ~$1.30, ~10 min)
uv run code-reviewer examples/buggy_api.py

# Testy bez API (32 testů)
uv run pytest tests/ -k "not integration"

# Integrační test s reálným API
uv run pytest tests/ -m integration
```

## API klíč

V `.env`:
```
ANTHROPIC_API_KEY=sk-ant-...
```
Načte se přes `python-dotenv` v `__main__.py` a předá se do každého `ClaudeAgentOptions`
přes parametr `env=sdk_env()` z `__init__.py`.

## Konvence pro rozšíření

- **Nový sub-agent (vrstva 2):** přidej `AgentDefinition` + `_XXX_SYSTEM` v `supervisor.py` → `asyncio.create_task(_run_agent(...))` → přidej do `gather()` → předej do `_run_supervisor()`
- **Nový security checker (vrstva 3):** přidej `_XXX_SYSTEM` + `AgentDefinition` v `security_swarm.py` → přidej do `CHECKERS`
- **System prompty** drž jako konstanty ve stejném souboru jako `AgentDefinition`
- **Sub-agenti mají jen `["Read"]`** — nesmí modifikovat soubory
- **Supervisor nemá nástroje** — `allowed_tools=[]`, jen syntetizuje text
- **Supervisor nepíše soubory** — to dělá `__main__.py`

## Všechny parametry ClaudeAgentOptions

| Parametr | Typ | V projektu | Možné využití |
|---|---|---|---|
| `system_prompt` | `str` | ✅ | — |
| `allowed_tools` | `list[str]` | ✅ `["Read"]` nebo `[]` | — |
| `model` | `str` | ✅ `"claude-sonnet-4-6"` | — |
| `max_turns` | `int` | ✅ 1 / 3 / 5 | — |
| `cwd` | `str\|Path` | ✅ | — |
| `permission_mode` | `str` | ✅ `acceptEdits` | — |
| `env` | `dict[str,str]` | ✅ API klíč | — |
| `max_budget_usd` | `float` | ✅ 0.15 / 0.25 / 0.35 | — |
| `mcp_servers` | `dict` | ✅ Quality Agent | přidat `GitBlame`, `LintCheck` |
| `effort` | `str` | ❌ | checkers `low`, supervisor `high` |
| `thinking` | `dict` | ❌ | hlubší analýza u supervisora |
| `output_format` | `dict` | ❌ | supervisor vrací JSON místo Markdown |
| `betas` | `list[str]` | ❌ | review velkých souborů (>200K tokenů) |
| `fallback_model` | `str` | ❌ | resilience při výpadku modelu |
| `skills` | `list[str]` | ❌ (zkoušeno — viz docs/skills/) | agent je ignoruje pokud má plný system_prompt |
| `agents` | `dict` | ❌ | alternativa ke swarm logice |
| `hooks` | `dict` | ❌ | logování tool volání (alternativa k ToolUseBlock) |
| `can_use_tool` | `Callable` | ❌ | omezit agenta aby četl jen reviewovaný soubor |
| `sandbox` | `SandboxSettings` | ❌ | bezpečnější spuštění v produkci |
| `task_budget` | `TaskBudget` | ❌ | limit tokenů na checker |
| `session_store` | `SessionStore` | ❌ | audit log agentních volání |
| `setting_sources` | `list[str]` | ❌ | načíst skills/settings z projektu |
| `stderr` | `Callable` | ❌ | debug logování při vývoji |
