"""
Python Linter MCP Server

Poskytuje nástroje pro statickou analýzu Python kódu:
- check_code: ruff linter (styl, importy, bugy)
- type_check: mypy type checker
"""

import subprocess
from pathlib import Path

from mcp.server.fastmcp import FastMCP

mcp = FastMCP("python-linter")


@mcp.tool()
def check_code(file_path: str) -> str:
    """
    Spustí ruff na zadaném souboru a vrátí nalezené problémy.
    Použij před commitem nebo při code review.

    Args:
        file_path: Cesta k Python souboru (relativní nebo absolutní)
    """
    path = Path(file_path)
    if not path.exists():
        return f"Soubor nenalezen: {file_path}"
    if path.suffix != ".py":
        return f"Není Python soubor: {file_path}"

    result = subprocess.run(
        ["ruff", "check", "--output-format", "text", str(path)],
        capture_output=True,
        text=True,
    )

    output = result.stdout.strip()
    if not output:
        return f"✅ {path.name}: žádné problémy (ruff)"
    return f"🔍 ruff — {path.name}:\n{output}"


@mcp.tool()
def type_check(file_path: str) -> str:
    """
    Spustí mypy na zadaném souboru a vrátí typové chyby.
    Použij při kontrole type hints nebo před refaktoringem.

    Args:
        file_path: Cesta k Python souboru (relativní nebo absolutní)
    """
    path = Path(file_path)
    if not path.exists():
        return f"Soubor nenalezen: {file_path}"
    if path.suffix != ".py":
        return f"Není Python soubor: {file_path}"

    result = subprocess.run(
        ["mypy", "--ignore-missing-imports", "--no-error-summary", str(path)],
        capture_output=True,
        text=True,
    )

    output = result.stdout.strip()
    if not output or "Success" in output:
        return f"✅ {path.name}: žádné typové chyby (mypy)"
    return f"🔍 mypy — {path.name}:\n{output}"


@mcp.tool()
def check_all(file_path: str) -> str:
    """
    Spustí ruff i mypy najednou a vrátí souhrnný report.
    Použij pro kompletní analýzu souboru.

    Args:
        file_path: Cesta k Python souboru
    """
    ruff_result = check_code(file_path)
    mypy_result = type_check(file_path)
    return f"{ruff_result}\n\n{mypy_result}"


if __name__ == "__main__":
    mcp.run()
