# Python Learning Assistant – Nastavení kódovacích agentů

> **Úkol HW_02** · Deadline: 8. 5. 2026
> Konfigurace kódovacích agentů Claude Code (Anthropic) a Codex CLI (OpenAI) pro výuku Pythonu.

---

## O projektu

Tento repozitář obsahuje kompletní, produkčně připravené nastavení dvou kódovacích agentů.
Namísto generické konfigurace byl zvolen konkrétní use-case: **interaktivní Python mentor**.

Agent funguje jako průvodce výukou – nevypisuje hotová řešení, ale klade otázky,
spouští testy, sleduje pokrok studenta a přizpůsobuje vysvětlení jeho úrovni.

```
Student: "/check exercises/rekurze_cviceni.py"

Agent:
  ✅ Test test_faktorial     PASSED
  ✅ Test test_fibonacci      PASSED
  ❌ Test test_hanoi          FAILED

  Chyba: RecursionError: maximum recursion depth exceeded

  Otázka: Co si myslíš, proč program nikdy neskončí?
  (Nápověda: podívej se na svůj base case – kdy by se rekurze měla zastavit?)
```

---

## Architektura systému

```
┌─────────────────────────────────────────────────────────────────┐
│                        STUDENT (CLI)                            │
│                  claude "/lesson rekurze"                       │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                   CLAUDE CODE (orchestrátor)                    │
│                                                                 │
│   ┌─────────────┐   ┌─────────────┐   ┌─────────────────────┐  │
│   │   CLAUDE.md  │   │settings.json│   │ .claude/commands/   │  │
│   │  instrukce  │   │ konfigurace │   │  /lesson /check ... │  │
│   └─────────────┘   └─────────────┘   └─────────────────────┘  │
│                                                                 │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │                    SUB-AGENTI                            │  │
│   │  ┌─────────┐   ┌──────────┐   ┌──────────────────────┐  │  │
│   │  │ Explore │   │  Plan    │   │   General-purpose    │  │  │
│   │  │ hledání │   │ osnova   │   │   výzkum + tvorba    │  │  │
│   │  └────┬────┘   └────┬─────┘   └──────────┬───────────┘  │  │
│   │       └─────────────┴──────────────────────┘             │  │
│   │                  paralelní běh                           │  │
│   └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│   ┌──────────────────────────────────────────────────────────┐  │
│   │                    MCP SERVERY                           │  │
│   │  filesystem │ github │ brave-search │ sqlite │ memory    │  │
│   └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Struktura repozitáře

```
coding-agent-setup/
│
├── README.md                              ← tento soubor
│
├── claude-code/                           ← konfigurace Claude Code
│   ├── CLAUDE.md                          ← instrukce projektu (role, pravidla, kontext)
│   ├── .claude/
│   │   ├── settings.json                  ← MCP servery, oprávnění, hooks
│   │   └── commands/                      ← vlastní skills (slash příkazy)
│   │       ├── lesson.md                  ← /lesson <téma>
│   │       ├── check.md                   ← /check <soubor>
│   │       ├── explain.md                 ← /explain <koncept>
│   │       ├── progress.md                ← /progress
│   │       └── subagent-workflow.md       ← /workflow <téma>
│   └── examples/
│       ├── mcp_filesystem_demo.py         ← demo MCP filesystem operací
│       └── subagent_demo.py               ← demo paralelního sub-agentního workflow
│
├── codex/                                 ← konfigurace Codex CLI
│   ├── AGENTS.md                          ← instrukce pro Codex agenta
│   ├── config.yaml                        ← model, schvalování, výstup
│   └── examples/
│       └── refactor_demo.py               ← demo refaktoringu (před/po)
│
└── docs/
    ├── claude-code-setup.md               ← detailní návod + vnitřní anatomie nastavení
    ├── codex-setup.md                     ← návod na Codex + srovnávací tabulka
    └── mcp-servers.md                     ← přehled všech MCP serverů + vlastní server
```

---

## Rychlý start

### Způsob A – Docker (doporučeno, bez instalace Node.js)

```bash
# 1. Přihlaš se do Claude Code (jednorázově, sdílí se s kontejnerem)
claude login

# 2. Sestav image
docker-compose build

