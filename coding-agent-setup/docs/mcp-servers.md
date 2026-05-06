# MCP Servery – přehled a použití

MCP (Model Context Protocol) je otevřený standard pro rozšiřování schopností AI agentů.

## Jak MCP funguje

```
Claude Code ←→ MCP Server ←→ Zdroj dat/nástroj
              (protokol)
```

Agent komunikuje s MCP serverem přes standardizovaný protokol.
Každý server poskytuje sadu **nástrojů** (tools), které agent může volat.

## Nastavení v settings.json

```json
{
  "mcpServers": {
    "filesystem": {
      "command": "npx",
      "args": ["-y", "@modelcontextprotocol/server-filesystem", "/cesta/k/projektu"],
      "env": {}
    }
  }
}
```

## Dostupné servery

### 1. Filesystem (`@modelcontextprotocol/server-filesystem`)

**Nástroje:**
- `read_file` – čtení obsahu souboru
- `write_file` – zápis do souboru
- `list_directory` – seznam souborů v adresáři
- `create_directory` – vytvoření adresáře
- `move_file` – přesunutí souboru
- `search_files` – hledání souborů dle vzoru
- `get_file_info` – metadata souboru

**Použití v projektu:**
```
Agent: "Přečti exercises/rekurze_cviceni.py a zkontroluj implementaci"
→ Volá: mcp__filesystem__read_file("exercises/rekurze_cviceni.py")
```

---

### 2. GitHub (`@modelcontextprotocol/server-github`)

**Vyžaduje:** `GITHUB_TOKEN` env proměnná

**Nástroje:**
- `create_repository` – vytvoření repozitáře
- `get_file_contents` – obsah souboru z repo
- `create_issue` – vytvoření issue
- `list_commits` – seznam commitů
- `create_pull_request` – vytvoření PR
- `fork_repository` – fork repozitáře

**Použití v projektu:**
```
Agent: "Vytvoř GitHub repo pro tento projekt a nahraj lekce"
→ Volá: mcp__github__create_repository(...)
→ Volá: mcp__github__create_or_update_file(...)
```

---

### 3. Brave Search (`@modelcontextprotocol/server-brave-search`)

**Vyžaduje:** `BRAVE_API_KEY` env proměnná

**Nástroje:**
- `brave_web_search` – webové vyhledávání
- `brave_local_search` – lokální vyhledávání (restaurace, místa)

**Použití v projektu:**
```
Agent: "Najdi nejlepší vysvětlení list comprehensions v češtině"
→ Volá: mcp__brave-search__brave_web_search("python list comprehensions tutorial czech")
```

---

### 4. SQLite (`@modelcontextprotocol/server-sqlite`)

**Nástroje:**
- `read_query` – SELECT dotazy
- `write_query` – INSERT/UPDATE/DELETE
- `create_table` – vytvoření tabulky
- `list_tables` – seznam tabulek
- `describe_table` – schéma tabulky
- `append_insight` – přidání poznámky

**Použití v projektu:**
```sql
-- Agent sleduje pokrok studenta
SELECT lesson_name, status, tests_passed
FROM student_progress
WHERE status != 'completed'
ORDER BY last_activity DESC;
```

---

### 5. Memory (`@modelcontextprotocol/server-memory`)

Perzistentní Knowledge Graph – agent si pamatuje informace mezi sezeními.

**Nástroje:**
- `create_entities` – vytvoření entit (student, lekce, chyba)
- `create_relations` – vztahy mezi entitami
- `search_nodes` – vyhledání v grafu
- `open_nodes` – detail entity

**Použití v projektu:**
```
Po sezení agent uloží:
  Entity: Student("Ivan")
  Entity: Chyba("off-by-one error")
  Relace: Student --opakuje_chybu--> Chyba

V příštím sezení:
  Agent: "Ivan znovu udělal off-by-one error, upozorni ho citlivě"
```

---

## Vlastní MCP server (pokročilé)

Lze napsat vlastní MCP server v Pythonu:

```python
from mcp.server import Server
from mcp.server.models import InitializationOptions
import mcp.types as types

app = Server("muj-server")

@app.list_tools()
async def handle_list_tools() -> list[types.Tool]:
    return [
        types.Tool(
            name="spust_testy",
            description="Spustí pytest testy pro daný soubor",
            inputSchema={
                "type": "object",
                "properties": {
                    "soubor": {"type": "string"}
                },
                "required": ["soubor"]
            }
        )
    ]

@app.call_tool()
async def handle_call_tool(name: str, arguments: dict):
    if name == "spust_testy":
        import subprocess
        result = subprocess.run(
            ["pytest", arguments["soubor"], "-v"],
            capture_output=True, text=True
        )
        return [types.TextContent(type="text", text=result.stdout)]
```

Nastavení vlastního serveru:
```json
{
  "mcpServers": {
    "muj-server": {
      "command": "python3",
      "args": ["/cesta/k/muj_server.py"]
    }
  }
}
```
