# Code Review Supervisor

Multi-agent systém postavený na **Claude Agent SDK**, který provede automatizované
code review Python souboru a vygeneruje přehledný Markdown report.

## Co to dělá

Zadáš soubor → tři AI agenti ho paralelně zkontrolují → dostaneš report.

```bash
uv run code-reviewer examples/buggy_app.py
# → vygeneruje examples/buggy_app.review.md
```

## Architektura — Supervisor + Security Sub-Swarm

Dva multi-agent patterny v jednom projektu: **Supervisor** na vrstvě 1, **Swarm** na vrstvě 3.

```
          ┌─────────────────────────────────────────────────────┐
          │                   Supervisor                        │
          │           (orchestruje + syntetizuje)               │
          └───────────────────┬─────────────────────────────────┘
                              │  spustí paralelně
          ┌───────────────────┼───────────────────┐
          │                   │                   │
   ┌──────▼──────┐     ┌──────▼──────┐     ┌──────▼──────┐
   │  Security   │     │   Quality   │     │    Tests    │
   │   Swarm     │     │    Agent    │     │    Agent    │
   │             │     │ DRY, type   │     │ pytest,     │
   │  5 checkerů │     │ hints,      │     │ fixtures,   │
   │  paralelně  │     │ complexity..│     │ regrese...  │
   └──────┬──────┘     └──────┬──────┘     └──────┬──────┘
          │                   │                   │
          │  Security Swarm (autonomní, bez koordinace):
          │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
          │  │SQL Inject│ │ Secrets  │ │Deserializ│ │Path Trav.│ │   Auth   │
          │  └──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘
          │
          └───────────────────┼───────────────────┘
                              │  výsledky jako text
          ┌───────────────────▼─────────────────────────────────┐
          │                   Supervisor                        │
          │        deduplikuje + prioritizuje + akční plán      │
          └───────────────────┬─────────────────────────────────┘
                              │
                        Markdown report
```

Každý agent je definován jako `AgentDefinition` a spouštěn přes `ClaudeSDKClient`
z Claude Agent SDK. Agenti na vrstvě 2 jedou **paralelně** přes `asyncio.gather()`.
Na vrstvě 3 (Security Swarm) běží všech 5 checkerů také paralelně — autonomně,
bez vzájemné komunikace.

## Agenti

### Vrstva 2 — paralelní sub-agenti

| Agent | Co analyzuje | Výstup |
|-------|-------------|--------|
| **Security Swarm** | Orchestruje 5 specializovaných security checkerů | Agregovaný security report |
| **Quality Agent** | Architektura (DRY, SRP), složitost (parametry, zanořování), ošetření chyb, type hints, docstrings, Pythonic kód | Nálezy podle kategorií + ukázky lepšího kódu |
| **Tests Agent** | Chybějící unit testy pro každou funkci/třídu — happy path, edge cases, error cases, security regrese | Konkrétní pytest kód + fixture návrhy |
| **Supervisor** | Syntetizuje výsledky, deduplikuje překrývající se nálezy, sestaví prioritizovaný akční plán | Finální Markdown report |

### Vrstva 3 — Security Swarm (5 autonomních checkerů)

| Checker | Co hledá |
|---------|---------|
| **SQL Injection** | f-stringy, %-formát, `.format()` přímo v SQL dotazech |
| **Hardcoded Secrets** | Hesla, API klíče, tokeny, connection stringy v kódu |
| **Deserializace** | `pickle.loads()`, `yaml.load()` bez `SafeLoader` |
| **Path Traversal** | `open()` s uživatelským vstupem bez normalizace cesty |
| **Auth & Autorizace** | Slabé hashování hesel (MD5/SHA1), chybějící kontrola oprávnění |

## Ukázka výstupu

Report má tuto strukturu:

```markdown
## Shrnutí
Kód obsahuje 4 kritické bezpečnostní zranitelnosti...

## Bezpečnostní nálezy
### KRITICKÁ — SQL Injection v get_user() (řádek 20)
...konkrétní kód + oprava...

## Kvalita kódu
### Příliš mnoho parametrů — process_order() (10 parametrů)
...

## Doporučení pro testy
### test_get_user_sql_injection_returns_none
```python
def test_authenticate_sql_injection(um):
    with patch("app.get_user", return_value=None):
        assert um.authenticate("' OR '1'='1", "x") is False
```

## Prioritizovaný akční plán
🔴 Okamžitě: SQL injection × 4, hardcoded secrets...
🟠 Tento sprint: MD5 → bcrypt, přidat validaci vstupu...
🟡 Backlog: type hints, docstrings, refaktorovat DB vrstvu...

## Celkové hodnocení
| Dimenze | Skóre |
|---------|-------|
| Bezpečnost | 1/10 |
...
```

## Instalace

```bash
# Klonuj repo
git clone <repo-url>
cd project

# Nastav API klíč
echo "ANTHROPIC_API_KEY=sk-ant-..." > .env

# Instalace závislostí
uv sync
```

## Použití

```bash
# Review libovolného souboru
uv run code-reviewer cesta/k/souboru.py

# Demo s připraveným buggy souborem
uv run code-reviewer examples/buggy_app.py
```

Report se uloží jako `<soubor>.review.md` vedle původního souboru.

## Struktura projektu

```
project/
├── src/code_reviewer/
│   ├── __main__.py          ← CLI entry point
│   ├── supervisor.py        ← Supervisor + Quality/Tests AgentDefinition + system prompty
│   └── security_swarm.py   ← Security Sub-Swarm (5 AgentDefinition checkerů)
├── tests/
│   ├── test_supervisor.py   ← smoke + unit testy (mock ClaudeSDKClient) + 1 integrační
│   └── test_security_swarm.py ← testy swarm struktury, checkerů a agregace
├── examples/
│   └── buggy_app.py         ← demo soubor s úmyslnými chybami
├── chat_history/            ← záznamy konverzací při vývoji
├── README.md
├── CLAUDE.md                ← technická dokumentace pro Claude Code
├── pyproject.toml
└── .env                     ← ANTHROPIC_API_KEY (nikdy necommituj!)
```

## Testy

```bash
# Všechny smoke + unit testy — bez API, s mocky (21 testů)
uv run pytest tests/ -k "not integration"

# Plný integrační test — volá reálné API
uv run pytest tests/ -m integration
```

## Technologie

- **Claude Agent SDK** (`claude-agent-sdk`) — Python wrapper kolem Claude Code CLI
- **`AgentDefinition`** — deklarativní popis každého agenta (description, prompt, tools, model)
- **`ClaudeSDKClient`** — async context manager pro spouštění agentů
- **Model** — `claude-sonnet-4-6`
- **Paralelismus** — `asyncio.gather()` pro souběžné sub-agenty
- **API klíč** — předáván přes `env={"ANTHROPIC_API_KEY": ...}` v `ClaudeAgentOptions`
- **Python** 3.10+, `uv` pro správu závislostí

## Poznámky k architektuře

### Kde žijí instrukce agentů

Každý agent je řízen `system_prompt` — string konstanta přímo v Pythonu (`supervisor.py`, `security_swarm.py`). Agent ji dostane vždy při každém spuštění.

V `docs/skills/` jsou SKILL.md soubory se stejným obsahem jako referenční dokumentace. Kód je nepoužívá — jsou tam pro přehlednost a jako základ pro případný budoucí refaktor (přesun instrukci z kódu do externích souborů).

## Kontext

Projekt vznikl jako školní assignment hw02 na kurzu [learn.l-a-b-a.cz](https://learn.l-a-b-a.cz).
Požadavky: Claude Code SDK + Supervisor multi-agent pattern + praktické použití.
Deadline: 8. 5. 2026.