# 3. Spusť agenta
docker-compose run --rm claude
```

Tvoje soubory jsou v `workspace/` – sdílená složka mezi hostem a kontejnerem.

```bash
# Jednorázové příkazy
docker-compose run --rm claude claude "/lesson rekurze"
docker-compose run --rm claude claude "/check exercises/ukazka_cviceni.py"

# Testy bez agenta
docker-compose run --rm tests

# Bash shell uvnitř kontejneru
docker-compose run --rm claude bash
```

### Simulace mentor↔student (ukázka multi-agentního workflow)

Nejzajímavější funkce projektu – orchestrátor spustí **student agenta** a **mentor agenta**
paralelně a jejich dialog je viditelný v reálném čase.

**Terminál 1** – spusť Claude v debug módu:

```bash
docker-compose run --rm debug
```

Pak v Claude REPL zadej:

```
/simulate faktorial
```

**Terminál 2** – sleduj infrastrukturu pod kapotou:

```bash
./watch-debug.sh
```

Výstup v Terminálu 1 vypadá takto:

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
👨‍🎓 STUDENT píše kód pro téma: faktorial
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
(student agent napíše kód s úmyslnou chybou)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🧪 MENTOR spouští testy
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
FAILED test_base_case – assert None == 1

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🧑‍🏫 MENTOR reaguje
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
"Co si myslíš, co vrátí tvoje funkce pro n=0?"

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
👨‍🎓 STUDENT přemýšlí a opravuje
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
"Hmm, asi jsem zapomněl base case..."

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🧪 FINÁLNÍ TESTY po opravě
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
PASSED 3/3 ✅
```

Terminál 2 (`watch-debug.sh`) zobrazí barevně co se děje pod kapotou:

```
[AGENT]  spawning student subagent
[TOOL]   Write → workspace/exercises/simulate_cviceni.py
[HOOK]   PostToolUse Edit → ruff check
[SKILL]  Loaded 6 unique skills
```

### Způsob B – Lokální instalace

#### Předpoklady

- Node.js 22+ (`node --version`)
- Python 3.10+ (`python3 --version`)

```bash
# Instalace CLI nástrojů
npm install -g @anthropic-ai/claude-code @openai/codex

# MCP servery
npm install -g \
  @modelcontextprotocol/server-filesystem \
  @modelcontextprotocol/server-github \
  @modelcontextprotocol/server-brave-search \
  @modelcontextprotocol/server-sqlite \
  @modelcontextprotocol/server-memory
```

#### Claude Code

```bash
# Konfigurace
cp -r claude-code/.claude /cesta/k/projektu/
cp claude-code/CLAUDE.md /cesta/k/projektu/

# Env proměnné
export ANTHROPIC_API_KEY=sk-ant-...
export GITHUB_TOKEN=ghp_...
export BRAVE_API_KEY=BSA...

# Spuštění
cd /cesta/k/projektu && claude
```

#### Codex CLI

```bash
# Konfigurace
mkdir -p ~/.codex
cp codex/config.yaml ~/.codex/config.yaml

export OPENAI_API_KEY=sk-...

# Spuštění
cd /cesta/k/projektu && codex
```

---

## Claude Code – detailní popis

### Jak funguje nastavení

Claude Code hledá konfiguraci ve třech vrstvách (každá přepisuje předchozí):

```
~/.claude/settings.json          ← 1. globální (pro všechny projekty)
    ↓ přepsáno
<projekt>/.claude/settings.json  ← 2. projektové (jen pro tento projekt)
    ↓ přepsáno
<projekt>/.claude/settings.local.json  ← 3. lokální (osobní, v .gitignore)
```

`CLAUDE.md` v kořeni projektu definuje roli a pravidla chování agenta –
načte se automaticky při každém spuštění.

---

### MCP Servery

MCP (Model Context Protocol) jsou pluginy rozšiřující schopnosti agenta.
Nakonfigurováno je 5 serverů:

| Server | Balíček | Co přidá agentovi |
|--------|---------|-------------------|
| **filesystem** | `@modelcontextprotocol/server-filesystem` | Čtení/zápis souborů, procházení adresářů |
| **github** | `@modelcontextprotocol/server-github` | Issues, PRs, commity, forky repozitářů |
| **brave-search** | `@modelcontextprotocol/server-brave-search` | Webové vyhledávání dokumentace |
| **sqlite** | `@modelcontextprotocol/server-sqlite` | SQL dotazy na progress databázi |
| **memory** | `@modelcontextprotocol/server-memory` | Pamatuje si chyby studenta mezi sezeními |

