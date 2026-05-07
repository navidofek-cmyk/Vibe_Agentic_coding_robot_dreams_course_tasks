"""
Testy pro simulaci mentor↔student session.
Spouštěno přes: /simulate <téma>
Student agent zapíše řešení do exercises/simulate_cviceni.py
"""
import pytest
import importlib
import sys
from pathlib import Path


def load_module():
    """Načte simulate_cviceni.py dynamicky – student ho zapíše za běhu."""
    path = Path(__file__).parent.parent / "exercises" / "simulate_cviceni.py"
    if not path.exists():
        pytest.skip("simulate_cviceni.py ještě neexistuje – čeká se na student agenta")
    spec = importlib.util.spec_from_file_location("simulate_cviceni", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


# ── faktorial ────────────────────────────────────────────────────

class TestFaktorial:
    def setup_method(self):
        mod = load_module()
        if not hasattr(mod, "faktorial"):
            pytest.skip("funkce faktorial nenalezena v simulate_cviceni.py")
        self.fn = mod.faktorial

    def test_base_case(self):
        assert self.fn(0) == 1
        assert self.fn(1) == 1

    def test_male_hodnoty(self):
        assert self.fn(2) == 2
        assert self.fn(3) == 6
        assert self.fn(4) == 24

    def test_vetsi_hodnoty(self):
        assert self.fn(5) == 120
        assert self.fn(10) == 3628800


# ── fibonacci ────────────────────────────────────────────────────

class TestFibonacci:
    def setup_method(self):
        mod = load_module()
        if not hasattr(mod, "fibonacci"):
            pytest.skip("funkce fibonacci nenalezena")
        self.fn = mod.fibonacci

    def test_base_case(self):
        assert self.fn(0) == 0
        assert self.fn(1) == 1

    def test_dalsi(self):
        assert self.fn(2) == 1
        assert self.fn(5) == 5
        assert self.fn(6) == 8
        assert self.fn(10) == 55


# ── obecná funkce (cokoliv student napíše) ───────────────────────

class TestObecna:
    def setup_method(self):
        self.mod = load_module()

    def test_modul_se_nacte(self):
        """Základní test – soubor musí jít importovat bez chyby."""
        assert self.mod is not None

    def test_obsahuje_alespon_jednu_funkci(self):
        """Student musí napsat alespoň jednu funkci."""
        funkce = [k for k, v in vars(self.mod).items()
                  if callable(v) and not k.startswith("_")]
        assert len(funkce) >= 1, f"Soubor neobsahuje žádnou funkci"
