# Nastavení agentů – OpenFOAM Learning Assistant

Systém používá **4 agenty** — 1 hlavní orchestrátor + 3 specializované sub-agenty
+ extra **student-teacher simulace** přes `claude -p` CLI.

---

## Agent 1 – Hlavní orchestrátor

| Vlastnost | Hodnota |
|-----------|---------|
| Typ | Claude Code CLI (interaktivní) |
| Spuštění | `claude` v adresáři projektu |
| Model | `claude-sonnet-4-6` |
| Identita | `claude-code/CLAUDE.md` |
| Slash příkazy | `/case` `/check` `/explain` `/run` `/progress` |
| Permissions | `settings.json → permissions.allow/deny` |
| Hooks | PreToolUse, PostToolUse, Stop |

**Role:** CFD mentor. Přijímá příkazy studenta, volá sub-agenty, spravuje MCP servery.
Nikdy nedává přímé řešení — klade sókratovské otázky.

**Klíčová pravidla (z CLAUDE.md):**
- Vždy nejdřív VYSVĚTLI fyzikální princip, pak ukaž konfiguraci
- Pokud student udělá chybu, nezapiš opravu sám — ptej se
- Preferuj jednoduché příklady (cavity, pipe) před komplexními geometriemi

**Povolené operace bez potvrzení:**
```
Bash(blockMesh:*), Bash(checkMesh:*), Bash(foamRun:*),
Bash(icoFoam:*), Bash(simpleFoam:*), Bash(pytest:*),
Bash(python3:*), Bash(find:*), Bash(grep:*), Bash(docker:*)
```

**Zakázané operace:**
```
Bash(rm -rf:*), Bash(git push --force:*), Bash(sudo:*)
```

---

## Agent 2 – Explore Sub-agent

| Vlastnost | Hodnota |
|-----------|---------|
| Typ | `subagent_type: "Explore"` |
| Trigger | Automaticky při `/check` |
| Nástroje | Read, Bash (grep, find) — **jen čtení** |
| Výstup | Seznam chyb a nesrovnalostí v case souborech |

**Co dělá:**
1. Projde všechny soubory v `workspace/cases/<case>/`
2. Zkontroluje, zda nejsou zbývající `???` placeholdery
3. Ověří konzistenci názvů patchů mezi `0/U`, `0/p` a `blockMeshDict`
4. Hledá code smells v Python souborech (nepoužité importy, duplicity)

**Ukázka volání orchestrátorem:**
```
Spusť Explore sub-agenta: prohledej workspace/cases/cavity_cviceni/
Najdi všechna místa kde je stále ??? nebo nevyplněné TODO.
Zkontroluj konzistenci názvů patchů.
```

**Proč samostatný agent:**
Explore je read-only a paralelizovatelný — může prohledávat více case
souborů zároveň, aniž by zasahoval do práce orchestrátora.

---

## Agent 3 – Plan Sub-agent

| Vlastnost | Hodnota |
|-----------|---------|
| Typ | `subagent_type: "Plan"` |
| Trigger | Automaticky při `/case <téma>` |
| Nástroje | Read, WebFetch, WebSearch — **research + plánování** |
| Výstup | Strukturovaný plán cvičení (solver, geometrie, TODO pole, osnova testů) |

**Co dělá:**
1. Navrhne vhodný solver pro zadanou fyziku (icoFoam / simpleFoam / buoyantSimpleFoam)
2. Určí geometrii a klíčové parametry (Re, délkové škály, hustotu sítě)
3. Naplánuje, která pole budou mít `???` pro studenta
4. Vytvoří osnovu pytest testů

**Ukázka volání orchestrátorem:**
```
Spusť Plan sub-agenta: navrhni strukturu OpenFOAM cvičení pro
"proudění v L-tvaru potrubí, Re=500".
Výstup: solver, geometrie, seznam TODO polí, 5 testovacích případů.
```

**Proč samostatný agent:**
Plánování je odlišná kognitivní úloha od generování souborů.
Plan agent může prohledat web (OpenFOAM dokumentace, CFD Stack Exchange)
bez toho, aby orchestrátor ztrácel kontext probíhající studentské lekce.

---

## Agent 4 – General-Purpose Sub-agent

| Vlastnost | Hodnota |
|-----------|---------|
| Typ | `subagent_type: "general-purpose"` |
| Trigger | Po schválení plánu (Agent 3) — generuje soubory paralelně |
| Nástroje | Všechny nástroje včetně **Write, Edit** |
| Výstup | OpenFOAM case soubory + pytest testy + dokumentace |