#### Jak agent MCP servery používá

```
Student: "/explain list comprehensions"

Agent (interně):
  1. mcp__brave-search__brave_web_search("python list comprehensions tutorial")
     → vrátí URL na docs.python.org
  2. mcp__filesystem__read_file("lessons/cykly.md")
     → zkontroluje co student již zná
  3. mcp__memory__search_nodes("list comprehensions Ivan")
     → zjistí zda student s tímto tématem dříve bojoval
  4. Sestaví vysvětlení přizpůsobené kontextu studenta
```

#### Nastavení v settings.json (zkráceno)

```json
{
  "mcpServers": {
    "filesystem": {
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-filesystem", "/home/user/projects"]
    },
    "github": {
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-github"],
      "env": { "GITHUB_PERSONAL_ACCESS_TOKEN": "${GITHUB_TOKEN}" }
    },
    "brave-search": {
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-brave-search"],
      "env": { "BRAVE_API_KEY": "${BRAVE_API_KEY}" }
    }
  }
}
```

---

### Skills (vlastní slash příkazy)

Skills jsou `.md` soubory v `.claude/commands/`. Každý soubor = jeden příkaz.

```
.claude/commands/lesson.md   →   /lesson rekurze
.claude/commands/check.md    →   /check exercises/rekurze_cviceni.py
.claude/commands/explain.md  →   /explain lambda
.claude/commands/progress.md →   /progress
.claude/commands/subagent-workflow.md  →  /workflow rekurze
```

#### Přehled implementovaných skills

**`/lesson <téma>`** – vygeneruje kompletní lekci
- Plan agent vytvoří 5-bodovou osnovu
- Explore agent zkontroluje existující materiály
- GP agent najde dokumentaci přes Brave Search
- Výstup: `lessons/<téma>.md` + `exercises/<téma>_cviceni.py` + `tests/test_<téma>.py`

**`/check <soubor>`** – zkontroluje řešení studenta
- Spustí `pytest tests/test_<soubor>.py -v`
- Explore agent hledá code smells
- Pedagogická zpětná vazba (ptá se, nevysvětluje hned)
- Aktualizuje `progress.json`

**`/explain <koncept>`** – vysvětlí kód nebo Python koncept
- Detekuje typ vstupu (soubor / klíčové slovo / chybová zpráva / koncept)
- Přizpůsobí vysvětlení úrovni studenta
- Načte historii z MCP memory (co student nepochopil dříve)

**`/progress`** – přehled pokroku
- SQL dotaz na sqlite databázi pokroku
- ASCII vizualizace progress baru
- Doporučení další lekce

**`/workflow <téma>`** – ukázka paralelních sub-agentů
- Spustí 3 sub-agenty simultánně (~35s vs ~90s sekvenčně)
- Demonstrace orchestrace Explore + Plan + GP agentů

---

### Sub-agenti

Claude Code má 3 typy specializovaných sub-agentů.
Orchestrátor (hlavní agent) je spouští **paralelně** a čeká na všechny výsledky.

```
Orchestrátor
    │
    ├── Agent(subagent_type="Explore")
    │   Rychlé read-only prohledávání kódu.
    │   Hledá soubory, symboly, vzory.
    │   → vrátí: seznam souborů a nalezených vzorů
    │
    ├── Agent(subagent_type="Plan")
    │   Architektonické rozhodování.
    │   Vytváří implementační plány.
    │   → vrátí: osnovu / seznam kroků
    │
    └── Agent(subagent_type="general-purpose")
        Všestranný agent se všemi nástroji.
        Vhodný pro výzkum, tvorbu obsahu, komplexní úkoly.
        → vrátí: výsledek dle zadání
```

**Proč sub-agenti?**

| | Bez sub-agentů | S sub-agenty |
|-|---------------|-------------|
| 3 nezávislé úkoly | ~90 sekund (sekvenčně) | ~35 sekund (paralelně) |
| Context window | jeden sdílený | oddělené, čistší |
| Specializace | jeden generalist | každý na svůj úkol |

---

### Hooks

Hooks jsou shell příkazy automaticky spouštěné při událostech.

