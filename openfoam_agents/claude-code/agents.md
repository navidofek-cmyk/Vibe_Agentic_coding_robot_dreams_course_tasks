# Nastavení agentů – OpenFOAM Learning Assistant

## Přehled: kolik agentů a jak jsou nastaveni

Systém používá **4 agenty** — 1 hlavní orchestrátor + 3 specializované sub-agenty.

---

## Agent 1 – Hlavní orchestrátor (Claude Code CLI)

**Spuštění:** `claude` v adresáři projektu  
**Model:** `claude-sonnet-4-6` (nastaveno v `settings.json`)  
**Identita:** definována v `CLAUDE.md` — CFD mentor, pedagogická pravidla  

**Co dělá:**
- Přijímá slash příkazy od studenta (`/case`, `/check`, `/explain`, `/run`, `/progress`)
- Rozhoduje, kdy zavolat který sub-agent
- Spravuje MCP servery a hooks
- Nikdy nepíše řešení přímo — klade otázky

**Konfigurace:**
```
claude-code/
├── CLAUDE.md              ← role, pravidla, zakázané operace
└── .claude/
    ├── settings.json      ← model, MCP servery, permissions, hooks
    └── commands/          ← definice slash příkazů
```

---

## Agent 2 – Explore Sub-agent

**Typ:** `subagent_type: "Explore"`  
**Kdy se volá:** Automaticky při `/check` — hledá chyby v case souborech  
**Nástroje:** Read, Bash (grep, find) — pouze čtení, bez editace

**Úkoly:**
- Projde všechny soubory v `workspace/cases/<case>/`
- Zkontroluje, zda nejsou zbývající `???` placeholdery
- Ověří konzistenci názvů patchů (musí být stejné v `0/U`, `0/p`, `blockMeshDict`)
- Najde nepoužité importy nebo code smells v Python souborech

**Jak ho orchestrátor volá:**
```
Spusť Explore sub-agenta: prohledej workspace/cases/cavity_cviceni/
a najdi všechna místa kde je stále ??? nebo nevyplněné TODO.
```

---

## Agent 3 – Plan Sub-agent

**Typ:** `subagent_type: "Plan"`  
**Kdy se volá:** Automaticky při `/case` — plánuje strukturu nového cvičení  
**Nástroje:** Read, WebFetch, WebSearch — research + plánování, bez zápisu

**Úkoly:**
- Navrhne vhodný solver pro zadanou fyziku
- Určí geometrii a klíčové parametry (Re, Mach, síť)
- Naplánuje, která pole budou mít TODO pro studenta
- Vytvoří osnovu testů

**Jak ho orchestrátor volá:**
```
Spusť Plan sub-agenta: navrhni strukturu OpenFOAM cvičení pro
"proudění v L-tvaru potrubí, Re=500". Výstup: solver, geometrie,
seznam TODO polí, 5 testovacích případů.
```

---

## Agent 4 – General-Purpose Sub-agent

**Typ:** `subagent_type: "general-purpose"`  
**Kdy se volá:** Pro paralelní úkoly — generuje testy a dokumentaci zároveň  
**Nástroje:** Všechny nástroje včetně Write

**Úkoly (paralelně):**
- Větev A: generuje `workspace/tests/test_<case>.py`
- Větev B: generuje dokumentaci pro nový případ
- Větev C: aktualizuje `progress.json`

**Jak ho orchestrátor volá:**
```
Spusť dva general-purpose sub-agenty paralelně:
  Agent A: vygeneruj workspace/tests/test_pipe.py pro případ pipe_cviceni
  Agent B: aktualizuj README.md – přidej sekci pro pipe_cviceni
```

---

## MCP Servery (dostupné všem agentům)

| Server | Příkaz | Popis |
|--------|--------|-------|
| `filesystem` | `npx @modelcontextprotocol/server-filesystem` | R/W k case souborům |
| `github` | `npx @modelcontextprotocol/server-github` | fork, issues, sdílení |
| `brave-search` | `npx @modelcontextprotocol/server-brave-search` | OF dokumentace |
| `sqlite` | `npx @modelcontextprotocol/server-sqlite` | pokrok studenta v DB |
| `memory` | `npx @modelcontextprotocol/server-memory` | kontext mezi sezeními |

---

## Hooks (automatické akce)

| Událost | Akce |
|---------|------|
| `PreToolUse(Bash)` | Log každého shell příkazu do stderr |
| `PostToolUse(Edit\|Write)` | Kontrola OpenFOAM syntaxe upraveného souboru |
| `Stop` | Oznámení "Agent dokončil práci" |

---

## Schéma volání agentů

```
Student zadá: /check cavity_cviceni
                        │
              Orchestrátor (Agent 1)
              ├── spustí pytest → výsledek
              ├── volá Explore agent (Agent 2)
              │     └── vrátí: seznam chyb v case souborech
              └── sestaví pedagogickou zpětnou vazbu
                        │
              Student vidí: "Test X selhal. Co si myslíš, proč?"
```

```
Student zadá: /case pipe
                        │
              Orchestrátor (Agent 1)
              ├── volá Plan agent (Agent 3)
              │     └── vrátí: plán struktury, solver, TODO pole
              ├── volá 2× General-purpose agent paralelně (Agent 4)
              │     ├── větev A: generuje OF case soubory
              │     └── větev B: generuje pytest testy
              └── aktualizuje progress.json
```
