"""
MCP nástroje pro code reviewer — in-process MCP server s AST analýzou.

Quality Agent používá tento server pro získání strukturálních metadat souboru
před vlastní analýzou kódu.
"""

from __future__ import annotations

import ast
from pathlib import Path
from typing import Any

from claude_agent_sdk import create_sdk_mcp_server, tool


@tool(
    "analyze_code_structure",
    (
        "Analyzuje strukturu Python souboru pomocí AST. "
        "Vrátí počty funkcí, tříd, metod, importů, řádků kódu a seznam všech "
        "definovaných funkcí/tříd — užitečné jako kontext před manuální revizí."
    ),
    {"file_path": str},
)
async def analyze_code_structure(args: dict[str, Any]) -> dict[str, Any]:
    """Strukturální analýza Python souboru přes ast modul."""
    file_path = Path(args["file_path"])

    if not file_path.exists():
        return {"content": [{"type": "text", "text": f"Soubor nenalezen: {file_path}"}]}

    if file_path.suffix != ".py":
        return {"content": [{"type": "text", "text": f"Není Python soubor: {file_path}"}]}

    source = file_path.read_text(encoding="utf-8")

    try:
        tree = ast.parse(source)
    except SyntaxError as e:
        return {"content": [{"type": "text", "text": f"Chyba parsování: {e}"}]}

    functions: list[str] = []
    classes: list[str] = []
    methods: list[str] = []
    imports: list[str] = []

    for node in ast.walk(tree):
        if isinstance(node, ast.FunctionDef | ast.AsyncFunctionDef):
            # Rozlišíme metody (parent je ClassDef) od volných funkcí
            functions.append(node.name)
        elif isinstance(node, ast.ClassDef):
            classes.append(node.name)
            for item in node.body:
                if isinstance(item, ast.FunctionDef | ast.AsyncFunctionDef):
                    methods.append(f"{node.name}.{item.name}")
        elif isinstance(node, ast.Import):
            for alias in node.names:
                imports.append(alias.name)
        elif isinstance(node, ast.ImportFrom):
            imports.append(f"from {node.module}")

    lines = source.splitlines()
    code_lines = [l for l in lines if l.strip() and not l.strip().startswith("#")]

    report = (
        f"## Strukturální analýza: {file_path.name}\n\n"
        f"**Celkem řádků:** {len(lines)} (kód: {len(code_lines)}, "
        f"prázdné/komentáře: {len(lines) - len(code_lines)})\n\n"
        f"**Třídy ({len(classes)}):** {', '.join(classes) or 'žádné'}\n\n"
        f"**Funkce/metody ({len(functions)}):** {', '.join(functions) or 'žádné'}\n\n"
        f"**Metody ve třídách ({len(methods)}):** {', '.join(methods) or 'žádné'}\n\n"
        f"**Importy ({len(imports)}):** {', '.join(imports) or 'žádné'}\n"
    )

    return {"content": [{"type": "text", "text": report}]}


# In-process MCP server — předán Quality Agentu přes ClaudeAgentOptions.mcp_servers
CODE_ANALYSIS_SERVER = create_sdk_mcp_server(
    name="code_analysis",
    version="1.0.0",
    tools=[analyze_code_structure],
)

MCP_TOOL_NAME = "mcp__code_analysis__analyze_code_structure"