```json
"hooks": {
  "PostToolUse": [
    {
      "matcher": "Edit|Write",
      "hooks": [{
        "type": "command",
        "command": "ruff check --quiet \"$CLAUDE_TOOL_RESULT_FILE_PATH\""
      }]
    }
  ],
  "Stop": [
    {
      "hooks": [{
        "type": "command",
        "command": "echo 'Agent dokončil práci.' >&2"
      }]
    }
  ]
}
```

| Událost | Kdy se spustí |
|---------|--------------|
| `PreToolUse` | Před každým voláním nástroje |
| `PostToolUse` | Po každém volání nástroje |
| `Stop` | Když agent dokončí celou odpověď |
| `Notification` | Při odesílání notifikace uživateli |

V tomto projektu hook po každé editaci souboru spustí `ruff check` –
agent okamžitě vidí linting chyby a může je opravit.

---

## Codex CLI – detailní popis

### Konfigurace (`~/.codex/config.yaml`)

```yaml
model: o4-mini        # rychlý a levný pro výukové úkoly
provider: openai
approval: suggest     # agent navrhuje, student potvrzuje každý krok
output: stream        # průběžný výstup v reálném čase
```

### Módy schvalování (`approval`)

| Mód | Chování | Vhodné pro |
|-----|---------|-----------|
| `suggest` | Navrhuje akce, student každou potvrdí | Výuka, bezpečné prostředí |
| `auto-edit` | Automaticky mění soubory, ptá se na příkazy | Zkušení vývojáři |
| `full-auto` | Vše automaticky bez potvrzení | CI/CD skripty |

### AGENTS.md

Soubor `AGENTS.md` v kořeni projektu = instrukce pro Codex agenta.
Automaticky načten při každém spuštění, stejně jako `CLAUDE.md` u Claude Code.

Definuje:
- Roli agenta (co dělá, jak se chová)
- Povolené příkazy bez potvrzení (`pytest`, `git status`, `ls`...)
- Příkazy vždy vyžadující potvrzení (`git commit`, `rm`, `pip install`...)
- Formát zpětné vazby studentovi

### Příklady použití z terminálu

```bash
# Interaktivní sezení
codex

# Jednorázový dotaz se souborem
codex "vysvětli co dělá tato funkce" exercises/rekurze_cviceni.py

# Kontrola cvičení (quiet mód – bez interakce)
codex -q "spusť testy a vrať výsledky" exercises/funkce_cviceni.py

# Refaktoring s konkrétním modelem
codex --model gpt-4o "zrefaktoruj na Pythonic styl"

# Nastavení schvalování pro celé sezení
codex --approval auto-edit
```

---

## Srovnání: Claude Code vs Codex CLI

| Vlastnost | Claude Code | Codex CLI |
|-----------|:-----------:|:---------:|
| Výrobce | Anthropic | OpenAI |
| Model | Claude Sonnet / Opus / Haiku | GPT-4o, o4-mini, o3 |
| **MCP servery** | **Ano** (5 serverů) | Ne |
| **Vlastní skills** | **Ano** (.claude/commands/) | Ne |
| **Sub-agenti** | **Ano** (3 typy, paralelně) | Ne |
| **Hooks** | **Ano** (4 typy událostí) | Ne |
| Instrukce projektu | CLAUDE.md | AGENTS.md |
| Schvalování akcí | permissions v settings.json | approval v config.yaml |
| IDE integrace | VS Code, JetBrains | – |
| Paměť mezi sezeními | MCP memory server | – |
| Webové vyhledávání | MCP brave-search | – |
| Přístup k GitHub | MCP github server | – |

**Závěr:** Pro výukový projekt je Claude Code výrazně výkonnější díky MCP serverům,
skills a sub-agentům. Codex je jednodušší na nastavení a výhodný pro rychlé
jednorázové dotazy z terminálu.

---

## Docker – detailní popis

### Co je v image

```
node:22-slim (base)
├── Python 3 + pytest + ruff
├── Claude Code CLI  (@anthropic-ai/claude-code)
├── Codex CLI        (@openai/codex)
└── MCP servery (předinstalované, ne stahované za běhu):
    ├── @modelcontextprotocol/server-filesystem
    ├── @modelcontextprotocol/server-github
    ├── @modelcontextprotocol/server-brave-search
    ├── @modelcontextprotocol/server-sqlite
    └── @modelcontextprotocol/server-memory
```

### Adresáře v kontejneru

