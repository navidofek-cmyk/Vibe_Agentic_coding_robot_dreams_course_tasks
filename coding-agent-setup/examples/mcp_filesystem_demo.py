"""
Ukázka použití MCP Filesystem Serveru z Claude Code

Tento skript ukazuje, co Claude Code může dělat s MCP filesystem serverem.
V praxi tyto akce Claude provádí automaticky – tento soubor slouží jako
dokumentace/demonstrace.

Spuštění: claude "Spusť mcp_filesystem_demo.py a vysvětli výstup"
"""

import os
import json
from pathlib import Path


def ukazka_cteni_souboru():
    """MCP filesystem: čtení obsahu souboru."""
    print("=== MCP Filesystem: Čtení souborů ===")

    # V Claude Code: agent zavolá mcp__filesystem__read_file
    # Ekvivalentní Python kód:
    soubory = list(Path(".").glob("**/*.py"))
    print(f"Nalezeno Python souborů: {len(soubory)}")
    for s in soubory[:3]:
        print(f"  - {s}")


def ukazka_struktury_projektu():
    """MCP filesystem: procházení adresářové struktury."""
    print("\n=== MCP Filesystem: Struktura projektu ===")

    def vypis_strom(cesta: Path, uroven: int = 0):
        odsazeni = "  " * uroven
        print(f"{odsazeni}{'📁' if cesta.is_dir() else '📄'} {cesta.name}")
        if cesta.is_dir() and uroven < 2:
            for polozka in sorted(cesta.iterdir())[:5]:
                vypis_strom(polozka, uroven + 1)

    vypis_strom(Path("."))


def ukazka_progress_db():
    """Ukázka formátu progress.json pro sledování pokroku."""
    print("\n=== Progress Tracking ===")

    progress = {
        "student": "Ivan",
        "started": "2026-05-01",
        "lessons": {
            "zakladni_datove_typy": {
                "status": "completed",
                "tests_passed": 3,
                "tests_total": 3,
                "attempts": 2
            },
            "podminky_a_cykly": {
                "status": "in_progress",
                "tests_passed": 2,
                "tests_total": 4,
                "attempts": 5
            },
            "funkce": {
                "status": "not_started",
                "tests_passed": 0,
                "tests_total": 3,
                "attempts": 0
            }
        }
    }

    print(json.dumps(progress, indent=2, ensure_ascii=False))

    celkem = sum(l["tests_total"] for l in progress["lessons"].values())
    splneno = sum(l["tests_passed"] for l in progress["lessons"].values())
    print(f"\nCelkový pokrok: {splneno}/{celkem} testů ({splneno/celkem*100:.0f}%)")


if __name__ == "__main__":
    ukazka_cteni_souboru()
    ukazka_struktury_projektu()
    ukazka_progress_db()

    print("\n" + "="*50)
    print("Jak použít v Claude Code:")
    print("  claude '/lesson funkce'")
    print("  claude '/check exercises/funkce_cviceni.py'")
    print("  claude '/progress'")
    print("  claude '/explain lambda funkce'")
