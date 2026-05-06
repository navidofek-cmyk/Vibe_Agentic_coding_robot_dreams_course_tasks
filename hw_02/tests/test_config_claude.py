"""
Testy konfigurace Claude Code (.claude/settings.json)
"""
import json
import pytest
from pathlib import Path

SETTINGS = Path(__file__).parent.parent / "claude-code" / ".claude" / "settings.json"


@pytest.fixture(scope="module")
def settings():
    return json.loads(SETTINGS.read_text())


def test_settings_soubor_existuje():
    assert SETTINGS.exists(), "settings.json neexistuje"


def test_settings_validni_json():
    # Pokud by JSON byl poškozený, fixture by hodila výjimku
    data = json.loads(SETTINGS.read_text())
    assert isinstance(data, dict)


def test_mcp_servery_existuji(settings):
    assert "mcpServers" in settings, "Chybí sekce mcpServers"


def test_mcp_servery_neprazdne(settings):
    assert len(settings["mcpServers"]) > 0, "mcpServers je prázdný"


@pytest.mark.parametrize("server", [
    "filesystem", "github", "brave-search", "sqlite", "memory"
])
def test_mcp_server_je_definovan(settings, server):
    assert server in settings["mcpServers"], f"MCP server '{server}' chybí"


@pytest.mark.parametrize("server", [
    "filesystem", "github", "brave-search", "sqlite", "memory"
])
def test_mcp_server_ma_command(settings, server):
    cfg = settings["mcpServers"][server]
    assert "command" in cfg, f"Server '{server}' nemá 'command'"
    assert cfg["command"] in ("npx", "node", "python3"), \
        f"Server '{server}' má neznámý command: {cfg['command']}"


@pytest.mark.parametrize("server", [
    "filesystem", "github", "brave-search", "sqlite", "memory"
])
def test_mcp_server_ma_args(settings, server):
    cfg = settings["mcpServers"][server]
    assert "args" in cfg, f"Server '{server}' nemá 'args'"
    assert isinstance(cfg["args"], list), f"Server '{server}': args musí být seznam"
    assert len(cfg["args"]) > 0, f"Server '{server}': args je prázdný"


def test_permissions_existuji(settings):
    assert "permissions" in settings, "Chybí sekce permissions"


def test_permissions_ma_allow(settings):
    assert "allow" in settings["permissions"], "Chybí permissions.allow"
    assert isinstance(settings["permissions"]["allow"], list)
    assert len(settings["permissions"]["allow"]) > 0


def test_permissions_ma_deny(settings):
    assert "deny" in settings["permissions"], "Chybí permissions.deny"
    assert isinstance(settings["permissions"]["deny"], list)


def test_deny_obsahuje_nebezpecne_prikazy(settings):
    deny = settings["permissions"]["deny"]
    # Alespoň jeden nebezpečný příkaz musí být blokován
    nebezpecne = ["rm -rf", "git push --force", "sudo"]
    nalezeny = [p for p in nebezpecne if any(p in d for d in deny)]
    assert len(nalezeny) > 0, f"Žádný nebezpečný příkaz není blokován. deny={deny}"


def test_hooks_existuji(settings):
    assert "hooks" in settings, "Chybí sekce hooks"


def test_hooks_ma_post_tool_use(settings):
    assert "PostToolUse" in settings["hooks"], "Chybí hooks.PostToolUse"


def test_hooks_post_tool_use_ma_matcher(settings):
    for hook_group in settings["hooks"]["PostToolUse"]:
        assert "matcher" in hook_group, "Hook nemá 'matcher'"
        assert "hooks" in hook_group, "Hook nemá 'hooks'"


def test_model_je_nastaven(settings):
    assert "model" in settings, "Chybí nastavení modelu"
    assert "claude" in settings["model"].lower(), \
        f"Model nevypadá jako Claude model: {settings['model']}"
