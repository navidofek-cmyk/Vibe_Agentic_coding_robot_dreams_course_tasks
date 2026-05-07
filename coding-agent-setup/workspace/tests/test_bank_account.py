"""
Testy pro projekt: BankAccount
Student agent zapíše exercises/bank_account.py
"""
import pytest
import importlib.util
from pathlib import Path


def load_module():
    path = Path(__file__).parent.parent / "exercises" / "bank_account.py"
    if not path.exists():
        pytest.skip("bank_account.py neexistuje – čeká se na student agenta")
    spec = importlib.util.spec_from_file_location("bank_account", path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


@pytest.fixture
def BankAccount():
    mod = load_module()
    if not hasattr(mod, "BankAccount"):
        pytest.skip("třída BankAccount nenalezena")
    return mod.BankAccount


# ── Základní funkčnost ────────────────────────────────────────────

def test_vytvoreni_uctu(BankAccount):
    ucet = BankAccount("Pavel", 1000)
    assert ucet.get_balance() == 1000

def test_vychozi_zustatek(BankAccount):
    ucet = BankAccount("Anna")
    assert ucet.get_balance() == 0

def test_vklad(BankAccount):
    ucet = BankAccount("Jana", 500)
    ucet.deposit(200)
    assert ucet.get_balance() == 700

def test_vyber(BankAccount):
    ucet = BankAccount("Petr", 1000)
    ucet.withdraw(300)
    assert ucet.get_balance() == 700

# ── Validace ──────────────────────────────────────────────────────

def test_vyber_nedostatek_penez(BankAccount):
    ucet = BankAccount("Eva", 100)
    with pytest.raises((ValueError, Exception)):
        ucet.withdraw(500)

def test_zaporna_castka_vklad(BankAccount):
    ucet = BankAccount("Tom", 100)
    with pytest.raises((ValueError, Exception)):
        ucet.deposit(-50)

def test_zaporna_castka_vyber(BankAccount):
    ucet = BankAccount("Lea", 100)
    with pytest.raises((ValueError, Exception)):
        ucet.withdraw(-10)

def test_zustatek_nikdy_zaporn(BankAccount):
    ucet = BankAccount("Max", 200)
    try:
        ucet.withdraw(300)
    except Exception:
        pass
    assert ucet.get_balance() >= 0

# ── Historie transakcí ────────────────────────────────────────────

def test_historie_existuje(BankAccount):
    ucet = BankAccount("Mia", 0)
    assert hasattr(ucet, "history") or hasattr(ucet, "transactions") or \
           hasattr(ucet, "historie") or hasattr(ucet, "transakce")

def test_vlastnik(BankAccount):
    ucet = BankAccount("Karel", 500)
    assert hasattr(ucet, "owner") or hasattr(ucet, "vlastnik") or \
           hasattr(ucet, "name") or hasattr(ucet, "jmeno")
