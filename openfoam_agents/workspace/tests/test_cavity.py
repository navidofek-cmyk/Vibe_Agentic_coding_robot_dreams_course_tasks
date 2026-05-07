"""
Validace cvičení 1 – Lid-Driven Cavity
Testy ověřují, zda student správně doplnil OpenFOAM case soubory.
"""

import re
from pathlib import Path

CASE = Path(__file__).parent.parent / "cases" / "cavity_cviceni"


def _read(rel_path: str) -> str:
    return (CASE / rel_path).read_text()


class TestControlDict:
    def test_endTime_is_set(self):
        content = _read("system/controlDict")
        match = re.search(r"endTime\s+([0-9.]+)\s*;", content)
        assert match, "endTime není nastaveno (stále obsahuje ???)"
        assert float(match.group(1)) >= 0.5, "endTime musí být alespoň 0.5 s"

    def test_deltaT_is_set(self):
        content = _read("system/controlDict")
        match = re.search(r"deltaT\s+([0-9.eE+-]+)\s*;", content)
        assert match, "deltaT není nastaveno (stále obsahuje ???)"
        assert float(match.group(1)) <= 0.005, "deltaT je příliš velké – porušuje podmínku CFL"

    def test_no_todo_placeholders(self):
        content = _read("system/controlDict")
        assert "???" not in content, "V controlDict jsou stále nedoplněné hodnoty (???)"


class TestVelocityBC:
    def test_moving_wall_velocity_set(self):
        content = _read("0/U")
        assert "???" not in content, "Rychlost víka není nastavena – soubor 0/U stále obsahuje ???"

    def test_moving_wall_has_fixed_value(self):
        content = _read("0/U")
        moving_section = re.search(
            r"movingWall\s*\{(.+?)\}", content, re.DOTALL
        )
        assert moving_section, "Sekce movingWall nebyla nalezena v 0/U"
        assert "fixedValue" in moving_section.group(1), \
            "movingWall musí mít typ fixedValue"

    def test_moving_wall_x_velocity_positive(self):
        content = _read("0/U")
        # Hledáme blok movingWall { ... value uniform (X ...) }
        block = re.search(r"movingWall\s*\{([^}]+)\}", content, re.DOTALL)
        assert block, "Blok movingWall nebyl nalezen v 0/U"
        match = re.search(r"value\s+uniform\s*\(\s*([0-9.+-]+)", block.group(1))
        assert match, "Hodnota rychlosti víka není ve správném formátu"
        assert float(match.group(1)) > 0, "Rychlost víka musí být kladná (pohyb v ose x)"

    def test_fixed_walls_no_slip(self):
        content = _read("0/U")
        assert "noSlip" in content, "fixedWalls musí mít typ noSlip"


class TestRequiredFiles:
    def test_all_required_files_exist(self):
        required = [
            "0/U",
            "0/p",
            "constant/transportProperties",
            "system/controlDict",
            "system/fvSchemes",
            "system/fvSolution",
            "system/blockMeshDict",
        ]
        missing = [f for f in required if not (CASE / f).exists()]
        assert not missing, f"Chybí soubory: {missing}"
