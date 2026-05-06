"""
Testy ukázkového workspace – cvičení, testy, struktura
"""
import pytest
from pathlib import Path
import ast

WORKSPACE = Path(__file__).parent.parent / "workspace"
EXERCISES = WORKSPACE / "exercises"
TESTS_DIR = WORKSPACE / "tests"


def test_workspace_existuje():
    assert WORKSPACE.exists()


def test_exercises_adresar_existuje():
    assert EXERCISES.exists()


def test_tests_adresar_existuje():
    assert TESTS_DIR.exists()


def test_ukazka_cviceni_existuje():
    assert (EXERCISES / "ukazka_cviceni.py").exists()


def test_ukazka_testy_existuji():
    assert (TESTS_DIR / "test_ukazka_cviceni.py").exists()


def test_ukazka_cviceni_je_validni_python():
    kod = (EXERCISES / "ukazka_cviceni.py").read_text()
    try:
        ast.parse(kod)
    except SyntaxError as e:
        pytest.fail(f"ukazka_cviceni.py má syntax error: {e}")


def test_ukazka_cviceni_ma_fibonacci():
    kod = (EXERCISES / "ukazka_cviceni.py").read_text()
    assert "def fibonacci" in kod, "Cvičení neobsahuje funkci fibonacci"


def test_ukazka_cviceni_ma_faktorial():
    kod = (EXERCISES / "ukazka_cviceni.py").read_text()
    assert "def faktorial" in kod, "Cvičení neobsahuje funkci faktorial"


def test_ukazka_cviceni_ma_todo():
    """Student musí vidět co má doplnit."""
    kod = (EXERCISES / "ukazka_cviceni.py").read_text()
    assert "TODO" in kod, "Cvičení neobsahuje TODO komentář"
    assert "pass" in kod, "Cvičení neobsahuje pass (prázdná implementace)"


def test_ukazka_testy_jsou_validni_python():
    kod = (TESTS_DIR / "test_ukazka_cviceni.py").read_text()
    try:
        ast.parse(kod)
    except SyntaxError as e:
        pytest.fail(f"test_ukazka_cviceni.py má syntax error: {e}")


def test_ukazka_testy_importuji_cviceni():
    kod = (TESTS_DIR / "test_ukazka_cviceni.py").read_text()
    assert "from exercises.ukazka_cviceni import" in kod or \
           "import ukazka_cviceni" in kod, \
        "test_ukazka_cviceni.py neimportuje z exercises/"


def test_ukazka_testy_maji_alespon_4_testy():
    kod = (TESTS_DIR / "test_ukazka_cviceni.py").read_text()
    pocet = kod.count("def test_")
    assert pocet >= 4, f"Jen {pocet} testy – očekáváme alespoň 4"


def test_kazde_cviceni_ma_test():
    """Pro každý soubor v exercises/ musí existovat test v tests/."""
    cviceni = list(EXERCISES.glob("*.py"))
    for soubor in cviceni:
        ocekavany_test = TESTS_DIR / f"test_{soubor.name}"
        assert ocekavany_test.exists(), \
            f"Chybí testovací soubor pro {soubor.name}: {ocekavany_test}"