**Paralelní větve (spouštěny současně):**
```
Agent 4a → generuje workspace/cases/<case>/ (0/, constant/, system/)
Agent 4b → generuje workspace/tests/test_<case>.py
Agent 4c → aktualizuje progress.json
```

**Ukázka volání orchestrátorem:**
```
Spusť dva general-purpose sub-agenty paralelně:
  Agent A: vygeneruj workspace/cases/pipe_cviceni/ podle plánu [...]
  Agent B: vygeneruj workspace/tests/test_pipe.py se třídami
           TestRequiredFiles, TestBoundaryConditions, TestSolverSettings
```

**Proč samostatný agent:**
Generování souborů je write-heavy a nezávislé na probíhající konverzaci.
Paralelní spuštění zkrátí čas generování cvičení 2–3×.

---

## Student-Teacher Simulace (`simulation.py`)

Extra vrstva nad standardními agenty — dva Claude instance přes `claude -p` CLI,
bez potřeby API klíče.

| Vlastnost | Student agent | Učitel agent |
|-----------|--------------|--------------|
| Příkaz | `claude -p <prompt>` | `claude -p <prompt>` |
| Model | `haiku` | `haiku` |
| System prompt | CFD student, doplňuje `???` | Sókratovský mentor, max 80 slov |
| Nástroje | `--tools ""` (žádné) | `--tools ""` (žádné) |
| Session | `--no-session-persistence` | `--no-session-persistence` |
| Auth | OAuth (Claude Code) | OAuth (Claude Code) |

**Smyčka:**
```
read_files() → student_fill() → write_files() → pytest
                                                    │
                                            passed? → ✅ konec
                                            failed? → teacher_response()
                                                           │
                                                    feedback → další iterace
```

---

## MCP Servery

| Server | npm balíček | Účel | Auth |
|--------|-------------|------|------|
| `filesystem` | `@modelcontextprotocol/server-filesystem` | R/W case souborů | — |
| `github` | `@modelcontextprotocol/server-github` | fork, issues, PRs | `GITHUB_TOKEN` |
| `brave-search` | `@modelcontextprotocol/server-brave-search` | OF dokumentace, CFD Stack Exchange | `BRAVE_API_KEY` |
| `sqlite` | `@modelcontextprotocol/server-sqlite` | pokrok studenta v DB | — |
| `memory` | `@modelcontextprotocol/server-memory` | kontext chyb mezi sezeními | — |

Všechny servery se spouštějí přes `npx` — žádná instalace, žádný marketplace.

---

## Hooks

| Událost | Matcher | Akce |
|---------|---------|------|
| `PreToolUse` | `Bash` | Log příkazu do stderr |
| `PostToolUse` | `Edit\|Write` | Kontrola OpenFOAM syntaxe (hledá chybějící středníky) |
| `Stop` | — | Oznámení "Agent dokončil práci" |

---

## Slash příkazy a jejich agenti

| Příkaz | Orchestrátor | Sub-agenti |
|--------|-------------|------------|
| `/case <téma>` | plánuje a koordinuje | Plan (Agent 3) → General-purpose ×2 (Agent 4) |
| `/check <case>` | spouští pytest | Explore (Agent 2) |
| `/explain <téma>` | odpovídá přímo | — (+ MCP brave-search pro docs) |
| `/run <case>` | spouští simulaci | — (bash: blockMesh → foamRun → Python post-processing) |
| `/progress` | čte DB | — (MCP sqlite) |

---

## Schéma volání

```
/case pipe
    │
    ├─▶ Plan agent (Agent 3)
    │       hledá dokumentaci, navrhuje solver + geometrii
    │       vrátí: { solver, Re, TODO_pole, osnova_testů }
    │
    ├─▶ General-purpose A (Agent 4) ──┐  paralelně
    │       generuje case soubory     │
    │                                 │
    └─▶ General-purpose B (Agent 4) ──┘
            generuje pytest testy


/check cavity_cviceni
    │
    ├─▶ pytest workspace/tests/test_cavity.py
    │
    ├─▶ Explore agent (Agent 2)
    │       prohledá case soubory, zkontroluje ???, konzistenci patchů
    │
    └─▶ Orchestrátor sestaví zpětnou vazbu
            "Test X selhal. Co si myslíš, proč?"
```