```
/workspace/          ← sdílený s hostem (./workspace na disku)
  exercises/         ← cvičení studenta
  lessons/           ← generované lekce
  tests/             ← pytest testy
  CLAUDE.md          ← instrukce pro agenta
  AGENTS.md          ← instrukce pro Codex

/root/.claude/       ← konfigurace Claude Code (persistentní volume)
  settings.json      ← MCP servery, hooks, oprávnění
  commands/          ← slash příkazy

/root/.codex/
  config.yaml        ← model, schvalování
```

### Volumes

| Volume | Co obsahuje | Persistence |
|--------|-------------|-------------|
| `./workspace` | Kód studenta, lekce, testy | Sdílený s hostem |
| `pla-claude-config` | Historie, paměť Claude | Přežije restart |

### Services v docker-compose.yml

```yaml
claude   ← Claude Code (interaktivní, -it)
codex    ← Codex CLI   (interaktivní, -it)
tests    ← pytest runner (bez agenta, jednorázový)
```

### Předání API klíčů

Klíče se předávají přes `.env` soubor (nikdy není v gitu):

```bash
cp .env.example .env
# Vyplň ANTHROPIC_API_KEY, OPENAI_API_KEY, ...
docker compose run --rm claude
```

Nebo přímo z příkazové řádky:

```bash
ANTHROPIC_API_KEY=sk-ant-... docker compose run --rm claude
```

---

## Technické detaily

### Všechny vrstvy konfigurace Claude Code CLI

Claude Code skládá výslednou konfiguraci z **7 vrstev** seřazených podle priority.
Vyšší vrstva vždy přepíše nižší. Tady jsou všechny vrstvy – obecně i co máme v naší implementaci.

---

#### Vrstva 1 – Systémové výchozí hodnoty (vestavěné v CLI)

Anthropic dodává v instalaci Claude Code výchozí chování, seznam zabudovaných nástrojů
a definice tří typů sub-agentů. Uživatel tuto vrstvu **nemůže měnit**.

```
Zabudované nástroje:   Read, Write, Edit, Bash, Agent, WebFetch, WebSearch, ...
Zabudovaní sub-agenti: Explore, Plan, general-purpose
Výchozí model:         claude-sonnet (nejnovější dostupný)
```

**Naše implementace:** tuto vrstvu neměníme, jen ji využíváme – sub-agenti
`Explore`, `Plan` a `general-purpose` jsou volání v skills (`/lesson`, `/workflow`).

---

#### Vrstva 2 – Globální uživatelská konfigurace

```
~/.claude/settings.json     ← platí pro VŠECHNY projekty daného uživatele
~/.claude/CLAUDE.md         ← globální instrukce (např. "vždy komunikuj česky")
~/.claude/commands/*.md     ← globální slash příkazy dostupné všude
```

Typický obsah: API klíče, oblíbený model, globální oprávnění, dark/light mode.

**Naše implementace:** globální vrstvu v repozitáři nedodáváme (je osobní).
Uživatel si ji nastaví dle svého. Doporučená minimální konfigurace:

```json
{
  "model": "claude-sonnet-4-6",
  "theme": "dark",
  "permissions": {
    "allow": ["Bash(git status)", "Bash(ls:*)", "Bash(find:*)"]
  }
}
```

---

#### Vrstva 3 – Projektová konfigurace ✅ (naše hlavní nastavení)

```
<projekt>/.claude/settings.json    ← verzováno v gitu, sdíleno s týmem
```

Toto je naše klíčová vrstva. Soubor `claude-code/.claude/settings.json` obsahuje:

```
mcpServers:
  filesystem   → npx @modelcontextprotocol/server-filesystem /home/user/projects
  github       → npx @modelcontextprotocol/server-github        (vyžaduje GITHUB_TOKEN)
  brave-search → npx @modelcontextprotocol/server-brave-search  (vyžaduje BRAVE_API_KEY)
  sqlite       → npx @modelcontextprotocol/server-sqlite --db-path .../app.db
  memory       → npx @modelcontextprotocol/server-memory

permissions:
  allow: pytest, git status/diff/log/add/commit, python3, npm, npx, find, grep, ls, cat
  deny:  rm -rf, git push --force, sudo

hooks:
  PreToolUse(Bash)        → echo '[Hook] Spouštím Bash příkaz...'
  PostToolUse(Edit|Write) → ruff check $CLAUDE_TOOL_RESULT_FILE_PATH
  PostToolUse(Bash)       → echo '[Hook] Bash příkaz dokončen.'
  Stop                    → echo '[Hook] Claude dokončil práci.'

env:
  PYTHONPATH = /home/user/projects
  NODE_ENV   = development

model: claude-sonnet-4-6
theme: dark
```

