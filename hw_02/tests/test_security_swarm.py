"""
Testy pro Security Sub-Swarm (Swarm pattern).

Ověřuje spuštění všech checkerů, agregaci výsledků a správnou strukturu výstupu.
Testy bez API — používají mock ClaudeSDKClient.
"""

from __future__ import annotations

from unittest.mock import AsyncMock, MagicMock, patch

import pytest

from claude_agent_sdk import AgentDefinition
from code_reviewer.security_swarm import (
    CHECKERS,
    _run_checker,
    run_security_swarm,
)


EXPECTED_CHECKER_COUNT = 5


# ---------------------------------------------------------------------------
# Pomocné funkce
# ---------------------------------------------------------------------------

def _make_mock_client(text: str):
    """Vytvoří mock ClaudeSDKClient (async context manager) vracející jeden AssistantMessage."""
    from claude_agent_sdk import AssistantMessage, TextBlock

    block = MagicMock(spec=TextBlock)
    block.text = text
    msg = MagicMock(spec=AssistantMessage)
    msg.content = [block]

    async def _receive_response():
        yield msg

    mock = AsyncMock()
    mock.__aenter__ = AsyncMock(return_value=mock)
    mock.__aexit__ = AsyncMock(return_value=False)
    mock.receive_response = _receive_response
    return mock


def _make_empty_client():
    """Vytvoří mock ClaudeSDKClient bez žádného výstupu."""
    async def _receive_nothing():
        return
        yield  # prázdný async generátor

    mock = AsyncMock()
    mock.__aenter__ = AsyncMock(return_value=mock)
    mock.__aexit__ = AsyncMock(return_value=False)
    mock.receive_response = _receive_nothing
    return mock


# ---------------------------------------------------------------------------
# Testy struktury swarm
# ---------------------------------------------------------------------------

class TestSwarmStructure:
    def test_checkers_count(self):
        assert len(CHECKERS) == EXPECTED_CHECKER_COUNT

    def test_checkers_names(self):
        expected = {"SQL Injection", "Hardcoded Secrets", "Deserializace", "Path Traversal", "Auth & Autorizace"}
        assert set(CHECKERS.keys()) == expected

    def test_all_checker_definitions_are_agent_definitions(self):
        for name, agent_def in CHECKERS.items():
            assert isinstance(agent_def, AgentDefinition), f"'{name}' není AgentDefinition"

    def test_all_system_prompts_nonempty(self):
        for name, agent_def in CHECKERS.items():
            assert len(agent_def.prompt) > 50, f"System prompt pro '{name}' je příliš krátký"

    def test_all_checkers_have_read_tool(self):
        for name, agent_def in CHECKERS.items():
            assert "Read" in (agent_def.tools or []), f"'{name}' nemá nástroj Read"


# ---------------------------------------------------------------------------
# Testy _run_checker
# ---------------------------------------------------------------------------

class TestRunChecker:
    @pytest.mark.asyncio
    async def test_returns_text(self):
        agent_def = AgentDefinition(
            description="test checker",
            prompt="system prompt",
            tools=["Read"],
            model="claude-sonnet-4-6",
        )
        mock_client = _make_mock_client("SQL injection nalezena na řádku 42.")

        with patch("code_reviewer.security_swarm.ClaudeSDKClient", return_value=mock_client):
            result = await _run_checker("SQL Injection", agent_def, "task", "/tmp")

        assert "SQL injection" in result

    @pytest.mark.asyncio
    async def test_empty_response_returns_fallback(self):
        agent_def = AgentDefinition(
            description="test",
            prompt="system",
            tools=["Read"],
            model="claude-sonnet-4-6",
        )

        with patch("code_reviewer.security_swarm.ClaudeSDKClient", return_value=_make_empty_client()):
            result = await _run_checker("test", agent_def, "task", "/tmp")

        assert "[test: žádný výstup]" in result


# ---------------------------------------------------------------------------
# Testy run_security_swarm — agregace výsledků
# ---------------------------------------------------------------------------

class TestRunSecuritySwarm:
    @pytest.mark.asyncio
    async def test_all_checkers_called(self):
        """Ověří, že swarm spustí všech 5 checkerů (= 5 instancí ClaudeSDKClient)."""
        instances: list = []

        def make_client(*args, **kwargs):
            client = _make_mock_client("nález")
            instances.append(client)
            return client

        with patch("code_reviewer.security_swarm.ClaudeSDKClient", side_effect=make_client):
            await run_security_swarm("Analyzuj soubor: test.py", "/tmp")

        assert len(instances) == EXPECTED_CHECKER_COUNT

    @pytest.mark.asyncio
    async def test_output_contains_all_checker_names(self):
        """Agregovaný výstup musí obsahovat sekci pro každý checker."""
        def make_client(*args, **kwargs):
            return _make_mock_client("výsledek checkeru")

        with patch("code_reviewer.security_swarm.ClaudeSDKClient", side_effect=make_client):
            result = await run_security_swarm("task", "/tmp")

        for checker_name in CHECKERS:
            assert checker_name in result, f"Výstup neobsahuje sekci '{checker_name}'"

    @pytest.mark.asyncio
    async def test_output_contains_individual_results(self):
        """Každý checker dostane svůj vlastní výstup, který se objeví v agregátu."""
        counter = [0]

        def make_client(*args, **kwargs):
            text = f"výsledek_{counter[0]}"
            counter[0] += 1
            return _make_mock_client(text)

        with patch("code_reviewer.security_swarm.ClaudeSDKClient", side_effect=make_client):
            result = await run_security_swarm("task", "/tmp")

        for i in range(EXPECTED_CHECKER_COUNT):
            assert f"výsledek_{i}" in result

    @pytest.mark.asyncio
    async def test_returns_string(self):
        def make_client(*args, **kwargs):
            return _make_mock_client("text")

        with patch("code_reviewer.security_swarm.ClaudeSDKClient", side_effect=make_client):
            result = await run_security_swarm("task", "/tmp")

        assert isinstance(result, str)
        assert len(result) > 0
