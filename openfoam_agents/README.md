# OpenFOAM Learning Assistant – Claude Code Agent

Interaktivní CFD mentor postavený na Claude Code CLI. Agent provází studenty
světem OpenFOAM od základní struktury případu až po analýzu výsledků simulace.

---

## Co to dělá

- **Generuje cvičení** (příkaz `/case`) – kompletní OpenFOAM case se záměrně
  chybějícími nebo neúplnými nastaveními
- **Kontroluje řešení studenta** (`/check`) – validuje soubory bez opravování;
  místo odpovědí klade otázky
- **Vysvětluje fyziku a numeriku** (`/explain`) – od okrajových podmínek po
  diskretizační schémata
- **Spouští simulaci a analyzuje** (`/run`) – volá `foamRun`, čte log, vykreslí
  residuály přes Python
- **Sleduje pokrok** (`/progress`) – ukazuje stav všech cvičení ze `progress.json`

---

## Architektura projektu

```
openfoam_agents/
├── claude-code/
│   ├── CLAUDE.md                    ← instrukce pro agenta
│   └── .claude/
│       ├── settings.json            ← MCP servery, permissions, hooks
│       └── commands/
│           ├── case.md              ← /case  – nový OpenFOAM případ
│           ├── check.md             ← /check – validace student. řešení
│           ├── explain.md           ← /explain – vysvětlení konceptu
│           ├── run.md               ← /run   – spuštění simulace
│           └── progress.md         ← /progress – přehled pokroku
├── workspace/
│   ├── cases/
│   │   ├── cavity_cviceni/          ← cvičení 1 (začátečník)
│   │   └── pitzDaily_cviceni/       ← cvičení 2 (středně pokročilý)
│   └── tests/
│       ├── test_cavity.py
│       └── test_pitzDaily.py
├── examples/
│   ├── check_convergence.py         ← čtení log souborů
│   └── plot_residuals.py            ← vykreslení residuálů
├── docs/
│   ├── openfoam-setup.md
│   ├── case-structure.md
│   └── post-processing.md
├── pytest.ini
└── .env.example
```

---

## Vrstvy Claude Code CLI

### 1. CLAUDE.md – identita a pravidla agenta
Definuje roli, pedagogická pravidla a povolené/zakázané operace.
Agent musí vždy vysvětlit PROČ, než ukáže řešení.

### 2. settings.json – MCP servery

| Server         | Účel                                                    |
|----------------|---------------------------------------------------------|
| `filesystem`   | Čtení/zápis OpenFOAM case souborů                       |
| `github`       | Sdílení výsledků, fork referenčních případů             |
| `brave-search` | Dokumentace OpenFOAM, CFD Stack Exchange                |
| `sqlite`       | DB pokroku (spuštěné simulace, počet pokusů)            |
| `memory`       | Pamatování chyb studenta mezi sezeními                  |

### 3. Slash commands
Vlastní příkazy registrované v `.claude/commands/`.
Agent je vyvolá jako `/<command> <argumenty>`.

### 4. Sub-agenti
- **Explore agent** – prohledá case soubory, najde chybějící pole
- **Plan agent** – naplánuje postup generování nového cvičení
- **general-purpose agent** – paralelní úkoly (generování testů + docs)

### 5. Hooks
- `PreToolUse(Bash)` – loguje každý shell příkaz
- `PostToolUse(Edit|Write)` – spustí kontrolu syntaxe OF souborů
- `Stop` – oznámí dokončení práce

---

## Jak začít

```bash
# 1. Naklonuj repo
git clone https://github.com/navidofek-cmyk/Vibe_Agentic_coding_robot_dreams_course_tasks
cd coding-agent-setup

# 2. Nastav proměnné prostředí
cp .env.example .env
# vyplň ANTHROPIC_API_KEY, GITHUB_TOKEN, BRAVE_API_KEY

# 3. Spusť Claude Code v adresáři cvičení
cd workspace/cases/cavity_cviceni
claude

# 4. Pracuj s agentem
# /case cavity       ← vygeneruj nový případ lid-driven cavity
# /check 0/U         ← zkontroluj své okrajové podmínky
# /explain simpleFoam  ← vysvětli solver
# /run cavity        ← spusť simulaci
# /progress          ← zobraz pokrok
```

---

## Cvičení

### Cvičení 1 – Lid-Driven Cavity (začátečník)
**Solver:** `icoFoam` (nestacionární laminární proudění)  
**Fyzika:** 2D kavita s pohyblivou horní stěnou  
**Co student doplní:**
- rychlostní okrajové podmínky v `0/U` (rychlost víka = 1 m/s)
- časové parametry v `system/controlDict` (endTime, deltaT)

### Cvičení 2 – Pitz Daily (středně pokročilý)
**Solver:** `simpleFoam` (stacionární turbulentní proudění)  
**Fyzika:** zpětný stupeň (backward-facing step), Re ≈ 10 000  
**Co student doplní:**
- počáteční podmínky turbulence `k`, `epsilon` v `0/`
- relaxační faktory v `system/fvSolution`

---

## Závislosti

- OpenFOAM v10+ (nebo OpenFOAM.com v2312+)
- Python 3.12 s `fluidfoam`, `matplotlib`, `pytest`
- Node.js 18+ pro MCP servery (spouštěny přes `npx`)
- Claude Code CLI (`npm install -g @anthropic-ai/claude-code`)

---

## Pedagogický přístup

Agent **nikdy nezapíše opravu sám**. Místo toho:
1. Spustí validační testy a ukáže výstup
2. Položí otázku: *"Proč si myslíš, že to selhalo?"*
3. Nabídne nápovědu pouze po dalším dotazu studenta
4. Ukáže řešení pouze pokud student sám požádá