---

#### Vrstva 4 – Lokální osobní přepsání

```
<projekt>/.claude/settings.local.json    ← v .gitignore, jen pro tebe
```

Sem patří věci které nechceš sdílet s týmem: vlastní API klíče, debug nastavení,
dočasné povolení nebezpečných příkazů pro konkrétní task.

**Naše implementace:** soubor v repozitáři není (je v `.gitignore`).
Příklad jak může vypadat:

```json
{
  "permissions": {
    "allow": ["Bash(rm:*)"]
  },
  "env": {
    "DEBUG": "1"
  }
}
```

---

#### Vrstva 5 – CLAUDE.md (instrukce projektu) ✅

```
<projekt>/CLAUDE.md    ← automaticky načten při každém spuštění
```

Nástrojový soubor pro definici **role, pravidel a kontextu** agenta v projektu.
Není to konfigurace (JSON/YAML) – je to text který Claude čte jako instrukce.

**Naše implementace:** `claude-code/CLAUDE.md` definuje:

```
Role:               Interaktivní Python mentor
Pravidla:           Vysvětli nejdřív, neopravuj za studenta, ptej se
Struktura projektu: lessons/, exercises/, tests/, progress.json
Jazyk:              čeština
Povolené operace:   čtení exercises/, spouštění pytest
Zakázané operace:   přepis solutions/, git push bez potvrzení, mazání
MCP přehled:        filesystem, github, sqlite, brave-search, memory
Sub-agenti:         Explore (hledání), Plan (osnova), GP (výzkum)
```

---

#### Vrstva 6 – Skills / slash příkazy ✅

```
<projekt>/.claude/commands/*.md    ← každý soubor = jeden /příkaz
```

`.md` soubory obsahují instrukce pro agenta. `$ARGUMENTS` se nahradí tím,
co uživatel napíše za příkazem.

**Naše implementace** – 5 příkazů v `claude-code/.claude/commands/`:

```
/lesson <téma>    → lesson.md
  Plan agent vytvoří osnovu, Explore zkontroluje duplicity,
  GP agent najde dokumentaci. Zapíše 3 soubory: lekci, cvičení, testy.

/check <soubor>   → check.md
  Spustí pytest, Explore hledá code smells.
  Pedagogická zpětná vazba – ptá se, neopravuje.

/explain <koncept> → explain.md
  Detekuje typ vstupu (soubor / keyword / chybová zpráva / koncept).
  Přizpůsobí vysvětlení úrovni studenta. Načte historii z MCP memory.

/progress         → progress.md
  SQL dotaz na SQLite, ASCII progress bar, doporučení další lekce.

/workflow <téma>  → subagent-workflow.md
  Ukázka: spustí Explore + Plan + GP agenty PARALELNĚ (~35s vs ~90s).
```

---

#### Vrstva 7 – Runtime (proměnné prostředí + CLI flagy)

Nejvyšší priorita – přepíše vše výše. Platí jen pro aktuální spuštění.

```bash
# Env proměnné
ANTHROPIC_API_KEY=sk-ant-...    # povinné
GITHUB_TOKEN=ghp_...            # GitHub MCP server
BRAVE_API_KEY=BSA...            # Brave Search MCP server

# CLI flagy (přepíší settings.json)
claude --model claude-opus-4-7          # jiný model pro toto sezení
claude --permission-mode acceptEdits    # jiný mód oprávnění
claude "/lesson rekurze"                # přímý příkaz bez REPL
```

**Naše implementace:** klíče jsou v `.env` souboru (není v gitu),
Docker Compose je předá kontejneru přes `environment:` sekci.

---

#### Přehled všech vrstev – kde co je v tomto projektu

