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
   │
   ├── [SQLInjectionChecker]   ┐
   ├── [SecretsChecker]        │
   ├── [DeserializationChecker]├── Swarm (5 checkerů paralelně,
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

| Checker | `AgentDefinition` | Hledá |
|---------|------------------|-------|
| `_SQL_AGENT` | `_SQL_SYSTEM` | f-stringy, %-formát, `.format()` v SQL dotazech |
| `_SECRETS_AGENT` | `_SECRETS_SYSTEM` | hardcoded hesla, API klíče, tokeny |
| `_DESERIALIZATION_AGENT` | `_DESERIALIZATION_SYSTEM` | pickle.loads(), yaml.load() bez SafeLoader |
| `_PATH_TRAVERSAL_AGENT` | `_PATH_TRAVERSAL_SYSTEM` | open() s uživatelským vstupem bez normalizace |
| `_AUTH_AGENT` | `_AUTH_SYSTEM` | MD5/SHA1 pro hesla, chybějící autorizace |

### QualityAgent (`supervisor.py` — `_QUALITY_AGENT`)
Kontroluje kódovou kvalitu podle Python best practices:
- **Architektura:** SRP, DRY, God object, tight coupling, hardcoded závislosti
- **Složitost:** funkce >5 parametrů, hluboké zanořování, magická čísla
- **Ošetření chyb:** holé except:, tiché spolknutí výjimky, nechráněné zdroje
- **Typová bezpečnost:** chybějící type hints, nevhodné Any
- **Dokumentace:** chybějící docstrings, zastaralé komentáře
- **Pythonic kód:** nepoužití enumerate/zip, context managerů, comprehensions

### TestsAgent (`supervisor.py` — `_TESTS_AGENT`)
Navrhuje pytest testy pro každou funkci/třídu:
- Happy path, edge cases, hraniční hodnoty, error cases
- Security regresní testy (dokumentují bugy — označeny `# REGRESNÍ TEST — BUG`)
- Konkrétní spustitelný pytest kód s fixtures a mock patchy
- Prioritizovaná tabulka: funkce × počet testů × priorita

### Supervisor (`supervisor.py` — `_SUPERVISOR_AGENT`)
Syntetizuje výsledky všech tří agentů:
- Deduplikuje překrývající se nálezy
- Sestaví prioritizovaný akční plán (🔴 okamžitě / 🟠 tento sprint / 🟡 backlog)
- Dimenzionální hodnocení: Bezpečnost / Kvalita / Testovatelnost / Čitelnost (1–10)

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

### Všechny parametry ClaudeAgentOptions

| Parametr | Typ | Popis | V projektu | Možné využití |
|---|---|---|---|---|
| `system_prompt` | `str` | System prompt agenta | ✅ | — |
| `allowed_tools` | `list[str]` | Nástroje povolené bez promptu | ✅ `["Read"]` nebo `[]` | — |
| `model` | `str` | Model (např. `"claude-sonnet-4-6"`) | ✅ | — |
| `max_turns` | `int` | Max počet otáček konverzace | ✅ 1 / 3 / 5 | — |
| `cwd` | `str\|Path` | Pracovní adresář subprocesu | ✅ | — |
| `permission_mode` | `str` | `acceptEdits / default / bypassPermissions / plan / dontAsk` | ✅ `acceptEdits` | — |
| `env` | `dict[str,str]` | Env proměnné pro subprocess (API klíč) | ✅ | — |
| `effort` | `str` | `low / medium / high / max` — hloubka přemýšlení (default: `high`) | ❌ | checkers `low`, supervisor `high` |
| `max_budget_usd` | `float` | Strop nákladů v USD na jedno volání | ❌ | ochrana před drahým runem |
| `thinking` | `dict` | Extended thinking: `adaptive / enabled / disabled` | ❌ | hlubší analýza u supervisora |
| `output_format` | `dict` | Strukturovaný JSON výstup (schema) | ❌ | supervisor vrací JSON místo Markdown |
| `betas` | `list[str]` | Beta funkce — `"context-1m-2025-08-07"` pro 1M token context | ❌ | review velkých souborů (>200K tokenů) |
| `fallback_model` | `str` | Záložní model pokud hlavní selže | ❌ | resilience při výpadku modelu |
| `tools` | `list[str]` | Základní sada nástrojů (alternativa k `allowed_tools`) | ❌ | — |
| `disallowed_tools` | `list[str]` | Nástroje explicitně zakázané | ❌ | — |
| `agents` | `dict` | Programaticky definované subagenty přístupné přes Agent tool | ❌ | alternativa ke swarm logice v `security_swarm.py` |
| `hooks` | `dict` | Callbacky na události (PreToolUse, PostToolUse, ...) | ❌ | logování každého `Read` volání agenta |
| `can_use_tool` | `Callable` | Custom handler pro povolení nástrojů | ❌ | omezit agenta aby četl jen reviewovaný soubor |
| `sandbox` | `SandboxSettings` | Izolace filesystému a sítě | ❌ | bezpečnější spuštění agentů v produkci |
| `skills` | `list[str]` | SKILL.md soubory s custom instrukcemi pro agenta | ❌ | nahradit system prompty externími SKILL.md soubory |
| `mcp_servers` | `dict` | MCP servery s custom nástroji | ❌ | přidat nástroje `GitBlame`, `LintCheck`, `SastScan` |
| `task_budget` | `TaskBudget` | Token budget pro model | ❌ | limit tokenů na checker |
| `session_store` | `SessionStore` | Ukládání transkriptů externě | ❌ | audit log všech agentních volání |
| `enable_file_checkpointing` | `bool` | Možnost rewindu souborů | ❌ | — |
| `stderr` | `Callable` | Callback pro debug výstup subprocesu | ❌ | debug logování při vývoji |
| `extra_args` | `dict` | Raw CLI argumenty navíc | ❌ | — |

### SDK použití — ClaudeSDKClient
```python
from claude_agent_sdk import AgentDefinition, AssistantMessage, ClaudeAgentOptions, ClaudeSDKClient, TextBlock

def _sdk_env() -> dict[str, str]:
    env = {}
    api_key = os.environ.get("ANTHROPIC_API_KEY")
    if api_key:
        env["ANTHROPIC_API_KEY"] = api_key
    return env

options = ClaudeAgentOptions(
    system_prompt=agent_def.prompt,
    allowed_tools=agent_def.tools or [],
    max_turns=5,
    model=agent_def.model,
    cwd=cwd,
    permission_mode="acceptEdits",
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
```

### Paralelní spuštění — asyncio.gather()
```python
# Vrstva 2: 3 větve paralelně
security_task = asyncio.create_task(run_security_swarm(task, cwd))
quality_task  = asyncio.create_task(_run_agent("quality", _QUALITY_AGENT, task, cwd))
tests_task    = asyncio.create_task(_run_agent("tests",   _TESTS_AGENT,   task, cwd))

security, quality, tests = await asyncio.gather(
    security_task, quality_task, tests_task
)

# Vrstva 3 (uvnitř security_swarm.py): 5 checkerů paralelně
checker_tasks = [
    asyncio.create_task(_run_checker(name, agent_def, task, cwd))
    for name, agent_def in CHECKERS.items()
]
results = await asyncio.gather(*checker_tasks)
```

### Předání výsledků Supervisorovi
Sub-agenti vrací plain text. Supervisor je dostane jako sekce vložené přímo do promptu:
```python
prompt = (
    f"{task}\n\n"
    "## Bezpečnostní analýza\n" + findings['security'] + "\n\n"
    "## Analýza kvality kódu\n" + findings['quality'] + "\n\n"
    "## Analýza testovacího pokrytí\n" + findings['tests'] + "\n\n"
    "Vytvoř finální Markdown report."
)
```

## Struktura projektu

```
project/
├── src/code_reviewer/
│   ├── __init__.py          — package marker
│   ├── __main__.py          — CLI, načte .env, zavolá supervisor, uloží report
│   ├── supervisor.py        — _sdk_env(), _run_agent(), _run_supervisor(),
│   │                          CodeReviewSupervisor, AgentDefinition konstanty,
│   │                          3× system prompt (_QUALITY, _TESTS, _SUPERVISOR)
│   └── security_swarm.py   — run_security_swarm(), _run_checker(),
│                              5× AgentDefinition checker + system prompt
├── tests/
│   ├── test_supervisor.py   — TestCodeReviewSupervisorInit, TestExamplesExist,
│   │                          TestRunAgentMocked, TestSupervisorMocked,
│   │                          test_full_review_integration (marker: integration)
│   └── test_security_swarm.py — TestSwarmStructure, TestRunChecker,
│                                 TestRunSecuritySwarm
├── examples/
│   └── buggy_app.py         — demo soubor s úmyslnými chybami:
│                              SQL injection ×4, hardcoded secrets, pickle RCE,
│                              path traversal, MD5, funkce s 10 parametry
├── chat_history/
│   └── session_2026-05-04.md — záznam celé konverzace při vývoji
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

# Demo
uv run code-reviewer examples/buggy_app.py

# Testy bez API (21 testů)
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
přes parametr `env=_sdk_env()`. Soubor `.env` je v `.gitignore`.

## Konvence pro rozšíření

- **Nový sub-agent (vrstva 2):** přidej `AgentDefinition` konstantu v `supervisor.py` → `asyncio.create_task(_run_agent("xxx", _XXX_AGENT, ...))` → přidej do `gather()` → předej výsledek do `_run_supervisor()`
- **Nový security checker (vrstva 3):** přidej `_XXX_SYSTEM` konstantu + `AgentDefinition` v `security_swarm.py` → přidej do `CHECKERS`
- **System prompty** drž jako konstanty ve stejném souboru jako `AgentDefinition`
- **Sub-agenti mají jen `["Read"]`** — nesmí modifikovat soubory
- **Supervisor nemá nástroje** — `allowed_tools=[]`, jen syntetizuje text
- **Supervisor nepíše soubory** — to dělá `__main__.py`
