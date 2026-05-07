# Python Learning Assistant – Nastavení kódovacích agentů

> **Úkol HW_02** · Deadline: 8. 5. 2026
> Konfigurace kódovacích agentů Claude Code (Anthropic) a Codex CLI (OpenAI) pro výuku Pythonu.

---

## O projektu

Produkčně připravené nastavení dvou kódovacích agentů pro konkrétní use-case:
**interaktivní Python mentor s vlastním student agentem**.

Mentor nevypisuje hotová řešení – klade otázky, spouští testy a provází studenta
k pochopení. Klíčová funkce je **simulace výukové session**: orchestrátor spustí
student agenta (píše kód s úmyslnými chybami) a mentor agenta (testuje, ptá se)
a jejich dialog je viditelný v reálném čase.

```
/simulate faktoriál

  👨‍🎓 STUDENT píše kód...
  → faktorial(0) vrátí 0 místo 1  # záměrná chyba

  🧪 MENTOR spouští testy → 3 FAILED

  🧑‍🏫 MENTOR: "Co si myslíš, co vrátí faktorial(0) ve tvém kódu?
               A jak to ovlivní výsledek 3 * 2 * 1 * ????"

  👨‍🎓 STUDENT: "Hmm... vrátí 0. Takže celý výsledek bude 0!"
  → opraví return 0 → return 1

  🧪 FINÁLNÍ TESTY → 5/5 PASSED ✅
```

---

## Architektura systému

```
┌──────────────────────────────────────────────────────────────────┐
│                     DOCKER CONTAINER                             │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐     │
│  │              CLAUDE CODE (orchestrátor/mentor)          │     │
│  │                                                         │     │
│  │  CLAUDE.md        settings.json    .claude/commands/    │     │
│  │  (role mentora)   (MCP, hooks,     /lesson  /check      │     │
│  │                    permissions)    /simulate /projekt   │     │
│  │                                    /explain /progress   │     │
│  └──────────────────────┬──────────────────────────────────┘     │
│                         │ Agent(subagent_type=...)               │
│           ┌─────────────┼──────────────┐                         │
│           ▼             ▼              ▼                         │
│      ┌─────────┐  ┌──────────┐  ┌───────────┐                   │
│      │ student │  │ Explore  │  │   Plan    │                   │
│      │ agent   │  │ hledání  │  │  osnova   │                   │
│      │ (OOP    │  │ v kódu   │  │  lekce    │                   │
│      │  chyby) │  └──────────┘  └───────────┘                   │
│      └─────────┘                                                 │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐     │
│  │  MCP SERVERY                                            │     │
│  │  filesystem │ github │ brave-search │ sqlite │ memory   │     │
│  └─────────────────────────────────────────────────────────┘     │
│                                                                  │
│  /workspace  ←──────────────────── sdíleno s hostem             │
│  ~/.claude   ←──────────────────── auth z hostu (claude login)  │
└──────────────────────────────────────────────────────────────────┘
```

---

## Struktura repozitáře

```
coding-agent-setup/
│
├── README.md
├── CLAUDE.md                          ← instrukce agenta (role mentora, pravidla)
├── Dockerfile                         ← node:22-slim + uv + Python 3.12 + MCP servery
├── docker-compose.yml                 ← 4 services: claude, codex, tests, debug
├── docker-entrypoint.sh               ← auth setup + watchdog (auto-ukončení)
├── watch-debug.sh                     ← sleduj agenty v reálném čase (terminál 2)
│
├── .claude/
│   ├── settings.json                  ← MCP servery, oprávnění, hooks
│   ├── commands/                      ← skills (slash příkazy)
│   │   ├── lesson.md                  ← /lesson <téma>
│   │   ├── check.md                   ← /check <soubor>
│   │   ├── explain.md                 ← /explain <koncept>
│   │   ├── progress.md                ← /progress
│   │   ├── simulate.md                ← /simulate – dialog mentor↔student
│   │   ├── projekt.md                 ← /projekt – BankAccount, 2 kola zpětné vazby
│   │   └── subagent-workflow.md       ← /workflow – ukázka paralelních agentů
│   └── agents/
│       └── student.md                 ← vlastní student agent (OOP chyby, přemýšlí nahlas)
│
├── codex/
│   ├── AGENTS.md                      ← instrukce pro Codex agenta
│   ├── config.yaml                    ← model o4-mini, approval: suggest
│   └── examples/
│       └── refactor_demo.py
│
├── examples/
│   ├── mcp_filesystem_demo.py
│   └── subagent_demo.py
│
├── docs/
│   ├── claude-code-setup.md
│   ├── codex-setup.md
│   └── mcp-servers.md
│
└── workspace/                         ← sdíleno s hostem (student zde pracuje)
    ├── CLAUDE.md
    ├── pyproject.toml                 ← pytest config (pythonpath, no cache)
    ├── exercises/
    │   ├── rekurze_cviceni.py         ← faktorial, fibonacci, hanoi
    │   └── ukazka_cviceni.py
    └── tests/
        ├── test_rekurze_cviceni.py    ← 10 testů ✅
        ├── test_ukazka_cviceni.py     ← 4 testy ✅
        ├── test_simulate_cviceni.py   ← 7 testů (skip → pass po /simulate)
        └── test_bank_account.py       ← 10 testů (skip → pass po /projekt)
```

---

## Rychlý start

### Předpoklady

- Docker + docker-compose
- Claude Code nainstalovaný lokálně: `npm install -g @anthropic-ai/claude-code`

### Spuštění

