# Claude Code CLI — Reference

## Spuštění

```bash
claude                        # interaktivní session
claude "dotaz"                # session s prvním promptem
claude -p "dotaz"             # jednorázový dotaz (bez session)
claude -c                     # pokračovat v poslední session
```

---

## Slash příkazy (v interaktivní session)

### Session

| Příkaz | Popis |
|--------|-------|
| `/clear` | Nová session |
| `/continue` | Pokračovat v kontextu |
| `/resume` | Obnovit předchozí session |
| `/rename` | Přejmenovat session |

### Model a konfigurace

| Příkaz | Popis |
|--------|-------|
| `/model` | Přepnout model |
| `/config` | Nastavení |
| `/effort` | Úroveň úsilí |
| `/theme` | Barevné téma |
| `/permissions` | Správa oprávnění |

### Kód a soubory

| Příkaz | Popis |
|--------|-------|
| `/review` | Code review |
| `/edit [soubor]` | Otevřít v externím editoru |
| `/diff` | Zobrazit git diff |

### Nástroje

| Příkaz | Popis |
|--------|-------|
| `/mcp` | Správa MCP serverů |
| `/doctor` | Diagnostika |
| `/help` | Nápověda |

---

## Klávesové zkratky

| Zkratka | Akce |
|---------|------|
| `Enter` | Odeslat |
| `Ctrl+C` | Zrušit |
| `Ctrl+D` | Ukončit |
| `Shift+Enter` | Nový řádek |
| `Shift+Tab` | Přepnout permission mode |
| `Alt+P` | Přepnout model |
| `Alt+T` | Extended thinking |
| `Ctrl+R` | Hledat v historii |
| `Ctrl+G` | Otevřít v externím editoru |
| `Ctrl+O` | Zobrazit transcript |

---

## Speciální prefixové příkazy

| Prefix | Akce |
|--------|------|
| `!příkaz` | Spustit shell příkaz přímo |
| `@cesta` | Autocomplete cesty k souboru |
| `/dotaz` | Otevřít menu příkazů |

---

## Permission módy (cyklovat přes `Shift+Tab`)

| Mód | Popis |
|-----|-------|
| `default` | Ptá se na každou akci |
| `acceptEdits` | Auto-přijme úpravy souborů |
| `plan` | Pouze plánuje, neprovádí |
| `auto` | Auto-schvaluje bezpečné operace |
| `bypassPermissions` | Přeskočí vše (nejvolnější) |

---

## Užitečné CLI přepínače

```bash
--model sonnet/opus/haiku      # volba modelu
--permission-mode auto         # nastav permission mód
--dangerously-skip-permissions # přeskočit všechna oprávnění
--debug                        # debug mód
--add-dir <cesta>              # přidat pracovní adresář
-p --output-format json        # JSON výstup pro scripty
```
