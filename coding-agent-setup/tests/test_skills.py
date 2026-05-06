"""
Testy skills (slash příkazů) v .claude/commands/
"""
import pytest
from pathlib import Path

COMMANDS_DIR = Path(__file__).parent.parent / "claude-code" / ".claude" / "commands"

OCEKAVANE_SKILLS = ["lesson.md", "check.md", "explain.md", "progress.md", "subagent-workflow.md"]


@pytest.fixture(scope="module")
def skill_soubory():
    return {f.name: f.read_text() for f in COMMANDS_DIR.glob("*.md")}


def test_commands_adresar_existuje():
    assert COMMANDS_DIR.exists(), ".claude/commands/ adresář neexistuje"


@pytest.mark.parametrize("nazev", OCEKAVANE_SKILLS)
def test_skill_existuje(nazev):
    soubor = COMMANDS_DIR / nazev
    assert soubor.exists(), f"Skill '{nazev}' neexistuje"


@pytest.mark.parametrize("nazev", OCEKAVANE_SKILLS)
def test_skill_neni_prazdny(nazev):
    soubor = COMMANDS_DIR / nazev
    obsah = soubor.read_text().strip()
    assert len(obsah) > 50, f"Skill '{nazev}' je příliš krátký ({len(obsah)} znaků)"


@pytest.mark.parametrize("nazev", OCEKAVANE_SKILLS)
def test_skill_ma_h1_nadpis(nazev):
    obsah = (COMMANDS_DIR / nazev).read_text()
    radky = obsah.splitlines()
    assert any(r.startswith("# ") for r in radky), \
        f"Skill '{nazev}' nemá H1 nadpis (# Název)"


@pytest.mark.parametrize("nazev", ["lesson.md", "check.md", "explain.md", "subagent-workflow.md"])
def test_skill_ma_arguments_placeholder(nazev):
    """Skills které přijímají argument musí obsahovat $ARGUMENTS."""
    obsah = (COMMANDS_DIR / nazev).read_text()
    assert "$ARGUMENTS" in obsah, \
        f"Skill '{nazev}' neobsahuje $ARGUMENTS placeholder"


@pytest.mark.parametrize("nazev", OCEKAVANE_SKILLS)
def test_skill_ma_instrukce(nazev):
    """Každý skill musí mít sekci Instrukce nebo Krok."""
    obsah = (COMMANDS_DIR / nazev).read_text()
    has_instrukce = "## Instrukce" in obsah or "### Krok" in obsah or "## Krok" in obsah
    assert has_instrukce, \
        f"Skill '{nazev}' nemá instrukce pro agenta"


def test_check_skill_obsahuje_pytest(skill_soubory):
    """check.md musí spouštět pytest."""
    assert "pytest" in skill_soubory["check.md"], \
        "check.md nespouští pytest"


def test_check_skill_neposkytuje_reseni(skill_soubory):
    """Mentor nesmí psát opravy za studenta – musí se ptát."""
    obsah = skill_soubory["check.md"].lower()
    assert "otázka" in obsah or "otázku" in obsah or "zamysli" in obsah, \
        "check.md neobsahuje pedagogickou zpětnou vazbu (otázky pro studenta)"


def test_lesson_skill_vytvari_tri_soubory(skill_soubory):
    """Lekce musí vytvořit lekci, cvičení i testy."""
    obsah = skill_soubory["lesson.md"]
    assert "lessons/" in obsah, "lesson.md nevytváří soubor v lessons/"
    assert "exercises/" in obsah, "lesson.md nevytváří soubor v exercises/"
    assert "tests/" in obsah, "lesson.md nevytváří soubor v tests/"


def test_workflow_skill_zminuje_sub_agenty(skill_soubory):
    """workflow.md musí dokumentovat použití sub-agentů."""
    obsah = skill_soubory["subagent-workflow.md"]
    assert "Explore" in obsah, "workflow.md nezmiňuje Explore agenta"
    assert "Plan" in obsah, "workflow.md nezmiňuje Plan agenta"
    assert "paralelně" in obsah or "paralel" in obsah or "simultánně" in obsah, \
        "workflow.md nezmiňuje paralelní běh"


def test_progress_skill_obsahuje_sql(skill_soubory):
    """progress.md musí používat SQLite pro data."""
    obsah = skill_soubory["progress.md"]
    assert "SELECT" in obsah or "sqlite" in obsah.lower(), \
        "progress.md nepoužívá SQLite"