```
Vrstva 1  Systémové výchozí hodnoty     vestavěné v CLI              –
Vrstva 2  Globální uživatelská          ~/.claude/settings.json      není v repozitáři (osobní)
Vrstva 3  Projektová              ✅    .claude/settings.json        5 MCP, permissions, hooks
Vrstva 4  Lokální osobní               .claude/settings.local.json  není v repozitáři (.gitignore)
Vrstva 5  CLAUDE.md              ✅    CLAUDE.md                    role mentora, pravidla
Vrstva 6  Skills                 ✅    .claude/commands/*.md        /lesson /check /explain /progress /workflow
Vrstva 7  Runtime                ✅    .env + CLI flagy             API klíče, model override
```

---

### Kde se ukládají soubory

```
~/.claude/                   Globální konfigurace Claude Code
~/.codex/config.yaml         Globální konfigurace Codex CLI
<projekt>/CLAUDE.md          Instrukce projektu pro Claude Code
<projekt>/AGENTS.md          Instrukce projektu pro Codex CLI
<projekt>/.claude/           Projektová konfigurace Claude Code
```

### Env proměnné

```bash
ANTHROPIC_API_KEY=sk-ant-...    # Claude Code – povinné
GITHUB_TOKEN=ghp_...            # GitHub MCP server
BRAVE_API_KEY=BSA...            # Brave Search MCP server
OPENAI_API_KEY=sk-...           # Codex CLI – povinné
```

---

## Příklady interakcí

### Výuka nové látky

```
Student: /lesson list-comprehensions

Claude Code (interně):
  [Explore agent]  → prohledá lessons/ a exercises/ pro existující materiál
  [Plan agent]     → navrhne 5-bodovou osnovu lekce
  [GP agent]       → najde dokumentaci přes MCP brave-search
  [Orchestrátor]   → zkombinuje výsledky a vytvoří soubory

Výstup:
  ✅ Vytvořeno: lessons/list-comprehensions.md
  ✅ Vytvořeno: exercises/list-comprehensions_cviceni.py
  ✅ Vytvořeno: tests/test_list-comprehensions.py
  📖 Začni čtením lekce: lessons/list-comprehensions.md
```

### Kontrola řešení

```
Student: /check exercises/list-comprehensions_cviceni.py

Claude Code:
  [Spustí pytest] → 2/3 testy prošly
  [Explore agent] → najde code smell: zbytečný range(len(...))

Výstup:
  ✅ test_cviceni_1   PASSED
  ✅ test_cviceni_2   PASSED
  ❌ test_cviceni_3   FAILED

  Chyba: AssertionError: expected [1, 4, 9], got []

  Otázka: Tvůj list comprehension vrací prázdný seznam.
  Kdy by podmínka `if x > 0` byla pravdivá pro vstup [-1, 2, -3]?
```

### Sledování pokroku

```
Student: /progress

Claude Code (SQL dotaz na SQLite přes MCP):

  ═══════════════════════════════════════
    PYTHON LEARNING ASSISTANT – POKROK
  ═══════════════════════════════════════
  Celkový pokrok:    [████████░░] 80%
  Dokončené lekce:   8 / 10

  LEKCE:
    ✅ Základní datové typy   (3/3 testy)
    ✅ Podmínky a cykly       (4/4 testy)
    🔄 List comprehensions   (2/3 testy)  ← jsi zde
    ⬜ Generátory
    ⬜ Dekorátory

  DOPORUČENÍ: Dokonči cvičení 3 v list-comprehensions
  ═══════════════════════════════════════
```

---

## Soubory konfigurace

| Soubor | Popis |
|--------|-------|
| `claude-code/.claude/settings.json` | Hlavní konfigurace Claude Code – MCP servery, oprávnění, hooks |
| `claude-code/CLAUDE.md` | Instrukce projektu – role mentora, pravidla, zakázané operace |
| `claude-code/.claude/commands/*.md` | Definice 5 vlastních slash příkazů |
| `codex/config.yaml` | Konfigurace Codex CLI – model, schvalování |
| `codex/AGENTS.md` | Instrukce pro Codex – role, povolené příkazy, formát výstupu |
| `docs/claude-code-setup.md` | Podrobná dokumentace anatomie nastavení Claude Code |
| `docs/codex-setup.md` | Podrobný návod na Codex CLI + srovnání s Claude Code |
| `docs/mcp-servers.md` | Přehled všech MCP serverů včetně návodu na vlastní server |

---

## Autor

**Ivan Dofek** · i.dofek@seznam.cz
Úkol HW_02 · Deadline 8. 5. 2026