```bash
# 1. Přihlaš se do Claude Code (jednorázově – sdílí se s kontejnerem)
claude login

# 2. Sestav image
docker-compose build

# 3. Spusť agenta
docker-compose run --rm claude
```

Soubory studenta jsou v `workspace/` – sdílená složka, změny jsou vidět hned na hostu.

```bash
# Jednorázové příkazy
docker-compose run --rm claude claude "/lesson rekurze"
docker-compose run --rm claude claude "/check exercises/rekurze_cviceni.py"

# Jen testy (bez agenta)
docker-compose run --rm tests

# Bash shell uvnitř kontejneru
docker-compose run --rm claude bash
```

---

## Multi-agentní simulace

Nejzajímavější funkce – orchestrátor spouští **student agenta** který záměrně dělá
začátečnické chyby, a **mentor agent** ho pedagogicky provede k opravě.

### /simulate – jednoduchá rekurze

**Terminál 1:**
```bash
docker-compose run --rm debug
# pak zadej:
/simulate
```

**Terminál 2** – sleduj co se děje pod kapotou:
```bash
./watch-debug.sh
```

Terminál 2 zobrazuje barevně:
```
[AGENT]  source=agent:custom:student  ← student agent volá API
[TOOL]   File simulate_cviceni.py written atomically
[HOOK]   PostToolUse:Write → ruff check  ← linter se spustí automaticky
[HOOK]   PostToolUse:Bash → Bash příkaz dokončen.
```

### /projekt – BankAccount (2 kola zpětné vazby)

Složitější simulace – student napíše celou OOP třídu `BankAccount`,
mentor provede **dvě kola** zpětné vazby:

```bash
docker-compose run --rm debug
# pak zadej:
/projekt
```

Typické OOP chyby studenta:
- `withdraw()` nedokontroluje záporný zůstatek
- `deposit()` přijme zápornou částku
- chybějící validace vstupů

---

## Docker services

| Service | Příkaz | Co dělá |
|---------|--------|---------|
| `claude` | `docker-compose run --rm claude` | Interaktivní Claude Code |
| `codex` | `docker-compose run --rm codex` | Interaktivní Codex CLI |
| `tests` | `docker-compose run --rm tests` | pytest bez agenta |
| `debug` | `docker-compose run --rm debug` | Claude s debug logem + watchdog |

**Debug service** zapisuje logy do `workspace/debug.log` a automaticky ukončí
session po **8 minutách nečinnosti**.

---

## Skills (slash příkazy)

| Příkaz | Co dělá |
|--------|---------|
| `/lesson <téma>` | Vygeneruje lekci + cvičení + testy |
| `/check <soubor>` | Spustí pytest, pedagogická zpětná vazba |
| `/explain <koncept>` | Vysvětlí kód nebo Python koncept |
| `/progress` | Přehled pokroku studenta (SQLite) |
| `/simulate` | Dialog mentor↔student, jednoduchá rekurze |
| `/projekt` | Dialog mentor↔student, OOP projekt BankAccount |
| `/workflow <téma>` | Ukázka paralelních sub-agentů |

---

## Student agent

Vlastní sub-agent definovaný v `.claude/agents/student.md`.

```
Agent(subagent_type="student")
```

Simuluje začátečníka v Pythonu – záměrně dělá typické chyby
(špatný base case, chybějící return, záporný zůstatek) a přemýšlí nahlas
při opravě. V debug logu viditelný jako `source=agent:custom:student`.

---

## Technické detaily

### Autentizace (bez API klíče)

Claude Code podporuje OAuth přihlášení přes `claude login`. Auth token je uložen
v `~/.claude/` na hostu a při startu kontejneru se automaticky sdílí:

```yaml
volumes:
  - ${HOME}/.claude:/root/.claude
  - ${HOME}/.claude.json:/tmp/host-claude.json:ro  # entrypoint zkopíruje jako writable
```

### Python toolchain

Místo `pip` používáme `uv` – výrazně rychlejší instalace:

```dockerfile
COPY --from=ghcr.io/astral-sh/uv:latest /uv /usr/local/bin/uv
RUN uv python install 3.12
RUN uv venv /opt/venv --python 3.12
RUN uv pip install pytest ruff
```

SQLite MCP server je Python balíček – instalován přes `uv pip install mcp-server-sqlite`,
ostatní MCP servery přes npm.

### Hooks

Automaticky spouštěné po každé editaci souboru:

```json
"PostToolUse": [{
  "matcher": "Edit|Write",
  "command": "ruff check --select E,W --quiet \"$CLAUDE_TOOL_RESULT_FILE_PATH\""
}]
```

---

## Srovnání: Claude Code vs Codex CLI

| Vlastnost | Claude Code | Codex CLI |
|-----------|:-----------:|:---------:|
| Výrobce | Anthropic | OpenAI |
| Model | Claude Sonnet 4.6 | o4-mini |
| MCP servery | Ano (5) | Ne |
| Vlastní skills | Ano (.claude/commands/) | Ne |
| Vlastní agenti | Ano (.claude/agents/) | Ne |
| Sub-agenti | Ano (paralelně) | Ne |
| Hooks | Ano (4 typy) | Ne |
| Auth | claude login (OAuth) | OPENAI_API_KEY |

---

## Autor

**Ivan Dofek** · i.dofek@seznam.cz
Úkol HW_02 · Deadline 8. 5. 2026
GitHub: https://github.com/navidofek-cmyk/Vibe_Agentic_coding_robot_dreams_course_tasks/tree/main/coding-agent-setup
