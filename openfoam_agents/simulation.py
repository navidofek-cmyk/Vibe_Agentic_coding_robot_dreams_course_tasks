"""
OpenFOAM Learning Assistant – Student-Teacher Simulation
Agenti běží přes `claude -p` CLI (OAuth, žádný API klíč).

Spuštění:
    uv run python simulation.py
    # nebo přímo:
    python simulation.py
"""

import re
import subprocess
import sys
from pathlib import Path

ROOT     = Path(__file__).parent
CASE_DIR = ROOT / "workspace/cases/cavity_cviceni"
TESTS    = ROOT / "workspace/tests/test_cavity.py"

MAX_ITERATIONS = 6
MODEL          = "haiku"   # rychlý a levný; změň na "sonnet" pro lepší výsledky

TODO_FILES = ["0/U", "system/controlDict"]

# ── System prompty ──────────────────────────────────────────────────────────

STUDENT_SYSTEM = (
    "Jsi student učící se OpenFOAM CFD simulace. "
    "Rozumíš základní fyzice tekutin, ale v OpenFOAM jsi nový/á. "
    "Pokud dostaneš soubor s ??? nebo TODO, DOPLŇ je skutečnými hodnotami. "
    "Odpovídáš POUZE kompletním obsahem souboru – nic jiného nepíšeš. "
    "Nepiš žádné vysvětlení, jen soubor."
)

TEACHER_SYSTEM = (
    "Jsi sókratovský mentor OpenFOAM. Nikdy nedáváš přímou odpověď. "
    "Na každé selhání odpovíš 1–2 cílenými otázkami, ne řešením. "
    "Odkazuj na konkrétní test, který selhal, a co očekával. "
    "Maximum 80 slov. Piš česky."
)


def claude(system: str, prompt: str) -> str:
    """Zavolá `claude -p` s daným system promptem a promptem."""
    result = subprocess.run(
        [
            "claude", "-p", prompt,
            "--system-prompt", system,
            "--model", MODEL,
            "--tools", "",               # žádné nástroje – čistý text output
            "--no-session-persistence",  # každé volání je nezávislé
            "--output-format", "text",
        ],
        capture_output=True, text=True, cwd=ROOT,
    )
    if result.returncode != 0:
        print(f"  [CLI chyba]: {result.stderr.strip()[:200]}", file=sys.stderr)
    text = result.stdout.strip()
    # Odstraň markdown code block pokud ho Claude přidal
    if text.startswith("```"):
        text = re.sub(r"^```[a-z+]*\n?", "", text)
        text = re.sub(r"\n?```$", "", text)
    return text


def read_files() -> dict[str, str]:
    return {p: (CASE_DIR / p).read_text() for p in TODO_FILES}


def write_files(files: dict[str, str]):
    for path, content in files.items():
        (CASE_DIR / path).write_text(content)


def run_pytest() -> tuple[int, int, str]:
    result = subprocess.run(
        [sys.executable, "-m", "pytest", str(TESTS), "-v", "--tb=short", "--no-header", "-q"],
        capture_output=True, text=True, cwd=ROOT,
    )
    out = result.stdout + result.stderr
    passed = out.count(" PASSED")
    failed = out.count(" FAILED") + out.count(" ERROR")
    return passed, passed + failed, out


def student_fill(files: dict[str, str], feedback: str) -> dict[str, str]:
    filled = {}
    for path, content in files.items():
        if "???" not in content:
            filled[path] = content
            continue

        prompt = (
            f"Doplň ??? v tomto OpenFOAM souboru ({path}).\n\n"
            + (f"Zpětná vazba učitele:\n{feedback}\n\n" if feedback else "")
            + f"Soubor:\n{content}"
        )
        filled[path] = claude(STUDENT_SYSTEM, prompt)

    return filled


def teacher_response(pytest_out: str, passed: int, total: int) -> str:
    prompt = (
        f"Student dosáhl {passed}/{total} testů.\n\n"
        f"Výstup pytest:\n{pytest_out[-1200:]}"
    )
    return claude(TEACHER_SYSTEM, prompt)


def show_changes(original: str, filled: str, path: str):
    changes = [
        (i + 1, o.strip(), f.strip())
        for i, (o, f) in enumerate(zip(original.splitlines(), filled.splitlines()))
        if o != f
    ]
    if changes:
        print(f"    Změny v {path}:")
        for lineno, old, new in changes[:5]:
            print(f"      ř.{lineno}: '{old}' → '{new}'")


def main():
    print("OpenFOAM Learning Assistant – Simulace student/učitel (via claude CLI)")
    print("=" * 68)

    original = read_files()
    todos = {p: c for p, c in original.items() if "???" in c}

    if not todos:
        print("Žádné ??? nenalezeny – spouštím testy přímo.")
    else:
        print(f"Soubory k doplnění: {list(todos.keys())}")

    feedback = ""
    success  = False

    for iteration in range(1, MAX_ITERATIONS + 1):
        print(f"\n{'─'*68}")
        print(f"Iterace {iteration}/{MAX_ITERATIONS}")
        print(f"{'─'*68}")

        current = read_files()

        # Student doplní soubory
        print("  Student doplňuje soubory...")
        filled = student_fill(current, feedback)

        for path in todos:
            show_changes(current[path], filled[path], path)

        write_files(filled)

        # Pytest
        passed, total, pytest_out = run_pytest()
        print(f"\n  Výsledky: {passed}/{total} testů prošlo")

        for line in pytest_out.splitlines():
            if "PASSED" in line:
                print(f"    ✓ {line.strip()[:70]}")
            elif "FAILED" in line or "AssertionError" in line:
                print(f"    ✗ {line.strip()[:70]}")

        if passed == total and total > 0:
            print(f"\n✅  Student splnil všechna cvičení v iteraci {iteration}!")
            success = True
            break

        # Učitel
        print("\n  Učitel přemýšlí...")
        feedback = teacher_response(pytest_out, passed, total)
        print(f"\n  Učitel: {feedback}")

        # Obnov originál pro příští iteraci
        write_files(original)

    if not success:
        print(f"\n⚠️  Student nedosáhl 100 % v {MAX_ITERATIONS} iteracích.")

    write_files(original)
    print("\nOriginální soubory s ??? obnoveny.")


if __name__ == "__main__":
    main()
