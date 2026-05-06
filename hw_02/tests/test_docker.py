"""
Testy Docker setupu (Dockerfile, docker-compose.yml, entrypoint)
"""
import pytest
from pathlib import Path

ROOT = Path(__file__).parent.parent
DOCKERFILE = ROOT / "Dockerfile"
COMPOSE = ROOT / "docker-compose.yml"
ENTRYPOINT = ROOT / "docker-entrypoint.sh"
ENV_EXAMPLE = ROOT / ".env.example"


@pytest.fixture(scope="module")
def dockerfile():
    return DOCKERFILE.read_text()


@pytest.fixture(scope="module")
def compose():
    return COMPOSE.read_text()


@pytest.fixture(scope="module")
def entrypoint():
    return ENTRYPOINT.read_text()


# ── Dockerfile ─────────────────────────────────────────────────────────

def test_dockerfile_existuje():
    assert DOCKERFILE.exists()


def test_dockerfile_pouziva_node_base(dockerfile):
    assert dockerfile.startswith("FROM node:"), \
        "Dockerfile musí vycházet z node image (Claude Code potřebuje Node.js)"


def test_dockerfile_instaluje_python(dockerfile):
    assert "python3" in dockerfile, \
        "Dockerfile neinstaluje Python 3"


def test_dockerfile_instaluje_pytest(dockerfile):
    assert "pytest" in dockerfile, \
        "Dockerfile neinstaluje pytest"


def test_dockerfile_instaluje_claude_code(dockerfile):
    assert "@anthropic-ai/claude-code" in dockerfile, \
        "Dockerfile neinstaluje Claude Code CLI"


def test_dockerfile_instaluje_codex(dockerfile):
    assert "@openai/codex" in dockerfile, \
        "Dockerfile neinstaluje Codex CLI"


def test_dockerfile_instaluje_mcp_servery(dockerfile):
    mcp_servery = [
        "server-filesystem",
        "server-github",
        "server-brave-search",
        "server-sqlite",
        "server-memory",
    ]
    chybejici = [s for s in mcp_servery if s not in dockerfile]
    assert not chybejici, f"Dockerfile neinstaluje MCP servery: {chybejici}"


def test_dockerfile_ma_workdir(dockerfile):
    assert "WORKDIR" in dockerfile, \
        "Dockerfile nedefinuje WORKDIR"


def test_dockerfile_ma_entrypoint(dockerfile):
    assert "ENTRYPOINT" in dockerfile, \
        "Dockerfile nedefinuje ENTRYPOINT"


def test_dockerfile_kopiruje_claude_config(dockerfile):
    assert ".claude" in dockerfile, \
        "Dockerfile nekopíruje .claude/ konfiguraci"


# ── docker-compose.yml ─────────────────────────────────────────────────

def test_compose_existuje():
    assert COMPOSE.exists()


def test_compose_ma_claude_service(compose):
    assert "claude:" in compose, \
        "docker-compose.yml nemá service 'claude'"


def test_compose_ma_codex_service(compose):
    assert "codex:" in compose, \
        "docker-compose.yml nemá service 'codex'"


def test_compose_ma_tests_service(compose):
    assert "tests:" in compose, \
        "docker-compose.yml nemá service 'tests'"


def test_compose_claude_ma_tty(compose):
    assert "tty: true" in compose, \
        "Claude service potřebuje tty: true pro interaktivní CLI"


def test_compose_ma_anthropic_key(compose):
    assert "ANTHROPIC_API_KEY" in compose, \
        "docker-compose.yml nepředává ANTHROPIC_API_KEY"


def test_compose_ma_workspace_volume(compose):
    assert "workspace" in compose, \
        "docker-compose.yml nemá volume pro workspace"


# ── entrypoint.sh ──────────────────────────────────────────────────────

def test_entrypoint_existuje():
    assert ENTRYPOINT.exists()


def test_entrypoint_je_bash(entrypoint):
    assert entrypoint.startswith("#!/bin/bash"), \
        "entrypoint.sh musí začínat #!/bin/bash"


def test_entrypoint_kontroluje_api_key(entrypoint):
    assert "ANTHROPIC_API_KEY" in entrypoint, \
        "entrypoint.sh nekontroluje přítomnost ANTHROPIC_API_KEY"


def test_entrypoint_ma_help(entrypoint):
    assert "help" in entrypoint, \
        "entrypoint.sh nemá help příkaz"


def test_entrypoint_spousti_claude(entrypoint):
    assert "exec claude" in entrypoint or "claude" in entrypoint, \
        "entrypoint.sh nespouští claude"


def test_entrypoint_spousti_codex(entrypoint):
    assert "exec codex" in entrypoint or "codex" in entrypoint, \
        "entrypoint.sh nespouští codex"


# ── .env.example ───────────────────────────────────────────────────────

def test_env_example_existuje():
    assert ENV_EXAMPLE.exists(), ".env.example neexistuje"


def test_env_example_obsahuje_anthropic_key():
    obsah = ENV_EXAMPLE.read_text()
    assert "ANTHROPIC_API_KEY" in obsah


def test_env_example_obsahuje_openai_key():
    obsah = ENV_EXAMPLE.read_text()
    assert "OPENAI_API_KEY" in obsah


def test_env_neni_v_gitu():
    gitignore = ROOT / ".gitignore"
    if gitignore.exists():
        assert ".env" in gitignore.read_text(), \
            ".gitignore neobsahuje .env – API klíče by mohly být commitnuty!"
