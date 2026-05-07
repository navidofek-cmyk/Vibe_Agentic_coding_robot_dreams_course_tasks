"""
Validace cvičení 2 – Pitz Daily (turbulentní proudění)
Testy ověřují počáteční podmínky turbulence a relaxační faktory.
"""

import re
from pathlib import Path

CASE = Path(__file__).parent.parent / "cases" / "pitzDaily_cviceni"


def _read(rel_path: str) -> str:
    return (CASE / rel_path).read_text()


class TestTurbulenceIC:
    def test_k_internal_field_set(self):
        content = _read("0/k")
        assert "???" not in content, "Hodnota k není doplněna (stále obsahuje ???)"

    def test_k_value_reasonable(self):
        content = _read("0/k")
        match = re.search(r"internalField\s+uniform\s+([0-9.eE+-]+)\s*;", content)
        assert match, "internalField pro k není ve správném formátu"
        k = float(match.group(1))
        assert 0.05 < k < 5.0, \
            f"Hodnota k={k} je mimo očekávaný rozsah (0.05–5.0 m²/s²). Zkontroluj výpočet: k = 1.5*(U*I)²"

    def test_epsilon_internal_field_set(self):
        content = _read("0/epsilon")
        assert "???" not in content, "Hodnota epsilon není doplněna (stále obsahuje ???)"

    def test_epsilon_value_reasonable(self):
        content = _read("0/epsilon")
        match = re.search(r"internalField\s+uniform\s+([0-9.eE+-]+)\s*;", content)
        assert match, "internalField pro epsilon není ve správném formátu"
        eps = float(match.group(1))
        assert 1.0 < eps < 10000.0, \
            f"Hodnota epsilon={eps} je mimo očekávaný rozsah (1–10000 m²/s³)"


class TestRelaxationFactors:
    def test_no_todo_in_fvSolution(self):
        content = _read("system/fvSolution")
        assert "???" not in content, "Relaxační faktory nejsou nastaveny (stále obsahují ???)"

    def test_pressure_relaxation_valid(self):
        content = _read("system/fvSolution")
        match = re.search(r"fields\s*\{[^}]*p\s+([0-9.]+)\s*;", content, re.DOTALL)
        assert match, "Relaxační faktor pro tlak p nebyl nalezen"
        rf = float(match.group(1))
        assert 0.1 <= rf <= 0.5, \
            f"Relaxační faktor tlaku {rf} je mimo doporučený rozsah (0.1–0.5)"

    def test_velocity_relaxation_valid(self):
        content = _read("system/fvSolution")
        match = re.search(r"equations\s*\{[^}]*U\s+([0-9.]+)\s*;", content, re.DOTALL)
        assert match, "Relaxační faktor pro rychlost U nebyl nalezen"
        rf = float(match.group(1))
        assert 0.3 <= rf <= 0.9, \
            f"Relaxační faktor pro U={rf} je mimo doporučený rozsah (0.3–0.9)"


class TestRequiredFiles:
    def test_all_required_files_exist(self):
        required = [
            "0/U", "0/p", "0/k", "0/epsilon",
            "constant/transportProperties",
            "constant/turbulenceProperties",
            "system/controlDict",
            "system/fvSchemes",
            "system/fvSolution",
        ]
        missing = [f for f in required if not (CASE / f).exists()]
        assert not missing, f"Chybí soubory: {missing}"
