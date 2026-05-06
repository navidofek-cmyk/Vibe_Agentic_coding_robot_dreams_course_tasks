# Podrobný návod: Claude Code nastavení

## Instalace

```bash
# Nainstaluj Claude Code CLI
npm install -g @anthropic-ai/claude-code

# Ověř instalaci
claude --version
```

## Vnitřní struktura nastavení Claude Code

### Kde se ukládají konfigurace

```
~/.claude/                        ← globální nastavení (uživatelská úroveň)
│   settings.json                 ← MCP servery, oprávnění, hooks
│   CLAUDE.md                     ← globální instrukce pro všechny projekty
│   keybindings.json              ← vlastní klávesové zkratky
│   commands/                     ← globální skills (slash příkazy)
│       review.md
│       ...
│
<projekt>/.claude/                ← projektová úroveň (přepíše globální)
    settings.json
    commands/
        lesson.md                 ← /lesson pouze pro tento projekt
        check.md                  ← /check pouze pro tento projekt
        ...

<projekt>/CLAUDE.md               ← instrukce specifické pro projekt
```

### Priorita nastavení

```
Globální (~/.claude/settings.json)
    ↓ přepsáno
Projektové (.claude/settings.json)
    ↓ přepsáno
Lokální (.claude/settings.local.json)  ← .gitignore, osobní nastavení
```

---

## Anatomie settings.json

```json
{
  // ① MCP Servery – rozšíření schopností agenta
  "mcpServers": { ... },

  // ② Oprávnění – co smí agent dělat automaticky
  "permissions": {
    "allow": [...],
    "deny": [...]
  },

  // ③ Hooks – automatické akce při událostech
  "hooks": {
    "PreToolUse": [...],    // před použitím nástroje
    "PostToolUse": [...],   // po použití nástroje
    "Stop": [...]           // když agent skončí
  },

  // ④ Prostředí – env proměnné pro agenta
  "env": { ... },

  // ⑤ Model
  "model": "claude-sonnet-4-6"
}
```

---

## ① MCP Servery – detailní přehled

MCP (Model Context Protocol) jsou pluginy, které rozšiřují schopnosti agenta.

### Dostupné MCP servery (bez marketplace)

| Server | npm balíček | Co umí |
|--------|------------|--------|
| Filesystem | `@modelcontextprotocol/server-filesystem` | Čtení/zápis souborů |
| GitHub | `@modelcontextprotocol/server-github` | Issues, PRs, repozitáře |
| Brave Search | `@modelcontextprotocol/server-brave-search` | Webové vyhledávání |
| SQLite | `@modelcontextprotocol/server-sqlite` | SQL dotazy |
| Memory | `@modelcontextprotocol/server-memory` | Perzistentní kontext |
| Fetch | `@modelcontextprotocol/server-fetch` | HTTP požadavky |

### Nastavení MCP serveru

```json
"mcpServers": {
  "nazev-serveru": {
    "command": "npx",
    "args": ["-y", "@modelcontextprotocol/server-filesystem", "/cesta"],
    "env": {
      "API_KEY": "${ENV_PROMENNA}"
    }
  }
}
```

### Nutné env proměnné

```bash
export GITHUB_TOKEN=ghp_...          # Pro GitHub MCP server
export BRAVE_API_KEY=BSA...          # Pro Brave Search MCP server
export ANTHROPIC_API_KEY=sk-ant-...  # Pro Claude Code samotný
```

---

## ② Oprávnění

```json
"permissions": {
  "allow": [
    "Bash(git status)",        // konkrétní příkaz
    "Bash(pytest:*)",          // všechny pytest příkazy
    "Bash(npm:*)",             // všechny npm příkazy
    "mcp__filesystem__*"       // všechny filesystem MCP nástroje
  ],
  "deny": [
    "Bash(rm -rf:*)",          // nikdy nemazat rekurzivně
    "Bash(git push --force:*)" // nikdy force push
  ]
}
```

---

## ③ Hooks

Hooks jsou shell příkazy spouštěné automaticky při událostech.

```json
"hooks": {
  "PostToolUse": [
    {
      "matcher": "Edit|Write",  // regex na název nástroje
      "hooks": [
        {
          "type": "command",
          "command": "ruff check --fix $CLAUDE_TOOL_RESULT_FILE_PATH"
        }
      ]
    }
  ]
}
```

### Dostupné události
- `PreToolUse` – před každým voláním nástroje
- `PostToolUse` – po každém volání nástroje  
- `Stop` – když agent dokončí práci
- `Notification` – při odesílání notifikace

### Dostupné proměnné v hooks
- `$CLAUDE_TOOL_NAME` – název použitého nástroje
- `$CLAUDE_TOOL_RESULT_FILE_PATH` – cesta k upravenému souboru

---

## ④ Skills (Slash příkazy)

Skills jsou soubory `.md` v `.claude/commands/`.

```markdown
# /nazev-prikazu – Popis

Instrukce pro agenta...

$ARGUMENTS se nahradí tím, co uživatel napsal za příkazem.
```

### Použití
```bash
claude "/lesson rekurze"
claude "/check exercises/funkce.py"
claude "/explain lambda funkce"
```

---

## ⑤ Sub-agenti

Claude Code má 3 typy specializovaných sub-agentů:

```
Agent(
    subagent_type="Explore",          # rychlé hledání v kódu
    prompt="Najdi všechny TODO..."
)

Agent(
    subagent_type="Plan",             # architektonické plánování
    prompt="Navrhni strukturu..."
)

Agent(
    subagent_type="general-purpose",  # všestranný agent
    prompt="Proveď komplexní úkol..."
)
```

Sub-agenti běží **paralelně** – orchestrátor čeká na všechny a zkombinuje výsledky.

---

## CLAUDE.md – Instrukce projektu

`CLAUDE.md` v kořeni projektu definuje, jak se agent chová v daném projektu.

```markdown
# Název projektu

## Role agenta
<co agent dělá>

## Pravidla
<jak se má chovat>

## Struktura projektu
<kde jsou jaké soubory>

## Povolené/zakázané operace
<explicitní limity>
```

Claude Code automaticky načte `CLAUDE.md` při každém spuštění.
