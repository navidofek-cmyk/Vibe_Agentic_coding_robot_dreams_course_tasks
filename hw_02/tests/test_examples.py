"""
Testy ukázkových skriptů v claude-code/examples/ a codex/examples/
"""
import ast
import json
import sys
import importlib.util
import pytest
from pathlib import Path

CLAUDE_EXAMPLES = Path(__file__).parent.parent / "claude-code" / "examples"
CODEX_EXAMPLES  = Path(__file__).parent.parent / "codex" / "examples"


def nacti_modul(cesta: Path):
    """Dynamicky načte Python soubor jako modul."""
    spec = importlib.util.spec_from_file_location(cesta.stem, cesta)
    modul = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(modul)
    return modul


def parsuj(cesta: Path):
    return ast.parse(cesta.read_text())


# ── Existence souborů ──────────────────────────────────────────────────

def test_mcp_filesystem_demo_existuje():
    assert (CLAUDE_EXAMPLES / "mcp_filesystem_demo.py").exists()


def test_subagent_demo_existuje():
    assert (CLAUDE_EXAMPLES / "subagent_demo.py").exists()


def test_refactor_demo_existuje():
    assert (CODEX_EXAMPLES / "refactor_demo.py").exists()


# ── Syntax ────────────────────────────────────────────────────────────

@pytest.mark.parametrize("soubor", [
    "claude-code/examples/mcp_filesystem_demo.py",
    "claude-code/examples/subagent_demo.py",
    "codex/examples/refactor_demo.py",
])
def test_soubor_ma_validni_python_syntax(soubor):
    cesta = Path(__file__).parent.parent / soubor
    try:
        ast.parse(cesta.read_text())
    except SyntaxError as e:
        pytest.fail(f"Syntax error v {soubor}: {e}")


# ── mcp_filesystem_demo.py ────────────────────────────────────────────

@pytest.fixture(scope="module")
def mcp_demo():
    return nacti_modul(CLAUDE_EXAMPLES / "mcp_filesystem_demo.py")


def test_mcp_demo_ma_funkci_cteni(mcp_demo):
    assert hasattr(mcp_demo, "ukazka_cteni_souboru"), \
        "Chybí funkce ukazka_cteni_souboru()"


def test_mcp_demo_ma_funkci_struktury(mcp_demo):
    assert hasattr(mcp_demo, "ukazka_struktury_projektu"), \
        "Chybí funkce ukazka_struktury_projektu()"


def test_mcp_demo_ma_funkci_progress(mcp_demo):
    assert hasattr(mcp_demo, "ukazka_progress_db"), \
        "Chybí funkce ukazka_progress_db()"


def test_mcp_demo_progress_vraci_spravny_pocet(mcp_demo, capsys):
    """ukazka_progress_db() musí spočítat pokrok ze 3 lekcí."""
    mcp_demo.ukazka_progress_db()
    vystup = capsys.readouterr().out
    assert "10" in vystup, "Celkový počet testů (10) není ve výstupu"
    assert "5" in vystup, "Počet splněných testů (5) není ve výstupu"


def test_mcp_demo_progress_json_je_validni(mcp_demo):
    """Progress data musí být serializovatelná jako JSON."""
    import io
    from contextlib import redirect_stdout

    f = io.StringIO()
    with redirect_stdout(f):
        mcp_demo.ukazka_progress_db()

    vystup = f.getvalue()
    # Najdi JSON blok (od první { do poslední })
    zacatek = vystup.find("{")
    konec = vystup.rfind("}") + 1
    assert zacatek != -1, "Výstup neobsahuje JSON"
    json_cast = vystup[zacatek:konec]
    try:
        data = json.loads(json_cast)
    except json.JSONDecodeError as e:
        pytest.fail(f"Progress JSON není validní: {e}")
    assert "student" in data
    assert "lessons" in data
    assert len(data["lessons"]) == 3


def test_mcp_demo_cteni_najde_python_soubory(mcp_demo, capsys, tmp_path, monkeypatch):
    """ukazka_cteni_souboru() musí najít alespoň jeden .py soubor."""
    monkeypatch.chdir(tmp_path)
    (tmp_path / "test.py").write_text("pass")
    (tmp_path / "another.py").write_text("pass")

    mcp_demo.ukazka_cteni_souboru()
    vystup = capsys.readouterr().out
    assert "Nalezeno Python souborů:" in vystup
    # V tmp_path jsou 2 soubory
    assert "2" in vystup or "1" in vystup


# ── subagent_demo.py ──────────────────────────────────────────────────

@pytest.fixture(scope="module")
def subagent_demo():
    return nacti_modul(CLAUDE_EXAMPLES / "subagent_demo.py")


def test_subagent_demo_ma_workflow_konstantu(subagent_demo):
    assert hasattr(subagent_demo, "SUBAGENT_WORKFLOW_PRIKLAD"), \
        "Chybí SUBAGENT_WORKFLOW_PRIKLAD konstanta"


def test_subagent_workflow_zminuje_tri_agenty(subagent_demo):
    workflow = subagent_demo.SUBAGENT_WORKFLOW_PRIKLAD
    assert "Explore" in workflow, "Workflow nezmiňuje Explore agenta"
    assert "Plan" in workflow, "Workflow nezmiňuje Plan agenta"
    assert "general-purpose" in workflow, "Workflow nezmiňuje general-purpose agenta"


