"""
Testy pro Code Review Supervisor.

Smoke testy ověřují správnou inicializaci a strukturu bez volání API.
Integrační testy (označené pytest.mark.integration) vyžadují ANTHROPIC_API_KEY.
"""

from __future__ import annotations

import asyncio
from pathlib import Path
from unittest.mock import AsyncMock, MagicMock, patch

import pytest
from claude_agent_sdk import AgentDefinition

from code_reviewer.supervisor import CodeReviewSupervisor, _run_agent, _run_supervisor
from code_reviewer.security_swarm import run_security_swarm


EXAMPLES_DIR = Path(__file__).parent.parent / "examples"
BUGGY_APP = EXAMPLES_DIR / "buggy_app.py"


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


def _test_agent_def(description: str = "test agent") -> AgentDefinition:
    return AgentDefinition(
        description=description,
        prompt="system prompt",
        tools=["Read"],
        model="claude-sonnet-4-6",
    )


# ---------------------------------------------------------------------------
# Smoke testy — bez API
# ---------------------------------------------------------------------------

class TestCodeReviewSupervisorInit:
    def test_init_sets_target(self):
        supervisor = CodeReviewSupervisor(BUGGY_APP)
        assert supervisor.target == BUGGY_APP.resolve()

    def test_init_sets_cwd(self):
        supervisor = CodeReviewSupervisor(BUGGY_APP)
        assert supervisor.cwd == str(BUGGY_APP.resolve().parent)

    def test_init_resolves_relative_path(self, tmp_path):
        f = tmp_path / "test.py"
        f.write_text("x = 1")
        supervisor = CodeReviewSupervisor(f)
        assert supervisor.target.is_absolute()


class TestExamplesExist:
    def test_buggy_app_exists(self):
        assert BUGGY_APP.exists(), "examples/buggy_app.py musí existovat pro demo"

    def test_buggy_app_has_content(self):
        content = BUGGY_APP.read_text()
        assert len(content) > 100

    def test_buggy_app_contains_known_issues(self):
        content = BUGGY_APP.read_text()
        assert "SECRET_KEY" in content        # hardcoded secret
        assert "pickle.loads" in content      # nebezpečná deserializace
        assert "md5" in content               # slabé hashování
        assert "f\"SELECT" in content or "f'SELECT" in content  # SQL injection


# ---------------------------------------------------------------------------
# Unit testy se mockovaným SDK
# ---------------------------------------------------------------------------

class TestRunAgentMocked:
    @pytest.mark.asyncio
    async def test_returns_text(self):
        mock_client = _make_mock_client("Mock výstup agenta.")

        with patch("code_reviewer.supervisor.ClaudeSDKClient", return_value=mock_client):
            result = await _run_agent("test", _test_agent_def(), "task", "/tmp")

        assert "Mock výstup" in result

    @pytest.mark.asyncio
    async def test_empty_response_returns_fallback(self):
        with patch("code_reviewer.supervisor.ClaudeSDKClient", return_value=_make_empty_client()):
            result = await _run_agent("test", _test_agent_def(), "task", "/tmp")

        assert "[test: žádný výstup]" in result


class TestSupervisorMocked:
    @pytest.mark.asyncio
    async def test_review_returns_string(self):
        def make_client(*args, **kwargs):
            return _make_mock_client("Mock výstup agenta.")

        with (
            patch("code_reviewer.supervisor.ClaudeSDKClient", side_effect=make_client),
            patch("code_reviewer.supervisor.run_security_swarm", return_value="mock security"),
        ):
            supervisor = CodeReviewSupervisor(BUGGY_APP)
            report = await supervisor.review()

        assert isinstance(report, str)
        assert len(report) > 0

    @pytest.mark.asyncio
    async def test_review_calls_all_agents(self):
        """Quality Agent + Tests Agent + Supervisor = 3 volání ClaudeSDKClient.
        Security Swarm je mockovaný zvlášť (běží ve vlastním modulu)."""
        call_count = 0

        def make_client(*args, **kwargs):
            nonlocal call_count
            call_count += 1
            return _make_mock_client("výstup")

        with (
            patch("code_reviewer.supervisor.ClaudeSDKClient", side_effect=make_client),
            patch("code_reviewer.supervisor.run_security_swarm", return_value="mock security"),
        ):
            supervisor = CodeReviewSupervisor(BUGGY_APP)
            await supervisor.review()

        assert call_count == 3


# ---------------------------------------------------------------------------
# Integrační test — vyžaduje ANTHROPIC_API_KEY
# ---------------------------------------------------------------------------

@pytest.mark.integration
@pytest.mark.asyncio
async def test_full_review_integration():
    """Plný běh s reálným API. Spusť: pytest -m integration"""
    supervisor = CodeReviewSupervisor(BUGGY_APP)
    report = await supervisor.review()

    assert len(report) > 200
    assert "#" in report          # Markdown hlavičky
    assert "SQL" in report or "injection" in report.lower()
