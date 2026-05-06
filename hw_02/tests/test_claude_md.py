"""
Testy CLAUDE.md a AGENTS.md – instrukční soubory agentů
"""
import pytest
from pathlib import Path

CLAUDE_MD = Path(__file__).parent.parent / "claude-code" / "CLAUDE.md"
AGENTS_MD = Path(__file__).parent.parent / "codex" / "AGENTS.md"

POVINNE_SEKCE_CLAUDE = [
    "## Role",
    "## Pravidla",
    "## Struktura projektu",
    "## MCP",
    "## Sub-agent",
]

POVINNE_SEKCE_AGENTS = [
    "## Role",
    "## Chování",
    "## Příkazy",
]


@pytest.fixture(scope="module")
def claude_md():
    return CLAUDE_MD.read_text()


@pytest.fixture(scope="module")
def agents_md():
    return AGENTS_MD.read_text()


def test_claude_md_existuje():
    assert CLAUDE_MD.exists()


def test_agents_md_existuje():
    assert AGENTS_MD.exists()


@pytest.mark.parametrize("sekce", POVINNE_SEKCE_CLAUDE)
def test_claude_md_ma_sekci(claude_md, sekce):
    assert sekce in claude_md, f"CLAUDE.md chybí sekce: '{sekce}'"


@pytest.mark.parametrize("sekce", POVINNE_SEKCE_AGENTS)
def test_agents_md_ma_sekci(agents_md, sekce):
    assert sekce in agents_md, f"AGENTS.md chybí sekce: '{sekce}'"


def test_claude_md_definuje_jazyk(claude_md):
    """Agent musí vědět v jakém jazyce komunikovat."""
    assert "češtin" in claude_md.lower() or "czech" in claude_md.lower() or \
           "jazyk" in claude_md.lower(), \
        "CLAUDE.md nedefinuje jazyk komunikace"


def test_claude_md_zakazuje_reseni(claude_md):
    """Mentor nesmí psát řešení za studenta."""
    claude_lower = claude_md.lower()
    ma_zakazane = (
        "neoprav" in claude_lower or
        "nevypisuj" in claude_lower or
        "nepište" in claude_lower or
        "ptej se" in claude_lower or
        "zamysli" in claude_lower
    )
    assert ma_zakazane, \
        "CLAUDE.md neobsahuje pravidlo zakazující psaní řešení za studenta"


def test_claude_md_definuje_zakazane_operace(claude_md):
    assert "zakázané" in claude_md.lower() or "zakazane" in claude_md.lower() or \
           "## Zakáz" in claude_md, \
        "CLAUDE.md nedefinuje zakázané operace"


def test_claude_md_zminuje_mcp_servery(claude_md):
    servery = ["filesystem", "github", "sqlite", "memory"]
    nalezene = [s for s in servery if s in claude_md.lower()]
    assert len(nalezene) >= 3, \
        f"CLAUDE.md zmiňuje pouze {len(nalezene)}/4 MCP serverů: {nalezene}"


def test_claude_md_zminuje_sub_agenty(claude_md):
    assert "Explore" in claude_md or "Plan" in claude_md or "sub-agent" in claude_md.lower(), \
        "CLAUDE.md nezmiňuje sub-agenty"


def test_agents_md_zminuje_pytest(agents_md):
    assert "pytest" in agents_md, \
        "AGENTS.md neobsahuje pytest – agent by měl umět testovat kód"


def test_agents_md_rozlisuje_povolene_a_zakazane(agents_md):
    """Musí být jasné co agent smí a co ne."""
    ma_povolene = "bez potvrzení" in agents_md or "povolené" in agents_md.lower() or "smíš" in agents_md
    ma_zakazane = "potvrzení" in agents_md or "zakázané" in agents_md.lower() or "VŽDY" in agents_md
    assert ma_povolene and ma_zakazane, \
        "AGENTS.md jasně nerozlišuje povolené a zakázané příkazy"
