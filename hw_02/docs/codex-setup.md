# Podrobný návod: Codex CLI nastavení

## Instalace

```bash
# Nainstaluj Codex CLI
npm install -g @openai/codex

# Ověř instalaci
codex --version
```

## Vnitřní struktura nastavení Codex

```
~/.codex/
    config.yaml         ← hlavní konfigurace
    instructions.md     ← globální instrukce (alternativa k AGENTS.md)

<projekt>/
    AGENTS.md           ← instrukce pro projekt (automaticky načteno)
```

## Anatomie config.yaml

```yaml
# Model
model: o4-mini          # nebo gpt-4o, o3, o4-mini-high

# Schvalování akcí
approval: suggest       # suggest | auto-edit | full-auto

# Výstup
output: stream          # stream | full
```

### Módy schvalování

| Mód | Popis | Kdy použít |
|-----|-------|-----------|
| `suggest` | Navrhuje, student potvrzuje každý krok | Výuka, bezpečné prostředí |
| `auto-edit` | Automaticky mění soubory, ptá se na příkazy | Zkušení vývojáři |
| `full-auto` | Vše automaticky | CI/CD, skripty |

## AGENTS.md – Instrukce projektu

```markdown
# Název projektu

## Role
<co agent dělá>

## Povolené příkazy
<seznam příkazů bez potvrzení>

## Chování
<pravidla pro odpovědi>
```

Codex automaticky načte `AGENTS.md` z aktuálního adresáře.

## Příklady použití z CLI

```bash
# Interaktivní mód
codex

# Jednorázový dotaz se souborem
codex "vysvětli tuto funkci" soubor.py

# Quiet mód (neinteraktivní)
codex -q "přidej type hints" funkce.py

# Konkrétní model
codex --model gpt-4o "refaktoruj tento kód"

# Schvalovací mód pro sezení
codex --approval auto-edit
```

## Srovnání Claude Code vs Codex

| Vlastnost | Claude Code | Codex CLI |
|-----------|------------|-----------|
| Model | Claude Sonnet/Opus/Haiku | GPT-4o, o4-mini, o3 |
| MCP servery | Ano (filesystem, github, ...) | Ne |
| Vlastní skills | Ano (.claude/commands/) | Ne |
| Sub-agenti | Ano (Explore, Plan, GP) | Ne |
| Hooks | Ano (Pre/PostToolUse) | Ne |
| Instrukce | CLAUDE.md | AGENTS.md |
| Cena | Dle Anthropic API | Dle OpenAI API |
| Interaktivita | CLI + IDE | CLI |
