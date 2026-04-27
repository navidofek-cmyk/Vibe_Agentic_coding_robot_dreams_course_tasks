# Code Review Multi-Agent Pipeline — OpenAI edition

> **HW 02 — Agentic Coding (v2)**  
> Praktická ukázka multi-agentní orchestrace pomocí **OpenAI API** a **uv**.

---

## Co projekt dělá

Spustíte jej na Python souboru a systém provede automatický code review pomocí čtyř specializovaných AI agentů běžících paralelně. Výsledky syntézuje nadřazený Supervisor agent, který vydá finální verdikt (`APPROVED` / `NEEDS_WORK` / `BLOCKED`).

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
│           ├──────────────┬──────────────┬──────────────         │
│  ┌────────▼───┐  ┌───────▼────┐  ┌──────▼──────┐  ...         │
│  │  Security  │  │    Bugs    │  │ Performance  │              │
│  └────────────┘  └────────────┘  └─────────────┘              │
│           │                                                     │
│  Stage 3 — Supervisor  (multi-agent synthesis)                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Supervisor agent  →  final verdict + top issues         │   │
│  └──────────────────────────────────────────────────────────┘   │
│           │                                                     │
│  Stage 4 — Conditional  (verdict-based branching)               │
│           ├── APPROVED   → exit 0                               │
│           ├── NEEDS_WORK → exit 1                               │
│           └── BLOCKED    → exit 2 + blocking banner             │
└─────────────────────────────────────────────────────────────────┘
```

---

## Demonstrované vzory orchestrace

| Vzor | Kde | Popis |
|------|-----|-------|
| **Sekvenční** | `pipeline.py` Stage 1→4 | Každá fáze závisí na předchozí |
| **Paralelní** | `pipeline.py` Stage 2 | `asyncio.gather` — 4 agenti současně |
| **Supervisor** | `agents.py` `run_supervisor()` | Nadřazený agent agreguje výstupy specialistů |
| **Podmíněný** | `pipeline.py` Stage 4 | Různý výstup + exit code podle verdiktu |

---

## Instalace

```bash
# 1. Naklonujte repo
git clone <repo-url>
cd hw_02_agentic_coding_v2

# 2. Nastavte API klíč
cp .env.example .env
# doplňte OPENAI_API_KEY do .env
export OPENAI_API_KEY="sk-..."

# 3. Nainstalujte závislosti přes uv
uv sync
```

---

## Použití

```bash
# Review jednoho souboru
uv run python main.py examples/vulnerable_app.py

# Review čistého kódu
uv run python main.py examples/clean_service.py

# Uložení reportu
uv run python main.py examples/vulnerable_app.py --output report.md
```

### Exit kódy

| Kód | Verdikt |
|-----|---------|
| `0` | APPROVED |
| `1` | NEEDS_WORK |
| `2` | BLOCKED |

---

## Technické detaily

- **SDK**: `openai` Python SDK (`AsyncOpenAI` client)
- **Model**: `gpt-4o-mini` — rychlý a cenově efektivní pro paralelní volání
- **Správa závislostí**: `uv` — `pyproject.toml` + `uv.lock`
- **Paralelismus**: `asyncio.gather` — 4 API volání probíhají současně
- **Strukturovaný výstup**: Function calling (`tool_choice` forced) — model vrací garantované JSON
- **Supervisor pattern**: Supervisor dostane kompletní reporty a vydá finální verdikt

---

## Struktura projektu

```
hw_02_agentic_coding_v2/
├── pyproject.toml       # uv projekt, závislosti
├── main.py              # CLI vstupní bod
├── pipeline.py          # Orchestrace (Sequential + Parallel + Conditional)
├── agents.py            # Specializovaní agenti + Supervisor (OpenAI function calling)
├── .env.example
└── examples/
    ├── vulnerable_app.py    # Flask app se záměrnými chybami (pro demo)
    └── clean_service.py     # Dobře napsaná async service (pro kontrast)
```
