# Code Review Multi-Agent Pipeline

> **HW 02 — Agentic Coding**  
> Praktická ukázka multi-agentní orchestrace pomocí **Claude Code (Anthropic SDK)**.

---

## Co projekt dělá

Spustíte jej na Python souboru (nebo adresáři) a systém provede automatický code review pomocí čtyř specializovaných AI agentů běžících paralelně. Výsledky syntézuje nadřazený Supervisor agent, který vydá finální verdikt (`APPROVED` / `NEEDS_WORK` / `BLOCKED`).

---

## Architektura

```
┌─────────────────────────────────────────────────────────────────┐
│                    CODE REVIEW PIPELINE                         │
│                                                                 │
│  Stage 1 — Sequential                                           │
│  ┌──────────────────┐                                           │
│  │   Load & parse   │  čte soubor / adresář                    │
│  └────────┬─────────┘                                           │
│           │                                                     │
│  Stage 2 — Parallel  (asyncio.gather)                           │
│           ├──────────────────┬──────────────────┬────────────── │
│  ┌────────▼──────┐  ┌────────▼──────┐  ┌────────▼──────┐  ... │
│  │ Security agent│  │  Bugs agent   │  │ Perf. agent   │       │
│  └───────────────┘  └───────────────┘  └───────────────┘       │
│           │                                                     │
│  Stage 3 — Supervisor  (multi-agent synthesis)                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Supervisor agent  →  final verdict + top issues         │   │
│  └──────────────────────────────────────────────────────────┘   │
│           │                                                     │
│  Stage 4 — Conditional  (verdict-based branching)               │
│           ├── APPROVED   → exit 0, green report                 │
│           ├── NEEDS_WORK → exit 1, warnings                     │
│           └── BLOCKED    → exit 2, blocking banner              │
└─────────────────────────────────────────────────────────────────┘
```

---

## Demonstrované vzory orchestrace

| Vzor | Kde | Popis |
|------|-----|-------|
| **Sekvenční** | `pipeline.py` Stage 1 → 2 → 3 → 4 | Každá fáze závisí na výsledku předchozí |
| **Paralelní** | `pipeline.py` Stage 2 | `asyncio.gather` spouští 4 agenty současně |
| **Supervisor** | `agents.py` `run_supervisor()` | Nadřazený agent agreguje výstupy specialistů |
| **Podmíněný** | `pipeline.py` Stage 4 | Různý výstup a exit code podle verdiktu |

---

## Specializovaní agenti

| Agent | Zaměření |
|-------|----------|
| **Security** | SQL/cmd injection, hardcoded secrets, path traversal, XSS, kryptografie |
| **Bugs** | Null pointery, off-by-one, unhandled exceptions, resource leaks |
| **Performance** | O(n²) algoritmy, N+1 queries, zbytečné výpočty, chybějící cache |
| **Style** | Pojmenování, délka funkcí, duplicity, SOLID, error handling |

Každý agent vrací strukturovaný výstup přes **tool use** (Anthropic function calling), takže výsledky jsou strojově čitelné JSON objekty.

---

## Instalace

**Žádný API klíč ani žádné Python balíčky nejsou potřeba.**  
Pipeline používá `claude` CLI (Claude Code), které je již nainstalované a autentizované přes aktivní Claude Code session.

```bash
# 1. Naklonujte repo
git clone <repo-url>
cd hw_02_agentic_coding

# 2. Ověřte, že máte Claude Code CLI
claude --version
```

---

## Použití

```bash
# Review jednoho souboru
python main.py examples/vulnerable_app.py

# Review čistého kódu
python main.py examples/clean_service.py

# Review celého adresáře (všechny .py soubory)
python main.py examples/

# Uložení reportu do souboru
python main.py examples/vulnerable_app.py --output report.md
```

### Exit kódy

| Kód | Verdikt |
|-----|---------|
| `0` | APPROVED |
| `1` | NEEDS_WORK |
| `2` | BLOCKED |

CI/CD integrace: `python main.py src/ || exit 1` zablokuje merge při kritických problémech.

---

## Ukázkový výstup

```
📂 Reviewing: vulnerable_app.py  (73 lines)

🔍 Parallel phase: launching specialized agents…
   ↳ 4 agents completed.

🧠 Supervisor phase: synthesizing reports…
   ↳ Verdict ready.

══════════════════════════════════════════════════════════════
  CODE REVIEW REPORT  ·  vulnerable_app.py
══════════════════════════════════════════════════════════════

  🚫  BLOCKED — critical issues found, merge is not allowed.

  Overall score : 2.5 / 10
  Critical      : 3
  High          : 4

  Recommendation:
  Rewrite authentication flow using parameterized queries,
  remove all hardcoded credentials, and replace pickle
  deserialization before this code can be merged.

  ┌─ ACTION REQUIRED ──────────────────────────────────────┐
  │  Fix all CRITICAL issues before requesting another      │
  │  review. Do NOT merge this branch.                      │
  └────────────────────────────────────────────────────────┘

──────────────────────────────────────────────────────────────
  DETAILED FINDINGS BY AGENT
──────────────────────────────────────────────────────────────

  🤖  SECURITY AGENT   score 1/10
      Multiple critical vulnerabilities found ...

      🔴 [CRITICAL]  SQL Injection in /login route
         User input is directly interpolated into SQL query.
         💡 Use parameterized queries: cursor.execute("... WHERE username=?", (username,))
  ...
```

---

## Struktura projektu

```
hw_02_agentic_coding/
├── main.py              # CLI vstupní bod
├── pipeline.py          # Orchestrační pipeline (Sequential + Parallel + Conditional)
├── agents.py            # Specializovaní agenti + Supervisor (tool use)
├── requirements.txt
├── .env.example
└── examples/
    ├── vulnerable_app.py    # Flask app s záměrnými chybami (pro demo)
    └── clean_service.py     # Dobře napsaná async service (pro kontrast)
```

---

## Technické detaily

- **CLI**: `claude -p --output-format json` — Claude Code CLI v print módu, nevyžaduje API klíč
- **Paralelismus**: `asyncio.create_subprocess_exec` + `asyncio.gather` — 4 subprocesy běží současně, celková latence ≈ latence nejpomalejšího agenta
- **Strukturovaný výstup**: JSON schema je součástí promptu, odpověď je parsována pomocí regex + `json.loads`
- **Supervisor pattern**: Supervisor dostane kompletní reporty všech agentů a vydá finální verdikt s agregovanými metrikami
- **Žádné závislosti**: `requirements.txt` je prázdný — potřebná je jen standardní knihovna Pythonu a `claude` CLI