def test_subagent_workflow_zminuje_paralelni_beh(subagent_demo):
    workflow = subagent_demo.SUBAGENT_WORKFLOW_PRIKLAD
    assert "PARALELNĚ" in workflow or "paralelně" in workflow, \
        "Workflow nezmiňuje paralelní běh agentů"


def test_subagent_workflow_zminuje_casovou_usporu(subagent_demo):
    """Demo musí ukázat proč paralelní běh pomáhá (časová úspora)."""
    workflow = subagent_demo.SUBAGENT_WORKFLOW_PRIKLAD
    assert "35s" in workflow or "90s" in workflow or "sekvenčně" in workflow, \
        "Workflow neobsahuje porovnání času sekvenční vs paralelní"


def test_subagent_demo_ma_funkci_lekce(subagent_demo):
    assert hasattr(subagent_demo, "vytvor_ukazku_lekce_rekurze"), \
        "Chybí funkce vytvor_ukazku_lekce_rekurze()"


def test_subagent_lekce_rekurze_obsahuje_kod(subagent_demo):
    lekce = subagent_demo.vytvor_ukazku_lekce_rekurze()
    assert isinstance(lekce, str)
    assert "def faktorial" in lekce, "Ukázková lekce neobsahuje příklad faktoriálu"
    assert "base case" in lekce.lower() or "base" in lekce, \
        "Ukázková lekce nezmiňuje base case"


def test_subagent_lekce_rekurze_obsahuje_navod(subagent_demo):
    lekce = subagent_demo.vytvor_ukazku_lekce_rekurze()
    assert "pytest" in lekce, "Ukázková lekce neobsahuje instrukci ke spuštění testů"


def test_subagent_demo_ma_popis_systemu(subagent_demo):
    assert hasattr(subagent_demo, "popis_subagentniho_systemu"), \
        "Chybí funkce popis_subagentniho_systemu()"


# ── refactor_demo.py ──────────────────────────────────────────────────

@pytest.fixture(scope="module")
def refactor_demo():
    return nacti_modul(CODEX_EXAMPLES / "refactor_demo.py")


def test_refactor_ma_puvodni_funkce(refactor_demo):
    """Demo musí obsahovat záměrně špatný kód (pro ukázku)."""
    assert hasattr(refactor_demo, "f"), "Chybí původní funkce f()"
    assert hasattr(refactor_demo, "g"), "Chybí původní funkce g()"
    assert hasattr(refactor_demo, "h"), "Chybí původní funkce h()"


def test_refactor_ma_refaktorovane_funkce(refactor_demo):
    assert hasattr(refactor_demo, "ctverec_sudych_cisel")
    assert hasattr(refactor_demo, "stridat_velikost_pismen")
    assert hasattr(refactor_demo, "pocet_vyskytu")


def test_refactor_puvodni_a_nova_funkce_jsou_ekvivalentni(refactor_demo):
    """Původní f() a refaktorovaná verze musí vracet stejný výsledek."""
    vstup = [1, 2, 3, 4, 5, 6, 7, 8]
    assert refactor_demo.f(vstup) == refactor_demo.ctverec_sudych_cisel(vstup), \
        "f() a ctverec_sudych_cisel() vrací různé výsledky"


@pytest.mark.parametrize("vstup,ocekavany", [
    ([1, 2, 3, 4],  [4, 16]),
    ([2, 4, 6],     [4, 16, 36]),
    ([1, 3, 5],     []),
    ([],            []),
    ([-2, -1, 0, 1, 2], [4, 0, 4]),
])
def test_ctverec_sudych_cisel(refactor_demo, vstup, ocekavany):
    assert refactor_demo.ctverec_sudych_cisel(vstup) == ocekavany


@pytest.mark.parametrize("vstup,ocekavany", [
    ("hello",  "HeLlO"),
    ("python", "PyThOn"),
    ("a",      "A"),
    ("",       ""),
    ("ab",     "Ab"),
])
def test_stridat_velikost_pismen(refactor_demo, vstup, ocekavany):
    assert refactor_demo.stridat_velikost_pismen(vstup) == ocekavany


@pytest.mark.parametrize("vstup,ocekavany", [
    (["a", "b", "a"],         {"a": 2, "b": 1}),
    ([1, 1, 1],                {1: 3}),
    ([],                       {}),
    (["x"],                    {"x": 1}),
    (["a", "b", "c", "a", "b"], {"a": 2, "b": 2, "c": 1}),
])
def test_pocet_vyskytu(refactor_demo, vstup, ocekavany):
    assert refactor_demo.pocet_vyskytu(vstup) == ocekavany


def test_refactor_ma_otazky_pro_studenta(refactor_demo):
    assert hasattr(refactor_demo, "OTAZKY_PRO_STUDENTA"), \
        "Chybí OTAZKY_PRO_STUDENTA"
    assert isinstance(refactor_demo.OTAZKY_PRO_STUDENTA, list)
    assert len(refactor_demo.OTAZKY_PRO_STUDENTA) >= 3, \
        "Mělo by být alespoň 3 otázky pro studenta"


def test_refactor_otazky_zminuji_list_comprehension(refactor_demo):
    otazky = " ".join(refactor_demo.OTAZKY_PRO_STUDENTA).lower()
    assert "list comprehension" in otazky or "comprehension" in otazky, \
        "Otázky nezmiňují list comprehension"
