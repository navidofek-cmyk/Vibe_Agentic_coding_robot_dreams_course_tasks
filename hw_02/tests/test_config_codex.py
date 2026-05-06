"""
Testy konfigurace Codex CLI (config.yaml + AGENTS.md)
"""
import pytest
from pathlib import Path

CODEX_DIR = Path(__file__).parent.parent / "codex"
CONFIG = CODEX_DIR / "config.yaml"
AGENTS_MD = CODEX_DIR / "AGENTS.md"

PLATNE_MODELY = {"o4-mini", "o3", "gpt-4o", "gpt-4o-mini", "o4-mini-high"}
PLATNE_APPROVALS = {"suggest", "auto-edit", "full-auto"}


@pytest.fixture(scope="module")
def config_text():
    return CONFIG.read_text()


@pytest.fixture(scope="module")
def agents_text():
    return AGENTS_MD.read_text()


def test_config_soubor_existuje():
    assert CONFIG.exists(), "config.yaml neexistuje"


def test_agents_md_existuje():
    assert AGENTS_MD.exists(), "AGENTS.md neexistuje"


def test_config_obsahuje_model(config_text):
    assert "model:" in config_text, "config.yaml neobsahuje 'model:'"


def test_config_model_je_platny(config_text):
    for radek in config_text.splitlines():
        if radek.strip().startswith("model:"):
            model = radek.split(":", 1)[1].strip()
            assert model in PLATNE_MODELY, \
                f"Neznámý model '{model}'. Platné: {PLATNE_MODELY}"
            return
    pytest.fail("Řádek s modelem nenalezen")


def test_config_obsahuje_approval(config_text):
    assert "approval:" in config_text, "config.yaml neobsahuje 'approval:'"


def test_config_approval_je_platny(config_text):
    for radek in config_text.splitlines():
        if radek.strip().startswith("approval:"):
            approval = radek.split(":", 1)[1].strip()
            assert approval in PLATNE_APPROVALS, \
                f"Neznámý approval '{approval}'. Platné: {PLATNE_APPROVALS}"
            return
    pytest.fail("Řádek s approval nenalezen")


def test_config_nema_full_auto_pro_vyuku(config_text):
    """full-auto je nebezpečný pro výukové prostředí."""
    for radek in config_text.splitlines():
        if radek.strip().startswith("approval:"):
            approval = radek.split(":", 1)[1].strip()
            assert approval != "full-auto", \
                "Pro výukové prostředí nepoužívej full-auto – student ztrácí kontrolu"


def test_agents_md_ma_roli(agents_text):
    assert "## Role" in agents_text or "# Role" in agents_text, \
        "AGENTS.md nemá sekci Role"


def test_agents_md_ma_povolene_prikazy(agents_text):
    assert "pytest" in agents_text, \
        "AGENTS.md neobsahuje pytest (agent by měl umět spouštět testy)"


def test_agents_md_ma_zakazane_prikazy(agents_text):
    # Musí být zmíněn alespoň jeden nebezpečný příkaz jako zakázaný
    assert "git commit" in agents_text or "rm" in agents_text, \
        "AGENTS.md neobsahuje žádné zakázané příkazy"


def test_agents_md_neni_prazdny(agents_text):
    assert len(agents_text.strip()) > 100, "AGENTS.md je příliš krátký"


def test_config_provider_je_openai(config_text):
    for radek in config_text.splitlines():
        if radek.strip().startswith("provider:"):
            provider = radek.split(":", 1)[1].strip()
            assert provider == "openai", \
                f"Neočekávaný provider: {provider}"
            return
